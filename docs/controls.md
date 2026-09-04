# Controls

| Key | Action | Persisted |
| --- | --- | --- |
| `Super + Shift + V` | Toggle Omadrop | No |
| `Super + Alt + V` | Hide or restore the secondary display | No |
| `A` | Toggle ASCII and continuous rendering | Yes |
| `N` / `P` | Request the next or previous scene | No |
| `I` | Cycle local music-response intensity | Yes |
| `B` | Cycle brightness | Yes |
| `M` | Cycle ambient motion | Yes |
| `R` | Toggle reduced motion | Yes |
| `S` | Toggle flash limit | Yes |
| `H` | Toggle high contrast | Yes |
| `D` | Cycle director profile | Yes |
| `F` | Favorite or unfavorite the current scene | Yes |
| `X` | Hide the current scene and continue | Yes |
| `Shift + X` | Restore all hidden scenes | Yes |
| `[` / `]` | Adjust the current output's sync delay by 10 ms | Yes |
| `F11` | Toggle fullscreen | No |
| `Esc` | Quit | No |

Every key works from either Omadrop window. The leader applies the request and
sends one complete control and scene snapshot to every follower.

`N` and `P` do not select a manual mode. They request one scene change, then
automatic direction continues. The status label says `AUTO: <scene>` to make
that behavior explicit.

Favorites influence close automatic choices without overriding musical fit.
Hidden scenes are excluded from automatic direction, motif recall, opening
selection, and manual navigation. Omadrop keeps at least two scenes available.

Balanced, Kinetic, Restrained, and High Contrast change scene selection only.
They do not change musical timing or bypass per-scene response limits.
