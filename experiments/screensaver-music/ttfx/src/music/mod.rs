//! Music for the screensaver: Omadrop's audio engine drives the stock ttfx
//! effects instead of a fixed frame clock.
//!
//! The control contract has two roles, kept distinct:
//!
//! * **Time.** Every effect advances in engine ticks (one tick is one stock
//!   frame). The conductor decides how many ticks each display frame gets:
//!   none in silence, a drift between hits, and a move on each standout hit:
//!   a fixed stretch of the effect's own motion delivered with a quick start
//!   and an ease-out. Moves are kept to the strongest hits (a few per second
//!   at most), so fast music reads as distinct moves rather than a blur. The
//!   effect's own paths, easing, scenes and colors play exactly as authored,
//!   only at the pace the music sets.
//! * **Accents.** A standout hit is handed to the effect for one tick
//!   (`Cue::accent`). Effects whose choreography launches discrete things
//!   (fireworks shells, beam groups, lightning, ball drops, ...) hold a due
//!   launch until an accent arrives, and effects that reveal steadily release
//!   a burst on it, so launches and reveals land with the moves.
//!
//! These are measurements of the signal (onsets in bands, loudness), not
//! instrument or chorus recognition.

pub mod features;
pub mod fft;
pub mod rotation;
pub mod source;

use features::{Analyzer, Features, HOP};
use source::Source;

/// Most stock ticks one display frame may advance the effect. A frame that has
/// earned more banks the surplus in the carry and delivers it on later frames,
/// rather than dropping it at the cap.
pub const MAX_TICKS_PER_FRAME: u32 = 12;

/// What the music asks of the effect for the current tick.
#[derive(Debug, Clone, Copy, Default)]
pub struct Cue {
    /// False on stock runs: every effect behaves exactly as upstream.
    pub active: bool,
    /// Strength (about 0.5 to 1.6) of a standout hit heard since the last
    /// display frame; delivered on that frame's first tick only.
    pub accent: f32,
    /// Loudness relative to the recent loud passages, 0..1.
    pub energy: f32,
    /// How driving the music is right now, 0..1 (see `Music::intensity`).
    pub intensity: f32,
    /// 1 while music is playing, 0 in silence.
    pub presence: f32,
}

impl Cue {
    /// Whether a due launch may fire this tick. Stock runs launch at once.
    /// With music a launch waits for an accent; `waited` counts the ticks it
    /// has been held, and after `patience` ticks of music without one it
    /// fires anyway, so passages with no clear onsets still progress.
    pub fn launch(&self, waited: &mut u32, patience: u32) -> bool {
        if !self.active {
            return true;
        }
        if self.accent > 0.0 || *waited >= patience {
            *waited = 0;
            true
        } else {
            *waited += 1;
            false
        }
    }

    /// A stock launch count grown into a burst by the accent: about two to
    /// four times as many for a routine to a strong hit. The growth is capped
    /// so a very strong hit does not overdrive the effect into a flash.
    pub fn burst(&self, count: i64) -> i64 {
        if !self.active || self.accent <= 0.0 {
            return count;
        }
        let drive = 0.6 + 0.8 * self.intensity;
        let growth = ((1.0 + 2.0 * self.accent) * drive).clamp(1.0, 4.0);
        ((count as f32 * growth).round() as i64).max(1)
    }

    /// Scale a stock launch count by the accent's strength (at least 1).
    pub fn scale(&self, count: i64) -> i64 {
        if !self.active || self.accent <= 0.0 {
            return count;
        }
        ((count as f32 * (0.55 + 0.6 * self.accent)).round() as i64).max(1)
    }
}

