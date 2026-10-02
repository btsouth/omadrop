//! Music effect rotation: a weighted shuffle bag over the eligible effects.
//!
//! Each round offers every eligible effect once before any repeats. The last
//! `min(AVOID, count - 1)` selections are kept out of the bag while other
//! choices remain, so a round boundary does not immediately replay what just
//! played; a one-effect collection is allowed to repeat. The conductor's
//! intensity/calm weighting and the stock duration weighting decide the order
//! within the bag, and favorites are favoured in that choice (an extra share,
//! never an extra slot in the round).
//!
//! Preferences come from Omadrop's shared `effects.conf`, read-only here: the
//! UI owns the writes.

use std::path::{Path, PathBuf};

use crate::utils::rng::Rng;

/// How many recent selections to keep out of the bag across a round boundary.
const AVOID: usize = 8;

/// Share a favorite gets in the weighted choice (not an extra slot).
const FAVORITE_PRIORITY: f64 = 2.5;

/// One effect the rotation may choose, with the stock duration weight and the
/// calm/driving classification the conductor uses.
#[derive(Debug, Clone)]
pub struct Candidate {
    pub name: String,
    pub weight: f64,
    pub calm: bool,
}

/// The shared `effects.conf` contents. Unknown keys are ignored here; unknown
/// or duplicate slugs are ignored when they are applied against the available
/// effect names.
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct Preferences {
    pub favorites: Vec<String>,
    pub hidden: Vec<String>,
}

/// Why a preferences file could not be used.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum PreferencesError {
    /// A version newer than this build understands. The file is ignored whole
    /// rather than misread.
    UnsupportedVersion(u32),
    /// The file exists but could not be read.
    Io(String),
}

impl std::fmt::Display for PreferencesError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            PreferencesError::UnsupportedVersion(v) => {
                write!(f, "unsupported effects.conf version {v}")
            }
            PreferencesError::Io(e) => write!(f, "cannot read effects.conf: {e}"),
        }
    }
}

impl Preferences {
    /// `$XDG_CONFIG_HOME/omadrop/effects.conf`, else
    /// `$HOME/.config/omadrop/effects.conf`.
    pub fn path() -> Option<PathBuf> {
        if let Some(dir) = std::env::var_os("XDG_CONFIG_HOME").filter(|d| !d.is_empty()) {
            return Some(PathBuf::from(dir).join("omadrop").join("effects.conf"));
        }
        std::env::var_os("HOME")
            .filter(|h| !h.is_empty())
            .map(|home| PathBuf::from(home).join(".config/omadrop/effects.conf"))
    }

    /// Read the default file; a missing file is an empty preference set.
    pub fn load_default() -> Result<Preferences, PreferencesError> {
        match Preferences::path() {
            Some(path) => Preferences::read(&path),
            None => Ok(Preferences::default()),
        }
    }

    /// Read one preferences file. A missing file yields the defaults; a version
    /// this build does not understand is an error, so a future file is ignored
    /// rather than misread.
    pub fn read(path: &Path) -> Result<Preferences, PreferencesError> {
        let text = match std::fs::read_to_string(path) {
            Ok(text) => text,
            Err(e) if e.kind() == std::io::ErrorKind::NotFound => {
                return Ok(Preferences::default());
            }
            Err(e) => return Err(PreferencesError::Io(e.to_string())),
        };
        Preferences::parse(&text)
    }

    /// Parse `version=1`/`favorites=`/`hidden=` lines; anything else is ignored.
    pub fn parse(text: &str) -> Result<Preferences, PreferencesError> {
        let mut prefs = Preferences::default();
        let mut version = 1u32;
        for line in text.lines() {
            let line = line.trim();
            if line.is_empty() || line.starts_with('#') {
                continue;
            }
            let Some((key, value)) = line.split_once('=') else {
                continue;
            };
            match key.trim() {
                "version" => version = value.trim().parse().unwrap_or(0),
                "favorites" => prefs.favorites = split_names(value),
                "hidden" => prefs.hidden = split_names(value),
                _ => {}
            }
        }
        if version != 1 {
            return Err(PreferencesError::UnsupportedVersion(version));
        }
        Ok(prefs)
    }
}

fn split_names(value: &str) -> Vec<String> {
    value
        .split(',')
        .map(str::trim)
        .filter(|n| !n.is_empty())
        .map(str::to_string)
        .collect()
}

