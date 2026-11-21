'use strict';

let currentLang  = 'ar';
let currentTheme = 'auto';

document.addEventListener('DOMContentLoaded', () => {
    loadPrefs();
    applyLang(currentLang, false);
    applyTheme(currentTheme, false);
    bindEvents();
    detectOS();
    fetchVersion();
});

// ── Prefs ─────────────────────────────────────────────────────────────────────
function loadPrefs() {
    try {
        const l = localStorage.getItem('az-lang');
        const t = localStorage.getItem('az-theme');
        if (l === 'ar' || l === 'en') currentLang = l;
        if (t === 'light' || t === 'dark' || t === 'auto') currentTheme = t;
    } catch(_) {}
}
function savePrefs() {
    try { localStorage.setItem('az-lang', currentLang); localStorage.setItem('az-theme', currentTheme); } catch(_) {}
}

// ── Language ──────────────────────────────────────────────────────────────────
function applyLang(lang, save = true) {
    currentLang = lang;
    document.documentElement.setAttribute('lang', lang);
    document.documentElement.setAttribute('dir', lang === 'ar' ? 'rtl' : 'ltr');
    document.querySelectorAll('[data-ar]').forEach(el => {
        const v = lang === 'ar' ? el.dataset.ar : el.dataset.en;
        if (v !== undefined) el.textContent = v;
    });
    const btn = document.getElementById('langToggle');
    if (btn) btn.querySelector('span').textContent = lang === 'ar' ? 'English' : 'عربي';
    if (save) savePrefs();
}

// ── Theme ─────────────────────────────────────────────────────────────────────
function applyTheme(theme, save = true) {
    currentTheme = theme;
    const root = document.documentElement;
    if (theme === 'light') root.setAttribute('data-theme','light');
    else if (theme === 'dark') root.setAttribute('data-theme','dark');
    else root.removeAttribute('data-theme');
    if (save) savePrefs();
}
function cycleTheme() {
    if (currentTheme === 'auto') applyTheme('dark');
    else if (currentTheme === 'dark') applyTheme('light');
    else applyTheme('auto');
}

// ── Platform tab ──────────────────────────────────────────────────────────────
function activateTab(id) {
    document.querySelectorAll('.tab-btn').forEach(b => {
        const a = b.dataset.tab === id;
        b.classList.toggle('active', a);
        b.setAttribute('aria-selected', String(a));
    });
    document.querySelectorAll('.tab-panel').forEach(p => {
        p.classList.toggle('active', p.id === `panel-${id}`);
    });
}

function detectOS() {
    const ua = navigator.userAgent.toLowerCase();
    let os, pill, cmd;
    if (ua.includes('win')) {
        os = 'windows'; pill = 'Windows';
        cmd = 'irm https://raw.githubusercontent.com/arcalc/azkare/main/install.ps1 | iex';
    } else if (ua.includes('mac')) {
        os = 'macos'; pill = 'macOS';
        cmd = 'curl -fsSL https://raw.githubusercontent.com/arcalc/azkare/main/install.sh | bash';
    } else {
        os = 'linux'; pill = 'Linux';
        cmd = 'curl -fsSL https://raw.githubusercontent.com/arcalc/azkare/main/install.sh | bash';
    }
    activateTab(os);

    // Update hero quick-install box
    const heroCmd  = document.getElementById('heroCmd');
    const heroPill = document.getElementById('heroOsPill');
    if (heroCmd)  heroCmd.innerHTML  = `<span class="cmd-prompt">${os === 'windows' ? 'PS&gt; ' : '$ '}</span>${escHtml(cmd)}`;
    if (heroPill) heroPill.textContent = pill;
}

function escHtml(s) {
    return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
}

// ── GitHub release version ────────────────────────────────────────────────────
async function fetchVersion() {
    const el = document.getElementById('versionText');
    try {
        const res = await fetch('https://api.github.com/repos/arcalc/azkare/releases/latest',
            { headers: { Accept: 'application/vnd.github+json' } });
        if (!res.ok) throw new Error(res.status);
        const data = await res.json();
        const tag  = data.tag_name || 'v1.0.0';
        if (el) el.textContent = currentLang === 'ar'
            ? `أحدث إصدار: ${tag}`
            : `Latest: ${tag}`;
    } catch(_) {
        // Fallback: show from git tags
        try {
            const res2 = await fetch('https://api.github.com/repos/arcalc/azkare/tags',
                { headers: { Accept: 'application/vnd.github+json' } });
            if (res2.ok) {
                const tags = await res2.json();
                const tag  = (tags[0] && tags[0].name) || 'v1.0.0';
                if (el) el.textContent = currentLang === 'ar'
                    ? `أحدث إصدار: ${tag}`
                    : `Latest: ${tag}`;
            }
        } catch(_) {
            if (el) el.textContent = 'github.com/arcalc/azkare';
        }
    }
}

// ── Copy buttons ──────────────────────────────────────────────────────────────
function copyText(id) {
    const el = document.getElementById(id);
    if (!el) return;
    // Extract text content, strip the prompt span
    const full = el.innerText || el.textContent;
    // Remove leading prompt characters ($ , PS> , % )
    const text = full.replace(/^[\$%]\s|^PS>\s/m, '').trim();

    if (navigator.clipboard) {
        navigator.clipboard.writeText(text).then(() => flashCopied(id)).catch(() => fallbackCopy(text, id));
    } else {
        fallbackCopy(text, id);
    }
}

function fallbackCopy(text, sourceId) {
    const ta = document.createElement('textarea');
    ta.value = text; ta.style.position='fixed'; ta.style.opacity='0';
    document.body.appendChild(ta); ta.select();
    try { document.execCommand('copy'); flashCopied(sourceId); } catch(_) {}
    document.body.removeChild(ta);
}

function flashCopied(sourceId) {
    // Find the copy button that triggered this
    const btn = document.querySelector(`[data-copy="${sourceId}"]`);
    if (!btn) return;
    btn.classList.add('copied');
    const span = btn.querySelector('span');
    const orig = span ? span.textContent : '';
    if (span) span.textContent = currentLang === 'ar' ? 'تم النسخ ✓' : 'Copied! ✓';
    setTimeout(() => {
        btn.classList.remove('copied');
        if (span) span.textContent = orig;
    }, 2000);
}

// ── Events ────────────────────────────────────────────────────────────────────
function bindEvents() {
    document.getElementById('langToggle')?.addEventListener('click', () => {
        applyLang(currentLang === 'ar' ? 'en' : 'ar');
    });
    document.getElementById('themeToggle')?.addEventListener('click', cycleTheme);

    document.querySelectorAll('.tab-btn').forEach(b =>
        b.addEventListener('click', () => activateTab(b.dataset.tab)));

    document.querySelectorAll('.copy-btn[data-copy]').forEach(btn =>
        btn.addEventListener('click', () => copyText(btn.dataset.copy)));

    document.querySelectorAll('a[href^="#"]').forEach(a => {
        a.addEventListener('click', e => {
            const id = a.getAttribute('href').slice(1);
            const target = document.getElementById(id);
            if (target) { e.preventDefault(); target.scrollIntoView({ behavior: 'smooth', block: 'start' }); }
        });
    });
}
