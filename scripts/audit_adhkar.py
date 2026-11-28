#!/usr/bin/env python3
"""Re-verifies every verification claim in resources/data/adhkar.json against reference texts.

  python3 scripts/audit_adhkar.py [--refs DIR] [--markdown FILE]

Checks
  * quran-verbatim : text_ar equals the Uthmani Hafs words of the listed ayat (only change: the stray space the
                     dataset puts before a tanween-alef is removed); every English piece (split on ' / ') is a
                     verbatim substring of the Saheeh International ayah.
  * hisn-match*    : the entry's words (alef-insensitive, diacritics ignored) occur contiguously in a Hisn
                     al-Muslim reference item (evening variants: after the morning->evening word swap), and `count`
                     agrees with the reference count or with a documented count in the reference text.
  * not-in-reference : confirms the entry is still NOT found (so the flag stays honest).
Diacritics of Prophetic adhkar cannot be verified: the Hisn-based reference is itself inconsistently vocalized.
Exit code 1 if any claim fails.
"""
import json, re, sys, pathlib, argparse, unicodedata

ROOT = pathlib.Path(__file__).resolve().parent.parent
ap = argparse.ArgumentParser()
ap.add_argument("--refs", default=str(ROOT / ".audit-refs"))
ap.add_argument("--markdown")
args = ap.parse_args()
REFS = pathlib.Path(args.refs)

MARKS = re.compile('[\u0610-\u061A\u064B-\u065F\u0670\u06D6-\u06ED\u08D3-\u08FF\u0640\u200c-\u200f]')
def skel_words(s):
    s = re.sub('[﴿﴾()\\[\\]*،,.:;!؟?"«»\u06DD]|\\d+', ' ', s)
    s = MARKS.sub('', s).translate(str.maketrans({'أ':'ا','إ':'ا','آ':'ا','ٱ':'ا','ؤ':'و','ئ':'ي','ى':'ي','ة':'ه'})).replace('ا', '')
    return s.split()
COUNTW = r'(?:ثلاث\S*|وثلاثين|مرات|مرة|مائة|عشر\S*|اربع\S*|أربع\S*|سبع\S*)'
INSTR = re.compile(r'[\[(]\s*' + COUNTW + r'(?:\s+' + COUNTW + r'){0,2}\s*[\])]')   # "(ثلاثاً)", "[ثلاثاً]", "(ثلاثاً وثلاثين)"
def ref_words(t):
    return skel_words(INSTR.sub(' ', MARKS.sub('', t)))   # only pure count instructions are dropped; bracketed dhikr text stays
def fix_alef(s): return re.sub('(\u08F0[\u064B-\u065F\u0670]*)\\s+ا(?=\\s|$)', r'\1ا', s)
def load(p): return json.load(open(p, encoding="utf-8"))

data = load(ROOT / "resources/data/adhkar.json")
try:
    AR = {(v["chapter"], v["verse"]): v["text"] for v in load(REFS / "ar.json")["quran"]}
    EN = {(v["chapter"], v["verse"]): v["text"] for v in load(REFS / "en.json")["quran"]}
    REF = [(unicodedata.normalize('NFKC', c["category"]), it["id"], it["count"], it["text"]) for c in load(REFS / "adhkar_ref.json") for it in c["array"]]
except FileNotFoundError as ex:
    sys.exit(f"Reference file missing ({ex.filename}). Run scripts/fetch-audit-refs.sh or pass --refs DIR.")

def to_morning(w): return w.replace('مسين', 'صبحن').replace('مسيت', 'صبحت').replace('مسي', 'صبح')  # أمسينا/أمسى/أمسيت -> أصبحنا/أصبح/أصبحت
# counts the reference states in its TEXT rather than in the `count` field
TEXT_COUNTS = {  # id -> (category fragment, item id, regex that must appear in the item text, expected count)
    'pp001': ('بعد السلام', 1, 'ثلاث', 3), 'pp003': ('بعد السلام', 2, 'ثلاث', 3), 'i002': ('المجلس', 1, 'مائة|مِائَة', 100),
    'pp004': ('بعد السلام', 4, 'ثلاثا وثلاثين|ثلاثاً وثلاثين', 33), 'pp005': ('بعد السلام', 4, 'ثلاثا وثلاثين|ثلاثاً وثلاثين', 33),
    'pp006': ('بعد السلام', 4, 'ثلاثا وثلاثين|ثلاثاً وثلاثين', 33)}