/// Tunable mapping from measurements to the effect clock. Continuous rates are
/// ticks per second (one tick is one stock frame; the product uses 120 Hz,
/// so 120/s is stock speed there);
/// move sizes are ticks, and the rest are seconds. Rates are scaled by the
/// frame's elapsed time, so the conductor paces the same at any display rate.
#[derive(Debug, Clone, Copy)]
pub struct Tuning {
    /// Ticks per second the effect drifts between moves, in sparse and in
    /// driving music. The floor is the minimum progress while quiet music
    /// plays, so a slow song still finishes its choreography.
    pub drift_floor: f32,
    pub drift_gain: f32,
    /// Ticks one standout hit moves the effect, in sparse and driving music
    /// (scaled by how strong the hit is).
    pub move_base: f32,
    pub move_gain: f32,
    /// Ticks a routine kick or snare, and a hat, nudge the effect.
    pub nudge: f32,
    pub hat_nudge: f32,
    /// Seconds for a move to ease out (to 1/e of what is left).
    pub move_ease: f32,
    /// Moves are hits above this fraction of the recent hits' strengths, and
    /// at least `move_spacing` seconds apart.
    pub move_rank: f32,
    pub move_spacing: f32,
    /// An effect that has played longer than `finish_after` seconds of music
    /// drifts faster by `finish_rate` ticks/s per extra second, so it
    /// completes. The paused clock does not count toward this age.
    pub finish_after: f32,
    pub finish_rate: f32,
    /// Ceiling on the continuous drift rate, ticks per second.
    pub max_tempo: f32,
    /// Silence longer than this enters the screensaver idle drift. Audio alone
    /// cannot distinguish a stopped player from a long pause or quiet break.
    /// lets the effects drift on at `idle_tempo` ticks/s, like the plain
    /// screensaver.
    pub idle_after: f32,
    pub idle_tempo: f32,
}

impl Tuning {
    /// Overrides from OMADROP_MUSIC_TUNING, e.g. "move_base=30,drift_floor=0.2"
    /// (review experiments; unknown keys are ignored).
    pub fn from_env() -> Tuning {
        let mut t = Tuning::default();
        if let Ok(spec) = std::env::var("OMADROP_MUSIC_TUNING") {
            for pair in spec.split(',') {
                let mut kv = pair.splitn(2, '=');
                let (Some(key), Some(value)) = (kv.next(), kv.next()) else {
                    continue;
                };
                let Ok(value) = value.trim().parse::<f32>() else {
                    continue;
                };
                let field = match key.trim() {
                    "drift_floor" => &mut t.drift_floor,
                    "drift_gain" => &mut t.drift_gain,
                    "move_base" => &mut t.move_base,
                    "move_gain" => &mut t.move_gain,
                    "nudge" => &mut t.nudge,
                    "hat_nudge" => &mut t.hat_nudge,
                    "move_ease" => &mut t.move_ease,
                    "move_rank" => &mut t.move_rank,
                    "move_spacing" => &mut t.move_spacing,
                    "finish_after" => &mut t.finish_after,
                    "finish_rate" => &mut t.finish_rate,
                    "max_tempo" => &mut t.max_tempo,
                    "idle_after" => &mut t.idle_after,
                    "idle_tempo" => &mut t.idle_tempo,
                    _ => continue,
                };
                *field = value;
            }
        }
        t.sanitized()
    }

    /// Clamp a tuning to ranges the conductor can actually run with. Env
    /// overrides and experiments must not divide by zero (`move_ease`), invert
    /// the ranking, or crank the clock without bound.
    pub fn sanitized(mut self) -> Tuning {
        self.drift_floor = Self::clean(self.drift_floor, 0.0, 600.0, 60.0);
        self.drift_gain = Self::clean(self.drift_gain, 0.0, 600.0, 12.0);
        self.move_base = Self::clean(self.move_base, 0.0, 600.0, 30.0);
        self.move_gain = Self::clean(self.move_gain, 0.0, 600.0, 22.0);
        self.nudge = Self::clean(self.nudge, 0.0, 600.0, 5.0);
        self.hat_nudge = Self::clean(self.hat_nudge, 0.0, 600.0, 2.0);
        self.move_ease = Self::clean(self.move_ease, 0.02, 2.0, 0.12);
        self.move_rank = Self::clean(self.move_rank, 0.0, 1.0, 0.5);
        self.move_spacing = Self::clean(self.move_spacing, 0.0, 10.0, 0.28);
        self.finish_after = Self::clean(self.finish_after, 0.0, 3600.0, 8.0);
        self.finish_rate = Self::clean(self.finish_rate, 0.0, 600.0, 9.0);
        self.max_tempo = Self::clean(self.max_tempo, 1.0, 600.0, 180.0);
        self.idle_after = Self::clean(self.idle_after, 0.0, 3600.0, 6.0);
        self.idle_tempo = Self::clean(self.idle_tempo, 0.0, self.max_tempo, 42.0);
        self
    }

    fn clean(value: f32, lo: f32, hi: f32, fallback: f32) -> f32 {
        if value.is_finite() {
            value.clamp(lo, hi)
        } else {
            fallback
        }
    }
}

