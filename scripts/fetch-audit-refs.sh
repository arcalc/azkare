#!/usr/bin/env bash
# Downloads the public reference texts used by scripts/audit_adhkar.py into .audit-refs/ (git-ignored).
#   ar.json          Quran, Uthmani Hafs text           (github.com/fawazahmed0/quran-api, edition ara-quranuthmanihaf)
#   en.json          Quran, Saheeh International        (same repo, edition eng-ummmuhammad)
#   adhkar_ref.json  Hisn al-Muslim-based Arabic text   (github.com/rn0x/Adhkar-json)
# These are third-party transcriptions used as a cross-check, NOT scholarly editions. Replace them with your own
# authoritative sources (same JSON layout) for a stricter audit.
set -eu
DEST="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/.audit-refs"
mkdir -p "$DEST"
Q="https://raw.githubusercontent.com/fawazahmed0/quran-api/1/editions"
curl -fsSL "$Q/ara-quranuthmanihaf.json" -o "$DEST/ar.json"
curl -fsSL "$Q/eng-ummmuhammad.json" -o "$DEST/en.json"
curl -fsSL "https://raw.githubusercontent.com/rn0x/Adhkar-json/main/adhkar.json" -o "$DEST/adhkar_ref.json"
echo "References saved to $DEST"
