#!/usr/bin/env python3
"""Validates resources/data/adhkar.json (schema azkare-adhkar-v2). Offline; exit 1 on any ERROR.

Every entry needs Arabic text, an English translation, an integer `count`, and BOTH
`source_ar` (Arabic letters) and `source_en` (Latin letters), plus a `verification` block.
Entries that are not yet fully verified are listed as WARNINGS (use --strict to fail on them).
For checks against reference texts (Quran, Hisn al-Muslim) run scripts/audit_adhkar.py.
"""
import json, re, sys, pathlib

PATH = pathlib.Path(__file__).resolve().parent.parent / "resources/data/adhkar.json"
STRICT = "--strict" in sys.argv
AR = re.compile(r"[\u0600-\u06FF]")
LAT = re.compile(r"[A-Za-z]")
TEXT_OK = {"quran-verbatim", "hisn-match", "hisn-match-evening", "not-in-reference"}
TRANS_OK = {"saheeh-international", "unverified"}
REF_OK = {"verified", "unverified"}

d = json.load(open(PATH, encoding="utf-8"))
errors, warnings, ids, texts = [], [], set(), {}
count = 0
if d.get("schema") != "azkare-adhkar-v2":
    errors.append(f"schema must be azkare-adhkar-v2, got {d.get('schema')!r}")
for c in d.get("categories", []):
    for k in ("id", "name_ar", "name_en"):
        if not c.get(k):
            errors.append(f"category {c.get('id')}: missing {k}")
    if not c.get("entries"):
        errors.append(f"category {c.get('id')}: no entries")
    for e in c.get("entries", []):
        count += 1
        i = e.get("id")
        if not i:
            errors.append("entry without id"); continue
        if i in ids: errors.append(f"duplicate id {i}")
        ids.add(i)
        if not AR.search(e.get("text_ar", "")): errors.append(f"{i}: text_ar empty or not Arabic")
        if not e.get("translation_en"): errors.append(f"{i}: missing translation_en")
        if not isinstance(e.get("count"), int) or e["count"] < 1: errors.append(f"{i}: count must be an integer >= 1")
        sa, se = e.get("source_ar", ""), e.get("source_en", "")
        if not AR.search(sa): errors.append(f"{i}: source_ar must contain Arabic")
        if not se: errors.append(f"{i}: source_en missing")
        elif AR.search(se): errors.append(f"{i}: source_en must be English only (no Arabic letters)")
        elif not LAT.search(se): errors.append(f"{i}: source_en must contain Latin letters")
        if not sa: errors.append(f"{i}: source_ar missing")
        v = e.get("verification", {})
        if v.get("text") not in TEXT_OK: errors.append(f"{i}: verification.text must be one of {sorted(TEXT_OK)}")
        if v.get("translation") not in TRANS_OK: errors.append(f"{i}: verification.translation must be one of {sorted(TRANS_OK)}")
        if v.get("reference") not in REF_OK: errors.append(f"{i}: verification.reference must be one of {sorted(REF_OK)}")
        if v.get("text") == "quran-verbatim":
            if not e.get("quran"): errors.append(f"{i}: quran-verbatim entries need a 'quran' segment list")
            else:
                for s in e["quran"]:
                    if not all(isinstance(s.get(k), int) for k in ("surah", "ayah_from", "ayah_to", "skip_words")):
                        errors.append(f"{i}: bad quran segment {s}")
        if v.get("translation") == "unverified": warnings.append(f"{i}: translation not verified against a source")
        if v.get("reference") == "unverified": warnings.append(f"{i}: reference not verified")
        if v.get("text") == "not-in-reference": warnings.append(f"{i}: text not found in the Hisn al-Muslim reference")
        t = e.get("text_ar", "")
        if t in texts: errors.append(f"{i}: same text as {texts[t]}")
        texts[t] = i
if d.get("total") != count:
    errors.append(f'"total" is {d.get("total")} but the file contains {count} entries')

for e in errors: print("ERROR:", e)
unv_t = sum(1 for w in warnings if "translation" in w); unv_r = sum(1 for w in warnings if "reference not" in w)
nf = sum(1 for w in warnings if "not found" in w)
print(f"{count} entries in {len(d.get('categories', []))} categories | errors: {len(errors)} | "
      f"unverified translations: {unv_t} | unverified references: {unv_r} | text not in reference: {nf}")
if STRICT: [print("WARNING:", w) for w in warnings]
sys.exit(1 if errors or (STRICT and warnings) else 0)
