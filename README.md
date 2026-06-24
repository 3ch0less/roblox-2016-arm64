# Roblox 2016 Apple Silicon Port

An ARM64 port of the leaked Roblox 2016 source code, built to run natively on Apple Silicon (M1/M2/M3/M4) Macs running macOS 11 and above.

A quick warning before you scroll: I spent DAYS on this. Actual, real, calendar DAYS. Not "a long afternoon." I genuinely lost my mind inside compile errors and linker issues. At one point I was buried in Boost headers at 3am wondering what year it was. So if you clone this and it just builds, that is the result of a truly unreasonable amount of suffering. Pain in the ass. But here we are.

---

## Why this was hard

The original codebase was written around 2013 to 2016 targeting Clang 3.x and GCC 4.x on x86 Windows and Intel Mac. Modern Clang 16+ on ARM64 enforces C++11 strictly, so hundreds of things that silently compiled before now fail hard. Every pre built third party library was the wrong architecture, several source files were missing from the repo entirely, and a bunch of Apple APIs the old code depended on no longer exist.

---

## Build system

- Patched all 15 Xcode `.xcodeproj` files for `ARCHS=arm64`, `MACOSX_DEPLOYMENT_TARGET=11.0`
- Added `CONTRIB_PATH=$(SRCROOT)/Contribs` as a project level build variable
- Removed Breakpad crash reporter (no ARM64 build exists) from all references and link phases
- Set `GCC_TREAT_WARNINGS_AS_ERRORS=NO` so deprecated but valid code does not kill the build

## Dependencies rebuilt from source

| Library | Version | Notes |
|---------|---------|-------|
| OpenSSL | 1.1.1w | Replaced the ancient 1.0.0c |
| curl | 7.87.0 | Rebuilt against new OpenSSL |
| Boost | 1.56 | All ARM64 static libs. The repo only had headers, no source `.cpp` files for chrono/filesystem/iostreams/signals/system/thread. Copied compatible source from Boost 1.56. |
| TBB | 4.1 | Added `gcc_armv8.h` machine header, fixed pure virtual vtable crash |
| SDL2 | 2.0.4 | Fixed spinlock for aarch64, built as proper versioned `.framework` bundle |
| FMOD | stub | No ARM64 FMOD binary exists, created a no op stub dylib with all required exports |

## macOS API fixes

- **WebView to WKWebView migration**: `RobloxPlayerAppDelegate.mm` and `RbxWebView.xib`. The old `WebView` class was removed in macOS 14.
- **`MachOBaseAddr.mm`**: replaced `getsegbynamefromheader_64` (gone from modern SDKs) with `getsegbyname`.
- **`RobloxOgreView.mm`**: switched `HIToolbox/Events.h` to `Carbon/Carbon.h` and fixed `_window` ivar to `self.window`.
- **SSE2/NEON guards** in `Voxelizer.cpp` and `LightGrid.cpp`: the old guards incorrectly enabled SSE2 on ARM64 macOS. Fixed to check `__aarch64__`.
- **`ObjectiveCUtilities.h`**: the file was missing from the repo entirely. Created a stub with `NSString` to `std::string` converters.
- **Breakpad**: fully disabled at source level with a no op stub.
- **`Profiler.cpp`**: removed a pre C++11 shim that injected Boost types into `namespace std`. Modern macOS has real C++11 so the shim was actively harmful.

## C++11 and compiler fixes

Modern Clang on ARM64 treats implicit narrowing conversions in brace initializers as hard errors, and several C++03 idioms were removed from libc++. Highlights of roughly 20 individual fixes across the codebase:

