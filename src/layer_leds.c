/*
 * Per-key RGB indicators for the NUM and MAGNET layers.
 *
 * While one of those layers is the highest active layer, the underglow is switched off and the
 * keys of that layer are painted individually. Leaving the layer turns the underglow back on if
 * it was on before.
 *
 * The left half is the split central and sees layer changes, so it paints itself and relays the
 * layer to the right half through the lyrleds behavior (ZMK only sends 8 characters of a behavior
 * name over the split link, so keep that node name short).
 */

#define DT_DRV_COMPAT custom_behavior_layer_leds

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/logging/log.h>

#include <drivers/behavior.h>
#include <drivers/ext_power.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/rgb_underglow.h>
#include <zmk/workqueue.h>
#include <zmk/behavior.h>

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zmk/split/central.h>
#endif

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define STRIP_CHOSEN DT_CHOSEN(zmk_underglow)
#define NUM_LEDS DT_PROP(STRIP_CHOSEN, chain_length)

/* Layer indexes, in the order they appear in config/sofle_choc_pro.keymap */
#define LAYER_NUM 3
#define LAYER_MAGNET 5

enum color { OFF, WHITE, GREEN, BLUE, ORANGE, RED };

/* Canonical white lives in the rgb-white skill: 255,225,150 */
static const struct led_rgb palette[] = {
    [OFF] = {0, 0, 0},         [WHITE] = {255, 225, 150}, [GREEN] = {0, 255, 0},
    [BLUE] = {0, 0, 255},      [ORANGE] = {255, 90, 0},   [RED] = {255, 0, 0},
};

struct paint {
    uint8_t position;
    uint8_t color;
};

/* Key positions are the global ones from sofle_choc_pro-layouts.dtsi */
static const struct paint num_paint[] = {
    /* number row */
    {6, GREEN}, {7, WHITE}, {8, WHITE}, {9, WHITE}, {10, ORANGE},
    /* top row */
    {18, GREEN}, {19, WHITE}, {20, WHITE}, {21, WHITE}, {22, ORANGE}, {23, RED},
    /* home row */
    {30, GREEN}, {31, WHITE}, {32, WHITE}, {33, WHITE}, {34, ORANGE}, {35, BLUE},
    /* bottom row */
    {45, WHITE}, {47, WHITE}, {49, BLUE},
    /* the sticky NUM key */
    {57, WHITE},
};

static const struct paint magnet_paint[] = {
    /* left: thirds (top row) blue, two-thirds (home row) green */
    {14, BLUE}, {15, BLUE}, {16, BLUE}, {26, GREEN}, {27, GREEN}, {28, GREEN},
    /* left: previous/next display and center */
    {37, BLUE}, {38, BLUE}, {39, BLUE},
    /* left thumb: maximize */
    {54, GREEN},
    /* right: corners green, arrows white, restore blue */
    {19, GREEN}, {21, GREEN}, {45, GREEN}, {47, GREEN}, {20, WHITE}, {31, WHITE},
    {32, WHITE}, {33, WHITE}, {23, BLUE},
};

/*
 * LED index -> key position (0xFF = no key). THESE ORDERS ARE A GUESS (serpentine columns from
 * the outer column, then thumbs, then the encoder LED). Use CONFIG_SOFLE_LAYER_LEDS_PROBE to
 * find the real order and fix these tables.
 */
#define NONE 0xFF
static const uint8_t left_led_to_position[NUM_LEDS] = {
    0,  12, 24, 36, 37, 25, 13, 1,  2,  14, 26, 38, 39, 27, 15,
    3,  4,  16, 28, 40, 41, 29, 17, 5,  50, 51, 52, 53, 54, NONE,
};
static const uint8_t right_led_to_position[NUM_LEDS] = {
    11, 23, 35, 49, 48, 34, 22, 10, 9,  21, 33, 47, 46, 32, 20,
    8,  7,  19, 31, 45, 44, 30, 18, 6,  55, 56, 57, 58, 59, NONE,
};

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#define LED_TO_POSITION left_led_to_position
#else
#define LED_TO_POSITION right_led_to_position
#endif

static const struct device *strip = DEVICE_DT_GET(STRIP_CHOSEN);
#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER)
static const struct device *const ext_power = DEVICE_DT_GET(DT_INST(0, zmk_ext_power_generic));
#endif

static struct led_rgb pixels[NUM_LEDS];
static struct k_work_delayable update_work;

static uint32_t target_layer;
static bool taken_over;     /* underglow is off and we own the strip */
static bool underglow_was_on;
static int probe_led = -1;

static const struct paint *paint_for(uint32_t layer, size_t *len) {
    switch (layer) {
    case LAYER_NUM:
        *len = ARRAY_SIZE(num_paint);
        return num_paint;
    case LAYER_MAGNET:
        *len = ARRAY_SIZE(magnet_paint);
        return magnet_paint;
    default:
        *len = 0;
        return NULL;
    }
}

