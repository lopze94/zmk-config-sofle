# Per-layer, per-key RGB indicators for the Sofle (see src/layer_leds.c)
target_sources_ifdef(CONFIG_SOFLE_LAYER_LEDS app PRIVATE src/layer_leds.c)
