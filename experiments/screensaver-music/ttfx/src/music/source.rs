//! Where the music comes from: the default PipeWire output (what is playing
//! now, as Omadrop's MilkDrop app captures it) or a recorded fixture of
//! stereo 44.1 kHz little-endian f32 for reproducible review.

use std::io::Read;
use std::process::{Child, Command, Stdio};
use std::sync::mpsc::{self, Receiver};
use std::time::Instant;

pub enum Source {
    Live(Live),
    Fixture(Fixture),
}

impl Source {
    /// Append the interleaved samples that have arrived by `now` (seconds on
    /// the music clock) and return the clock time they became available.
    pub fn pull(&mut self, now: f64, out: &mut Vec<f32>) -> f64 {
        match self {
            Source::Live(live) => live.pull(out),
            Source::Fixture(fixture) => fixture.pull(now, out),
        }
    }
}

// ------------------------------------------------------------------ live

pub struct Live {
    target: Option<String>,
    child: Option<Child>,
    rx: Receiver<Vec<f32>>,
    start: Instant,
    /// Music clock time of the next check that the recorder is still running.
    next_check: f64,
}

impl Live {
    pub fn start(target: Option<&str>) -> std::io::Result<Live> {
        let (child, rx) = Live::spawn(target)?;
        Ok(Live {
            target: target.map(str::to_string),
            child: Some(child),
            rx,
            start: Instant::now(),
            next_check: 1.0,
        })
    }

    fn spawn(target: Option<&str>) -> std::io::Result<(Child, Receiver<Vec<f32>>)> {
        let program =
            std::env::var("OMADROP_PW_RECORD_COMMAND").unwrap_or_else(|_| "pw-record".into());
        let mut command = Command::new(program);
        command.args([
            "--raw",
            "--rate",
            "44100",
            "--channels",
            "2",
            "--format",
            "f32",
            "--latency",
            "20ms",
            "-P",
            "{ stream.capture.sink=true node.name=omadrop-screensaver }",
        ]);
        if let Some(target) = target {
            command.args(["--target", target]);
        }
        command
            .arg("-")
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::null());
        // The screensaver is stopped with pkill: the recorder must not outlive it.
        unsafe {
            use std::os::unix::process::CommandExt;
            command.pre_exec(|| {
                unsafe extern "C" {
                    fn prctl(option: i32, arg2: u64, arg3: u64, arg4: u64, arg5: u64) -> i32;
                }
                const PR_SET_PDEATHSIG: i32 = 1;
                const SIGTERM: u64 = 15;
                prctl(PR_SET_PDEATHSIG, SIGTERM, 0, 0, 0);
                Ok(())
            });
        }
        let mut child = command.spawn()?;
        let mut stdout = child.stdout.take().expect("piped stdout");
        let (tx, rx) = mpsc::channel();
        std::thread::Builder::new()
            .name("music-capture".into())
            .spawn(move || {
                let mut bytes = vec![0u8; 16384];
                let mut carry: Vec<u8> = Vec::new();
                loop {
                    let n = match stdout.read(&mut bytes) {
                        Ok(0) | Err(_) => return,
                        Ok(n) => n,
                    };
                    carry.extend_from_slice(&bytes[..n]);
                    let whole = carry.len() / 4 * 4;
                    let samples: Vec<f32> = carry[..whole]
                        .chunks_exact(4)
                        .map(|b| f32::from_le_bytes([b[0], b[1], b[2], b[3]]))
                        .collect();
                    carry.drain(..whole);
                    if tx.send(samples).is_err() {
                        return;
                    }
                }
            })?;
        Ok((child, rx))
    }

    fn pull(&mut self, out: &mut Vec<f32>) -> f64 {
        while let Ok(samples) = self.rx.try_recv() {
            out.extend_from_slice(&samples);
        }
        let now = self.start.elapsed().as_secs_f64();
        // PipeWire restarting (or not up yet) ends the recorder: start another
        if now >= self.next_check {
            self.next_check = now + 1.0;
            let running = match &mut self.child {
                Some(child) => matches!(child.try_wait(), Ok(None)),
                None => false,
            };
            if !running {
                if let Some(mut child) = self.child.take() {
                    let _ = child.wait();
                }
                if let Ok((child, rx)) = Live::spawn(self.target.as_deref()) {
                    self.child = Some(child);
                    self.rx = rx;
                }
                self.next_check = now + 2.0;
            }
        }
        now
    }

    pub fn start_instant(&self) -> Instant {
        self.start
    }
}

