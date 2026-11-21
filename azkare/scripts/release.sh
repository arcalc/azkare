#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════════════════════
# Azkare — Automated Version Bump & Release Script
#
# Usage:
#   ./scripts/release.sh patch      # 1.0.0 → 1.0.1
#   ./scripts/release.sh minor      # 1.0.0 → 1.1.0
#   ./scripts/release.sh major      # 1.0.0 → 2.0.0
#   ./scripts/release.sh 1.2.3      # explicit version
# ═══════════════════════════════════════════════════════════════════════════════
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERSION_FILE="${REPO_ROOT}/VERSION"
BUMP="${1:-patch}"

# ── Read current version ──────────────────────────────────────────────────────
CURRENT=$(cat "$VERSION_FILE" | tr -d '[:space:]')
IFS='.' read -r MAJOR MINOR PATCH <<< "$CURRENT"

# ── Compute new version ───────────────────────────────────────────────────────
case "$BUMP" in
    patch)   NEW_VER="${MAJOR}.${MINOR}.$((PATCH + 1))" ;;
    minor)   NEW_VER="${MAJOR}.$((MINOR + 1)).0" ;;
    major)   NEW_VER="$((MAJOR + 1)).0.0" ;;
    [0-9]*.*) NEW_VER="$BUMP" ;;
    *)
        echo "Usage: $0 [patch|minor|major|X.Y.Z]"
        exit 1
        ;;
esac

TAG="v${NEW_VER}"

echo "Current version : ${CURRENT}"
echo "New version     : ${NEW_VER} (${TAG})"
echo ""
read -rp "Proceed? [y/N] " confirm
[[ "$confirm" =~ ^[Yy]$ ]] || { echo "Aborted."; exit 0; }

# ── Ensure working tree is clean ──────────────────────────────────────────────
if ! git -C "$REPO_ROOT" diff --quiet HEAD; then
    echo "ERROR: Working tree has uncommitted changes. Commit or stash first."
    exit 1
fi

# ── Write new version ─────────────────────────────────────────────────────────
echo "$NEW_VER" > "$VERSION_FILE"

# ── Commit + tag + push ───────────────────────────────────────────────────────
git -C "$REPO_ROOT" add "$VERSION_FILE"
git -C "$REPO_ROOT" commit -m "chore(release): ${TAG}"
git -C "$REPO_ROOT" tag -a "$TAG" -m "Release ${TAG}"
git -C "$REPO_ROOT" push origin main
git -C "$REPO_ROOT" push origin "$TAG"

echo ""
echo "✓ Released ${TAG}"
echo "  GitHub Actions will now build and publish the release automatically."
echo "  View: https://github.com/arcalc/azkare/releases/tag/${TAG}"
