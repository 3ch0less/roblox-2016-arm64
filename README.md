# Roblox 2016 Apple Silicon Port

Native **ARM64** port of the 2016 Roblox client for Apple Silicon (macOS 11+). Offline play works: baseplate, R6, follow cam, WASD, GLSL shaders, disk CoreScripts, chat UI, settings.

## Screenshots

Offline spawn with classic forcefield (expires after a few seconds):

![Offline spawn forcefield](docs/screenshots/offline-spawn-forcefield.jpg)

In-game settings (shift lock, camera, graphics, reset / leave):

![Settings menu](docs/screenshots/settings-menu.jpg)

Chat and player list on baseplate:

![Offline chat](docs/screenshots/offline-chat.jpg)

## Build

Requires full **Xcode** (not just Command Line Tools):

```bash
sudo xcode-select -s /Applications/Xcode.app
cd /path/to/this/repo
bash build-arm64.sh Release
# or: xcodebuild -project MacClient.xcodeproj -scheme RobloxPlayer -configuration Debug
```

First builds are slow (Boost + ~800-900 TUs). See `BUILDING.md` / `BUILDING_CONTRIBS.md` for dependency rebuild notes.

## Play (offline)

```bash
./build/Debug/RobloxPlayer.app/Contents/MacOS/RobloxPlayer \
  --offline --rbxl "$(pwd)/Baseplate-1.rbxl" \
  -ApplePersistenceIgnoreState YES
```

## Status

| Works | Missing / limited |
|-------|-------------------|
| Offline place load, character, camera, input | Real FMOD arm64 audio (stub, silent) |
| Disk CoreScripts + 2016-style topbar path | Official Roblox servers (protocol dead) |
| Prebuilt GLSL packs in `shaders/` | Multiplayer / RCC on Mac |
| Local assets preferred over dead CDN | Studio arm64 |
| Spawn forcefield expires after a few seconds | |

**Do not break** the offline `--rbxl Baseplate-1.rbxl` path.

## Limits

- Client cannot join modern Roblox; use offline, local RCC, or a matching revival endpoint.
- CoreScriptConverter / ShaderCompiler build phases are stubbed on Mac; shaders ship prebuilt.
- Large Windows-only trees (Qt, Mesa, etc.) are gitignored, see `.gitignore`.

## Credits

Based on the 2016 source leak. Upstream notes: [P0L3NARUBA/roblox-2016-source-code](https://github.com/P0L3NARUBA/roblox-2016-source-code). Fork lineage via [kiemfp/Roblox](https://github.com/kiemfp/Roblox).
