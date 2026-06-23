#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
DIST_DIR="$ROOT_DIR/dist/macos"
APP_NAME="Pilgrim of the Thorn"
APP_BUNDLE="$DIST_DIR/$APP_NAME.app"
APP_CONTENTS="$APP_BUNDLE/Contents"
APP_MACOS="$APP_CONTENTS/MacOS"
APP_RESOURCES="$APP_CONTENTS/Resources"
ICONSET="$DIST_DIR/Pilgrim.iconset"
DMG_ROOT="$DIST_DIR/dmgroot"
VERSION_FILE="$ROOT_DIR/VERSION"
VERSION="${PILGRIM_VERSION:-$(tr -d '[:space:]' < "$VERSION_FILE")}"
DMG_PATH="$DIST_DIR/Pilgrim-of-the-Thorn-macOS-v$VERSION.dmg"
ZIP_PATH="$DIST_DIR/Pilgrim-of-the-Thorn-macOS-app-v$VERSION.zip"
IDENTIFIER="${PILGRIM_BUNDLE_ID:-com.pilgrimofthethorn.game}"

if [[ ! "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+([+-][0-9A-Za-z.-]+)?$ ]]; then
  echo "Invalid semantic version in $VERSION_FILE: $VERSION" >&2
  exit 1
fi

export COPYFILE_DISABLE=1

cd "$ROOT_DIR"
mkdir -p "$BUILD_DIR"
make build/pilgrim

if [[ -e "$DIST_DIR" ]]; then
  if ! rm -rf "$DIST_DIR" 2>/dev/null; then
    stale_dir="${TMPDIR:-/tmp}/pilgrim-macos-stale-$(date +%s)"
    echo "Could not remove $DIST_DIR; moving it to $stale_dir" >&2
    mv "$DIST_DIR" "$stale_dir"
  fi
fi
mkdir -p "$APP_MACOS" "$APP_RESOURCES/assets"

cp -X "$BUILD_DIR/pilgrim" "$APP_MACOS/pilgrim-bin"
chmod 755 "$APP_MACOS/pilgrim-bin"

cat > "$APP_MACOS/pilgrim" <<'SH'
#!/usr/bin/env bash
set -euo pipefail
APP_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$APP_DIR/Contents/Resources"
exec "$APP_DIR/Contents/MacOS/pilgrim-bin"
SH
chmod 755 "$APP_MACOS/pilgrim"

cp -X "$ROOT_DIR/assets/"*.png "$APP_RESOURCES/assets/"
cp -X "$ROOT_DIR/assets/pilgrim_tuning.txt" "$APP_RESOURCES/assets/"

mkdir -p "$ICONSET"
sips -c 887 887 "$ROOT_DIR/assets/cathedral_concept.png" --out "$DIST_DIR/pilgrim_icon_base.png" >/dev/null
for size in 16 32 128 256 512; do
  sips -z "$size" "$size" "$DIST_DIR/pilgrim_icon_base.png" --out "$ICONSET/icon_${size}x${size}.png" >/dev/null
  double_size=$((size * 2))
  sips -z "$double_size" "$double_size" "$DIST_DIR/pilgrim_icon_base.png" --out "$ICONSET/icon_${size}x${size}@2x.png" >/dev/null
done
iconutil -c icns "$ICONSET" -o "$APP_RESOURCES/Pilgrim.icns"

cat > "$APP_CONTENTS/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleDevelopmentRegion</key>
  <string>en</string>
  <key>CFBundleDisplayName</key>
  <string>$APP_NAME</string>
  <key>CFBundleExecutable</key>
  <string>pilgrim</string>
  <key>CFBundleIdentifier</key>
  <string>$IDENTIFIER</string>
  <key>CFBundleIconFile</key>
  <string>Pilgrim</string>
  <key>CFBundleInfoDictionaryVersion</key>
  <string>6.0</string>
  <key>CFBundleName</key>
  <string>$APP_NAME</string>
  <key>CFBundlePackageType</key>
  <string>APPL</string>
  <key>CFBundleShortVersionString</key>
  <string>$VERSION</string>
  <key>CFBundleVersion</key>
  <string>$VERSION</string>
  <key>LSMinimumSystemVersion</key>
  <string>10.15</string>
  <key>NSHighResolutionCapable</key>
  <true/>
</dict>
</plist>
PLIST

mkdir -p "$DMG_ROOT"
ditto --norsrc "$APP_BUNDLE" "$DMG_ROOT/$APP_NAME.app"
ln -s /Applications "$DMG_ROOT/Applications"
xattr -cr "$APP_BUNDLE" "$DMG_ROOT" 2>/dev/null || true
find "$APP_BUNDLE" "$DMG_ROOT" -exec xattr -d com.apple.provenance {} \; 2>/dev/null || true
find "$APP_BUNDLE" "$DMG_ROOT" -name '._*' -delete

hdiutil create \
  -volname "$APP_NAME" \
  -srcfolder "$DMG_ROOT" \
  -ov \
  -format UDZO \
  "$DMG_PATH" >/dev/null

ditto -c -k --norsrc --keepParent "$APP_BUNDLE" "$ZIP_PATH"
rm -rf "$ICONSET" "$DMG_ROOT" "$DIST_DIR/pilgrim_icon_base.png"

echo "Created app bundle: $APP_BUNDLE"
echo "Created drag-and-drop DMG: $DMG_PATH"
echo "Created app zip:    $ZIP_PATH"