static struct led_rgb scaled(enum color c) {
    struct led_rgb in = palette[c];
    return (struct led_rgb){
        .r = in.r * CONFIG_SOFLE_LAYER_LEDS_BRIGHTNESS / 100,
        .g = in.g * CONFIG_SOFLE_LAYER_LEDS_BRIGHTNESS / 100,
        .b = in.b * CONFIG_SOFLE_LAYER_LEDS_BRIGHTNESS / 100,
    };
}

static void paint_pixels(void) {
    memset(pixels, 0, sizeof(pixels));

    if (IS_ENABLED(CONFIG_SOFLE_LAYER_LEDS_PROBE)) {
        if (probe_led >= 0 && probe_led < NUM_LEDS) {
            pixels[probe_led] = scaled(WHITE);
        }
    } else {
        size_t len;
        const struct paint *table = paint_for(target_layer, &len);
        for (size_t i = 0; i < len; i++) {
            for (int led = 0; led < NUM_LEDS; led++) {
                if (LED_TO_POSITION[led] == table[i].position) {
                    pixels[led] = scaled(table[i].color);
                }
            }
        }
    }

    int err = led_strip_update_rgb(strip, pixels, NUM_LEDS);
    if (err < 0) {
        LOG_ERR("Layer LEDs: failed to update the strip (%d)", err);
    }
}

static void update_handler(struct k_work *work) {
    size_t len;
    bool want = IS_ENABLED(CONFIG_SOFLE_LAYER_LEDS_PROBE) ? probe_led >= 0
                                                          : paint_for(target_layer, &len) != NULL;

    if (want && !taken_over) {
        if (zmk_rgb_underglow_get_state(&underglow_was_on) || !underglow_was_on) {
            return; /* RGB is off: respect that and show nothing */
        }
        /* Turning the underglow off queues a blanking job, so paint on the next pass */
        zmk_rgb_underglow_off();
        taken_over = true;
        k_work_reschedule_for_queue(zmk_workqueue_lowprio_work_q(), &update_work, K_MSEC(40));
        return;
    }

    if (want) {
#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER)
        ext_power_enable(ext_power);
#endif
        paint_pixels();
    } else if (taken_over) {
        taken_over = false;
        if (underglow_was_on) {
            zmk_rgb_underglow_on();
        }
    }
}

static void show_layer(uint32_t layer) {
    target_layer = layer;
    k_work_reschedule_for_queue(zmk_workqueue_lowprio_work_q(), &update_work, K_MSEC(20));
}

/* Peripheral entry point: the central relays the active layer through this behavior */
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int on_binding_pressed(struct zmk_behavior_binding *binding,
                              struct zmk_behavior_binding_event event) {
    show_layer(binding->param1);
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_binding_released(struct zmk_behavior_binding *binding,
                               struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api layer_leds_driver_api = {
    .binding_pressed = on_binding_pressed,
    .binding_released = on_binding_released,
    .locality = BEHAVIOR_LOCALITY_CENTRAL,
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &layer_leds_driver_api);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
static int on_layer_state_changed(const zmk_event_t *eh) {
    uint32_t layer = zmk_keymap_highest_layer_active();

    show_layer(layer);

    struct zmk_behavior_binding binding = {
        .behavior_dev = DEVICE_DT_NAME(DT_INST(0, DT_DRV_COMPAT)),
        .param1 = layer,
    };
    struct zmk_behavior_binding_event event = {
        .layer = layer,
        .position = 0,
        .timestamp = k_uptime_get(),
        .source = ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL,
    };
    for (int i = 0; i < ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT; i++) {
        zmk_split_central_invoke_behavior(i, &binding, event, true);
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(layer_leds_layer, on_layer_state_changed);
ZMK_SUBSCRIPTION(layer_leds_layer, zmk_layer_state_changed);
#endif /* CONFIG_ZMK_SPLIT_ROLE_CENTRAL */

#endif /* DT_HAS_COMPAT_STATUS_OKAY */

#if IS_ENABLED(CONFIG_SOFLE_LAYER_LEDS_PROBE)
static int on_position_state_changed(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);

    if (ev && ev->state && ev->source == ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL) {
        probe_led = (probe_led + 1) % NUM_LEDS;
        LOG_INF("Layer LEDs probe: LED %d", probe_led);
        k_work_reschedule_for_queue(zmk_workqueue_lowprio_work_q(), &update_work, K_MSEC(20));
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(layer_leds_probe, on_position_state_changed);
ZMK_SUBSCRIPTION(layer_leds_probe, zmk_position_state_changed);
#endif

static int layer_leds_init(void) {
    k_work_init_delayable(&update_work, update_handler);
    return 0;
}

SYS_INIT(layer_leds_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
