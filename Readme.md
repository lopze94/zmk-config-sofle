# Keebart ZMK Firmware

This repository contains my ZMK firmware configuration for the Keebart
[Sofle Choc Pro BT](https://keebart.com/products/sofle-wireless) (Bluetooth
name `Sofle Choc BT`), forked from Keebart's config and trimmed to the Sofle
only. It includes the board definition, a macOS-oriented keymap, ZMK Studio
support, Sharp Memory-in-Pixel display support, RGB underglow, and GitHub
Actions firmware builds.

- Maintainer: [Keebart](https://github.com/Keebart)
- Firmware: [ZMK](https://zmk.dev/)
- Online keymap editor: [ZMK Keymap Editor](https://nickcoutsos.github.io/keymap-editor/)
- Runtime editor: [ZMK Studio](https://zmk.studio/)

## Building firmware

The GitHub Actions workflow builds the Sofle targets on every push and pull request.
It can also be started manually from the repository's **Actions** tab. Download
the merged `firmware` artifact after the build finishes.

The build matrix is defined in `build.yaml`. Normal firmware uses the custom
`sharp_mip` shield, with the 180-degree rotation enabled in the board
configuration.
The central, left-side builds include ZMK Studio support.

For a local build, first create a normal ZMK west workspace using the manifest
in `config/west.yml`. From the workspace root, run a command such as:

```sh
west build -s zmk/app -d build/sofle-left \
  -b sofle_choc_pro_left \
  -S studio-rpc-usb-uart -- \
  -DZMK_CONFIG=/path/to/zmk-config/config \
  -DZMK_EXTRA_MODULES=/path/to/zmk-config \
  -DSHIELD=sharp_mip \
  -DCONFIG_ZMK_STUDIO=y
```

Change the board target as required. The resulting firmware is written to
`build/sofle-left/zephyr/zmk.uf2`.

## Configuration settings

User-adjustable firmware settings belong in the matching `config/*.conf` file.
This includes the keyboard name, power management, RGB defaults, display idle
behavior, and an optional pointing setting. The files under `boards/` describe
the keyboard hardware and its internal defaults; normal users should not edit
them.

## Flashing

The halves use different firmware because the left half is the central side
and the right half is the peripheral side.

1. Download or build the firmware package.
2. Enter the bootloader on the right half and copy its matching right-side UF2
   file to the USB mass-storage device.
3. Enter the bootloader on the left half and copy its matching left-side UF2
   file.
4. Reconnect the keyboard and pair it with the host if necessary.

Enter the bootloader by double-pressing the physical reset button, or use the
bootloader key in the active keymap.

### Resetting saved settings

The build artifact also contains `settings_reset` firmware for the standard
Sofle targets. Use it when split pairing or stored settings
prevent normal operation:

1. Flash the appropriate settings-reset firmware to a half.
2. Allow it to boot and clear the saved settings.
3. Immediately flash the normal firmware for that half again.
4. Repeat for the other half when resetting split pairing.

Settings-reset firmware is temporary and is not a usable keyboard firmware.

## Editing keymaps

### ZMK Keymap Editor

The [ZMK Keymap Editor](https://nickcoutsos.github.io/keymap-editor/) edits the
source `.keymap` files in this repository. Open
`config/sofle_choc_pro.keymap`; `config/sofle_choc_pro.json` provides the
visual geometry.

Editing and committing a source keymap triggers a new GitHub Actions build.
This is the recommended workflow for maintaining and distributing defaults.

### ZMK Studio

[ZMK Studio](https://zmk.studio/) edits the runtime keymap stored on the
keyboard. Connect the left half directly by USB, unlock Studio from the
keymap, and select the device in the browser.

Studio changes do not update the `.keymap` source file. Conversely, flashing
new firmware may not replace a Studio-edited keymap because the runtime state
is saved. Use **Restore Stock Settings** in Studio whenever the compiled default should
be loaded again.

## Power management

Deep sleep is enabled on every half. The keyboard enters deep sleep after one
hour (`3600000` ms) without activity and wakes when a key is pressed. The long
timeout avoids the keyboard appearing to sleep during normal breaks while
still conserving battery during extended inactivity. Change these settings in
the matching `config/*.conf` file.

## RGB controls

RGB underglow is enabled with a conservative maximum brightness. Hue,
saturation, brightness, effect, and toggle controls are on the Adjust layer
(hold Lower and Raise together).

## Rotary encoders

Sofle Choc Pro uses one encoder for volume and the other for previous/next
media track. The encoder actions are available on its default, Lower, and
Raise layers.
