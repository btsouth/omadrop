//! Omadrop's audio engine (experiments/projectm-ascii/audio_features.h,
//! AudioFeatureBus) ported to Rust: six-role band flux, kick/snare/hat onset
//! detection with impact strengths, absolute bass body and the onset
//! autocorrelation beat clock. Same window, hop, bands, thresholds and decay
//! constants, so the screensaver hears music the way the MilkDrop app does.
//! The spectrum/chroma/harmonic outputs the screensaver does not use are left out.

use super::fft::Fft;

pub const SAMPLE_RATE: usize = 44100;
pub const WINDOW: usize = 2048;
/// One analysis frame per 1/60 s of audio.
pub const HOP: usize = SAMPLE_RATE / 60;
const ROLES: usize = 6;
const EDGES: [f32; ROLES + 1] = [25.0, 70.0, 150.0, 400.0, 1500.0, 4000.0, 12000.0];
const HISTORY: usize = 3600;
const MIN_LAG: usize = 19; // 190 BPM at 60 frames per second
const MAX_LAG: usize = 58; // 62 BPM

#[derive(Clone, Copy, Debug, Default)]
pub struct Features {
    /// Audio time at the end of this frame's window.
    pub time: f64,
    pub level: [f32; ROLES],
    pub flux: [f32; ROLES],
    pub bass_body: f32,
    pub kick: bool,
    pub snare: bool,
    pub hat: bool,
    pub kick_impact: f32,
    pub snare_impact: f32,
    pub hat_impact: f32,
    /// RMS of the mid channel over this hop.
    pub rms: f32,
    pub bpm: f32,
    pub beat_phase: f32,
    pub beat_confidence: f32,
    pub beat_crossed: bool,
}

pub struct Analyzer {
    fft: Fft,
    window: Vec<f32>,
    samples: Vec<f32>,
    re: Vec<f32>,
    im: Vec<f32>,
    previous: [f32; ROLES],
    flux_mean: [f32; ROLES],
    level_mean: [f32; ROLES],
    frame: u64,
    processed: u64,
    kick_cooldown: u32,
    snare_cooldown: u32,
    hat_cooldown: u32,
    onset_history: Vec<f32>,
    tempo_votes: [f32; MAX_LAG + 1],
    history_frames: usize,
    beat_period: f32,
    beat_index: u64,
    f: Features,
}

impl Default for Analyzer {
    fn default() -> Self {
        Self::new()
    }
}

impl Analyzer {
    pub fn new() -> Self {
        let window = (0..WINDOW)
            .map(|i| {
                0.5 - 0.5 * (2.0 * std::f32::consts::PI * i as f32 / (WINDOW - 1) as f32).cos()
            })
            .collect();
        Analyzer {
            fft: Fft::new(WINDOW),
            window,
            samples: vec![0.0; WINDOW],
            re: vec![0.0; WINDOW],
            im: vec![0.0; WINDOW],
            previous: [0.0; ROLES],
            flux_mean: [0.0; ROLES],
            level_mean: [0.0; ROLES],
            frame: 0,
            processed: 0,
            kick_cooldown: 0,
            snare_cooldown: 0,
            hat_cooldown: 0,
            onset_history: vec![0.0; HISTORY],
            tempo_votes: [0.0; MAX_LAG + 1],
            history_frames: 0,
            beat_period: 30.0,
            beat_index: 0,
            f: Features {
                bpm: 120.0,
                ..Features::default()
            },
        }
    }

    /// One hop of interleaved stereo (`HOP` frames).
    pub fn process_stereo(&mut self, stereo: &[f32]) -> &Features {
        let accepted = (stereo.len() / 2).min(WINDOW);
        self.samples.copy_within(accepted.., 0);
        let start = WINDOW - accepted;
        let mut energy = 0.0f32;
        for i in 0..accepted {
            let middle = 0.5 * (stereo[i * 2] + stereo[i * 2 + 1]);
            self.samples[start + i] = middle;
            energy += middle * middle;
        }
        self.f.rms = (energy / accepted.max(1) as f32).sqrt();
        self.processed += accepted as u64;
        self.f.time = self.processed as f64 / SAMPLE_RATE as f64;
        self.analyze();
        &self.f
    }

