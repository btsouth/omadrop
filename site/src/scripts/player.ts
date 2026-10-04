export {};

const video = document.querySelector<HTMLVideoElement>("#collection-video");
const playback = document.querySelector<HTMLElement>("#playback");
const milkdropPanel = document.querySelector<HTMLElement>("#milkdrop-preview");
const omarchyPanel = document.querySelector<HTMLElement>("#omarchy-preview");
const modeSwitch = document.querySelector<HTMLElement>(".mode-switch");
const milkdropTab = document.querySelector<HTMLButtonElement>("#milkdrop-tab");
const omarchyTab = document.querySelector<HTMLButtonElement>("#omarchy-tab");
const playerControls = document.querySelector<HTMLElement>(".player-controls");
const playButton = document.querySelector<HTMLButtonElement>("#play-toggle");
const soundButton = document.querySelector<HTMLButtonElement>("#sound-toggle");
const fullscreenButton = document.querySelector<HTMLButtonElement>("#fullscreen-toggle");
const seek = document.querySelector<HTMLInputElement>("#seek");
const time = document.querySelector<HTMLElement>("#time");
const status = document.querySelector<HTMLElement>("#player-status");
const previewKind = document.querySelector<HTMLElement>("#preview-kind");
const previewDescription = document.querySelector<HTMLElement>("#preview-description");
const osakaVideo = document.querySelector<HTMLVideoElement>("#osaka-video");

