#!/usr/bin/env bash
# Downloads the open-source (SIL OFL 1.1) Arabic fonts bundled into Azkare.
# Usage: scripts/fetch-fonts.sh [destination-dir]
# Works with bash 3.2+ (macOS). Exits non-zero if any download failed.
set -u
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEST="${1:-$ROOT/resources/fonts}"
BASE="https://raw.githubusercontent.com/google/fonts/main/ofl"
mkdir -p "$DEST"
failed=0

fetch() { # <file name> <url path under $BASE>
    local name="$1" url="$BASE/$2"
    if [ -s "$DEST/$name" ]; then echo "  $name (cached)"; return 0; fi
    if curl -fsSL --retry 3 --retry-delay 2 -o "$DEST/$name.part" "$url" && [ -s "$DEST/$name.part" ]; then
        mv "$DEST/$name.part" "$DEST/$name"; echo "  $name"
    else
        rm -f "$DEST/$name.part"; echo "  $name - download failed ($url)" >&2; failed=$((failed + 1))
    fi
}

echo "Fetching Arabic fonts into $DEST"
fetch Amiri-Regular.ttf             "amiri/Amiri-Regular.ttf"
fetch Amiri-Bold.ttf                "amiri/Amiri-Bold.ttf"
fetch ScheherazadeNew-Regular.ttf   "scheherazadenew/ScheherazadeNew-Regular.ttf"
fetch ScheherazadeNew-Bold.ttf      "scheherazadenew/ScheherazadeNew-Bold.ttf"
fetch NotoNaskhArabic.ttf           "notonaskharabic/NotoNaskhArabic%5Bwght%5D.ttf"
fetch Cairo.ttf                     "cairo/Cairo%5Bslnt%2Cwght%5D.ttf"
fetch IBMPlexSansArabic-Regular.ttf "ibmplexsansarabic/IBMPlexSansArabic-Regular.ttf"
fetch IBMPlexSansArabic-Bold.ttf    "ibmplexsansarabic/IBMPlexSansArabic-Bold.ttf"
fetch ArefRuqaa-Regular.ttf         "arefruqaa/ArefRuqaa-Regular.ttf"
fetch ArefRuqaa-Bold.ttf            "arefruqaa/ArefRuqaa-Bold.ttf"

if [ "$failed" -gt 0 ]; then echo "$failed font(s) failed - the app falls back to system fonts." >&2; exit 1; fi
fc-cache -f >/dev/null 2>&1 || true
exit 0