    fn analyze(&mut self) {
        for i in 0..WINDOW {
            self.re[i] = self.samples[i] * self.window[i];
            self.im[i] = 0.0;
        }
        self.fft.forward(&mut self.re, &mut self.im);
        let (re, im) = (&self.re, &self.im);
        let bin_of = |hz: f32| (hz * WINDOW as f32 / SAMPLE_RATE as f32) as usize;

        let mut magnitude = [0.0f32; ROLES];
        for role in 0..ROLES {
            let first = bin_of(EDGES[role]).max(1);
            let last = bin_of(EDGES[role + 1]).min(WINDOW / 2);
            let mut sum = 0.0;
            for bin in first..=last {
                sum += (re[bin] * re[bin] + im[bin] * im[bin]).sqrt().ln_1p();
            }
            magnitude[role] = sum / (last + 1 - first).max(1) as f32;
        }
        let mut bass_power = 0.0f32;
        for bin in 1..=bin_of(150.0) {
            bass_power += re[bin] * re[bin] + im[bin] * im[bin];
        }
        let f = &mut self.f;
        // Absolute low-frequency amplitude, not divided by its own history.
        f.bass_body = 1.0 - (-8.0 * bass_power.sqrt() / WINDOW as f32).exp();

        f.kick = false;
        f.snare = false;
        f.hat = false;
        f.kick_impact *= 0.88;
        f.snare_impact *= 0.84;
        f.hat_impact *= 0.72;
        let mut positive_flux = [0.0f32; ROLES];
        let mut detection = [0.0f32; ROLES];
        for role in 0..ROLES {
            let positive = (magnitude[role] - self.previous[role]).max(0.0);
            positive_flux[role] = positive;
            self.flux_mean[role] = self.flux_mean[role] * 0.94 + positive * 0.06;
            self.level_mean[role] = self.level_mean[role] * 0.992 + magnitude[role] * 0.008;
            f.flux[role] = positive / self.flux_mean[role].max(0.003);
            detection[role] = magnitude[role] / self.level_mean[role].max(0.004);
            f.level[role] = magnitude[role] / self.level_mean[role].max(0.02);
            self.previous[role] = magnitude[role];
        }

        let kick_flux = f.flux[0].max(f.flux[1]);
        let snare_flux = 0.25 * f.flux[2] + 0.35 * f.flux[3] + 0.40 * f.flux[4];
        let hat_flux = f.flux[5];
        let low_change = positive_flux[0].max(positive_flux[1]);
        let middle_change =
            0.25 * positive_flux[2] + 0.35 * positive_flux[3] + 0.40 * positive_flux[4];
        let high_change = positive_flux[5];
        let low_energy = magnitude[0].max(magnitude[1]);
        let middle_energy = 0.35 * magnitude[2] + 0.35 * magnitude[3] + 0.30 * magnitude[4];
        let high_energy = magnitude[5];
        let impact = |level: f32, threshold: f32| {
            0.45 + 0.90 * ((level - threshold) / 24.0).clamp(0.0, 1.0)
        };
        let warmed_up = self.frame >= 12;
        if warmed_up
            && self.kick_cooldown == 0
            && kick_flux > 2.25
            && low_energy > 0.030
            && detection[0].max(detection[1]) > 1.08
            && low_change > middle_change * 1.25
            && low_change > high_change * 1.45
            && low_energy > middle_energy * 1.20
            && low_energy > high_energy * 1.40
        {
            f.kick = true;
            f.kick_impact = impact(detection[0].max(detection[1]), 1.08);
            self.kick_cooldown = 16;
        }
        if warmed_up
            && self.snare_cooldown == 0
            && snare_flux > 1.45
            && detection[3].max(detection[4]) > 1.03
            && middle_change > low_change * 0.72
            && middle_change > high_change * 0.92
        {
            f.snare = true;
            f.snare_impact = impact(detection[3].max(detection[4]), 1.06);
            self.snare_cooldown = 10;
        }
        if warmed_up
            && self.hat_cooldown == 0
            && hat_flux > 2.0
            && detection[5] > 1.05
            && high_change > low_change * 0.55
            && high_change > middle_change * 0.88
        {
            f.hat = true;
            f.hat_impact = impact(detection[5], 1.05);
            self.hat_cooldown = 5;
        }
        self.kick_cooldown = self.kick_cooldown.saturating_sub(1);
        self.snare_cooldown = self.snare_cooldown.saturating_sub(1);
        self.hat_cooldown = self.hat_cooldown.saturating_sub(1);
        self.update_clock();
        self.frame += 1;
    }

