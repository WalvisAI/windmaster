/* WindMaster — lightweight SVG charts (no dependencies).
   Colours come from CSS classes (--series-*, --chart-*) so they follow the theme.
   Data are illustrative curves in realistic ranges; the app computes exact values. */
(function () {
  "use strict";
  var SVGNS = "http://www.w3.org/2000/svg";
  function el(tag, attrs) {
    var n = document.createElementNS(SVGNS, tag);
    if (attrs) for (var k in attrs) n.setAttribute(k, attrs[k]);
    return n;
  }
  function fmt(v, d) { var s = (Math.round(v * Math.pow(10, d)) / Math.pow(10, d)).toFixed(d); return s; }

  function render(mount, cfg) {
    var VBW = 680, VBH = 340;
    var pad = cfg.pad || { l: 50, r: 16, t: 14, b: 42 };
    var pw = VBW - pad.l - pad.r, ph = VBH - pad.t - pad.b;
    var xmin = cfg.x.min, xmax = cfg.x.max, ymin = cfg.y.min, ymax = cfg.y.max;
    var sx = function (x) { return pad.l + (x - xmin) / (xmax - xmin) * pw; };
    var sy = function (y) { return pad.t + (1 - (y - ymin) / (ymax - ymin)) * ph; };

    var svg = el("svg", { viewBox: "0 0 " + VBW + " " + VBH, role: "img", "aria-label": cfg.aria || cfg.title || "chart" });

    // optional shaded x-band
    if (cfg.band) {
      svg.appendChild(el("rect", {
        x: sx(cfg.band.x0), y: pad.t, width: sx(cfg.band.x1) - sx(cfg.band.x0), height: ph,
        fill: "var(--series-1)", opacity: 0.07
      }));
      if (cfg.band.label) {
        var bl = el("text", { x: (sx(cfg.band.x0) + sx(cfg.band.x1)) / 2, y: pad.t + 14, "text-anchor": "middle", class: "tick-label" });
        bl.textContent = cfg.band.label; svg.appendChild(bl);
      }
    }

    // horizontal gridlines + y ticks
    cfg.y.ticks.forEach(function (t) {
      svg.appendChild(el("line", { x1: pad.l, y1: sy(t), x2: pad.l + pw, y2: sy(t), class: "wm-grid" }));
      var lab = el("text", { x: pad.l - 9, y: sy(t) + 4, "text-anchor": "end", class: "tick-label tabular" });
      lab.textContent = cfg.y.fmt ? cfg.y.fmt(t) : t; svg.appendChild(lab);
    });
    // x ticks
    cfg.x.ticks.forEach(function (t) {
      var lab = el("text", { x: sx(t), y: pad.t + ph + 22, "text-anchor": "middle", class: "tick-label tabular" });
      lab.textContent = cfg.x.fmt ? cfg.x.fmt(t) : t; svg.appendChild(lab);
    });
    // axes
    svg.appendChild(el("line", { x1: pad.l, y1: pad.t, x2: pad.l, y2: pad.t + ph, class: "wm-axis" }));
    svg.appendChild(el("line", { x1: pad.l, y1: pad.t + ph, x2: pad.l + pw, y2: pad.t + ph, class: "wm-axis" }));
    // axis labels
    var xl = el("text", { x: pad.l + pw / 2, y: VBH - 6, "text-anchor": "middle", class: "axis-label" });
    xl.textContent = cfg.x.label; svg.appendChild(xl);
    var yl = el("text", { x: 14, y: pad.t + ph / 2, "text-anchor": "middle", class: "axis-label",
      transform: "rotate(-90 14 " + (pad.t + ph / 2) + ")" });
    yl.textContent = cfg.y.label; svg.appendChild(yl);

    // reference line (dashed gold) at a data-y
    if (cfg.ref) {
      svg.appendChild(el("line", { x1: pad.l, y1: sy(cfg.ref.y), x2: pad.l + pw, y2: sy(cfg.ref.y), class: "wm-ref" }));
      if (cfg.ref.label) {
        var rl = el("text", { x: pad.l + pw - 4, y: sy(cfg.ref.y) - 6, "text-anchor": "end", class: "wm-zerotxt" });
        rl.textContent = cfg.ref.label; svg.appendChild(rl);
      }
    }

    // area fill (single series option)
    if (cfg.area && cfg.series[0]) {
      var p0 = cfg.series[0].points, dArea = "M" + sx(p0[0][0]) + "," + sy(cfg.y.baseline != null ? cfg.y.baseline : ymin);
      p0.forEach(function (pt) { dArea += " L" + sx(pt[0]) + "," + sy(pt[1]); });
      dArea += " L" + sx(p0[p0.length - 1][0]) + "," + sy(cfg.y.baseline != null ? cfg.y.baseline : ymin) + " Z";
      svg.appendChild(el("path", { d: dArea, class: "wm-area1" }));
    }

    // series lines + point dots
    var dotClass = ["wm-dot1", "wm-dot2"];
    cfg.series.forEach(function (s, si) {
      var d = "";
      s.points.forEach(function (pt, i) { d += (i ? " L" : "M") + sx(pt[0]) + "," + sy(pt[1]); });
      svg.appendChild(el("path", { d: d, class: "wm-line " + s.cls }));
      s.points.forEach(function (pt) {
        svg.appendChild(el("circle", { cx: sx(pt[0]), cy: sy(pt[1]), r: 3, class: dotClass[si] || "wm-dot1" }));
      });
    });

    // hover layer: crosshair + enlarged dots
    var cross = el("line", { x1: 0, y1: pad.t, x2: 0, y2: pad.t + ph, class: "wm-cross" });
    svg.appendChild(cross);
    var hoverDots = cfg.series.map(function (s, si) {
      var c = el("circle", { r: 5, class: (dotClass[si] || "wm-dot1") + " wm-dot" });
      svg.appendChild(c); return c;
    });
    var overlay = el("rect", { x: pad.l, y: pad.t, width: pw, height: ph, fill: "transparent", style: "cursor:crosshair" });
    svg.appendChild(overlay);

    mount.appendChild(svg);

    // tooltip element
    var tip = document.createElement("div");
    tip.className = "chart-tip";
    mount.appendChild(tip);

    var xs = cfg.series[0].points.map(function (p) { return p[0]; });
    function show(clientX) {
      var rect = overlay.getBoundingClientRect();
      var frac = (clientX - rect.left) / rect.width;
      var xval = xmin + frac * (xmax - xmin);
      // nearest index on reference series
      var bi = 0, bd = Infinity;
      for (var i = 0; i < xs.length; i++) { var dd = Math.abs(xs[i] - xval); if (dd < bd) { bd = dd; bi = i; } }
      var ux = sx(xs[bi]);
      cross.setAttribute("x1", ux); cross.setAttribute("x2", ux); cross.style.opacity = "1";
      var scaleX = svg.clientWidth / VBW, scaleY = svg.clientHeight / VBH;
      var topY = Infinity, html = "<b>" + fmt(xs[bi], cfg.x.tipD || 0) + (cfg.x.unit || "") + "</b>";
      cfg.series.forEach(function (s, si) {
        var pt = s.points[bi]; if (!pt) { hoverDots[si].style.opacity = "0"; return; }
        hoverDots[si].setAttribute("cx", sx(pt[0])); hoverDots[si].setAttribute("cy", sy(pt[1]));
        hoverDots[si].style.opacity = "1";
        topY = Math.min(topY, sy(pt[1]));
        var nm = s.name ? "<span style='color:var(" + (si === 0 ? "--series-1" : "--series-2") + ")'>●</span> " + s.name + " " : "";
        html += "<br>" + nm + fmt(pt[1], cfg.y.tipD != null ? cfg.y.tipD : 1) + (cfg.y.unit || "");
      });
      tip.innerHTML = html;
      tip.style.left = (ux * scaleX) + "px";
      tip.style.top = (topY * scaleY) + "px";
      tip.style.opacity = "1";
    }
    function hide() {
      cross.style.opacity = "0"; tip.style.opacity = "0";
      hoverDots.forEach(function (d) { d.style.opacity = "0"; });
    }
    overlay.addEventListener("pointermove", function (e) { show(e.clientX); });
    overlay.addEventListener("pointerdown", function (e) { show(e.clientX); });
    overlay.addEventListener("pointerleave", hide);

    // accessible data table
    if (cfg.table !== false) {
      var wrap = document.createElement("div");
      var btn = document.createElement("button");
      btn.className = "data-table-toggle";
      btn.setAttribute("data-en", "View data table"); btn.setAttribute("data-es", "Ver tabla de datos");
      btn.textContent = "View data table";
      var tbl = document.createElement("table");
      tbl.className = "data-table"; tbl.hidden = true;
      var head = "<tr><th>" + cfg.x.label + "</th>";
      cfg.series.forEach(function (s) { head += "<th>" + (s.name || cfg.y.label) + "</th>"; });
      head += "</tr>";
      var body = "";
      xs.forEach(function (xv, i) {
        body += "<tr><td>" + fmt(xv, cfg.x.tipD || 0) + "</td>";
        cfg.series.forEach(function (s) { body += "<td>" + (s.points[i] ? fmt(s.points[i][1], cfg.y.tipD != null ? cfg.y.tipD : 1) : "–") + "</td>"; });
        body += "</tr>";
      });
      tbl.innerHTML = "<thead>" + head + "</thead><tbody>" + body + "</tbody>";
      btn.addEventListener("click", function () {
        tbl.hidden = !tbl.hidden;
        var l = document.documentElement.getAttribute("data-lang") || "en";
        btn.textContent = tbl.hidden ? (l === "es" ? "Ver tabla de datos" : "View data table") : (l === "es" ? "Ocultar tabla" : "Hide table");
      });
      wrap.appendChild(btn); wrap.appendChild(tbl);
      mount.appendChild(wrap);
    }
  }

  /* ------- data (illustrative; .177 8.44gr @ ~238 m/s, 25 m zero, 76 mm scope) ------- */
  var DATA = {
    trajectory: {
      title: "Pellet path vs line of sight",
      x: { min: 5, max: 55, ticks: [10, 20, 25, 30, 40, 50], label: "Range (m)", unit: " m" },
      y: { min: -36, max: 5, ticks: [0, -10, -20, -30], label: "Height vs sight line (cm)", unit: " cm", tipD: 1 },
      ref: { y: 0, label: "line of sight — zero 25 m" },
      series: [{ cls: "wm-s1", points: [[9,-0.3],[15,1.0],[20,0.9],[25,0.0],[30,-2.3],[35,-5.6],[40,-10.2],[45,-16.5],[50,-24.5],[55,-34.5]] }]
    },
    wind: {
      title: "Wind drift",
      x: { min: 5, max: 50, ticks: [10, 20, 30, 40, 50], label: "Range (m)", unit: " m" },
      y: { min: 0, max: 16, ticks: [0, 4, 8, 12, 16], label: "Drift (cm)", unit: " cm", tipD: 1 },
      series: [
        { cls: "wm-s1", name: ".177 ·8.4gr", points: [[9,0.3],[15,1.0],[20,1.9],[25,3.1],[30,4.6],[35,6.6],[40,9.0],[45,11.9],[50,15.3]] },
        { cls: "wm-s2", name: ".22 ·18gr", points: [[9,0.2],[15,0.8],[20,1.5],[25,2.4],[30,3.6],[35,5.1],[40,6.9],[45,9.1],[50,11.6]] }
      ]
    },
    velocity: {
      title: "Velocity retention",
      area: true,
      x: { min: 0, max: 50, ticks: [0, 10, 20, 30, 40, 50], label: "Range (m)", unit: " m" },
      y: { min: 150, max: 245, baseline: 150, ticks: [160, 180, 200, 220, 240], label: "Velocity (m/s)", unit: " m/s", tipD: 0 },
      series: [{ cls: "wm-s1", points: [[0,238],[10,225],[15,218],[20,212],[25,206],[30,200],[35,194],[40,189],[45,183],[50,178]] }]
    },
    drag: {
      title: "Drag curve: GA vs G1",
      band: { x0: 0.45, x1: 0.78, label: "airgun pellets" },
      x: { min: 0.1, max: 1.2, ticks: [0.2, 0.4, 0.6, 0.8, 1.0, 1.2], label: "Mach (speed / speed of sound)", unit: " M", tipD: 2, fmt: function (v) { return v.toFixed(1); } },
      y: { min: 0.15, max: 0.7, ticks: [0.2, 0.3, 0.4, 0.5, 0.6, 0.7], label: "Drag coefficient (Cd)", unit: "", tipD: 3, fmt: function (v) { return v.toFixed(1); } },
      series: [
        { cls: "wm-s1", name: "GA (pellet)", points: [[0.1,0.240],[0.2,0.225],[0.3,0.210],[0.4,0.197],[0.5,0.190],[0.6,0.189],[0.7,0.196],[0.8,0.216],[0.9,0.290],[1.0,0.420],[1.1,0.600],[1.2,0.660]] },
        { cls: "wm-s2", name: "G1 (bullet)", points: [[0.1,0.260],[0.2,0.246],[0.3,0.235],[0.4,0.229],[0.5,0.226],[0.6,0.227],[0.7,0.238],[0.8,0.280],[0.9,0.380],[1.0,0.520],[1.1,0.618],[1.2,0.645]] }
      ]
    }
  };

  function boot() {
    document.querySelectorAll("[data-chart]").forEach(function (m) {
      var key = m.getAttribute("data-chart");
      if (DATA[key]) render(m, DATA[key]);
    });
  }
  if (document.readyState !== "loading") boot();
  else document.addEventListener("DOMContentLoaded", boot);
})();
