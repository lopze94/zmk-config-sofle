---
name: space-enter-placement
description: Where Space and Enter live on the Sofle ZMK keymaps (config/sofle_full.keymap and config/sofle_small.keymap). Space goes on the LEFT half's inner thumb key, Enter on the RIGHT half's inner thumb key. Use whenever editing, adding, or reviewing thumb-cluster bindings, layers, or the keymap diagrams so the two keys are never swapped or duplicated on the wrong side.
---

# Space on the left, Enter on the right

Convention for every layout keymap in `config/` (`sofle_full.keymap` and `sofle_small.keymap`):

| Key | Half | Position | Binding |
|---|---|---|---|
| Space | Left | Inner thumb key (the one nearest the center, left of the gap) | full: `&kp SPACE`; small: `&lt NUM SPACE` (tap Space, hold = NUM layer) |
| Enter | Right | Inner thumb key (the one nearest the center, right of the gap) | `&kp RET` |

## Default layer thumb row

The thumb row in the `bindings` block reads left to right.

`sofle_full.keymap`:

```
//         | MAGNET |  OPT  |  NUM  | LOWER | SPACE |               | ENTER | RAISE |       |       |  CTRL |
&mo MAGNET  &kp LALT  &mo NUM  &mo LOWER  &kp SPACE      &kp RET  &mo RAISE  &none  &none  &kp RCTRL
```

`sofle_small.keymap`:

```
//               | GUI | ALT  |  FN  | LOWER|  SPACE |  | ENTER | RAISE|MAGNET | ALT   | GUI  |
&kp LGUI &kp LALT  &mo FN    &mo LOWER  &lt NUM SPACE  &kp RET  &mo RAISE  &mo MAGNET &kp RALT  &kp RGUI
```

The inner thumb keys are the 5th binding of the left thumb row and the 1st binding of the right thumb row.
Only the first binding in each half's thumb row changes. Do not touch the hold or layer keys around them.

## Rules

- Never put Space on the right half or Enter on the left half of the default layer.
- In the small layout Space is a layer-tap. Keep the tap side as Space when changing which layer the hold activates.
- Other layers use `&trans` on these two positions so they fall through to the default layer. Keep it that way.
  Exception: the `MAGNET` layer uses `MG(RET)` for its maximize binding. That is a window-management action, not a typed Enter, so leave it.
- Update the ASCII diagram comment above `bindings` in the same edit, so the diagram and bindings agree.
- If the Readme or the keymap-drawer JSON (`config/sofle_choc_pro.json`) shows the thumb row, update it too.

## Current state

Applied in both keymaps: Space is on the left inner thumb key and Enter on the right, with the diagram comments matching.
