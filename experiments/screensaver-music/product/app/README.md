# Omadrop desktop controller

Normal launch starts visuals immediately. Esc returns from MilkDrop; any key
returns from Omarchy. Reopening Omadrop brings up the existing control view.
The control window combines modes, display settings, ASCII, effect search,
favorites/hiding and selected previews. Space starts playback; Q quits; ? helps.

Actual renderers remain unchanged. The Qt app owns startup, failure, return and
shutdown rather than asking the user to manage separate tools. Playback uses
fullscreen backend windows, with a native control window shown on return;
rendering is not embedded inside the Qt window.

Build on devbox with qmake6 and make from app/build. Backend tests are under
tests; run the built backend-tests with QT_QPA_PLATFORM=offscreen on devbox.
Inspect and drive the actual GUI only in omabox. Product install.sh stages the
reviewed native binary at app/build/omadrop-ui. Package/build-bundle.py creates
the Omarchy candidate; existing MilkDrop remains an optional local integration.