- **Narrowing**: explicit casts added in `RakPeer.cpp` (char to unsigned char for `0xFF`), `TypesetterDynamic.cpp`/`TypesetterBitmap.cpp` (ptrdiff_t to size_t, bool pointer comparison bug), `ScreenSpaceEffect.cpp`, `FastCluster.cpp`, `uint128.cpp`, `HttpCacheEntry.cpp`.
- **`GuiObject.cpp`**: `std::abs<float>` is ambiguous on modern macOS. Switched to `std::fabs`.
- **`SerializerV2.cpp`**: replaced removed `std::bind1st(std::mem_fun1(...))` with a C++11 lambda.
- **`NetworkClusterPacketCache.cpp`**: explicit template instantiation moved inside its namespace block where newer Clang requires it.
- **`PathfindingService.h`**: dropped a `boost::fast_pool_allocator` that was incompatible with C++11 container requirements.
- **RakNet** (5 files): the macro pattern `%"PRINTF_64_BIT_MODIFIER"u` was parsed as a user defined literal. Added spaces to break the parse.
- **`w3c-libwww`**: the bundled library relied on `HAVE_*` defines that were never set on macOS, leaving `size_t` and other types undefined. Added an unconditional `__APPLE__` block to `wwwsys.h` pulling in standard POSIX headers, and added `<stddef.h>` to `HTUtils.h`.
- **FreeType `zconf.h`** and **g3d `pngconf.h`**: both had `TARGET_OS_MAC` guards that are always true on modern macOS, causing missing typedefs and attempts to include the long gone Classic Mac OS `<fp.h>` header. Fixed the guards.
- **`rapidjson.h`**: `memcpy` return value was used as `void*`, which fails in C++ strict mode. Split into a separate return.
- **`VMProtectSDK.h`**: Windows only anti tamper DLL. Created a no op stub header.

## Linker fixes

- **Duplicate FreeType symbols**: `ftbase.c` includes several other FreeType `.c` files directly, but those same files were also compiled individually in the Xcode project. Removed 52 redundant source entries from `GfxRender.xcodeproj`.
- **Missing string constants**: `sPostEffect`, `sBlurEffect`, and `sColorCorrectionEffect` had no compiled `.cpp` files. Defined them directly in `factoryregistration.cpp`.
- **LDAP**: `libcurl.a` was built with LDAP support. Added `-framework LDAP` to the linker flags.

## Build phase stubs

Two build phases depended on tools that cannot run on ARM64 macOS:

- **CoreScriptConverter**: the bundled binary was i386 only. Replaced with a shell script that copies a stub `LuaGenCS.inl` so the build phase passes. CoreScripts are not embedded; game Lua scripts load from disk.
- **ShaderCompiler**: the HLSL to GLSL cross compiler is Windows only. Replaced with a stub that exits 0. Pre compiled GLSL shaders must be provided separately.

---

## Building

You need the full **Xcode.app** from the App Store, not just CommandLineTools.

```bash
sudo xcode-select -s /Applications/Xcode.app
cd /path/to/this/repo
bash build-arm64.sh Release
```

The build is large (roughly 800 to 900 translation units) and Boost is slow to compile, so expect it to take a while.

---

## Playing

The built client cannot connect to official Roblox servers. Modern Roblox dropped the 2016 protocol years ago. You need a matching 2016 era server:

- **Local**: Build `RCCService` from this repo (also needs ARM64 porting) and run client + server on the same machine.
- **Revival servers**: Some private communities run matching old server software. Point the client at their endpoint by changing the hardcoded server URLs before building.

---

## Known limitations

- **CoreScripts are stubbed.** Game Lua scripts load from disk instead of being embedded.
- **Shaders are not compiled at build time.** Pre compiled GLSL shaders must be supplied separately.
- **FMOD is a no op stub.** Audio is completely silent.
- **RCCService is not ported.** Only the client (`RobloxPlayer`) builds on ARM64.
- **Runtime behavior is unverified beyond launch.** The binary links and produces a 101MB ARM64 Mach O, but full runtime testing has not been done.

---

## What was stripped from this repo

To keep the repo under GitHub's size limit, the following were excluded. None of them affect the Mac build:

- `Contribs/Qt/`: Windows only Qt 4.8.5
- `RobloxTest/`: test `.rbxl` asset files
- `Contribs/windows/`: Windows static libraries
- `App.BulletPhysics/NoOpt/`: prebuilt Windows lib
- `RCCService/Mesa-7.8.1/`: Windows Mesa renderer
- Built ARM64 binaries (OpenSSL, curl, Boost, TBB, SDL2, FMOD). Rebuild these with `build-arm64.sh`.

---

## Credits

Based on the `robloxsrc.zip` leak. Original README and Windows build notes from [P0L3NARUBA's repo](https://github.com/P0L3NARUBA/roblox-2016-source-code).

This was forked from [kiemfp/Roblox](https://github.com/kiemfp/Roblox). Shoutout to that fork, it is a fork of a fork I forked, and without it this ARM64 port would not exist.