// Keep native video controls and both previews available if enhancement cannot run.
if (video && playback && milkdropPanel && omarchyPanel && modeSwitch && milkdropTab && omarchyTab && playerControls && playButton && soundButton && fullscreenButton && seek && time && status && previewKind && previewDescription) {
  const reducedMotion = window.matchMedia("(prefers-reduced-motion: reduce)");
  let wantsPlayback = !reducedMotion.matches;
  let activeMode: "milkdrop" | "omarchy" = "milkdrop";
  let inView = true;
  let imageViewerOpen = false;
  let playRequest = 0;
  const milkdropDescription = previewDescription.cloneNode(true) as HTMLElement;
  const playText = playButton.querySelector<HTMLElement>("#play-text");
  const soundText = soundButton.querySelector<HTMLElement>("#sound-text");

  const formatTime = (seconds: number) => {
    const safeSeconds = Number.isFinite(seconds) ? Math.max(0, Math.floor(seconds)) : 0;
    return `${Math.floor(safeSeconds / 60)}:${String(safeSeconds % 60).padStart(2, "0")}`;
  };

  const syncPlayButton = () => {
    const playing = !video.paused;
    playButton.setAttribute("aria-label", playing ? "Pause recording" : "Play recording");
    if (playText) playText.textContent = playing ? "Pause" : "Play";
    playButton.querySelector(".play-icon")?.toggleAttribute("hidden", playing);
    playButton.querySelector(".pause-icon")?.toggleAttribute("hidden", !playing);
  };

  const syncSoundButton = () => {
    const audible = !video.muted;
    soundButton.setAttribute("aria-pressed", String(audible));
    soundButton.setAttribute("aria-label", audible ? "Mute sound" : "Enable sound");
    if (soundText) soundText.textContent = audible ? "Sound on" : "Sound off";
    soundButton.querySelector(".sound-off-icon")?.toggleAttribute("hidden", audible);
    soundButton.querySelector(".sound-on-icon")?.toggleAttribute("hidden", !audible);
  };

  const updateTime = () => {
    const duration = Number.isFinite(video.duration) ? video.duration : 53;
    seek.max = String(duration);
    seek.value = String(video.currentTime);
    seek.style.setProperty("--progress", `${duration > 0 ? video.currentTime / duration * 100 : 0}%`);
    seek.setAttribute("aria-valuetext", `${formatTime(video.currentTime)} of ${formatTime(duration)}`);
    time.textContent = `${formatTime(video.currentTime)} / ${formatTime(duration)}`;
  };

  const shouldPlay = () => wantsPlayback && activeMode === "milkdrop" && inView && !document.hidden && !imageViewerOpen;
  const pause = () => {
    playRequest++;
    video.pause();
  };
  const play = async () => {
    const request = ++playRequest;
    try {
      await video.play();
      // A mode change or visibility update can cancel an in-flight play request.
      if (!shouldPlay()) video.pause();
      if (request === playRequest) status.textContent = "";
    } catch {
      if (request !== playRequest) return;
      wantsPlayback = false;
      status.textContent = "Press Play to watch the recording.";
      syncPlayButton();
    }
  };
  const reconcilePlayback = () => {
    if (shouldPlay()) void play();
    else pause();
  };

  const selectMode = (mode: "milkdrop" | "omarchy") => {
    activeMode = mode;
    const isMilkdrop = mode === "milkdrop";
    milkdropTab.setAttribute("aria-selected", String(isMilkdrop));
    omarchyTab.setAttribute("aria-selected", String(!isMilkdrop));
    milkdropTab.tabIndex = isMilkdrop ? 0 : -1;
    omarchyTab.tabIndex = isMilkdrop ? -1 : 0;
    milkdropPanel.hidden = !isMilkdrop;
    omarchyPanel.hidden = isMilkdrop;
    previewKind.textContent = isMilkdrop ? "Recorded playback / 53 seconds" : "Osaka Jade / 24 seconds, no sound";
    if (isMilkdrop) previewDescription.replaceChildren(...Array.from(milkdropDescription.cloneNode(true).childNodes));
    else previewDescription.textContent = "Recorded from Omadrop v0.6.0 with music playing. In the app, the street reacts to whatever you play.";
    status.textContent = "";
    reconcilePlayback();
    if (osakaVideo) {
      if (!isMilkdrop && wantsPlayback) void osakaVideo.play().catch(() => undefined);
      else osakaVideo.pause();
    }
  };

  milkdropTab.addEventListener("click", () => selectMode("milkdrop"));
  omarchyTab.addEventListener("click", () => selectMode("omarchy"));
  modeSwitch.addEventListener("keydown", (event) => {
    if (!["ArrowLeft", "ArrowRight", "Home", "End"].includes(event.key)) return;
    event.preventDefault();
    const nextMode = event.key === "Home" ? "milkdrop" : event.key === "End" ? "omarchy" : activeMode === "milkdrop" ? "omarchy" : "milkdrop";
    selectMode(nextMode);
    (nextMode === "milkdrop" ? milkdropTab : omarchyTab).focus();
  });

  playButton.addEventListener("click", () => {
    wantsPlayback = video.paused;
    reconcilePlayback();
  });
  soundButton.addEventListener("click", () => {
    video.muted = !video.muted;
  });
  seek.addEventListener("input", () => {
    if (Number.isFinite(video.duration)) {
      video.currentTime = Number(seek.value);
      updateTime();
    }
  });
  video.addEventListener("play", syncPlayButton);
  video.addEventListener("pause", syncPlayButton);
  video.addEventListener("volumechange", syncSoundButton);
  video.addEventListener("timeupdate", updateTime);
  video.addEventListener("loadedmetadata", updateTime);
  video.addEventListener("error", () => {
    wantsPlayback = false;
    status.textContent = "The preview could not load. You can watch the launch film using the link below.";
  });

  fullscreenButton.addEventListener("click", async () => {
    try {
      if (document.fullscreenElement) await document.exitFullscreen();
      else if (playback.requestFullscreen && document.fullscreenEnabled) await playback.requestFullscreen();
      else {
        // iOS Safari exposes video fullscreen rather than element fullscreen.
        const iosVideo = video as HTMLVideoElement & { webkitEnterFullscreen?: () => void };
        if (iosVideo.webkitEnterFullscreen) iosVideo.webkitEnterFullscreen();
        else status.textContent = "Fullscreen is unavailable in this browser. You can open the launch film using the link below.";
      }
    } catch {
      status.textContent = "Fullscreen could not open. You can open the launch film using the link below.";
    }
  });
  document.addEventListener("fullscreenchange", () => {
    const fullscreen = document.fullscreenElement === playback;
    fullscreenButton.setAttribute("aria-label", fullscreen ? "Exit fullscreen" : "Enter fullscreen");
    const label = fullscreenButton.querySelector("span");
    if (label) label.textContent = fullscreen ? "Exit fullscreen" : "Fullscreen";
  });

  reducedMotion.addEventListener("change", () => {
    if (reducedMotion.matches) {
      wantsPlayback = false;
      pause();
    }
  });
  document.addEventListener("visibilitychange", reconcilePlayback);
  document.addEventListener("imageviewerchange", (event) => {
    imageViewerOpen = (event as CustomEvent<{ open: boolean }>).detail.open;
    reconcilePlayback();
  });
  if ("IntersectionObserver" in window) {
    new IntersectionObserver(([entry]) => {
      inView = entry.isIntersecting;
      reconcilePlayback();
    }, { threshold: 0.1 }).observe(playback);
  }

  video.controls = false;
  playerControls.hidden = false;
  modeSwitch.hidden = false;
  syncPlayButton();
  syncSoundButton();
  selectMode("milkdrop");
}

const copyButton = document.querySelector<HTMLButtonElement>("#copy-install");
const installCommand = document.querySelector<HTMLElement>("#install-command");
const copyStatus = document.querySelector<HTMLElement>("#copy-status");
if (copyButton && installCommand && copyStatus && window.isSecureContext && navigator.clipboard?.writeText) {
  let resetTimer: ReturnType<typeof setTimeout>;
  copyButton.hidden = false;
  copyButton.addEventListener("click", async () => {
    clearTimeout(resetTimer);
    copyButton.disabled = true;
    copyButton.textContent = "Copying…";
    try {
      await navigator.clipboard.writeText(installCommand.textContent?.trim() ?? "");
      copyButton.textContent = "Copied ✓";
      copyStatus.textContent = "Command copied.";
      resetTimer = setTimeout(() => {
        copyButton.textContent = "Copy command";
        copyStatus.textContent = "";
      }, 2500);
    } catch {
      copyButton.textContent = "Copy command";
      copyStatus.textContent = "Select and copy the command above.";
    } finally {
      copyButton.disabled = false;
    }
  });
}
