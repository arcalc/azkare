# Adhkar audit

Generated from `resources/data/adhkar.json` and `python3 scripts/audit_adhkar.py` (re-run it any time).

## Important limits (read first)
* **No reference sources were supplied in the conversation**, so nothing could be checked against your own Hisn al-Muslim / Sahih International files.
  As a substitute, public texts were used purely as a cross-check: Quran Uthmani Hafs and Saheeh International (`fawazahmed0/quran-api`) and a
  Hisn al-Muslim-based Arabic text (`rn0x/Adhkar-json`). These are third-party transcriptions, **not scholarly editions**.
* That Hisn-based text has **no hadith citations** (0 of 267 items) and is **inconsistently vocalized**, so for Prophetic adhkar the *words* and
  *counts* are checked, but **diacritics, English translations and hadith numbers are unverified** and flagged.
* The Saheeh International dataset omits verse-final punctuation; multi-verse translations therefore use ` / ` as a verse-boundary marker.
* Every entry has `source_ar` and `source_en`, shown together by the overlay, notifications and eww backend. `transliteration` was removed (unverified).

## Totals (44 entries)
| Check | Verified | Not verified |
|---|---|---|
| Arabic text | 11 Quranic verbatim + 30 Prophetic words matched to the Hisn text | 3 not found in reference (g003, g004, g006); diacritics of the 30 Prophetic entries not verifiable |
| English translation | 11 (Saheeh International, verbatim) | 33 |
| Reference | 10 (Quran surah:ayah) | 34 (hadith numbers) |
| Bilingual `source_ar` + `source_en` | 44 / 44 | 0 |

Legend: ✅ verified against the reference · ◐ words verified, tashkeel not · ⚠ unverified / flagged · — not checkable

