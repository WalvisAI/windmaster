# WindMaster — marketing site

Static marketing site for **WindMaster / Field Target Wind Master** (Android app
`com.walvisai.fieldtargetwindmaster`). Plain HTML + CSS + vanilla JS — no build
step, no framework, no external requests. Hosted free on **GitHub Pages**.

- **Bilingual** English / Spanish (in-page toggle; copy pulled from the app's own strings).
- **Light + dark** themes (follows the OS, with a manual toggle).
- **Self-contained**: all CSS/JS/images are local, so the page also works offline
  and leaks nothing to third parties (matching the app's no-tracking stance).

## Structure

```
index.html         Landing page
ballistics.html    "How the ballistics work" deep-dive
privacy.html       Privacy policy (EN authoritative + ES courtesy translation)
assets/css/styles.css   Design system (brand tokens, light/dark, components)
assets/js/main.js       Language toggle, theme toggle, mobile nav
assets/js/charts.js     Lightweight SVG charts (illustrative ballistics data)
assets/img/logo.png     Brand mark (from the app's drawable/logo1.png)
assets/img/favicon.svg  Tab icon
assets/img/mockups/*.svg  App-screen illustrations (placeholders — see below)
.nojekyll          Tells Pages to serve /assets untouched
```

## Publish on GitHub Pages (free)

1. Create a **public** repo (public is required for free Pages), e.g. `windmaster`
   under the `WalvisAI` account.
2. Push these files to the `main` branch.
   ```bash
   git init
   git add .
   git commit -m "WindMaster marketing site"
   git branch -M main
   git remote add origin https://github.com/WalvisAI/windmaster.git
   git push -u origin main
   ```
   (With the GitHub CLI: `gh repo create WalvisAI/windmaster --public --source . --push`.)
3. In the repo: **Settings → Pages → Build and deployment → Deploy from a branch**,
   pick `main` / `/ (root)`, Save.
4. Wait ~1 minute, then open **https://walvisai.github.io/windmaster/**.

## Swap in real screenshots

The phone frames currently show hand-built SVG illustrations in `assets/img/mockups/`.
To use real screenshots, either:

- replace a mockup file with your own (keep the same filename), **or**
- edit the `<img src="…">` inside a `.phone-screen` in `index.html` /
  `ballistics.html` to point at your PNG (put it in `assets/img/`).

Portrait screenshots around **1080×2160** look best in the frame. You can also
delete the small "Illustration · swap for real screenshots" tag in the hero once
real screenshots are in.

## Editing copy

- **Short inline text** (nav links, buttons) uses `data-en` / `data-es` attributes
  on the element — `main.js` swaps the text when the language changes.
- **Paragraphs / headings** use two sibling elements, `<span class="lang-en">…`
  and `<span class="lang-es">…` — CSS shows only the active language.

Keep both languages in sync when you edit.

## Chart data

`assets/js/charts.js` holds **illustrative** trajectory / wind / velocity / drag
curves in realistic ranges for a .177/.22 pellet — clearly labelled as such on the
pages. They are for communication, not a substitute for the app's solver. To use
exact numbers, edit the `DATA` object in that file.

## Moving to a custom domain later (e.g. fieldtargetwind.com)

Nothing here is tied to the `github.io` URL — all links are relative.

1. Buy the domain (~$10/yr).
2. Add a file named `CNAME` at the repo root containing just `fieldtargetwind.com`.
3. At your registrar, add DNS records:
   - four `A` records for the apex → `185.199.108.153`, `185.199.109.153`,
     `185.199.110.153`, `185.199.111.153`
   - (optional) a `CNAME` for `www` → `walvisai.github.io`
4. In **Settings → Pages**, set the custom domain and tick **Enforce HTTPS**
   (wait a few minutes for the certificate).
5. Update the privacy-policy URL in the **Google Play Console** to the new domain.

That's the whole migration — about ten minutes plus DNS propagation.

---
© WalvisAI · contact **windmasterft@gmail.com**