impl Default for Tuning {
    fn default() -> Self {
        Tuning {
            drift_floor: 60.0,
            drift_gain: 12.0,
            move_base: 30.0,
            move_gain: 22.0,
            nudge: 5.0,
            hat_nudge: 2.0,
            move_ease: 0.12,
            move_rank: 0.5,
            move_spacing: 0.28,
            finish_after: 8.0,
            finish_rate: 9.0,
            max_tempo: 180.0,
            idle_after: 6.0,
            idle_tempo: 42.0,
        }
    }
}

/// Per-display-frame state written to the review log.
#[derive(Debug, Clone, Copy, Default)]
pub struct Frame {
    pub time: f64,
    /// Stock ticks granted this display frame, including fractional progress.
    /// Multiply by display frames/second for ticks/s; at 120 Hz, 1 is stock speed.
    pub tempo: f32,
    pub ticks: u32,
    pub presence: f32,
    pub energy: f32,
    pub intensity: f32,
    /// Ticks still to come from the current move.
    pub push: f32,
    pub accent: f32,
    pub kick: bool,
    pub snare: bool,
    pub hat: bool,
    pub rms: f32,
}

pub struct Music {
    source: Source,
    analyzer: Analyzer,
    /// Captured samples not analyzed yet.
    pending: Vec<f32>,
    /// Analyzed frames waiting for the speaker delay: (ready time, features).
    queue: std::collections::VecDeque<(f64, Features)>,
    delay: f64,
    pub tuning: Tuning,
    /// Review comparison: analyze and log as usual, but run the effect at the
    /// stock one tick per frame with no cues.
    pub stock: bool,
    // presentation state
    rms_fast: f32,
    reference: f32,
    presence: f32,
    /// Ticks owed by moves and nudges, delivered with an ease-out.
    owed: f32,
    accent: f32,
    carry: f32,
    last_time: f64,
    /// Music clock time of the latest analysis frame applied.
    last_heard: f64,
    /// Consecutive analysis frames of digital silence.
    silent_hops: u32,
    /// When the current silence began, and whether any music was heard yet.
    silent_since: Option<f64>,
    heard_music: bool,
    /// Whether music was playing on the previous display frame (to notice a
    /// resume, when the old ranking history no longer applies).
    was_playing: bool,
    idle: f32,
    /// Slow loudness follower (absolute level, for intensity).
    loud: f32,
    /// Recent hits: (music time, density weight, strength; hats have none).
    hits: std::collections::VecDeque<(f64, f32, f32)>,
    /// Running onset flux of kicks, snares and hats when they hit, so a hit
    /// that stands out from the recent ones counts as stronger.
    hit_flux: [f32; 3],
    last_move: f64,
    intensity: f32,
    /// Music clock time the current effect started, and how much of that span
    /// the music was paused (the finishing clock counts music played, not wall
    /// time, so a pause cannot age an effect toward its finish boost).
    effect_start: f64,
    effect_paused: f64,
    pub frame: Frame,
}

impl Music {
    pub fn new(source: Source, delay_ms: u32) -> Music {
        Music {
            source,
            analyzer: Analyzer::new(),
            pending: Vec::new(),
            queue: std::collections::VecDeque::new(),
            delay: delay_ms as f64 / 1000.0,
            tuning: Tuning::from_env(),
            stock: false,
            rms_fast: 0.0,
            reference: 0.05,
            presence: 0.0,
            owed: 0.0,
            accent: 0.0,
            carry: 0.0,
            last_time: 0.0,
            last_heard: f64::NEG_INFINITY,
            // silent until something is heard
            silent_hops: 3,
            silent_since: Some(0.0),
            heard_music: false,
            was_playing: false,
            idle: 0.0,
            loud: 0.0,
            hits: std::collections::VecDeque::new(),
            hit_flux: [0.0; 3],
            last_move: f64::NEG_INFINITY,
            intensity: 0.0,
            effect_start: 0.0,
            effect_paused: 0.0,
            frame: Frame::default(),
        }
    }

    /// A new effect begins now (its finishing drift starts over).
    pub fn effect_started(&mut self, now: f64) {
        self.effect_start = now;
        self.effect_paused = 0.0;
    }