## Per entry
| ID | Arabic text | Count | English | Reference | What changed / note |
|---|---|---|---|---|---|
| m001 | ◐ words match Hisn | ✅ 1 | ⚠ unverified | ⚠ number unverified | sources made bilingual only |
| m002 | ◐ words match Hisn | ✅ 1 | ⚠ unverified | ⚠ number unverified | sources made bilingual only |
| m003 | ✅ verbatim Uthmani | ✅ 1 | ✅ SI verbatim | ✅ surah:ayah | Full verse, verbatim (Uthmani Hafs); English now SI (was my wording) |
| m004 | ✅ verbatim Uthmani | ✅ 3 | ✅ SI verbatim | ⚠ number unverified | Was Ikhlas only. Now Basmala+Ikhlas, Falaq, Nas as in the Hisn item (verbatim); count 3 confirmed |
| m005 | ◐ words match Hisn | ✅ 1 | ⚠ unverified | ⚠ number unverified | sources made bilingual only |
| m006 | ◐ words match Hisn | ✅ 1 | ⚠ unverified | ⚠ number unverified | sources made bilingual only |
| m007 | ◐ words match Hisn | ✅ 3 | ⚠ unverified | ⚠ number unverified | sources made bilingual only |
| m008 | ◐ words match Hisn | ✅ 3 | ⚠ unverified | ⚠ number unverified | sources made bilingual only |
| m009 | ◐ words match Hisn | ✅ 3 | ⚠ unverified | ⚠ number unverified | sources made bilingual only |
| m010 | ◐ words match Hisn | ✅ 1 | ⚠ unverified | ⚠ number unverified | sources made bilingual only |
| e001 | ◐ words match (evening variant) | ✅ 1 | ⚠ unverified | ⚠ number unverified | Evening variant = morning text with Hisn's morning->evening wording |
| e002 | ◐ words match Hisn | ✅ 1 | ⚠ unverified | ⚠ number unverified | sources made bilingual only |
| e003 | ◐ words match (evening variant) | ✅ 4 | ⚠ unverified | ⚠ number unverified | Evening variant (as e001) |
| e004 | ◐ words match (evening variant) | ✅ 1 | ⚠ unverified | ⚠ number unverified | Evening variant (as e001) |
| e005 | ◐ words match Hisn | ✅ 3 | ⚠ unverified | ⚠ number unverified | sources made bilingual only |
| s001 | ◐ words match Hisn | ✅ 1 | ⚠ unverified | ⚠ number unverified | sources made bilingual only |
| s002 | ◐ words match Hisn | ✅ 1 | ⚠ unverified | ⚠ number unverified | count 3 -> 1 (Hisn reference) |
| s003 | ◐ words match Hisn | ✅ 1 | ⚠ unverified | ⚠ number unverified | sources made bilingual only |
| pp001 | ◐ words match Hisn | ✅ 3 | ⚠ unverified | ⚠ number unverified | sources made bilingual only |
| pp002 | ◐ words match Hisn | ✅ 1 | ⚠ unverified | ⚠ number unverified | sources made bilingual only |
| pp003 | ◐ words match Hisn | ✅ 3 | ⚠ unverified | ⚠ number unverified | count 1 -> 3 ('[ثلاثاً]' in Hisn text) |
| pp004 | ◐ words match Hisn | ✅ 33 | ⚠ unverified | ⚠ number unverified | sources made bilingual only |
| pp005 | ◐ words match Hisn | ✅ 33 | ⚠ unverified | ⚠ number unverified | sources made bilingual only |
| pp006 | ◐ words match Hisn | ✅ 33 | ⚠ unverified | ⚠ number unverified | count 34 -> 33 (Hisn: ثلاثاً وثلاثين) |
| pp007 | ◐ words match Hisn | ✅ 1 | ⚠ unverified | ⚠ number unverified | Editorial dash -> ، as in Hisn. Hisn marks the first sentence x3; count left 1 (combined entry) |
| i001 | ◐ words match Hisn | ✅ 1 | ⚠ unverified | ⚠ number unverified | count 3 -> 1 (Hisn reference) |
| i002 | ◐ words match Hisn | ✅ 100 | ⚠ unverified | ⚠ number unverified | Wording الرَّحِيمُ -> الْغَفُورُ per Hisn text; count 100 kept (narration says مائة مرة) |
| i003 | ◐ words match Hisn | ✅ 100 | ⚠ unverified | ⚠ number unverified | sources made bilingual only |
| i004 | ✅ verbatim Uthmani | — 1 | ✅ SI verbatim | ✅ surah:ayah | Verbatim 21:87 fragment; English SI; dropped unverified hadith attribution |
| i005 | ◐ words match Hisn | ✅ 1 | ⚠ unverified | ⚠ number unverified | Missing words أَشْهَدُ أَنْ added; count 3 -> 1 |
| g001 | ✅ verbatim Uthmani | — 1 | ✅ SI verbatim | ✅ surah:ayah | Verbatim 2:201 fragment; English SI; dropped unverified hadith attribution |
| g002 | ◐ words match Hisn | ✅ 1 | ⚠ unverified | ⚠ number unverified | count 3 -> 1 (Hisn reference) |
| g003 | ⚠ not in reference | — 1 | ⚠ unverified | ⚠ number unverified | FLAG: not found in Hisn-based text; unchanged |
| g004 | ⚠ not in reference | — 1 | ⚠ unverified | ⚠ number unverified | FLAG: not found in Hisn-based text; unchanged |
| g005 | ◐ words match Hisn | ✅ 1 | ⚠ unverified | ⚠ number unverified | Articles corrected (الْعِلْمَ النَّافِعَ -> عِلْمًا نَافِعًا...); corrected wording now found in Hisn text |
| g006 | ⚠ not in reference | — 1 | ⚠ unverified | ⚠ number unverified | FLAG: not found in Hisn-based text; unchanged |
| g007 | ✅ verbatim Uthmani | — 1 | ✅ SI verbatim | ✅ surah:ayah | Verbatim 3:173 fragment; English SI; dropped unverified hadith attribution |
| g008 | ◐ words match Hisn | ✅ 1 | ⚠ unverified | ⚠ number unverified | sources made bilingual only |
| q001 | ✅ verbatim Uthmani | — 1 | ✅ SI verbatim | ✅ surah:ayah | Verbatim 3:8 (source diacritics adopted); English SI |
| q002 | ✅ verbatim Uthmani | — 1 | ✅ SI verbatim | ✅ surah:ayah | Verbatim 20:114 fragment (tanween-alef joined); English SI |
| q003 | ✅ verbatim Uthmani | — 1 | ✅ SI verbatim | ✅ surah:ayah | Verbatim 20:25-28 minus narrative قَالَ; fixed لِيٓ (was لِي); English SI |
| q004 | ✅ verbatim Uthmani | — 1 | ✅ SI verbatim | ✅ surah:ayah | Verbatim 25:74 fragment; English SI (same words as before) |
| q005 | ✅ verbatim Uthmani | — 1 | ✅ SI verbatim | ✅ surah:ayah | Extended from a mid-verse cut to the complete dua (end of 59:10); English SI |
| q006 | ✅ verbatim Uthmani | — 1 | ✅ SI verbatim | ✅ surah:ayah | Removed narrative وَقُل; verbatim 23:97-98; English SI |
