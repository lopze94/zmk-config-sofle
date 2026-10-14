---
name: space-enter-placement
description: Where Space and Enter live on the Sofle ZMK keymap (config/sofle_choc_pro.keymap). Space goes on the LEFT half's inner thumb key, Enter on the RIGHT half's inner thumb key. Use whenever editing, adding, or reviewing thumb-cluster bindings, layers, or the keymap diagrams so the two keys are never swapped or duplicated on the wrong side.
---

# Space on the left, Enter on the right

Convention for `config/sofle_choc_pro.keymap`:

| Key | Half | Position | Binding |
|---|---|---|---|
| Space | Left | Inner thumb key (the one nearest the center, left of the gap) | `&lt NUM SPACE` (tap Space, hold = NUM layer) |
| Enter | Right | Inner thumb key (the one nearest the center, right of the gap) | `&kp RET` |

## Default layer thumb row

The thumb row in the `bindings` block reads left to right:

```
//               | GUI | ALT  | CTRL | LOWER|  SPACE |  | ENTER | RAISE| CTRL  | ALT   | GUI  |
&kp LGUI &kp LALT &kp LCTRL &mo LOWER  &lt NUM SPACE  &kp RET  &mo RAISE  &kp RCTRL &kp RALT  &kp RGUI
```

The inner thumb keys are the 5th binding of the left thumb row and the 1st binding of the right thumb row.
Only the first binding in each half's thumb row changes. Do not touch the hold or layer keys around them.

## Rules

- Never put Space on the right half or Enter on the left half of the default layer.
- Space is a layer-tap. Keep the tap side as Space when changing which layer the hold activates.
- Other layers use `&trans` on these two positions so they fall through to the default layer. Keep it that way.
- Update the ASCII diagram comment above `bindings` in the same edit, so the diagram and bindings agree.
- If the Readme or the keymap-drawer JSON (`config/sofle_choc_pro.json`) shows the thumb row, update it too.

## Current state

Applied on top of Keebart's stock Sofle keymap (which ships Enter left, Space right): Space is on the left inner thumb key and Enter on the right, with the diagram comment matching.
The previous full layout (MAGNET/NUM layers) is preserved at the git tag `full-sofle-layout`.
