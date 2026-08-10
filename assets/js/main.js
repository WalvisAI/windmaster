/* WindMaster site — language, theme, and nav. No dependencies. */
(function () {
  "use strict";
  var root = document.documentElement;
  var LS_LANG = "wm-lang", LS_THEME = "wm-theme";

  /* ---------- Language ---------- */
  function initialLang() {
    var saved = null;
    try { saved = localStorage.getItem(LS_LANG); } catch (e) {}
    if (saved === "en" || saved === "es") return saved;
    var nav = (navigator.language || navigator.userLanguage || "en").toLowerCase();
    return nav.indexOf("es") === 0 ? "es" : "en";
  }
  function applyLang(lang) {
    root.setAttribute("data-lang", lang);
    root.setAttribute("lang", lang);
    // swap inline text nodes marked with data-en / data-es
    var nodes = document.querySelectorAll("[data-en][data-es]");
    for (var i = 0; i < nodes.length; i++) {
      var el = nodes[i];
      var val = el.getAttribute("data-" + lang);
      if (val !== null) el.textContent = val;
    }
    // swap attributes:  data-en-<attr> / data-es-<attr>  (e.g. data-en-aria-label)
    // href is in the list so a link can point each language at its own page — a shop with a
    // separate English storefront, say. The markup still carries a real href for no-JS readers.
    var attrNodes = document.querySelectorAll("[data-" + lang + "-aria-label], [data-" + lang + "-title], [data-" + lang + "-alt], [data-" + lang + "-href]");
    for (var j = 0; j < attrNodes.length; j++) {
      var a = attrNodes[j];
      ["aria-label", "title", "alt", "href"].forEach(function (attr) {
        var v = a.getAttribute("data-" + lang + "-" + attr);
        if (v !== null) a.setAttribute(attr, v);
      });
    }
    // toggle button state
    document.querySelectorAll("[data-set-lang]").forEach(function (b) {
      b.setAttribute("aria-pressed", b.getAttribute("data-set-lang") === lang ? "true" : "false");
    });
  }
  function setLang(lang) {
    try { localStorage.setItem(LS_LANG, lang); } catch (e) {}
    applyLang(lang);
  }

  /* ---------- Theme ---------- */
  function savedTheme() {
    try { return localStorage.getItem(LS_THEME); } catch (e) { return null; }
  }
  function effectiveTheme() {
    var t = root.getAttribute("data-theme");
    if (t === "light" || t === "dark") return t;
    return window.matchMedia && window.matchMedia("(prefers-color-scheme: light)").matches ? "light" : "dark";
  }
  function applyTheme(theme) {
    if (theme === "light" || theme === "dark") root.setAttribute("data-theme", theme);
    else root.removeAttribute("data-theme");
    document.querySelectorAll("[data-toggle-theme]").forEach(function (b) {
      b.setAttribute("aria-label", effectiveTheme() === "dark" ? "Switch to light theme" : "Switch to dark theme");
    });
  }
  function toggleTheme() {
    var next = effectiveTheme() === "dark" ? "light" : "dark";
    try { localStorage.setItem(LS_THEME, next); } catch (e) {}
    applyTheme(next);
  }

  /* ---------- Wire up ---------- */
  function ready(fn) {
    if (document.readyState !== "loading") fn();
    else document.addEventListener("DOMContentLoaded", fn);
  }
  ready(function () {
    applyLang(initialLang());
    var st = savedTheme();
    if (st === "light" || st === "dark") applyTheme(st);
    else applyTheme(null); // follow OS

    document.querySelectorAll("[data-set-lang]").forEach(function (b) {
      b.addEventListener("click", function () { setLang(b.getAttribute("data-set-lang")); });
    });
    document.querySelectorAll("[data-toggle-theme]").forEach(function (b) {
      b.addEventListener("click", toggleTheme);
    });

    // mobile nav
    var burger = document.querySelector("[data-nav-toggle]");
    var links = document.getElementById("nav-links");
    if (burger && links) {
      burger.addEventListener("click", function () {
        var open = links.classList.toggle("open");
        burger.setAttribute("aria-expanded", open ? "true" : "false");
      });
      links.querySelectorAll("a").forEach(function (a) {
        a.addEventListener("click", function () {
          links.classList.remove("open");
          burger.setAttribute("aria-expanded", "false");
        });
      });
    }

    // current year
    var y = document.querySelector("[data-year]");
    if (y) y.textContent = new Date().getFullYear();
  });

  // apply theme ASAP (already applied inline in <head> to avoid flash; keep OS listener)
  if (window.matchMedia) {
    window.matchMedia("(prefers-color-scheme: light)").addEventListener("change", function () {
      if (!savedTheme()) applyTheme(null);
    });
  }
})();