    /// Advance to `now` (seconds on the music clock) and return the ticks the
    /// effect gets this display frame, at most `max_ticks` (see
    /// `MAX_TICKS_PER_FRAME`); the rest stays banked in the carry.
    pub fn advance(&mut self, now: f64, max_ticks: u32) -> u32 {
        let ready = self.source.pull(now, &mut self.pending);
        let hops = self.pending.len() / (HOP * 2);
        for k in 0..hops {
            let hop = &self.pending[k * HOP * 2..(k + 1) * HOP * 2];
            let f = *self.analyzer.process_stereo(hop);
            // fixtures are exact: a hop is heard at its audio time; live
            // hops are stamped when they arrived from the recorder
            let at = match self.source {
                Source::Fixture(_) => f.time,
                Source::Live(_) => ready,
            };
            self.queue.push_back((at, f));
        }
        self.pending.drain(..hops * HOP * 2);

        let dt = ((now - self.last_time) as f32).clamp(0.0, 0.1);
        self.last_time = now;
        let t = self.tuning;
        // a second call for the same moment (the hold before an effect hands
        // over to it) keeps the onsets already heard, for the review log
        let again = self.frame.time == now;
        let mut kick = again && self.frame.kick;
        let mut snare = again && self.frame.snare;
        let mut hat = again && self.frame.hat;
        let mut rms = self.frame.rms;
        let mut resumed_on_hop = false;
        while let Some(&(at, f)) = self.queue.front() {
            if at + self.delay > now {
                break;
            }
            self.queue.pop_front();
            self.last_heard = now;
            kick |= f.kick;
            snare |= f.snare;
            hat |= f.hat;
            // Reset the old song's ranking BEFORE evaluating the first new hit.
            if f.rms >= 1e-4 && (self.silent_hops >= 3 || !self.was_playing)
                && !resumed_on_hop
            {
                self.hits.clear();
                self.hit_flux = [0.0; 3];
                self.last_move = f64::NEG_INFINITY;
                resumed_on_hop = true;
            }
            self.hear_hits(at, &f);
            rms = f.rms;
            self.silent_hops = if f.rms < 1e-4 { self.silent_hops + 1 } else { 0 };
            // loudness: fast follower against a slowly released reference
            let hop = HOP as f32 / features::SAMPLE_RATE as f32;
            self.rms_fast += (f.rms - self.rms_fast) * (1.0 - (-hop / 0.08).exp());
            self.loud += (f.rms - self.loud) * (1.0 - (-hop / 0.3).exp());
            self.reference = (self.reference * (-hop / 14.0).exp()).max(self.rms_fast).max(0.004);
        }
        // An idle output can stop delivering samples altogether: no analysis
        // for a moment is silence, not the last level held forever.
        if now - self.last_heard > 0.15 {
            rms = 0.0;
            self.rms_fast = 0.0;
        }
        // Paused playback is digital silence: three silent analysis frames
        // (50 ms) stop the clock at once, without a coast.
        let playing = self.silent_hops < 3 && now - self.last_heard <= 0.15;
        if !playing {
            // a stale accent or move debt must not survive the silence and
            // fire when music resumes
            self.owed = 0.0;
            // Clear music debt on pause, but retain fractional idle drift.
            if self.was_playing || self.idle == 0.0 {
                self.carry = 0.0;
            }
            self.accent = 0.0;
            self.effect_paused += dt as f64;
        }
        let target = if playing { 1.0 } else { 0.0 };
        self.presence += (target - self.presence) * (1.0 - (-dt / 0.03).exp());
        // Short silence holds the frame. After idle_after seconds of silence
        // (or starting before any music), drift resumes. This audio-only clock
        // cannot distinguish a long pause from stopped playback.
        if playing {
            self.silent_since = None;
            self.heard_music = true;
            self.idle = 0.0;
        } else if self.silent_since.is_none() {
            self.silent_since = Some(now);
        }
        // Resumed music may be another song: hits from before the pause say
        // nothing about the new one's ranking, so start the history over.
        if playing && !self.was_playing && !resumed_on_hop {
            self.hits.clear();
            self.hit_flux = [0.0; 3];
            self.last_move = f64::NEG_INFINITY;
        }
        self.was_playing = playing;
        let idle_wait = if self.heard_music { t.idle_after } else { 0.75 };
        let idle_target = match self.silent_since {
            Some(since) if (now - since) as f32 > idle_wait => t.idle_tempo,
            _ => 0.0,
        };
        self.idle += (idle_target - self.idle) * (1.0 - (-dt / 0.6).exp());

        let energy = (self.rms_fast / self.reference).clamp(0.0, 1.0);
        // Intensity: how driving the music is, from its absolute level (a
        // mastered dance track sits near -13 dBFS, solo piano near -26), how
        // many hits it has (hats count half) and its level against its own
        // recent loud passages.
        let heard = now - self.delay;
        while self.hits.front().is_some_and(|h| h.0 < heard - 3.0) {
            self.hits.pop_front();
        }
        let recent = self.hits.iter().filter(|h| h.0 >= heard - 2.0);
        let density = (recent.map(|h| h.1).sum::<f32>() / 2.0 / 7.0).min(1.0);
        let loud_db = 20.0 * (self.loud + 1e-9).log10();
        let level = ((loud_db + 34.0) / 22.0).clamp(0.0, 1.0);
        let intensity_target = if playing {
            0.4 * level + 0.35 * density + 0.25 * energy
        } else {
            0.0
        };
        let tau = if intensity_target > self.intensity { 0.2 } else { 0.6 };
        self.intensity += (intensity_target - self.intensity) * (1.0 - (-dt / tau).exp());

        // the drift between moves, plus a push to finish a long effect. The
        // age counts music played (a pause is subtracted), and the rate is
        // ticks per second, scaled by dt so the pace is frame-rate independent.
        let age = ((now - self.effect_start) - self.effect_paused).max(0.0) as f32;
        let finishing = (age - t.finish_after).max(0.0) * t.finish_rate;
        let drift =
            (t.drift_floor + t.drift_gain * self.intensity + finishing).min(t.max_tempo);
        // moves ease out: each frame delivers a share of what is owed (the
        // exponential is in dt, so delivery is frame-rate independent too)
        let delivered = self.owed * (1.0 - (-dt / t.move_ease).exp()) * self.presence;
        self.owed = (self.owed - delivered).max(0.0);
        let tempo = if playing { self.presence * drift * dt } else { 0.0 }
            + delivered
            + (1.0 - self.presence) * self.idle.min(t.max_tempo) * dt;
        self.carry += tempo;
        let mut ticks = self.carry.floor();
        // Only advance what this frame is allowed to; the rest stays banked in
        // the carry, so the cap never drops a tick that was already owed.
        if ticks > max_ticks as f32 {
            ticks = max_ticks as f32;
        }
        self.carry -= ticks;
        if self.stock {
            self.carry = 0.0;
            ticks = 1.0;
        }

        self.frame = Frame {
            time: now,
            tempo,
            ticks: ticks as u32,
            presence: self.presence,
            energy,
            intensity: self.intensity,
            push: self.owed,
            accent: self.accent,
            kick,
            snare,
            hat,
            rms,
        };
        ticks as u32
    }

