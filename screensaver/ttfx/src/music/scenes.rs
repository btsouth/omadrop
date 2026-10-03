//! Controls for the opt-in sustained native scenes. No source or analyzer
//! changes: consume the delayed six-band measurements the conductor heard.

use super::{features::Features, Cue};
use crate::utils::geometry::Coord;

/// Start the authored departure on an accent after 28 s; never wait past 30 s.
/// Allow at most four seconds for the stock home/wipe animation to finish.
pub const DEPART_AFTER: f64 = 28.0;
pub const DEPART_BY: f64 = 30.0;
pub const COMPLETE_BY: f64 = 34.0;

pub fn departure(cue: Cue) -> bool {
    cue.scene_age >= DEPART_BY || (cue.scene_age >= DEPART_AFTER && cue.accent > 0.0)
}

#[derive(Default)]
pub struct Levels {
    pub bands: [f32; 6],
    pub bass_body: f32,
}

impl Levels {
    pub fn advance(&mut self, f: &Features, playing: bool, dt: f32) {
        let dt = dt.clamp(0.0, 0.1);
        for (value, &raw) in self.bands.iter_mut().zip(&f.level) {
            // Relative band levels can exceed one during attacks. Smoothly
            // compress them, preserving differences without a hard plateau.
            let raw = clean(raw);
            let target = if playing { raw / (1.0 + raw) } else { 0.0 };
            follow(value, target, dt);
        }
        follow(
            &mut self.bass_body,
            if playing {
                clean(f.bass_body).min(1.0)
            } else {
                0.0
            },
            dt,
        );
    }
}

fn clean(x: f32) -> f32 {
    if x.is_finite() {
        x.max(0.0)
    } else {
        0.0
    }
}

fn follow(value: &mut f32, target: f32, dt: f32) {
    let tau = if target > *value { 0.12 } else { 0.32 };
    *value += (target - *value) * (1.0 - (-dt / tau).exp());
}

/// Change each ring's own next waypoint around its authored center. Bass
/// moves the whole formation's extent; middle/high sustained activity changes
/// alternating rings' aspect independently. Symbols, colors, path direction
/// and the stock interpolation between these points remain intact.
pub fn ring_target(coord: Coord, center: Coord, role: usize, cue: Cue) -> Coord {
    let extent = 0.60 + 0.60 * (cue.bass_body * 3.0).clamp(0.0, 1.0) as f64;
    let band = if role % 2 == 0 {
        cue.bands[3]
    } else {
        cue.bands[5]
    };
    let aspect = 0.72 + 0.38 * band.clamp(0.0, 1.0) as f64;
    Coord::new(
        center.column + ((coord.column - center.column) as f64 * extent).round() as i64,
        center.row + ((coord.row - center.row) as f64 * extent * aspect).round() as i64,
    )
}

/// Independent row/column sweep activity, in native groups per second. A
/// quiet scene retains a sparse trickle; sustained mids/highs populate their
/// own direction even without percussion. Bass controls adjacent beam width.
pub fn beam_rates(cue: Cue) -> [f64; 2] {
    [
        0.45 + 4.5 * cue.bands[3] as f64,
        0.35 + 5.0 * cue.bands[5] as f64,
    ]
}

pub fn beam_width(cue: Cue) -> usize {
    1 + ((cue.bass_body * 3.0).clamp(0.0, 1.0) * 3.0).round() as usize
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn sustained_levels_decay_and_return_without_replaying_an_onset() {
        let mut levels = Levels::default();
        let f = Features {
            level: [1.0; 6],
            bass_body: 0.3,
            ..Features::default()
        };
        for _ in 0..120 {
            levels.advance(&f, true, 1.0 / 120.0);
        }
        assert!(levels.bands[3] > 0.49);
        assert!(levels.bass_body > 0.29);
        for _ in 0..240 {
            levels.advance(&f, false, 1.0 / 120.0);
        }
        assert!(levels.bands.iter().all(|x| *x < 0.002));
        assert!(levels.bass_body < 0.001);
        levels.advance(&f, true, 1.0 / 120.0);
        assert!(levels.bands[3] > 0.02 && levels.bands[3] < 0.05);
    }

    #[test]
    fn bass_extent_band_shape_and_accent_are_independent() {
        let center = Coord::new(40, 20);
        let point = Coord::new(70, 35);
        let quiet = Cue::default();
        let bass = Cue {
            bass_body: 0.3,
            ..quiet
        };
        let mids = Cue {
            bands: [0.0, 0.0, 0.0, 1.0, 0.0, 0.0],
            ..quiet
        };
        let accent = Cue {
            accent: 1.6,
            ..quiet
        };
        let base = ring_target(point, center, 0, quiet);
        let larger = ring_target(point, center, 0, bass);
        let shaped = ring_target(point, center, 0, mids);
        assert!(larger.column > base.column && larger.row > base.row);
        assert_eq!(shaped.column, base.column);
        assert!(shaped.row > base.row);
        assert_eq!(
            ring_target(point, center, 1, mids),
            ring_target(point, center, 1, quiet)
        );
        assert_eq!(ring_target(point, center, 0, accent), base);
        assert!(beam_width(bass) > beam_width(quiet));
        assert_eq!(beam_rates(bass), beam_rates(quiet));
        assert!(beam_rates(mids)[0] > beam_rates(quiet)[0]);
        assert_eq!(beam_rates(mids)[1], beam_rates(quiet)[1]);
        assert_eq!(beam_width(mids), beam_width(quiet));
    }

    #[test]
    fn lifecycle_is_sustained_and_transition_wait_is_bounded() {
        assert!(!departure(Cue {
            scene_age: 27.9,
            accent: 1.0,
            ..Cue::default()
        }));
        assert!(!departure(Cue {
            scene_age: 29.0,
            ..Cue::default()
        }));
        assert!(departure(Cue {
            scene_age: 29.0,
            accent: 1.0,
            ..Cue::default()
        }));
        assert!(departure(Cue {
            scene_age: 30.0,
            ..Cue::default()
        }));
    }

    #[test]
    fn levels_sanitize_invalid_measurements_and_match_frame_rates() {
        let mut bad = Levels::default();
        bad.advance(
            &Features {
                level: [f32::NAN; 6],
                bass_body: f32::INFINITY,
                ..Features::default()
            },
            true,
            0.1,
        );
        assert_eq!(bad.bands, [0.0; 6]);
        assert_eq!(bad.bass_body, 0.0);
        let mut slow = Levels::default();
        let mut fast = Levels::default();
        let f = Features {
            level: [2.0; 6],
            bass_body: 0.4,
            ..Features::default()
        };
        for _ in 0..30 {
            slow.advance(&f, true, 1.0 / 30.0);
        }
        for _ in 0..120 {
            fast.advance(&f, true, 1.0 / 120.0);
        }
        assert!((slow.bands[0] - fast.bands[0]).abs() < 0.00001);
    }
}
