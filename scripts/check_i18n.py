#!/usr/bin/env python3
"""Checks that ar.json and en.json have identical keys and that every translation key used in
the C++ sources (app.*, tray.*, settings.*) exists. Exits non-zero on any problem."""
import json, re, sys, pathlib

root = pathlib.Path(__file__).resolve().parent.parent

def flat(d, p=""):
    out = {}
    for k, v in d.items():
        key = f"{p}.{k}" if p else k
        out.update(flat(v, key) if isinstance(v, dict) else {key: v})
    return out

ar = flat(json.load(open(root / "resources/i18n/ar.json", encoding="utf-8")))
en = flat(json.load(open(root / "resources/i18n/en.json", encoding="utf-8")))
errors = []
for k in sorted(set(ar) ^ set(en)):
    errors.append(f"key only in {'ar' if k in ar else 'en'}: {k}")
for k, v in {**ar, **{f"en:{k}": v for k, v in en.items()}}.items():
    if not str(v).strip():
        errors.append(f"empty translation: {k}")

used = set()
pat = re.compile(r'"((?:app|tray|settings)\.[a-z0-9_]+)"')
for path in (root / "src").rglob("*.cpp"):
    used.update(pat.findall(path.read_text(encoding="utf-8")))
for k in sorted(used - set(ar)):
    errors.append(f"used in code but missing from ar.json/en.json: {k}")
unused = sorted(set(ar) - used)

for e in errors:
    print("ERROR:", e)
print(f"{len(ar)} keys, {len(used)} used in code" + (f", unused: {', '.join(unused)}" if unused else ""))
sys.exit(1 if errors else 0)
