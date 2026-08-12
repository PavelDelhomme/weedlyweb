#include "core/PageEnhancements.h"

namespace PageEnhancements {

const char* darkModeEnableScript() {
    return R"JS(
(function () {
  try {
    var old = document.getElementById('weedlyweb-dark-mode');
    if (old) old.remove();
    var s = document.createElement('style');
    s.id = 'weedlyweb-dark-mode';
    s.textContent = [
      'html.weedlyweb-dark {',
      '  background:#0e0e0e !important;',
      '  color-scheme: dark !important;',
      '  filter: invert(1) hue-rotate(180deg) contrast(0.95) !important;',
      '}',
      'html.weedlyweb-dark img, html.weedlyweb-dark video, html.weedlyweb-dark picture,',
      'html.weedlyweb-dark canvas, html.weedlyweb-dark svg, html.weedlyweb-dark embed,',
      'html.weedlyweb-dark object, html.weedlyweb-dark iframe {',
      '  filter: invert(1) hue-rotate(180deg) !important;',
      '}',
      'html.weedlyweb-dark [style*="background-image"],',
      'html.weedlyweb-dark .weedlyweb-no-invert {',
      '  filter: invert(1) hue-rotate(180deg) !important;',
      '}',
      '#weedlyweb-reader-root {',
      '  filter: none !important;',
      '}'
    ].join('\n');
    (document.head || document.documentElement).appendChild(s);
    document.documentElement.classList.add('weedlyweb-dark');
    try { document.documentElement.style.colorScheme = 'dark'; } catch (e) {}
  } catch (e) { console.warn('WeedlyWeb dark mode', e); }
})();
)JS";
}

const char* darkModeDisableScript() {
    return R"JS(
(function () {
  try {
    document.documentElement.classList.remove('weedlyweb-dark');
    var s = document.getElementById('weedlyweb-dark-mode');
    if (s) s.remove();
    try { document.documentElement.style.colorScheme = ''; } catch (e) {}
  } catch (e) {}
})();
)JS";
}

const char* readerModeEnableScript() {
    return R"JS(
(function () {
  try {
    if (document.getElementById('weedlyweb-reader-root')) return;

    function scoreNode(el) {
      if (!el || !el.innerText) return 0;
      var text = (el.innerText || '').trim();
      if (text.length < 200) return 0;
      var p = el.querySelectorAll('p').length;
      var links = el.querySelectorAll('a').length;
      var linkPenalty = Math.min(links * 15, text.length / 3);
      return text.length + p * 80 - linkPenalty;
    }

    var candidates = [];
    ['article', 'main', '[role="main"]', '.post', '.entry-content', '.article-body',
     '#content', '.content', '.post-content', '.story-body'].forEach(function (sel) {
      document.querySelectorAll(sel).forEach(function (el) { candidates.push(el); });
    });
    if (!candidates.length) {
      document.querySelectorAll('div').forEach(function (el) {
        if (scoreNode(el) > 800) candidates.push(el);
      });
    }

    var best = null, bestScore = 0;
    candidates.forEach(function (el) {
      var sc = scoreNode(el);
      if (sc > bestScore) { bestScore = sc; best = el; }
    });
    if (!best) best = document.body;

    var title = (document.querySelector('h1') && document.querySelector('h1').innerText)
      || document.title || 'Article';
    var clone = best.cloneNode(true);
    clone.querySelectorAll('script, style, noscript, iframe, nav, aside, form, button, svg, .ad, [class*="ad-"], [id*="ad-"]').forEach(function (n) {
      try { n.remove(); } catch (e) {}
    });

    var root = document.createElement('div');
    root.id = 'weedlyweb-reader-root';
    root.innerHTML = '';
    var wrap = document.createElement('div');
    wrap.id = 'weedlyweb-reader-inner';
    var h = document.createElement('h1');
    h.textContent = title.trim();
    wrap.appendChild(h);
    var meta = document.createElement('p');
    meta.className = 'weedlyweb-reader-meta';
    meta.textContent = location.hostname + ' · Mode lecture WeedlyWeb';
    wrap.appendChild(meta);
    var body = document.createElement('div');
    body.className = 'weedlyweb-reader-body';
    body.appendChild(clone);
    wrap.appendChild(body);
    root.appendChild(wrap);

    var style = document.createElement('style');
    style.id = 'weedlyweb-reader-style';
    style.textContent = [
      'html, body { margin:0 !important; padding:0 !important; background:#f4f1ea !important; }',
      '#weedlyweb-reader-root {',
      '  position:fixed; inset:0; z-index:2147483646; overflow:auto;',
      '  background:#f4f1ea; color:#1a1a1a;',
      '  font-family: Georgia, "Times New Roman", serif;',
      '}',
      '#weedlyweb-reader-inner { max-width:42rem; margin:0 auto; padding:2.5rem 1.25rem 4rem; }',
      '#weedlyweb-reader-inner h1 { font-size:1.85rem; line-height:1.25; margin:0 0 0.5rem; font-weight:700; }',
      '.weedlyweb-reader-meta { color:#666; font-size:0.9rem; margin:0 0 1.75rem;',
      '  font-family: system-ui, sans-serif; }',
      '.weedlyweb-reader-body { font-size:1.15rem; line-height:1.7; }',
      '.weedlyweb-reader-body p { margin:0 0 1.1em; }',
      '.weedlyweb-reader-body img, .weedlyweb-reader-body video { max-width:100%; height:auto; border-radius:6px; }',
      '.weedlyweb-reader-body a { color:#0b57d0; }',
      'html.weedlyweb-dark #weedlyweb-reader-root { background:#121212 !important; color:#e8e6e3 !important; filter:none !important; }',
      'html.weedlyweb-dark #weedlyweb-reader-inner h1 { color:#f2f2f2; }',
      'html.weedlyweb-dark .weedlyweb-reader-meta { color:#aaa; }',
      'html.weedlyweb-dark .weedlyweb-reader-body a { color:#8ab4f8; }'
    ].join('\n');

    document.documentElement.appendChild(style);
    document.documentElement.appendChild(root);
    document.documentElement.setAttribute('data-weedlyweb-reader', '1');
    try { window.scrollTo(0, 0); } catch (e) {}
  } catch (e) { console.warn('WeedlyWeb reader mode', e); }
})();
)JS";
}

const char* readerModeIsActiveScript() {
    return "(document.getElementById('weedlyweb-reader-root') ? '1' : '0')";
}

} // namespace PageEnhancements