    /// Weigh one analysis frame's hits: standout kicks and snares become
    /// moves (and accents), the rest nudge.
    fn hear_hits(&mut self, at: f64, f: &Features) {
        let t = self.tuning;
        let fired = [f.kick, f.snare, f.hat];
        let flux = [
            f.flux[0].max(f.flux[1]),
            0.25 * f.flux[2] + 0.35 * f.flux[3] + 0.40 * f.flux[4],
            f.flux[5],
        ];
        let impact = [f.kick_impact, f.snare_impact, f.hat_impact];
        let mut strongest = 0.0f32;
        for i in 0..3 {
            if !fired[i] {
                continue;
            }
            let average = &mut self.hit_flux[i];
            if *average <= 0.0 {
                *average = flux[i];
            }
            let salience = (flux[i] / average.max(1e-3)).clamp(0.5, 2.5);
            *average += (flux[i] - *average) * 0.15;
            if i == 2 {
                self.owed += t.hat_nudge;
                self.hits.push_back((at, 0.5, 0.0));
                continue;
            }
            // kicks carry the beat: they count a little stronger
            let strength = impact[i] * salience.powf(1.2) * if i == 0 { 1.2 } else { 1.0 };
            strongest = strongest.max(strength);
        }
        if strongest <= 0.0 {
            return;
        }
        // rank against the recent kicks and snares
        let mut recent: Vec<f32> = self
            .hits
            .iter()
            .filter(|h| h.2 > 0.0)
            .map(|h| h.2)
            .collect();
        let rank = if recent.len() < 4 {
            1.0
        } else {
            recent.sort_by(|a, b| a.total_cmp(b));
            recent.iter().filter(|&&s| s < strongest).count() as f32 / recent.len() as f32
        };
        self.hits.push_back((at, 1.0, strongest));
        let spaced = at - self.last_move >= t.move_spacing as f64;
        if rank >= t.move_rank && spaced {
            self.last_move = at;
            let typical = if recent.is_empty() {
                strongest
            } else {
                recent[recent.len() / 2]
            };
            let size = (strongest / typical.max(1e-3)).clamp(0.7, 1.6);
            self.owed += (t.move_base + t.move_gain * self.intensity) * size;
            self.accent = self.accent.max(strongest.clamp(0.5, 1.6));
        } else {
            self.owed += t.nudge;
        }
    }