    fn update_clock(&mut self) {
        self.onset_history.copy_within(1.., 0);
        let onset: f32 = self
            .f
            .flux
            .iter()
            .map(|&flux| (flux - 0.8).clamp(0.0, 5.0).ln_1p())
            .sum();
        *self.onset_history.last_mut().unwrap() = onset;
        self.history_frames = (self.history_frames + 1).min(HISTORY);

        if self.history_frames >= 240 && self.frame.is_multiple_of(16) {
            let mut best_score = 0.0f32;
            let mut lag_scores = [0.0f32; MAX_LAG + 1];
            let start = HISTORY - self.history_frames;
            let h = &self.onset_history;
            for lag in MIN_LAG..=MAX_LAG {
                let (mut correlation, mut left, mut right) = (0.0f32, 0.0f32, 0.0f32);
                for i in start + lag..HISTORY {
                    let (a, b) = (h[i], h[i - lag]);
                    correlation += a * b;
                    left += a * a;
                    right += b * b;
                }
                let normalized = correlation / (left * right).max(1e-8).sqrt();
                let bpm = 3600.0 / lag as f32;
                let octave = (bpm.max(1.0) / 120.0).log2();
                let prior = (-0.5 * octave * octave / (2.5 * 2.5)).exp();
                let score = normalized * prior;
                lag_scores[lag] = score;
                best_score = best_score.max(score);
            }
            if best_score > 0.08 {
                for lag in MIN_LAG..=MAX_LAG {
                    self.tempo_votes[lag] = self.tempo_votes[lag] * 0.999 + lag_scores[lag];
                }
                let mut best_lag = MIN_LAG;
                for lag in MIN_LAG + 1..=MAX_LAG {
                    if self.tempo_votes[lag] > self.tempo_votes[best_lag] {
                        best_lag = lag;
                    }
                }
                let voted_bpm = 3600.0 / best_lag as f32;
                let double_lag = (best_lag as f32 * 0.5).round() as usize;
                if voted_bpm < 90.0
                    && double_lag >= MIN_LAG
                    && self.tempo_votes[double_lag] >= self.tempo_votes[best_lag] * 0.75
                {
                    best_lag = double_lag;
                }
                let mut estimated = best_lag as f32;
                if best_lag > MIN_LAG && best_lag < MAX_LAG {
                    let (y0, y1, y2) = (
                        self.tempo_votes[best_lag - 1],
                        self.tempo_votes[best_lag],
                        self.tempo_votes[best_lag + 1],
                    );
                    let denominator = y0 - 2.0 * y1 + y2;
                    if denominator.abs() > 1e-6 {
                        estimated += (0.5 * (y0 - y2) / denominator).clamp(-0.5, 0.5);
                    }
                }
                self.beat_period += (estimated - self.beat_period) * 0.22;
                self.f.beat_confidence +=
                    ((best_score * 1.35).min(1.0) - self.f.beat_confidence) * 0.25;
            } else {
                for vote in &mut self.tempo_votes {
                    *vote *= 0.999;
                }
                self.f.beat_confidence *= 0.92;
            }
        }

        let f = &mut self.f;
        f.beat_crossed = false;
        let previous = f.beat_phase;
        f.beat_phase = (f.beat_phase + 1.0 / self.beat_period.max(1.0)) % 1.0;
        if f.beat_phase < previous {
            f.beat_crossed = true;
            self.beat_index += 1;
        }
        if f.kick && f.beat_confidence >= 0.30 {
            if f.beat_phase > 0.55 {
                f.beat_phase = 0.0;
                if !f.beat_crossed {
                    f.beat_crossed = true;
                    self.beat_index += 1;
                }
            } else {
                f.beat_phase *= 0.45;
            }
        }
        f.bpm = 3600.0 / self.beat_period.max(1.0);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Synthetic kicks: a decaying 55 Hz burst every half second.
    fn kicks(seconds: f32) -> Vec<f32> {
        let n = (seconds * SAMPLE_RATE as f32) as usize;
        let mut out = Vec::with_capacity(n * 2);
        for i in 0..n {
            let t = i as f32 / SAMPLE_RATE as f32;
            let local = t % 0.5;
            let v = (2.0 * std::f32::consts::PI * 55.0 * local).sin() * (-local * 18.0).exp() * 0.8;
            out.push(v);
            out.push(v);
        }
        out
    }

    #[test]
    fn kicks_are_detected_and_silence_is_quiet() {
        let audio = kicks(6.0);
        let mut a = Analyzer::new();
        let mut detected = 0;
        for hop in audio.chunks_exact(HOP * 2) {
            if a.process_stereo(hop).kick {
                detected += 1;
            }
        }
        // twelve bursts, the first inside warm-up may be missed
        assert!((10..=12).contains(&detected), "detected {detected}");
        let silence = vec![0.0f32; HOP * 2];
        for _ in 0..60 {
            let f = a.process_stereo(&silence);
            assert!(!f.kick && !f.snare && !f.hat);
        }
        assert_eq!(a.f.rms, 0.0);
    }
}