/// What applying a preference set did to the pending candidates.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PrefsOutcome {
    /// At least one effect is eligible.
    Applied,
    /// Every available effect is hidden; the last eligible set (or, if there
    /// never was one, every effect) is kept so the rotation cannot run empty.
    EverythingHidden,
}

/// A weighted shuffle bag: each round offers every eligible effect once, the
/// weighting and preferences only decide the order.
pub struct Rotation {
    candidates: Vec<Candidate>,
    favorites: Vec<bool>,
    hidden: Vec<bool>,
    /// Eligible indices, in candidate order (kept as the all-hidden fallback).
    effective: Vec<usize>,
    /// Indices not yet played this round.
    bag: Vec<usize>,
    /// Indices already played this round, so an unhide cannot repeat one.
    played: Vec<bool>,
    /// The last few selections, oldest first, distinct (see `AVOID`).
    recent: Vec<usize>,
}

impl Rotation {
    pub fn new(candidates: Vec<Candidate>) -> Rotation {
        let n = candidates.len();
        let effective: Vec<usize> = (0..n).collect();
        Rotation {
            candidates,
            favorites: vec![false; n],
            hidden: vec![false; n],
            bag: effective.clone(),
            played: vec![false; n],
            effective,
            recent: Vec::new(),
        }
    }

    /// Re-read a preference set: hidden effects leave the pending candidates,
    /// favorites change the ordering, and a newly unhidden effect joins the
    /// current round if it has not already played in it.
    pub fn apply_preferences(&mut self, prefs: &Preferences) -> PrefsOutcome {
        self.favorites.fill(false);
        self.hidden.fill(false);
        for name in &prefs.favorites {
            if let Some(i) = self.index_of(name) {
                self.favorites[i] = true;
            }
        }
        for name in &prefs.hidden {
            if let Some(i) = self.index_of(name) {
                self.hidden[i] = true;
            }
        }
        let mut eligible: Vec<usize> =
            (0..self.candidates.len()).filter(|&i| !self.hidden[i]).collect();
        let outcome = if eligible.is_empty() {
            eligible = if self.effective.is_empty() {
                (0..self.candidates.len()).collect()
            } else {
                self.effective.clone()
            };
            PrefsOutcome::EverythingHidden
        } else {
            PrefsOutcome::Applied
        };
        self.effective = eligible.clone();
        // Drop now-hidden candidates, then let newly eligible ones join this
        // round unless they already played in it.
        let eligible_set = &eligible;
        self.bag.retain(|i| eligible_set.contains(i));
        for &i in eligible_set {
            if !self.played[i] && !self.bag.contains(&i) {
                self.bag.push(i);
            }
        }
        outcome
    }

    fn index_of(&self, name: &str) -> Option<usize> {
        self.candidates.iter().position(|c| c.name == name)
    }

    /// The next effect this round, avoiding the last `AVOID` selections while
    /// other choices remain. `None` only when there are no candidates at all.
    pub fn next(&mut self, intensity: f64, rng: &mut Rng) -> Option<String> {
        if self.candidates.is_empty() {
            return None;
        }
        if self.bag.is_empty() {
            self.refill();
        }
        let window = self.avoid_window();
        let pool: Vec<usize> = if window == 0 {
            self.bag.clone()
        } else {
            let avoid = &self.recent[self.recent.len().saturating_sub(window)..];
            let fresh: Vec<usize> = self
                .bag
                .iter()
                .copied()
                .filter(|i| !avoid.contains(i))
                .collect();
            if fresh.is_empty() { self.bag.clone() } else { fresh }
        };
        let chosen = self.weighted_pick(&pool, intensity, rng);
        self.bag.retain(|&i| i != chosen);
        self.played[chosen] = true;
        self.recent.retain(|&r| r != chosen);
        self.recent.push(chosen);
        if self.recent.len() > AVOID {
            self.recent.remove(0);
        }
        Some(self.candidates[chosen].name.clone())
    }

    /// Begin a new round over the current eligible set.
    fn refill(&mut self) {
        self.bag = self.effective.clone();
        self.played.fill(false);
    }

    /// How many recent selections to avoid: the single-effect case is 0, so it
    /// is allowed to repeat.
    fn avoid_window(&self) -> usize {
        AVOID.min(self.effective.len().saturating_sub(1))
    }

