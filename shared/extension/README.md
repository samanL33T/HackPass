# HackPass browser extension

Chrome Manifest V3 extension that talks to HackPass.exe over a localhost
WebSocket at `127.0.0.1:8765` for autofill on web pages.

## Install (developer mode)

1. Open `chrome://extensions`
2. Toggle **Developer mode** on
3. Click **Load unpacked** and select this directory
4. Open the extension's Options page (right-click the toolbar icon)
5. Paste the handshake token shown in HackPass's first-run wizard

## Icons

The four icon PNG files (icon-16.png, icon-32.png, icon-48.png, icon-128.png)
need to be generated from the SVG logo at `../app/src/resources/images/logo.svg`
into `icons/` before publishing. Any image converter that handles SVG will work,
e.g. `inkscape --export-png=icons/icon-128.png -w 128 -h 128 logo.svg`.
