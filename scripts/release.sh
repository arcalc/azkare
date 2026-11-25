#!/usr/bin/env bash
# Bump VERSION, commit, tag and push. A pushed v* tag triggers .github/workflows/release.yml.
#
#   ./scripts/release.sh patch|minor|major|X.Y.Z [-y]
set -eu

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
BUMP="${1:-}"
YES=0
if [ "${2:-}" = "-y" ] || [ "${2:-}" = "--yes" ]; then YES=1; fi
die() { echo "ERROR: $*" >&2; exit 1; }

[ -n "$BUMP" ] || die "usage: $0 patch|minor|major|X.Y.Z [-y]"
git rev-parse --git-dir >/dev/null 2>&1 || die "not a git repository"
[ "$(git rev-parse --abbrev-ref HEAD)" = "main" ] || die "releases are made from the main branch"
if ! git diff --quiet || ! git diff --cached --quiet; then die "uncommitted changes: commit or stash them first"; fi

CURRENT="$(tr -d '[:space:]' < VERSION)"
echo "$CURRENT" | grep -Eq '^[0-9]+\.[0-9]+\.[0-9]+$' || die "VERSION is not X.Y.Z: '$CURRENT'"
IFS=. read -r MAJOR MINOR PATCH <<EOV
$CURRENT
EOV
case "$BUMP" in
    patch) NEW="$MAJOR.$MINOR.$((PATCH + 1))" ;;
    minor) NEW="$MAJOR.$((MINOR + 1)).0" ;;
    major) NEW="$((MAJOR + 1)).0.0" ;;
    *) echo "$BUMP" | grep -Eq '^[0-9]+\.[0-9]+\.[0-9]+$' || die "'$BUMP' is not patch|minor|major|X.Y.Z"; NEW="$BUMP" ;;
esac
TAG="v$NEW"
HIGHEST="$(printf '%s\n%s\n' "$CURRENT" "$NEW" | sort -V | tail -n 1)"
if [ "$HIGHEST" != "$NEW" ] || [ "$CURRENT" = "$NEW" ]; then die "new version $NEW must be greater than $CURRENT"; fi

git fetch --quiet origin main --tags || die "cannot reach origin"
[ "$(git rev-parse HEAD)" = "$(git rev-parse origin/main)" ] || die "local main differs from origin/main: pull or push first"
git rev-parse -q --verify "refs/tags/$TAG" >/dev/null && die "tag $TAG already exists"

if command -v python3 >/dev/null 2>&1 && [ -f scripts/validate_adhkar.py ]; then
    python3 scripts/validate_adhkar.py >/dev/null || die "resources/data/adhkar.json failed validation"
    python3 scripts/check_i18n.py >/dev/null || die "translations failed validation"
fi

echo "Release: $CURRENT -> $NEW ($TAG)"
if [ "$YES" -ne 1 ]; then
    printf 'Proceed? [y/N] '; read -r a
    case "$a" in y|Y|yes) ;; *) echo "Aborted."; exit 0 ;; esac
fi

echo "$NEW" > VERSION
git add VERSION
git commit -q -m "chore(release): $TAG"
git tag -a "$TAG" -m "Release $TAG"
if ! git push origin main "refs/tags/$TAG"; then
    echo "Push failed. To undo locally: git tag -d $TAG && git reset --hard HEAD~1" >&2
    exit 1
fi
echo "Released $TAG - CI publishes it at https://github.com/arcalc/azkare/releases/tag/$TAG"