    fn weighted_pick(&self, pool: &[usize], intensity: f64, rng: &mut Rng) -> usize {
        let weight = |i: usize| -> f64 {
            let c = &self.candidates[i];
            let leaning = if c.calm { 1.5 - intensity } else { 0.5 + intensity };
            let favorite = if self.favorites[i] { FAVORITE_PRIORITY } else { 1.0 };
            (c.weight * leaning * favorite).max(0.0)
        };
        let total: f64 = pool.iter().map(|&i| weight(i)).sum();
        if !(total > 0.0) {
            return pool[rng.choice_index(pool.len())];
        }
        let mut pick = rng.random() * total;
        let mut chosen = pool[pool.len() - 1];
        for &i in pool {
            let w = weight(i);
            if pick < w {
                chosen = i;
                break;
            }
            pick -= w;
        }
        chosen
    }

    /// The candidate names currently eligible, in order.
    pub fn eligible_names(&self) -> Vec<&str> {
        self.effective
            .iter()
            .map(|&i| self.candidates[i].name.as_str())
            .collect()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::collections::HashSet;

    fn candidates(n: usize) -> Vec<Candidate> {
        (0..n)
            .map(|i| Candidate {
                name: format!("e{i}"),
                weight: 1.0,
                calm: i % 2 == 0,
            })
            .collect()
    }

    fn names(n: usize) -> Vec<String> {
        (0..n).map(|i| format!("e{i}")).collect()
    }

    #[test]
    fn a_round_plays_every_eligible_effect_once() {
        let mut r = Rotation::new(candidates(12));
        let mut rng = Rng::seeded(7);
        let mut seen = HashSet::new();
        for _ in 0..12 {
            let name = r.next(0.5, &mut rng).unwrap();
            assert!(seen.insert(name.clone()), "{name} repeated within a round");
        }
        assert_eq!(seen.len(), 12);
    }

    #[test]
    fn repeated_rounds_avoid_a_recent_repeat_across_the_boundary() {
        // The avoidance is a sliding window over the whole sequence, so it also
        // holds across the round boundary: nothing replays within `AVOID`
        // selections while other effects remain.
        for &n in &[2usize, 5, 9, 25] {
            let mut r = Rotation::new(candidates(n));
            let mut rng = Rng::seeded(11 + n as u64);
            let all: Vec<String> =
                (0..4 * n).map(|_| r.next(0.5, &mut rng).unwrap()).collect();
            let window = AVOID.min(n - 1);
            for i in 0..all.len() {
                for j in i + 1..(i + window + 1).min(all.len()) {
                    assert_ne!(
                        all[i], all[j],
                        "n={n}: {} replayed within {window} selections",
                        all[i]
                    );
                }
            }
        }
    }

    #[test]
    fn a_single_effect_is_allowed_to_repeat() {
        let mut r = Rotation::new(candidates(1));
        let mut rng = Rng::seeded(3);
        for _ in 0..5 {
            assert_eq!(r.next(0.5, &mut rng).as_deref(), Some("e0"));
        }
    }

    #[test]
    fn a_two_effect_set_plays_both_each_round() {
        let mut r = Rotation::new(candidates(2));
        let mut rng = Rng::seeded(5);
        for _ in 0..10 {
            let a = r.next(0.5, &mut rng).unwrap();
            let b = r.next(0.5, &mut rng).unwrap();
            assert_ne!(a, b);
        }
    }

    #[test]
    fn hiding_everything_keeps_a_usable_set() {
        let mut r = Rotation::new(candidates(4));
        let prefs = Preferences { favorites: Vec::new(), hidden: names(4) };
        assert_eq!(r.apply_preferences(&prefs), PrefsOutcome::EverythingHidden);
        let mut rng = Rng::seeded(2);
        let mut seen = HashSet::new();
        for _ in 0..4 {
            seen.insert(r.next(0.5, &mut rng).unwrap());
        }
        assert_eq!(seen.len(), 4, "the fallback set must still cover the round");
    }

    #[test]
    fn unknown_and_duplicate_names_are_ignored() {
        let mut r = Rotation::new(candidates(3));
        let prefs = Preferences {
            favorites: vec!["nope".into(), "e1".into(), "e1".into()],
            hidden: vec!["also-nope".into()],
        };
        assert_eq!(r.apply_preferences(&prefs), PrefsOutcome::Applied);
        assert_eq!(r.eligible_names(), vec!["e0", "e1", "e2"]);
    }

    #[test]
    fn hidden_effects_leave_the_pending_candidates() {
        let mut r = Rotation::new(candidates(5));
        let prefs = Preferences { favorites: Vec::new(), hidden: vec!["e3".into()] };
        assert_eq!(r.apply_preferences(&prefs), PrefsOutcome::Applied);
        let mut rng = Rng::seeded(17);
        let mut seen = HashSet::new();
        for _ in 0..4 {
            let name = r.next(0.5, &mut rng).unwrap();
            assert_ne!(name, "e3");
            assert!(seen.insert(name), "repeat within the shrunken round");
        }
        assert_eq!(seen.len(), 4);
    }

    #[test]
    fn unhiding_mid_round_adds_it_back_without_repeating() {
        let mut r = Rotation::new(candidates(6));
        let mut rng = Rng::seeded(13);
        r.apply_preferences(&Preferences {
            favorites: Vec::new(),
            hidden: vec!["e2".into(), "e4".into()],
        });
        let a = r.next(0.5, &mut rng).unwrap();
        let b = r.next(0.5, &mut rng).unwrap();
        assert!(!matches!(a.as_str(), "e2" | "e4"));
        assert!(!matches!(b.as_str(), "e2" | "e4"));
        // e2 is unhidden mid-round; it joins, but nothing repeats.
        r.apply_preferences(&Preferences { favorites: Vec::new(), hidden: vec!["e4".into()] });
        let mut seen: HashSet<String> = [a, b].into_iter().collect();
        for _ in 0..3 {
            let name = r.next(0.5, &mut rng).unwrap();
            assert_ne!(name, "e4");
            assert!(seen.insert(name.clone()), "{name} repeated within the round");
        }
        assert_eq!(seen.len(), 5);
    }

    #[test]
    fn favorites_do_not_duplicate_a_round() {
        let mut r = Rotation::new(candidates(12));
        r.apply_preferences(&Preferences {
            favorites: vec!["e0".into(), "e5".into()],
            hidden: Vec::new(),
        });
        let mut rng = Rng::seeded(31);
        let mut seen = HashSet::new();
        for _ in 0..12 {
            assert!(seen.insert(r.next(0.5, &mut rng).unwrap()));
        }
        assert_eq!(seen.len(), 12, "favorites must not duplicate a round");
    }

    #[test]
    fn favorites_are_favoured_in_the_order() {
        // A favorite gets an extra share of the choice, so it tends to arrive
        // early in the round rather than more often than once per round.
        let n = 10;
        let mut r = Rotation::new(candidates(n));
        r.apply_preferences(&Preferences { favorites: vec!["e0".into()], hidden: Vec::new() });
        let mut rng = Rng::seeded(21);
        let rounds = 400;
        let mut position_sum = 0usize;
        for _ in 0..rounds {
            for position in 0..n {
                if r.next(0.5, &mut rng).unwrap() == "e0" {
                    position_sum += position;
                }
            }
        }
        let average = position_sum as f64 / rounds as f64;
        assert!(
            average < (n - 1) as f64 / 2.0,
            "favorite appeared at average position {average}, not early in the round"
        );
    }

    #[test]
    fn selection_replays_for_a_seed() {
        let run = |seed: u64| {
            let mut r = Rotation::new(candidates(15));
            let mut rng = Rng::seeded(seed);
            (0..50).map(|_| r.next(0.3, &mut rng).unwrap()).collect::<Vec<_>>()
        };
        assert_eq!(run(42), run(42));
        assert_ne!(run(42), run(43));
    }

    #[test]
    fn preferences_parse_the_documented_format() {
        let prefs = Preferences::parse(
            "# comment\nversion=1\nfavorites=fireworks,swarm\nhidden=burn,pour\n",
        )
        .unwrap();
        assert_eq!(prefs.favorites, ["fireworks", "swarm"]);
        assert_eq!(prefs.hidden, ["burn", "pour"]);
    }

    #[test]
    fn a_future_version_is_not_misread() {
        assert_eq!(
            Preferences::parse("version=2\nfavorites=fireworks\n"),
            Err(PreferencesError::UnsupportedVersion(2))
        );
    }

    #[test]
    fn a_missing_file_is_an_empty_preference_set() {
        let path = std::env::temp_dir().join(format!(
            "ttfx-prefs-missing-{}-{}.conf",
            std::process::id(),
            std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .unwrap()
                .as_nanos()
        ));
        let _ = std::fs::remove_file(&path);
        assert_eq!(Preferences::read(&path).unwrap(), Preferences::default());
    }
}
