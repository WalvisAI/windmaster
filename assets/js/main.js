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
    // swap attributes:  data-en-<attr> / data-es-<attr>  (e.g. data-en-aria-label, data-en-placeholder)
    // href is in the list so a link can point each language at its own page — a shop with a
    // separate English storefront, say. The markup still carries a real href for no-JS readers.
    var attrNodes = document.querySelectorAll("[data-" + lang + "-aria-label], [data-" + lang + "-title], [data-" + lang + "-alt], [data-" + lang + "-href], [data-" + lang + "-placeholder]");
    for (var j = 0; j < attrNodes.length; j++) {
      var a = attrNodes[j];
      ["aria-label", "title", "alt", "href", "placeholder"].forEach(function (attr) {
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

    /* ---------- Contact form ----------
       Web3Forms takes the POST and forwards it to windmasterft@gmail.com. Until an access
       key is set the form falls back to a mailto:, so the page is never broken by a blank
       key -- it just degrades to the visitor's own mail app. Swap the constant and the
       form starts sending for real; nothing else needs to change. */
    var WEB3FORMS_KEY = "7c26d907-36c4-4871-a421-26e8abbd8e5d";  // public by design: Web3Forms keys are client-side

    var cform = document.getElementById("contact-form");
    if (cform) {
      var elMsg    = document.getElementById("cf-message");
      var elName   = document.getElementById("cf-name");
      var elEmail  = document.getElementById("cf-email");
      var elTopic  = document.getElementById("cf-topic");
      var elResult = document.getElementById("cf-result");
      var posting  = !!WEB3FORMS_KEY;

      // show whichever explanation and button label match the mode we are actually in
      var on  = posting ? ".note-post, .btn-label-post"     : ".note-mailto, .btn-label-mailto";
      var off = posting ? ".note-mailto, .btn-label-mailto" : ".note-post, .btn-label-post";
      cform.querySelectorAll(on ).forEach(function (n) { n.hidden = false; });
      cform.querySelectorAll(off).forEach(function (n) { n.hidden = true; });
      // an address is only required when we post; a mailto: carries its own sender
      if (posting) elEmail.setAttribute("required", "");
      else elEmail.closest(".field").hidden = true;

      var T = {
        needMessage: { en: "Please write a message first.",       es: "Escribe un mensaje primero." },
        needEmail:   { en: "Please add an email so I can reply.", es: "Añade un correo para que pueda responderte." },
        badEmail:    { en: "That email does not look right.",     es: "Ese correo no parece correcto." },
        sending:     { en: "Sending…",                        es: "Enviando…" },
        sent:        { en: "Thank you. Your message is on its way and I will reply to you by email.",
                       es: "Gracias. Tu mensaje va de camino y te responderé por correo." },
        failed:      { en: "That did not go through. Please write to windmasterft@gmail.com instead.",
                       es: "No se ha podido enviar. Escríbeme a windmasterft@gmail.com." }
      };
      function lang() { return root.getAttribute("data-lang") === "es" ? "es" : "en"; }
      function say(key, kind) {
        elResult.textContent = T[key][lang()];
        elResult.className = "form-result " + (kind || "");
        elResult.hidden = false;
      }
      function fail(field, key) {
        field.parentNode.classList.add("invalid");
        field.focus();
        say(key, "err");
      }

      [elMsg, elEmail].forEach(function (f) {
        f.addEventListener("input", function () {
          this.parentNode.classList.remove("invalid");
          if (elResult.classList.contains("err")) elResult.hidden = true;
        });
      });

      cform.addEventListener("submit", function (ev) {
        ev.preventDefault();
        cform.querySelectorAll(".field").forEach(function (f) { f.classList.remove("invalid"); });

        var body = (elMsg.value || "").trim();
        if (!body) { fail(elMsg, "needMessage"); return; }

        var topic = elTopic.value;
        var name  = (elName.value || "").trim();

        if (!posting) {
          if (name) body += "\n\n-- " + name;
          elResult.hidden = true;
          window.location.href = "mailto:windmasterft@gmail.com"
            + "?subject=" + encodeURIComponent("[WindMaster] " + topic)
            + "&body="    + encodeURIComponent(body);
          return;
        }

        var email = (elEmail.value || "").trim();
        if (!email) { fail(elEmail, "needEmail"); return; }
        if (!/^[^\s@]+@[^\s@]+\.[^\s@]{2,}$/.test(email)) { fail(elEmail, "badEmail"); return; }

        say("sending", "");
        cform.classList.add("sending");

        fetch("https://api.web3forms.com/submit", {
          method: "POST",
          headers: { "Content-Type": "application/json", "Accept": "application/json" },
          body: JSON.stringify({
            access_key: WEB3FORMS_KEY,
            subject:    "[WindMaster] " + topic,
            from_name:  "WindMaster website",
            name:       name || "(no name given)",
            email:      email,
            replyto:    email,
            topic:      topic,
            site_language: lang(),
            message:    body,
            botcheck:   cform.querySelector("[name=botcheck]").checked
          })
        })
        .then(function (r) { return r.json().catch(function () { return { success: r.ok }; }); })
        .then(function (data) {
          cform.classList.remove("sending");
          if (data && data.success) {
            say("sent", "ok");
            cform.reset();
          } else {
            say("failed", "err");
          }
        })
        .catch(function () {
          cform.classList.remove("sending");
          say("failed", "err");
        });
      });
    }

    // copy-to-clipboard buttons (falls back to selecting nothing rather than throwing)
    document.querySelectorAll("[data-copy]").forEach(function (b) {
      b.addEventListener("click", function () {
        var text = b.getAttribute("data-copy");
        var done = function () {
          var cur = function () { return root.getAttribute("data-lang") === "es" ? "es" : "en"; };
          b.textContent = cur() === "es" ? "¡Copiado!" : "Copied";
          setTimeout(function () {
            // read the label back off the data attributes so a language switch mid-timeout wins
            b.textContent = b.getAttribute("data-" + cur()) || b.textContent;
          }, 1800);
        };
        if (navigator.clipboard && navigator.clipboard.writeText) {
          navigator.clipboard.writeText(text).then(done, function () {});
        } else {
          var ta = document.createElement("textarea");
          ta.value = text; ta.setAttribute("readonly", ""); ta.style.position = "absolute"; ta.style.left = "-9999px";
          document.body.appendChild(ta); ta.select();
          try { document.execCommand("copy"); done(); } catch (e) {}
          document.body.removeChild(ta);
        }
      });
    });

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