impl Drop for Live {
    fn drop(&mut self) {
        if let Some(mut child) = self.child.take() {
            let _ = child.kill();
            let _ = child.wait();
        }
    }
}

/// The default output's node name, as `pactl get-default-sink` reports it.
pub fn default_sink() -> Option<String> {
    if let Ok(sink) = std::env::var("OMADROP_AUDIO_SINK") {
        if !sink.is_empty() {
            return Some(sink);
        }
    }
    let output = Command::new("pactl")
        .arg("get-default-sink")
        .stderr(Stdio::null())
        .output()
        .ok()?;
    let sink = String::from_utf8(output.stdout).ok()?.trim().to_string();
    (!sink.is_empty()).then_some(sink)
}

/// The speaker delay Omadrop's MilkDrop app saved for this output
/// (live_settings.cpp loadSyncDelay): OMADROP_SYNC_MS, else the per-sink
/// file, else 180 ms for Bluetooth and 35 ms otherwise.
pub fn saved_sync_ms(sink: Option<&str>) -> u32 {
    if let Ok(ms) = std::env::var("OMADROP_SYNC_MS") {
        if let Ok(ms) = ms.trim().parse::<i64>() {
            return ms.clamp(0, 500) as u32;
        }
    }
    let sink = sink.unwrap_or("");
    let fallback = if sink.starts_with("bluez_") { 180 } else { 35 };
    let config = match std::env::var_os("XDG_CONFIG_HOME") {
        Some(dir) => std::path::PathBuf::from(dir).join("omadrop"),
        None => match std::env::var_os("HOME") {
            Some(home) => std::path::PathBuf::from(home).join(".config/omadrop"),
            None => return fallback,
        },
    };
    let mut name: String = sink
        .chars()
        .map(|c| {
            if c.is_ascii_alphanumeric() || matches!(c, '-' | '_' | '.') {
                c
            } else {
                '_'
            }
        })
        .collect();
    if name.is_empty() {
        name = "default".into();
    }
    name.truncate(180);
    let path = config.join("sync-by-sink").join(format!("{name}.ms"));
    let saved = std::fs::read_to_string(path)
        .or_else(|_| std::fs::read_to_string(config.join("sync-ms")))
        .ok()
        .and_then(|s| s.split_whitespace().next()?.parse::<i64>().ok());
    saved.map_or(fallback, |ms| ms.clamp(0, 500) as u32)
}

// ------------------------------------------------------------------ fixture

pub struct Fixture {
    samples: Vec<f32>,
    /// Interleaved samples handed out so far.
    pos: usize,
    /// Fixture seconds skipped at the start.
    offset: f64,
}

impl Fixture {
    pub fn open(path: &std::path::Path, offset: f64) -> std::io::Result<Fixture> {
        let bytes = std::fs::read(path)?;
        let samples = bytes
            .chunks_exact(4)
            .map(|b| f32::from_le_bytes([b[0], b[1], b[2], b[3]]))
            .collect();
        Ok(Fixture {
            samples,
            pos: 0,
            offset,
        })
    }

    pub fn duration(&self) -> f64 {
        self.samples.len() as f64 / 2.0 / super::features::SAMPLE_RATE as f64 - self.offset
    }

    fn pull(&mut self, now: f64, out: &mut Vec<f32>) -> f64 {
        let rate = super::features::SAMPLE_RATE as f64;
        let start = ((self.offset.max(0.0)) * rate) as usize * 2;
        // past the end the fixture is silence: the run shows the pause
        let want = start + ((now.max(0.0) * rate) as usize) * 2;
        let from = start + self.pos;
        let begin = from.min(self.samples.len());
        let end = want.min(self.samples.len());
        if end > begin {
            out.extend_from_slice(&self.samples[begin..end]);
        }
        let silent = want.saturating_sub(from.max(end));
        out.extend(std::iter::repeat_n(0.0, silent));
        self.pos = want - start;
        now
    }
}
