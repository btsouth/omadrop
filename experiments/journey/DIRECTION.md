# Journey: Six Lines

Step inside an Omarchy theme. Each chapter is a held stage that comes alive;
travel happens between worlds, without cuts or whole-frame dissolves.

Four things persist through the whole journey:

- Six strands carry the six frequency bands as travelling light: power lines
  in Osaka Jade, lines on the swell in the crossing, the stripes of the great
  wave in Kanagawa. Band 0 is the bottom strand.
- One disc carries bass: the moon, then the red sun. Its halo breathes with
  bass and rings travel outward on strong bass hits.
- One mountain: a broad cone in Osaka Jade that recedes to Fuji at sea.
- A lantern-bearer in a straw hat: walks the street, boards the boat, rides
  at the bow.

## Preview timeline (movie seconds)

| Time | Chapter |
| --- | --- |
| 0 to 28 | Osaka Jade. Blue hour, moonrise, festival lanterns strung across the street come on at 3 s, panes waking by band, the noodle cart opening, shadow play behind paper, birds landing on the wires on onsets, a train in the valley, the cyclist, a shooting star on a strong onset, the shamisen player, a gust, then the surge window (22 to 27): flock burst, the main firework pair, finale shells on the bass hits that follow, panes and lanterns flaring as each burst's light arrives, sky lanterns. |
| 28 to 40 | The crossing. Camera glide to the quay, the bearer boards, valley fog rises, land gives way to water, the wires touch down and run on the swell, moon to sun, cone to Fuji, jade to ink and paper. |
| 40 to 75 | Kanagawa. The swell heaves up into the great wave (to 64 s), leans over the boats (61 to 70 s), grows claws on onsets and flings spray; it breaks in the surge window (64 to 71 s) and the foam washes across the frame. |

Music response in Osaka Jade, so it stays visible in dense, compressed mixes:
panes, festival lanterns, the valley's windows and moths answer how far their
band stands above its own recent level (`Ctx::lift`), not only its absolute
level. Each onset moves a light one lantern along the street string; bass hits
lift the lanterns, the izakaya neon, the room lamp, the cart's steam and the
ripples on the wet street (`Ctx::kick`). Every change to Osaka is checked
against the c502bf9 baseline for music-driven motion before it is shown.

World life runs on the scene clock and continues in silence. Music changes
objects inside the timeline; story events are gated on music only inside the
surge windows, which have timed fallbacks (the Kanagawa fallback lands on the
next strong bass hit when there is one). There is no chorus or drop detection.

Rendering styles follow each theme: Osaka Jade is luminous haze, silhouettes,
warm windows, a wet reflective street and soft bloom; Kanagawa is flat ink
planes, cream linework, paper grain and little bloom. Palettes come from each
theme's `colors.toml`.
