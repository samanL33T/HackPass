# tools

Small helper scripts and dev utilities. Run from the repo root.

## generate-license.ts

Mints an evaluation license blob compatible with HackPass's `EmbeddedLicense`
loader. Run with Bun:

```
bun run shared/tools/generate-license.ts samanl33t pro 365 > shared/app/src/resources/eval_license.bin
```

After regenerating, re-build HackPass to refresh the qrc-embedded copy.

## generate-icons.ts

Rasterises `shared/assets/logo.svg` into the four PNG sizes the Chrome
extension's `manifest.json` references (16, 32, 48, 128 px). Run once when
the logo changes:

```
bun run shared/tools/generate-icons.ts
```

Outputs land in `shared/extension/icons/`.

## generate-ico.ts

Glues the four PNGs from `shared/extension/icons/` into a multi-resolution
Windows `.ico` for the app's embedded icon (taskbar, Explorer, Inno
shortcuts). Run after `generate-icons.ts`:

```
bun run shared/tools/generate-ico.ts
```

Outputs `shared/assets/icon.ico`.

## generate-icns.sh

macOS-side icon generation. Renders `shared/assets/logo.svg` at the sizes
macOS uses for `.iconset` and compiles to `.icns` via `iconutil` (built into
macOS). Run once on a Mac and commit the result:

```
bash shared/tools/generate-icns.sh
```

Outputs `mac/installer/HackPass.icns`. Requires `rsvg-convert` (brew install
librsvg); falls back to `qlmanage -t` if rsvg is missing.
