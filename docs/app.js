'use strict';
/* Azkare site: bilingual RTL/LTR, light/dark, OS-aware install box, copy buttons, tabs, latest tag. */

const RAW = 'https://raw.githubusercontent.com/arcalc/azkare/main';
const CMD = {
  unix: `curl -fsSL ${RAW}/install.sh | bash`,
  win: `irm ${RAW}/install.ps1 | iex`,
};
const LABELS = {
  copy: { ar: 'نسخ', en: 'Copy' },
  copied: { ar: 'تم النسخ ✓', en: 'Copied! ✓' },
};
const state = { lang: 'ar', theme: null, tag: null, noRelease: false };

const $ = (sel, root = document) => root.querySelector(sel);
const $$ = (sel, root = document) => Array.from(root.querySelectorAll(sel));
const store = {
  get(k) { try { return localStorage.getItem(k); } catch (_) { return null; } },
  set(k, v) { try { localStorage.setItem(k, v); } catch (_) { /* private mode */ } },
};

/* ── language ─────────────────────────────────────────────────────────────── */
function applyLang(lang) {
  state.lang = lang;
  const root = document.documentElement;
  root.lang = lang;
  root.dir = lang === 'ar' ? 'rtl' : 'ltr';
  $$('[data-ar]').forEach((el) => {
    const v = el.dataset[lang];
    if (v !== undefined) el.textContent = v;
  });
  const btn = $('#langBtn span');
  if (btn) btn.textContent = lang === 'ar' ? 'English' : 'عربي';
  renderVersion();
  store.set('az-lang', lang);
}

/* ── theme ────────────────────────────────────────────────────────────────── */
function applyTheme(theme) {
  state.theme = theme;
  document.documentElement.dataset.theme = theme;
  const meta = $('meta[name="theme-color"]');
  if (meta) meta.content = theme === 'dark' ? '#243C4C' : '#E9E6E7';
  store.set('az-theme', theme);
}
function initialTheme() {
  const saved = store.get('az-theme');
  if (saved === 'light' || saved === 'dark') return saved;
  return window.matchMedia && matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light';
}

/* ── OS detection ─────────────────────────────────────────────────────────── */
function detectOS() {
  const hint = ((navigator.userAgentData && navigator.userAgentData.platform) || navigator.platform || '') + ' ' + (navigator.userAgent || '');
  const s = hint.toLowerCase();
  if (s.includes('win')) return 'windows';
  if (s.includes('mac') || s.includes('iphone') || s.includes('ipad')) return 'macos';
  return 'linux';
}
function applyOS(os) {
  activateTab(os, false);
  const code = $('#heroCmd');
  const pill = $('#osPill');
  if (!code) return;
  code.textContent = os === 'windows' ? CMD.win : CMD.unix;
  code.dataset.prompt = os === 'windows' ? 'PS>' : os === 'macos' ? '%' : '$';
  if (pill) pill.textContent = { windows: 'Windows', macos: 'macOS', linux: 'Linux' }[os];
}

/* ── tabs ─────────────────────────────────────────────────────────────────── */
function activateTab(id, focus) {
  $$('.tab').forEach((t) => {
    const on = t.dataset.tab === id;
    t.setAttribute('aria-selected', String(on));
    t.tabIndex = on ? 0 : -1;
    if (on && focus) t.focus();
  });
  $$('.panel').forEach((p) => { p.hidden = p.id !== `panel-${id}`; });
}
function onTabKey(e) {
  const tabs = $$('.tab');
  const i = tabs.indexOf(document.activeElement);
  if (i < 0) return;
  const rtl = document.documentElement.dir === 'rtl';
  const step = { ArrowRight: rtl ? -1 : 1, ArrowLeft: rtl ? 1 : -1 }[e.key];
  let next = null;
  if (step) next = (i + step + tabs.length) % tabs.length;
  else if (e.key === 'Home') next = 0;
  else if (e.key === 'End') next = tabs.length - 1;
  if (next === null) return;
  e.preventDefault();
  activateTab(tabs[next].dataset.tab, true);
}

/* ── copy ─────────────────────────────────────────────────────────────────── */
async function copyFrom(btn) {
  const target = document.getElementById(btn.dataset.copy);
  if (!target) return;
  const text = target.textContent.trim();       // the prompt is CSS-only, so this is the exact command
  let ok = false;
  try { await navigator.clipboard.writeText(text); ok = true; } catch (_) {
    const ta = document.createElement('textarea');
    ta.value = text; ta.style.cssText = 'position:fixed;opacity:0';
    document.body.appendChild(ta); ta.select();
    try { ok = document.execCommand('copy'); } catch (_) { ok = false; }
    ta.remove();
  }
  if (!ok) return;
  btn.classList.add('done');
  btn.textContent = LABELS.copied[state.lang];
  clearTimeout(btn._t);
  btn._t = setTimeout(() => { btn.classList.remove('done'); btn.textContent = LABELS.copy[state.lang]; }, 2000);
}

/* ── latest version from GitHub (release first, newest v* tag as fallback) ─── */
function semverKey(t) { return t.replace(/^v/, '').split('.').map((n) => parseInt(n, 10) || 0); }
function newer(a, b) { const x = semverKey(a), y = semverKey(b); for (let i = 0; i < 3; i++) { if (x[i] !== y[i]) return x[i] > y[i]; } return false; }

async function fetchLatest() {
  const api = 'https://api.github.com/repos/arcalc/azkare';
  const opts = { headers: { Accept: 'application/vnd.github+json' } };
  try {
    const r = await fetch(`${api}/releases/latest`, opts);
    if (r.ok) { state.tag = (await r.json()).tag_name; return; }
    if (r.status !== 404) throw new Error(String(r.status));
    const t = await fetch(`${api}/tags?per_page=100`, opts);
    if (!t.ok) throw new Error(String(t.status));
    const tags = (await t.json()).map((x) => x.name).filter((n) => /^v?\d+\.\d+\.\d+$/.test(n));
    if (tags.length) state.tag = tags.reduce((m, n) => (newer(n, m) ? n : m));
    else state.noRelease = true;
  } catch (_) { state.tag = null; }
}
function renderVersion() {
  const el = $('#verText');
  if (!el) return;
  const ar = state.lang === 'ar';
  if (state.tag) el.textContent = ar ? `أحدث إصدار: ${state.tag}` : `Latest: ${state.tag}`;
  else if (state.noRelease) el.textContent = ar ? 'لا إصدار منشور بعد — يُثبَّت من فرع main' : 'No release published yet — installs from main';
  else el.textContent = 'github.com/arcalc/azkare';
}

/* ── boot ─────────────────────────────────────────────────────────────────── */
function init() {
  if (init.done) return;
  init.done = true;
  const lang = store.get('az-lang');
  applyTheme(initialTheme());
  applyLang(lang === 'en' ? 'en' : 'ar');
  applyOS(detectOS());

  $('#langBtn').addEventListener('click', () => applyLang(state.lang === 'ar' ? 'en' : 'ar'));
  $('#themeBtn').addEventListener('click', () => applyTheme(state.theme === 'dark' ? 'light' : 'dark'));
  $$('.tab').forEach((t) => t.addEventListener('click', () => activateTab(t.dataset.tab, false)));
  $('.tabs').addEventListener('keydown', onTabKey);
  document.addEventListener('click', (e) => { const b = e.target.closest('.copy[data-copy]'); if (b) copyFrom(b); });

  renderVersion();
  fetchLatest().then(renderVersion);
}
document.addEventListener('DOMContentLoaded', init);
