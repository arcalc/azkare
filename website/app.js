/**
 * Azkare (أذكاري) — Website JS
 * Handles: bilingual RTL/LTR toggle, light/dark theme, GitHub release fetch,
 *           platform tab switcher, OS-detected download CTA.
 */

'use strict';

// ── State ─────────────────────────────────────────────────────────────────────
let currentLang  = 'ar';   // 'ar' | 'en'
let currentTheme = 'auto'; // 'auto' | 'light' | 'dark'
let releaseData  = null;

// ── Init ──────────────────────────────────────────────────────────────────────
document.addEventListener('DOMContentLoaded', () => {
  loadPreferences();
  applyLang(currentLang, false);
  applyTheme(currentTheme, false);
  bindEvents();
  detectPlatformTab();
  fetchRelease();
});

// ── Preferences (localStorage) ────────────────────────────────────────────────
function loadPreferences() {
  try {
    const lang  = localStorage.getItem('azkare-lang');
    const theme = localStorage.getItem('azkare-theme');
    if (lang  === 'ar' || lang  === 'en')                    currentLang  = lang;
    if (theme === 'light' || theme === 'dark' || theme === 'auto') currentTheme = theme;
  } catch (_) {}
}

function savePreferences() {
  try {
    localStorage.setItem('azkare-lang',  currentLang);
    localStorage.setItem('azkare-theme', currentTheme);
  } catch (_) {}
}

// ── Language ──────────────────────────────────────────────────────────────────
function applyLang(lang, save = true) {
  currentLang = lang;

  document.documentElement.setAttribute('lang', lang);
  document.documentElement.setAttribute('dir',  lang === 'ar' ? 'rtl' : 'ltr');

  // Update all elements that carry data-ar / data-en
  document.querySelectorAll('[data-ar]').forEach(el => {
    const val = lang === 'ar' ? el.dataset.ar : el.dataset.en;
    if (val !== undefined) el.textContent = val;
  });

  // Update the toggle button text
  const btn = document.getElementById('langToggle');
  if (btn) btn.querySelector('span').textContent = lang === 'ar' ? 'English' : 'عربي';

  if (save) savePreferences();
}

// ── Theme ─────────────────────────────────────────────────────────────────────
function applyTheme(theme, save = true) {
  currentTheme = theme;
  const root = document.documentElement;

  if (theme === 'light') root.setAttribute('data-theme', 'light');
  else if (theme === 'dark') root.setAttribute('data-theme', 'dark');
  else root.removeAttribute('data-theme');

  if (save) savePreferences();
}

function cycleTheme() {
  // auto → dark → light → auto
  if (currentTheme === 'auto')  applyTheme('dark');
  else if (currentTheme === 'dark')  applyTheme('light');
  else applyTheme('auto');
}

// ── Platform tab switcher ─────────────────────────────────────────────────────
function activateTab(tabId) {
  document.querySelectorAll('.tab-btn').forEach(btn => {
    const active = btn.dataset.tab === tabId;
    btn.classList.toggle('active', active);
    btn.setAttribute('aria-selected', String(active));
  });
  document.querySelectorAll('.tab-panel').forEach(panel => {
    panel.classList.toggle('active', panel.id === `panel-${tabId}`);
  });
}

function detectPlatformTab() {
  const ua = navigator.userAgent.toLowerCase();
  if (ua.includes('win'))   activateTab('windows');
  else if (ua.includes('mac')) activateTab('macos');
  else activateTab('linux');
}

