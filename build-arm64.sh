#!/usr/bin/env bash
# build-arm64.sh Build RobloxPlayer.app for Apple Silicon (arm64, macOS 11+)
# Requires: Xcode.app installed (not just CommandLineTools)
# Usage: bash build-arm64.sh [Release|Debug]

set -euo pipefail

REPO="$(cd "$(dirname "$0")" && pwd)"
CONFIG="${1:-Release}"

# ---- Verify Xcode.app is installed ----
if ! xcode-select -p 2>/dev/null | grep -q "Xcode.app"; then
    echo "ERROR: Full Xcode.app is required (not just CommandLineTools)."
    echo "Install from the App Store, then run: sudo xcode-select -s /Applications/Xcode.app"
    exit 1
fi

echo "==> Building RobloxPlayer ($CONFIG) for arm64"
echo "    Repo:  $REPO"
echo "    Xcode: $(xcode-select -p)"

# ---- Confirm all dependencies exist ----
check_file() {
    if [ ! -e "$1" ]; then
        echo "ERROR: Missing dependency: $1"
        exit 1
    fi
}

check_file "$REPO/Contribs/SDL2.0.4/Xcode/SDL/SDL2.framework/Versions/A/SDL2"
check_file "$REPO/Contribs/TBB/tbb_mac_30_056/lib/libtbb.dylib"
check_file "$REPO/fmod/mac/libfmod.dylib"
check_file "$REPO/Contribs/openssl/lib/libssl.a"
check_file "$REPO/Contribs/curl/lib/libcurl.a"
check_file "$REPO/Contribs/boost/lib/libboost_filesystem.a"

echo "==> Dependencies OK"

# ---- Run xcodebuild ----
cd "$REPO"

xcodebuild \
    -project MacClient.xcodeproj \
    -target RobloxPlayer \
    -configuration "$CONFIG" \
    ARCHS="arm64" \
    ONLY_ACTIVE_ARCH=NO \
    MACOSX_DEPLOYMENT_TARGET=11.0 \
    OTHER_CFLAGS="-Wno-deprecated-declarations" \
    OTHER_CPLUSPLUSFLAGS="-Wno-deprecated-declarations" \
    2>&1 | tee "build-$CONFIG.log"

echo ""
echo "==> Build log saved to build-$CONFIG.log"

# ---- Locate the .app bundle ----
APP=$(find "$REPO/build" -name "RobloxPlayer.app" -maxdepth 4 2>/dev/null | head -1)
if [ -z "$APP" ]; then
    APP=$(find ~/Library/Developer/Xcode/DerivedData -name "RobloxPlayer.app" -maxdepth 6 2>/dev/null | head -1)
fi

if [ -n "$APP" ]; then
    echo "==> Built: $APP"
    echo ""
    echo "To launch:"
    echo "  open \"$APP\""
else
    echo "==> Could not locate RobloxPlayer.app. Check build-$CONFIG.log for the build directory."
fi
