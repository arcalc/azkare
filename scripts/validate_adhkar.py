#!/usr/bin/env python3
"""Validates resources/data/adhkar.json: schema, unique ids, Arabic text, references,
and that "total" matches the real number of entries. Exit code 1 on any error."""
import json, re, sys, pathlib

path = pathlib.Path(__file__).resolve().parent.parent / "resources/data/adhkar.json"
d = json.load(open(path, encoding="utf-8"))
errors, ids, texts = [], set(), {}
arabic = re.compile(r"[\u0600-\u06FF]")
count = 0
for c in d.get("categories", []):
    for k in ("id", "name_ar", "name_en"):
        if not c.get(k):
            errors.append(f"category {c.get('id')}: missing {k}")
    if not c.get("entries"):
        errors.append(f"category {c.get('id')}: no entries")
    for e in c.get("entries", []):
        count += 1
        i = e.get("id")
        if not i: errors.append("entry without id"); continue
        if i in ids: errors.append(f"duplicate id {i}")
        ids.add(i)
        if not arabic.search(e.get("text_ar", "")): errors.append(f"{i}: text_ar empty or not Arabic")
        if not e.get("translation_en"): errors.append(f"{i}: missing translation_en")
        if not isinstance(e.get("repeat", 1), int) or e.get("repeat", 1) < 1: errors.append(f"{i}: bad repeat")
        src = e.get("source", {})
        if not src.get("ref") or not src.get("source_name_ar"): errors.append(f"{i}: source.ref / source.source_name_ar required")
        if "url" in src and not src["url"].startswith("https://"): errors.append(f"{i}: url must be https")
        t = e.get("text_ar", "")
        if t in texts: errors.append(f"{i}: same text as {texts[t]}")
        texts[t] = i
if d.get("total") != count:
    errors.append(f'"total" is {d.get("total")} but the file contains {count} entries')
for e in errors: print("ERROR:", e)
print(f"{count} entries in {len(d.get('categories', []))} categories")
sys.exit(1 if errors else 0)