// ── GitHub Release ────────────────────────────────────────────────────────────
async function fetchRelease() {
  const banner  = document.getElementById('versionText');
  const fallback = 'https://github.com/arcalc/azkare/releases';

  try {
    const res = await fetch('https://api.github.com/repos/arcalc/azkare/releases/latest', {
      headers: { 'Accept': 'application/vnd.github+json' }
    });

    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    releaseData = await res.json();

    const tag = releaseData.tag_name || 'v1.0.0';

    if (banner) {
      banner.textContent = currentLang === 'ar'
        ? `أحدث إصدار: ${tag}`
        : `Latest release: ${tag}`;
    }

    // Wire download buttons to actual release assets
    wireDownloadButtons(releaseData, fallback);

  } catch (err) {
    console.warn('[Azkare] Release fetch failed:', err.message);
    if (banner) {
      banner.textContent = currentLang === 'ar'
        ? 'تعذّر جلب الإصدار — تحقق من GitHub'
        : 'Could not fetch release — check GitHub';
    }
    // Wire buttons to releases page as fallback
    document.querySelectorAll('.btn-download').forEach(btn => {
      btn.href = fallback;
      btn.target = '_blank';
      btn.rel = 'noopener';
    });
  }
}

function wireDownloadButtons(release, fallback) {
  const assets = release.assets || [];

  const findAsset = (...keywords) => {
    for (const kw of keywords) {
      const asset = assets.find(a => a.name.toLowerCase().includes(kw));
      if (asset) return asset.browser_download_url;
    }
    return fallback;
  };

  const map = {
    'linux-appimage': findAsset('appimage', 'linux'),
    'linux-deb':      findAsset('.deb'),
    'windows-setup':  findAsset('setup', 'installer', '.exe', 'windows'),
    'macos-dmg':      findAsset('.dmg', 'macos', 'mac'),
  };

  document.querySelectorAll('.btn-download[data-asset]').forEach(btn => {
    const url = map[btn.dataset.asset] || fallback;
    btn.href   = url;
    btn.target = '_blank';
    btn.rel    = 'noopener';
  });

  // Update the main hero CTA too
  const heroCta = document.getElementById('heroCta');
  if (heroCta) {
    const ua = navigator.userAgent.toLowerCase();
    let primaryUrl = fallback;
    if (ua.includes('win'))    primaryUrl = map['windows-setup'];
    else if (ua.includes('mac')) primaryUrl = map['macos-dmg'];
    else primaryUrl = map['linux-appimage'];
    heroCta.href   = primaryUrl;
    heroCta.target = '_blank';
    heroCta.rel    = 'noopener';
  }
}

// ── Event bindings ────────────────────────────────────────────────────────────
function bindEvents() {
  // Language toggle
  document.getElementById('langToggle')?.addEventListener('click', () => {
    applyLang(currentLang === 'ar' ? 'en' : 'ar');
  });

  // Theme toggle
  document.getElementById('themeToggle')?.addEventListener('click', cycleTheme);

  // Platform tabs
  document.querySelectorAll('.tab-btn').forEach(btn => {
    btn.addEventListener('click', () => activateTab(btn.dataset.tab));
  });

  // Keyboard nav for tabs
  document.querySelector('.platform-tabs')?.addEventListener('keydown', e => {
    const tabs = [...document.querySelectorAll('.tab-btn')];
    const idx  = tabs.indexOf(document.activeElement);
    if (idx === -1) return;

    const isRtl = document.documentElement.dir === 'rtl';
    const next  = isRtl
      ? { ArrowLeft: 1, ArrowRight: -1 }
      : { ArrowLeft: -1, ArrowRight: 1 };

    if (next[e.key] !== undefined) {
      e.preventDefault();
      const newIdx = (idx + next[e.key] + tabs.length) % tabs.length;
      tabs[newIdx].focus();
      tabs[newIdx].click();
    }
  });

  // Smooth-scroll offset correction for sticky nav
  document.querySelectorAll('a[href^="#"]').forEach(a => {
    a.addEventListener('click', e => {
      const id = a.getAttribute('href').slice(1);
      const target = document.getElementById(id);
      if (target) {
        e.preventDefault();
        target.scrollIntoView({ behavior: 'smooth', block: 'start' });
      }
    });
  });
}