    /// The cue for this frame's ticks; the accent goes to the first only.
    pub fn cue(&mut self) -> Cue {
        Cue {
            active: !self.stock,
            accent: self.accent,
            energy: self.frame.energy,
            intensity: self.intensity,
            presence: self.presence,
        }
    }

    /// How driving the music is right now, 0..1.
    pub fn intensity(&self) -> f32 {
        self.intensity
    }

    /// No music for a while: the effects drift on their own.
    pub fn idling(&self) -> bool {
        self.idle > 0.05 && self.presence < 0.5
    }

    /// A standout hit is waiting for the next tick.
    pub fn has_accent(&self) -> bool {
        self.accent > 0.0
    }

    /// The accent was delivered (or there were no ticks to deliver it to and
    /// it is kept for the next frame that has one).
    pub fn accent_delivered(&mut self) {
        self.accent = 0.0;
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    /// A conductor fed by a zero-length fixture: the tests drive its state
    /// directly rather than synthesizing audio.
    fn silent_music() -> Music {
        let path = std::env::temp_dir().join(format!(
            "ttfx-music-test-{}-{}.raw",
            std::process::id(),
            std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .unwrap()
                .as_nanos()
        ));
        std::fs::write(&path, []).unwrap();
        let fixture = source::Fixture::open(&path, 0.0).unwrap();
        let _ = std::fs::remove_file(&path);
        let mut music = Music::new(Source::Fixture(fixture), 0);
        music.tuning = Tuning::default();
        music
    }

    /// One display frame with music heard: force the playing state the audio
    /// path would otherwise set from analyzed hops.
    fn play_frame(music: &mut Music, now: f64) -> u32 {
        music.silent_hops = 0;
        music.heard_music = true;
        music.last_heard = now;
        music.advance(now, MAX_TICKS_PER_FRAME)
    }

    fn total_ticks(hz: u32, seconds: f64) -> u32 {
        let mut music = silent_music();
        music.owed = 200.0;
        let frames = (seconds * hz as f64) as u32;
        let mut ticks = 0;
        for i in 0..frames {
            let now = (i + 1) as f64 / hz as f64;
            music.presence = 1.0;
            ticks += play_frame(&mut music, now);
        }
        ticks
    }

    #[test]
    fn drift_and_moves_pace_the_same_at_any_frame_rate() {
        let slow = total_ticks(60, 2.0);
        let fast = total_ticks(120, 2.0);
        assert!(
            (slow as i32 - fast as i32).abs() <= 3,
            "60 Hz delivered {slow}, 120 Hz delivered {fast}"
        );
    }

    #[test]
    fn the_frame_cap_banks_move_ticks_instead_of_dropping_them() {
        let mut music = silent_music();
        music.presence = 1.0;
        music.owed = 500.0;
        let first = play_frame(&mut music, 1.0 / 120.0);
        assert_eq!(first, MAX_TICKS_PER_FRAME);
        let mut total = first;
        for i in 2..600 {
            let now = i as f64 / 120.0;
            music.presence = 1.0;
            total += play_frame(&mut music, now);
        }
        assert!(music.owed < 0.5, "move debt left behind: {}", music.owed);
        assert!(total >= 500, "delivered {total} of a 500-tick move debt");
    }

    #[test]
    fn silence_freezes_and_drops_a_stale_accent() {
        let mut music = silent_music();
        music.heard_music = true;
        music.presence = 1.0;
        music.owed = 40.0;
        music.carry = 100.0; // previously capped movement must stop too
        music.accent = 1.0;
        music.silent_hops = 5;
        music.last_heard = -100.0;
        music.silent_since = Some(100.0);
        let ticks = music.advance(100.0, MAX_TICKS_PER_FRAME);
        assert_eq!(ticks, 0);
        assert!(!music.has_accent(), "an accent survived the silence");
        assert_eq!(music.owed, 0.0);
        assert_eq!(music.carry, 0.0);
    }

    #[test]
    fn silence_holds_then_idle_drift_accumulates() {
        let mut music = silent_music();
        music.heard_music = true;
        music.was_playing = true;
        music.carry = 100.0;
        let mut ticks = 0;
        for frame in 1..=480 {
            ticks += music.advance(frame as f64 / 120.0, MAX_TICKS_PER_FRAME);
        }
        assert_eq!(ticks, 0, "four-second pause must stay still");
        for frame in 481..=1440 {
            ticks += music.advance(frame as f64 / 120.0, MAX_TICKS_PER_FRAME);
        }
        assert!(ticks > 200, "idle drift lost fractional ticks: {ticks}");
    }

    #[test]
    fn the_finish_clock_counts_music_played_not_wall_time() {
        let mut music = silent_music();
        music.effect_started(0.0);
        for i in 0..600 {
            let now = (i + 1) as f64 / 120.0;
            music.presence = 1.0;
            play_frame(&mut music, now);
        }
        assert!(music.effect_paused.abs() < 1e-9, "paused while playing");

        music.silent_hops = 5;
        music.last_heard = -100.0;
        for i in 0..600 {
            let now = 5.0 + (i + 1) as f64 * 0.05;
            music.advance(now, MAX_TICKS_PER_FRAME);
        }
        let age = (35.0 - music.effect_start) - music.effect_paused;
        assert!(
            age < 5.05,
            "a 30 s pause aged the effect to {age} s of finish clock"
        );
        assert!((music.effect_paused - 30.0).abs() < 0.1);
    }

    #[test]
    fn quiet_playing_music_still_progresses() {
        let mut music = silent_music();
        let mut ticks = 0;
        for i in 0..120 {
            let now = (i + 1) as f64 / 120.0;
            music.presence = 1.0;
            ticks += play_frame(&mut music, now);
        }
        let floor = music.tuning.drift_floor;
        assert!(
            ticks as f32 >= floor - 1.0,
            "quiet music stalled: {ticks} ticks in 1 s (floor {floor})"
        );
        assert!(
            ticks as f32 <= floor + 2.0,
            "quiet music raced: {ticks} ticks in 1 s (floor {floor})"
        );
    }

    #[test]
    fn resumed_music_forgets_the_old_ranking() {
        let mut music = silent_music();
        music.presence = 1.0;
        play_frame(&mut music, 1.0 / 120.0);
        music.hits.push_back((2.9, 1.0, 1.0));
        music.hit_flux = [1.0, 1.0, 1.0];
        music.last_move = 2.9;

        // pause, then resume on what may be a different song. The resume uses
        // a small step so the fixture's own silence does not stand in for the
        // music that would reset `silent_hops` in a real run.
        music.silent_hops = 5;
        music.last_heard = -100.0;
        music.presence = 1.0;
        music.advance(2.0, MAX_TICKS_PER_FRAME);
        assert!(!music.hits.is_empty(), "history trimmed before the resume");

        music.presence = 1.0;
        play_frame(&mut music, 2.001);
        assert!(music.hits.is_empty(), "ranking history survived the resume");
        assert_eq!(music.hit_flux, [0.0; 3]);
        assert_eq!(music.last_move, f64::NEG_INFINITY);
    }

    #[test]
    fn invalid_tuning_values_are_clamped() {
        let bad = Tuning {
            move_ease: 0.0,
            max_tempo: -3.0,
            move_rank: 4.0,
            move_spacing: -1.0,
            drift_floor: f32::NAN,
            idle_tempo: 1e9,
            ..Tuning::default()
        }
        .sanitized();
        assert!(bad.move_ease > 0.0 && bad.move_ease.is_finite());
        assert!(bad.max_tempo >= 1.0 && bad.max_tempo.is_finite());
        assert!((0.0..=1.0).contains(&bad.move_rank));
        assert!(bad.move_spacing >= 0.0);
        assert!(bad.drift_floor.is_finite() && bad.drift_floor >= 0.0);
        assert!(bad.idle_tempo <= bad.max_tempo);
    }
}
