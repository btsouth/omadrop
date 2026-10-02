//! The screensaver runner owns terminal input and requests effect changes by signal.

use std::sync::atomic::{AtomicU8, Ordering};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Change {
    Next = 1,
    Previous = 2,
}

// Coalesce requests between display frames, keeping the latest direction.
static REQUEST: AtomicU8 = AtomicU8::new(0);

extern "C" fn handle_change(signal: libc::c_int) {
    let change = if signal == libc::SIGUSR1 {
        Change::Next
    } else {
        Change::Previous
    };
    REQUEST.store(change as u8, Ordering::SeqCst);
}

/// Install only for music playback; stock runs retain their signal behavior.
pub fn install() -> std::io::Result<()> {
    // SAFETY: sigaction is initialized with an empty mask and a handler that
    // only stores an atomic flag. No allocation or I/O happens in the handler.
    unsafe {
        let mut action: libc::sigaction = std::mem::zeroed();
        action.sa_sigaction = handle_change as *const () as usize;
        action.sa_flags = libc::SA_RESTART;
        libc::sigemptyset(&mut action.sa_mask);
        for signal in [libc::SIGUSR1, libc::SIGUSR2] {
            if libc::sigaction(signal, &action, std::ptr::null_mut()) != 0 {
                return Err(std::io::Error::last_os_error());
            }
        }
    }
    Ok(())
}

pub fn pending() -> bool {
    REQUEST.load(Ordering::SeqCst) != 0
}

pub fn take() -> Option<Change> {
    match REQUEST.swap(0, Ordering::SeqCst) {
        1 => Some(Change::Next),
        2 => Some(Change::Previous),
        _ => None,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn requests_are_consumed_and_latest_direction_wins() {
        assert!(!pending());
        handle_change(libc::SIGUSR1);
        assert!(pending());
        assert_eq!(take(), Some(Change::Next));
        assert!(!pending());
        assert_eq!(take(), None);
        handle_change(libc::SIGUSR1);
        handle_change(libc::SIGUSR2);
        assert_eq!(take(), Some(Change::Previous));
        assert!(!pending());
    }
}
