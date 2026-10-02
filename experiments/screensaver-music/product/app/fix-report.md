# Native controller review corrections

The controller now uses an absolute runtime socket plus a process lock. IPC quit waits for asynchronous stop completion. Stop cancels the dispatcher process group, retains a unique `OMADROP_CANCEL_FILE` marker for delayed compositor launches, and performs two bounded cleanup passes. Shutdown also performs bounded cleanup. Parent-owned launcher/runner changes must carry and honor the marker.

Startup has an independent deadline. Compositor queries and effects helpers have process timeouts; failed, malformed, or structurally invalid polls preserve playback state. A pre-launch PID snapshot excludes old windows, and MilkDrop matching no longer trusts titles. Runtime controller crashes, effects failures, and settings write failures are visible. Effects discovery reruns when a toggle completes during an existing refresh.

Fresh accounts select an available Omarchy backend when MilkDrop is absent. `OMADROP_UI_MODE` is honored for both fresh launches and IPC requests. Existing renderer and effect implementations are unchanged.

Focused regressions cover these behaviors, including inherited children and a detached late-launch cancellation simulation. The test entry point uses QCoreApplication. The parent reported a successful devbox build and 26 passing cases before the final invalid-client row and destructor correction. That correction prevents helper completion callbacks from spawning new processes during teardown. Final build/test verification and contained GUI testing remain with the parent. No local builds, GUI actions, or installations were performed.
