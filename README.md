# WindMaster — marketing site

Static marketing site for **WindMaster / Field Target Wind Master** (Android app
`com.walvisai.fieldtargetwindmaster`). Plain HTML + CSS + vanilla JS — no build
step, no framework. Hosted free on **GitHub Pages**.

- **Bilingual** English / Spanish (in-page toggle; copy pulled from the app's own strings).
- **Light + dark** themes (follows the OS, with a manual toggle).
- **Self-contained**: all CSS/JS/images are local, so the page loads with no
  third-party requests at all (matching the app's no-tracking stance).
- **One deliberate exception**: pressing send on the contact form POSTs to
  `api.web3forms.com`, which forwards the message to `windmasterft@gmail.com`.
  It fires only on that click, never on page load, so a visitor who does not use
  the form still reaches no third party. The access key in `main.js` is public by
  design — Web3Forms keys are meant to sit in client-side code. Blank the
  `WEB3FORMS_KEY` constant and the form falls back to a `mailto:` link, which is
  how it behaved before. Disclosed in `privacy.html` § 10; keep those in sync.

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
assets/img/screens/*.jpg  Real app screenshots (status bar cropped — see below)
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

## Updating the screenshots

The phone frames show real app screenshots in `assets/img/screens/` (portrait,
720×1500, with the Android status bar cropped off the top).

To replace or add one:

- overwrite a file in `assets/img/screens/` (keep the same filename), **or**
- edit the `<img src="…">` inside a `.phone-screen` in `index.html` /
  `ballistics.html` to point at your new image.

New screenshots should have the **top status bar (clock / Wi-Fi / battery)
cropped off** so they match the rest. A ready-made cropper lives in the repo
history — on Windows it trims the top ~90 px and downscales portrait shots to
720 px wide (landscape shots also get the right-edge nav bar trimmed). Keep
portrait shots at the same aspect ratio (~720×1500) so they fill the `.phone.shot`
frame without cropping.

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