def contiguous(words, hay):
    n = len(words)
    return any(hay[k:k + n] == words for k in range(len(hay) - n + 1))

def check_count(e, words):
    i, want = e["id"], e["count"]
    if i in TEXT_COUNTS:
        cat, iid, rx, exp = TEXT_COUNTS[i]
        item = next((t for c, x, n, t in REF if cat in c and x == iid), None)
        plain = re.sub('[\u064B-\u065F\u0670]', '', item) if item else ''
        ok = bool(item) and want == exp and re.search(rx, plain) is not None
        return ok, f"count {want}: stated in reference item {iid} ({exp})" if ok else f"count {want}: NOT supported by reference text"
    counts = set()
    for c, x, n, t in REF:
        if contiguous(words, ref_words(t)): counts.add(n)
    return (want in counts), (f"count {want}: matches reference" if want in counts else f"count {want}: reference says {sorted(counts)}")

rows, failures = [], 0
for cat in data["categories"]:
    for e in cat["entries"]:
        v, i = e["verification"], e["id"]
        t_ok, t_msg, c_ok, c_msg, tr_msg = True, "", True, "", ""
        if v["text"] == "quran-verbatim":
            parts, pieces = [], []
            for s in e["quran"]:
                for k, a in enumerate(range(s["ayah_from"], s["ayah_to"] + 1)):
                    toks = AR[(s["surah"], a)].split()
                    parts.append(' '.join(toks[s["skip_words"]:] if k == 0 else toks)); pieces.append((s["surah"], a))
            built = fix_alef(' '.join(parts))
            t_ok = built == e["text_ar"]; t_msg = "Arabic == Uthmani Hafs text" if t_ok else "Arabic differs from reference"
            en = e["translation_en"].split(' / ')
            tr_ok = len(en) == len(pieces) and all(p in EN[k] for p, k in zip(en, pieces))
            tr_msg = "English verbatim in Saheeh International" if tr_ok else "English NOT verbatim"
            if not tr_ok: failures += 1
            c_ok, c_msg = True, f"count {e['count']}: n/a (the Quran text states no count)"
            if i in ('m003', 'm004'):
                iid = 1 if i == 'm003' else 2
                item = next((n for c, x, n, t in REF if 'الصباح' in c and x == iid), None)
                c_ok = item == e['count']; c_msg = f"count {e['count']}: matches reference item {iid}" if c_ok else "count differs"
        else:
            words = skel_words(e["text_ar"])
            if v["text"] == "hisn-match-evening": words = [to_morning(w) for w in words]
            found = any(contiguous(words, ref_words(t)) for _, _, _, t in REF)
            if v["text"] == "not-in-reference":
                t_ok = not found; t_msg = "not found in reference (flag confirmed)" if t_ok else "NOW FOUND: update verification"
                c_ok, c_msg = True, "count not checkable"
            else:
                t_ok = found; t_msg = "words match Hisn reference" + (" (evening variant)" if v["text"].endswith("evening") else "") if found else "words NOT found in reference"
                c_ok, c_msg = check_count(e, words)
            tr_msg = "unverified (no source supplied)"
        if v["translation"] == "saheeh-international" and v["text"] != "quran-verbatim": t_ok = False; t_msg = "inconsistent verification flags"
        if not (t_ok and c_ok): failures += 1
        rows.append((i, t_ok, t_msg, c_ok, c_msg, tr_msg, v["reference"]))

for r in rows:
    print(f"{r[0]:5} text:{'OK ' if r[1] else 'FAIL'} {r[2]:<42} | {r[4]:<48} | translation: {r[5]} | reference: {r[6]}")
print(f"\n{len(rows)} entries audited, {failures} failed claim(s)")
if args.markdown:
    with open(args.markdown, "w", encoding="utf-8") as f:
        f.write("| ID | Arabic text | Count | English translation | Reference |\n|---|---|---|---|---|\n")
        for r in rows: f.write(f"| {r[0]} | {'✅ ' if r[1] else '❌ '}{r[2]} | {'✅ ' if r[3] else '❌ '}{r[4]} | {r[5]} | {r[6]} |\n")
sys.exit(1 if failures else 0)
