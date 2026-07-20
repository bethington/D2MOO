/* Pairing view, anchored on PD2 DC6 ART FILES.
   LEFT  = the original game artwork (rendered from the DC6).
   RIGHT = the Meshy generations matched to it — several when you re-imagined the same
           sprite more than once (invtgl has four), so you choose which one to use. */
const $ = (s) => document.querySelector(s);
let PAIRS = [], UNPAIRED = [], IGNORED = [], ALL_ITEMS = [];
const PAIRPICK = {};   // invfile -> {left: task_id, right: task_id}

function toast(msg, bad) {
  const t = $("#toast");
  t.textContent = msg; t.className = "toast" + (bad ? " bad" : "");
  setTimeout(() => (t.className = "toast hidden"), 3800);
}
const esc = (s) => String(s).replace(/[&<>"]/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]));
const dc6Url = (f) => `/api/dc6/${encodeURIComponent(f)}.png`;
const spriteUrl = (id) => `/api/item/${encodeURIComponent(id)}/original.png`;

async function loadAllItems() {
  try { ALL_ITEMS = (await (await fetch("/api/items")).json()).items || []; }
  catch (e) { ALL_ITEMS = []; }
}

/* Rescan re-derives links from the re-imagined art library, then reloads the view. */
async function rescan() {
  $("#scanState").textContent = "scanning…";
  try {
    const r = await (await fetch("/api/meshy/links/scan", {
      method: "POST", headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ pages: 2 }),
    })).json();
    if (r.ok) $("#scanState").textContent =
      `${r.scanned} scanned · ${(r.auto || []).length} matched exactly`;
  } catch (e) { /* fall through to load() which reports */ }
  await load();
}

async function load() {
  $("#rows").innerHTML = '<div class="empty">Loading…</div>';
  let d;
  try { d = await (await fetch("/api/meshy/pairs")).json(); }
  catch (e) { d = { ok: false, error: String(e) }; }
  if (!d.ok) {
    $("#rows").innerHTML = `<div class="empty">Failed: ${esc(d.error || "")}</div>`;
    return;
  }
  PAIRS = d.pairs || []; UNPAIRED = d.unpaired || []; IGNORED = d.ignored || [];
  const gens = PAIRS.reduce((n, p) => n + p.generations.length, 0);
  $("#scanState").textContent = "matched via your re-imagined art library";
  $("#counts").textContent =
    `${PAIRS.length} art files paired · ${gens} generations · ${UNPAIRED.length} unplaced` +
    (IGNORED.length ? ` · ${IGNORED.length} marked none` : "");
  render();
}

function genCard(p, g) {
  const id = `g_${p.invfile}_${g.task_id}`;
  const label = g.name || g.prompt || g.task_id.slice(0, 8);
  return `<div class="cand${g.primary ? " chosen" : ""}">
    <input type="radio" name="p_${p.invfile}" id="${id}" ${g.primary ? "checked" : ""}
           data-invfile="${esc(p.invfile)}" data-task="${esc(g.task_id)}">
    <label for="${id}">
      <div class="ph checker">${g.input_image
        ? `<img loading="lazy" src="${esc(g.input_image)}" onerror="this.style.opacity=.15">` : ""}</div>
      <div class="nm" title="${esc(label)}">${esc(label)}</div>
      <div class="why">${g.preview ? "3D ✓" : "no model"} · ${g.retries_left ?? "?"} left</div>
    </label>
    ${g.preview ? `<img class="mini" src="${esc(g.preview)}" title="generated 3D model">` : ""}
    <button class="xbtn" data-unlink="${esc(g.task_id)}" title="not this art file — free this generation">✕</button>
  </div>`;
}

/* A split-art (glove) row: LEFT and RIGHT slots. The hand comes from the matched
   library filename, so it is known, not guessed. A missing hand is mirrored at build
   time -- the game must never get a one-handed sprite. */
function pairSlots(p) {
  const vars = p.variants || [];
  if (!PAIRPICK[p.invfile]) {
    // default to a COMPLETE variant (both hands real) so nothing gets mirrored
    // unnecessarily; the list is already sorted complete-first.
    const v = vars[0];
    PAIRPICK[p.invfile] = v ? { variant: v.variant, left: v.left, right: v.right }
                            : { variant: null, left: null, right: null };
  }
  const cur = PAIRPICK[p.invfile];
  const v = vars.find((x) => x.variant === cur.variant) || vars[0] || {};

  const slot = (hand) => {
    const tid = v[hand];
    const other = hand === "left" ? "right" : "left";
    return `<div class="slot${tid ? "" : " empty"}">
      <div class="slotlbl">${hand.toUpperCase()}${tid ? "" : " (mirrored)"}</div>
      <div class="ph checker">${tid
        ? `<img src="/api/pair/hand/${encodeURIComponent(tid)}.png" onerror="this.style.opacity=.15">`
        : (v[other] ? `<img class="mir" src="/api/pair/hand/${encodeURIComponent(v[other])}.png">` : "")}</div>
      <div class="why">${tid ? esc(p.invfile) + "-" + (hand === "left" ? "l" : "r") + (v.variant || "")
                             : "no " + hand + " art"}</div>
    </div>`;
  };

  const opts = vars.map((x) => `<option value="${esc(x.variant)}" ${x.variant === v.variant ? "selected" : ""}>
      ${esc(x.label)}${x.complete ? " ✓ both hands" : (x.left ? " — left only" : " — right only")}</option>`).join("");

  const warn = v.complete ? ""
    : `<div class="mirrorwarn">this variant has only the ${v.left ? "left" : "right"} hand — the other
       is mirrored. Generate <b>${esc(p.invfile)}-${v.left ? "r" : "l"}${esc(v.variant || "")}</b> in Meshy for a true pair.</div>`;

  return `<div class="pairbox">
    <div class="varrow">
      <span class="slotlbl">pair set</span>
      <select data-variant="${esc(p.invfile)}">${opts}</select>
      <span class="why">${vars.filter((x) => x.complete).length} of ${vars.length} complete</span>
    </div>
    <div class="slots">${slot("left")}${slot("right")}</div>
    ${warn}
    <div class="pairacts">
      <button data-tune="${esc(p.invfile)}">⌗ Tune layout</button>
      <button class="gold" data-build="${esc(p.invfile)}">Build pair sprite</button>
    </div>
  </div>`;
}

function render() {
  const rows = $("#rows");
  if (!PAIRS.length && !UNPAIRED.length) {
    rows.innerHTML = '<div class="empty">Nothing paired yet — hit Rescan.</div>';
    return;
  }
  const paired = PAIRS.map((p) => `
    <div class="prow" data-row="${esc(p.invfile)}">
      <div class="head">
        <span class="tname">${esc(p.invfile)}.dc6</span>
        <span class="tmeta">${p.item_count
          ? esc(p.items.slice(0, 4).join(", ")) + (p.item_count > 4 ? ` +${p.item_count - 4} more` : "")
          : "no catalog item uses this file"}</span>
        <span class="rowact">
          <a href="/studio?item=${encodeURIComponent(p.item_id || "")}"><button>⚒ Studio</button></a>
        </span>
      </div>
      <div class="body">
        <div class="anchor">
          <figure>
            <img class="checker" src="${p.item_id ? spriteUrl(p.item_id) : dc6Url(p.invfile)}"
                 onerror="this.src='${dc6Url(p.invfile)}'">
            <figcaption>original game art</figcaption>
          </figure>
        </div>
        <div class="cands">
          ${p.pairable ? pairSlots(p) : ""}
          ${p.generations.map((g) => genCard(p, g)).join("")}
          <div class="cand none">
            <input type="radio" name="p_${p.invfile}" id="none_${p.invfile}"
                   data-none="${esc(p.invfile)}">
            <label for="none_${p.invfile}">none —<br>don't link<br>this art file</label>
          </div>
        </div>
      </div>
    </div>`).join("");

  const unp = UNPAIRED.length ? `
    <div class="prow unplaced">
      <div class="head">
        <span class="tname">Unplaced generations</span>
        <span class="tmeta">no re-imagined art file matched these — assign them by hand</span>
      </div>
      <div class="body"><div class="cands">
        ${UNPAIRED.map((u) => `
          <div class="cand">
            <label class="static">
              <div class="ph checker">${u.input_image
                ? `<img loading="lazy" src="${esc(u.input_image)}">` : ""}</div>
              <div class="nm">${esc(u.name || u.prompt || u.task_id.slice(0, 8))}</div>
            </label>
            <input type="search" class="assign" data-task="${esc(u.task_id)}"
                   placeholder="assign to item…" spellcheck="false">
            <button class="nonebtn" data-ignore="${esc(u.task_id)}"
                    title="never pair this one">none</button>
            <div class="hits" data-hits="${esc(u.task_id)}"></div>
          </div>`).join("")}
      </div></div>
    </div>` : "";

  const ign = IGNORED.length ? `
    <div class="prow ignored">
      <div class="head">
        <span class="tname">Marked "none"</span>
        <span class="tmeta">kept out of pairing and out of rescans — restore any time</span>
      </div>
      <div class="body"><div class="cands">
        ${IGNORED.map((u) => `
          <div class="cand">
            <label class="static">
              <div class="ph checker">${u.input_image
                ? `<img loading="lazy" src="${esc(u.input_image)}">` : ""}</div>
              <div class="nm">${esc(u.name || u.prompt || u.task_id.slice(0, 8))}</div>
            </label>
            <button class="nonebtn" data-restore="${esc(u.task_id)}">restore</button>
          </div>`).join("")}
      </div></div>
    </div>` : "";

  rows.innerHTML = paired + unp + ign;

  rows.querySelectorAll('input[type=radio]').forEach((r) => {
    r.onchange = async () => {
      const res = await (await fetch("/api/meshy/primary", {
        method: "POST", headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ task_id: r.dataset.task, invfile: r.dataset.invfile }),
      })).json();
      if (!res.ok) return toast("could not set: " + (res.error || ""), true);
      r.closest(".cands").querySelectorAll(".cand").forEach((c) => c.classList.remove("chosen"));
      r.closest(".cand").classList.add("chosen");
      toast(`${r.dataset.invfile}.dc6 will use this generation`);
    };
  });
  rows.querySelectorAll(".assign").forEach((inp) => {
    inp.oninput = () => renderHits(inp.dataset.task, inp.value.trim());
  });
  rows.querySelectorAll("[data-none]").forEach((r) => {
    r.onchange = async () => {
      const f = r.dataset.none;
      const res = await (await fetch("/api/meshy/none", {
        method: "POST", headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ invfile: f }),
      })).json();
      if (!res.ok) return toast("failed: " + (res.error || ""), true);
      toast(`${f}.dc6 unlinked — ${res.freed.length} generation(s) moved to Unplaced`);
      load();
    };
  });
  rows.querySelectorAll("[data-unlink]").forEach((b) => {
    b.onclick = async () => {
      await fetch(`/api/meshy/links/${b.dataset.unlink}`, { method: "DELETE" });
      toast("freed — it's back in Unplaced");
      load();
    };
  });
  rows.querySelectorAll("[data-ignore]").forEach((b) => {
    b.onclick = async () => {
      await fetch("/api/meshy/ignore", {
        method: "POST", headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ task_id: b.dataset.ignore, ignored: true }),
      });
      toast("marked none — it won't come back on rescan");
      load();
    };
  });
  rows.querySelectorAll("[data-variant]").forEach((sel) => {
    sel.onchange = () => {
      const f = sel.dataset.variant;
      const p = PAIRS.find((x) => x.invfile === f);
      const v = (p.variants || []).find((x) => x.variant === sel.value);
      if (!v) return;
      PAIRPICK[f] = { variant: v.variant, left: v.left, right: v.right };
      render();   // redraw the row's slots for the chosen set
    };
  });
  rows.querySelectorAll("[data-build]").forEach((b) => {
    b.onclick = async () => {
      const f = b.dataset.build, pick = PAIRPICK[f] || {};
      b.disabled = true; b.textContent = "rendering both hands…";
      const r = await (await fetch("/api/pair/build", {
        method: "POST", headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ invfile: f, left_task: pick.left || null, right_task: pick.right || null }),
      })).json();
      b.disabled = false; b.textContent = "Build pair sprite";
      if (!r.ok) return toast("build failed: " + (r.error || ""), true);
      const m = r.mirrored_left ? " (left mirrored)" : r.mirrored_right ? " (right mirrored)" : "";
      toast(`${f}.dc6 paired sprite built + activated${m} — Push to game to see it`);
    };
  });
  rows.querySelectorAll("[data-tune]").forEach((b) => { b.onclick = () => openTuner(b.dataset.tune); });
  rows.querySelectorAll("[data-restore]").forEach((b) => {
    b.onclick = async () => {
      await fetch("/api/meshy/ignore", {
        method: "POST", headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ task_id: b.dataset.restore, ignored: false }),
      });
      toast("restored");
      load();
    };
  });
}

function renderHits(taskId, q) {
  const box = document.querySelector(`[data-hits="${CSS.escape(taskId)}"]`);
  if (!box) return;
  if (q.length < 2) { box.innerHTML = ""; return; }
  const ql = q.toLowerCase();
  const seen = new Set(), hits = [];
  for (const i of ALL_ITEMS) {
    const f = (i.invfile || "").toLowerCase();
    if (seen.has(f)) continue;                    // one entry per ART FILE, not per item
    if (i.name.toLowerCase().includes(ql) || i.code.toLowerCase().includes(ql) || f.includes(ql)) {
      seen.add(f); hits.push(i);
      if (hits.length >= 6) break;
    }
  }
  box.innerHTML = hits.map((i) => `
    <div class="hit" data-item="${esc(i.id)}" data-invfile="${esc((i.invfile || "").toLowerCase())}"
         data-task="${esc(taskId)}" title="${esc(i.name)}">
      <img src="${spriteUrl(i.id)}" onerror="this.style.opacity=.15">
      <span>${esc((i.invfile || "").toLowerCase())}.dc6</span>
    </div>`).join("") || "<div class='why'>no match</div>";
  box.querySelectorAll(".hit").forEach((h) => {
    h.onclick = async () => {
      const r = await (await fetch("/api/meshy/links", {
        method: "POST", headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ task_id: h.dataset.task, item_id: h.dataset.item,
                               invfile: h.dataset.invfile, source: "manual" }),
      })).json();
      if (!r.ok) return toast("link failed: " + (r.error || ""), true);
      toast(`assigned to ${h.dataset.invfile}.dc6`);
      load();
    };
  });
}

$("#rescanBtn").onclick = rescan;
loadAllItems().then(load);


/* Template tuner: the original DC6 as a ghost underneath, each hand draggable on top.
   drag = move, wheel = scale, [ / ] = rotate the hand you last touched. Saved per art
   file and reused by every variant pair of that glove. */
function openTuner(invfile) {
  const p = PAIRS.find((x) => x.invfile === invfile);
  if (!p) return;
  const pick = PAIRPICK[invfile] || {};
  const tpl = JSON.parse(JSON.stringify(p.template || {}));
  const authored = JSON.parse(JSON.stringify(p.template || {}));   // pre-clamp intent
  const K = 6;
  const sel = { hand: "left" };
  // A hand with no generation is mirrored from the other at build time, so the tuner
  // must preview it mirrored too -- otherwise you would position a picture the build
  // never produces.
  const borrowed = { left: !pick.left && !!pick.right, right: !pick.right && !!pick.left };
  // what the build actually renders = XOR(borrowed art gets mirrored, template flip)
  const mirrored = {
    get left() { return borrowed.left !== !!(tpl.left && tpl.left.flip); },
    get right() { return borrowed.right !== !!(tpl.right && tpl.right.flip); },
  };
  const handSrc = (h) => {
    const t = pick[h] || pick[h === "left" ? "right" : "left"];
    return t ? `/api/pair/hand/${encodeURIComponent(t)}.png` : "";
  };

  // Silhouette outlines drive the clamp: a rotated glove's bounding-box corners are
  // empty, so clamping the box would hold the art away from the border.
  const OUT = { left: null, right: null };
  const out = p.out_size || [56, 56];      // real output size, drives the 1px inset
  const NEED = ["left", "right"].filter((h) => pick[h] || pick[h === "left" ? "right" : "left"]);
  let outlinesReady = NEED.length === 0;
  for (const h of ["left", "right"]) {
    const t = pick[h] || pick[h === "left" ? "right" : "left"];
    if (!t) continue;
    fetch(`/api/pair/outline/${encodeURIComponent(t)}`)
      .then((r) => r.json())
      .then((d) => {
        if (d.ok) OUT[h] = d;
        if (NEED.every((n) => OUT[n])) {
          // real shapes known: restore the authored layout and clamp against THOSE
          outlinesReady = true;
          Object.assign(tpl, JSON.parse(JSON.stringify(authored)));
          layout();
        }
      })
      .catch(() => { outlinesReady = true; });
  }

  const m = document.createElement("div");
  m.className = "tuner";
  m.innerHTML = `
    <div class="tunerbox">
      <h3>${esc(invfile)}.dc6 — position the hands</h3>
      <div class="tunerwrap">
        <div class="tunerstage">
          <img class="ghost" src="/api/pair/ghost/${encodeURIComponent(invfile)}.png?k=${K}">
          <img class="hand" data-hand="left" src="${handSrc("left")}">
          <img class="hand" data-hand="right" src="${handSrc("right")}">
          <div class="bounds"><div class="cellgrid"></div></div>
        </div>
      </div>
      <div class="boundsinfo">
        <span class="outdim"></span>
        <span class="clipstat"></span>
      </div>
      <div class="tunerhelp">${mirrored.left || mirrored.right
        ? `<b>${mirrored.left ? "left" : "right"} hand is mirrored</b> (no generation for it yet) · ` : ""}drag a hand to offset it ·
        front <select id="frontSel"><option value="right">right over left</option><option value="left">left over right</option></select></div>
      ${["left", "right"].map((h) => `
        <div class="ctlrow" data-ctl="${h}">
          <span class="ctlname">${h.toUpperCase()}</span>
          <label>rotation <input type="range" data-p="rot" data-h="${h}" min="-180" max="180" step="1"></label>
          <output data-o="rot" data-h="${h}"></output>
          <label>scale <input type="range" data-p="scale" data-h="${h}" min="0.1" max="6" step="0.01"></label>
          <output data-o="scale" data-h="${h}"></output>
          <output data-o="off" data-h="${h}" class="offout"></output>
          <button data-zero="${h}" title="back to neutral: offset 0, scale 1, rotation 0">zero</button>
        </div>`).join("")}
      <div class="tuneracts">
        <button id="tAutofit" title="Rotate each pinky edge parallel to its border, fill the height, snap to the side">✦ Auto-fit</button>
        <button id="tReset">revert</button>
        <button id="tZeroAll">all neutral</button>

        <button id="tCancel">cancel</button>
        <button class="gold" id="tSave">Save layout</button>
      </div>
    </div>`;
  document.body.appendChild(m);
  const stage = m.querySelector(".tunerstage");
  const ghost = m.querySelector(".ghost");
  const cells = p.cells || [2, 2];
  m.querySelector(".outdim").textContent =
    `output ${out[0]}x${out[1]}px · ${cells[0]}x${cells[1]} cells`;
  // cell guide lines inside the frame
  m.querySelector(".cellgrid").style.backgroundSize =
    `${100 / cells[0]}% ${100 / cells[1]}%`;
  m.querySelector("#frontSel").value = tpl.front || "right";

  // must mirror glove_pairs.BASE_ANCHOR / BASE_FIT so the preview matches the build
  const ANCHOR = { left: [0.34, 0.50], right: [0.66, 0.50] };
  const BASE_FIT = 0.52;

  /* Content aspect (w/h) of a hand image: the server's measured value once the
     outline arrives, else the loaded image's own natural aspect. */
  function handAspect(hand) {
    const o = OUT[hand];
    if (o && o.aspect) return o.aspect;
    const el = m.querySelector(`.hand[data-hand="${hand}"]`);
    if (el && el.naturalWidth && el.naturalHeight) return el.naturalWidth / el.naturalHeight;
    return 1;
  }

  /* Extent of a hand's SILHOUETTE, in canvas fractions, for a given scale+rotation.
     Returns the offsets from the hand's centre to its outermost opaque pixels. */
  function extent(hand) {
    const t = tpl[hand], o = OUT[hand];
    const w = BASE_FIT * t.scale;                    // longest side, canvas fractions
    const asp = handAspect(hand);
    const bw = asp >= 1 ? w : w * asp;               // content box, fractions of canvas
    const bh = asp >= 1 ? w / asp : w;
    const rad = (t.rot * Math.PI) / 180;
    const cos = Math.cos(rad), sin = Math.sin(rad);
    const pts = (o && o.points && o.points.length)
      ? o.points
      : [[0, 0], [1, 0], [0, 1], [1, 1]];            // fall back to the box
    // A hand drawn mirrored has a mirrored silhouette. CSS applies `scaleX(-1)` BEFORE
    // the rotation, so mirror the points first -- measuring the un-mirrored shape made
    // the clamp asymmetric and held the right hand ~0.2 short of its border.
    const mir = mirrored[hand];
    let l = Infinity, r = -Infinity, tp = Infinity, b = -Infinity;
    for (const [px0, py] of pts) {
      const px = mir ? (1 - px0) : px0;
      // point relative to the content centre, then rotated the way CSS rotates it
      const x = (px - 0.5) * bw, y = (py - 0.5) * bh;
      const rx = x * cos - y * sin, ry = x * sin + y * cos;
      if (rx < l) l = rx; if (rx > r) r = rx;
      if (ry < tp) tp = ry; if (ry > b) b = ry;
    }
    return { l, r, t: tp, b };
  }

  /* Keep a hand's silhouette inside the 0..1 canvas. Position hard-stops at the edge;
     scale/rotation nudge the offset inward rather than jamming, so the sliders keep
     working up to the true maximum. If the shape simply cannot fit, the caller shrinks. */
  function clampHand(hand) {
    const t = tpl[hand], a = ANCHOR[hand], e = extent(hand);
    const eps = 1 / Math.max(8, out[0]);             // one output pixel of slack
    let cx = a[0] + t.dx, cy = a[1] + t.dy;
    const wSpan = e.r - e.l, hSpan = e.b - e.t;
    if (wSpan > 1 - 2 * eps || hSpan > 1 - 2 * eps) return false;   // too big anywhere
    cx = Math.min(Math.max(cx, eps - e.l), 1 - eps - e.r);
    cy = Math.min(Math.max(cy, eps - e.t), 1 - eps - e.b);
    t.dx = cx - a[0];
    t.dy = cy - a[1];
    return true;
  }

  /* Enforce for both hands. A scale or rotation that cannot fit at any position is
     walked back until it does, so a control never leaves an illegal layout. */
  function enforce() {
    if (!outlinesReady) return;   // a box-shaped guess would clamp far too hard
    for (const hand of ["left", "right"]) {
      let guard = 0;
      while (!clampHand(hand) && guard++ < 60) tpl[hand].scale *= 0.97;
    }
  }

  function layout() {
    enforce();
    const W = ghost.clientWidth || 1, H = ghost.clientHeight || 1;
    for (const el of m.querySelectorAll(".hand")) {
      const h = el.dataset.hand, t = tpl[h], a = ANCHOR[h];
      const longest = BASE_FIT * t.scale * W;
      const asp = handAspect(h);
      const ew = asp >= 1 ? longest : longest * asp;   // matches place_hand()
      const eh = asp >= 1 ? longest / asp : longest;
      el.style.width = ew + "px";
      el.style.height = eh + "px";
      el.style.left = ((a[0] + t.dx) * W - ew / 2) + "px";
      el.style.top = ((a[1] + t.dy) * H - eh / 2) + "px";
      el.style.transform = `rotate(${t.rot}deg)` + (mirrored[h] ? " scaleX(-1)" : "");
      el.style.zIndex = (tpl.front === h) ? 3 : 2;
      el.classList.toggle("sel", sel.hand === h);
    }
    // Report loss against the SILHOUETTE, not the image box: a rotated glove's box
    // corners are transparent and may legitimately overhang, but no opaque pixel can.
    // With the clamp in force this should always read zero -- it stays as a check that
    // the clamp is actually holding rather than as a routine warning.
    let outside = 0;
    for (const h of ["left", "right"]) {
      const el = m.querySelector(`.hand[data-hand="${h}"]`);
      if (!el || !el.getAttribute("src")) continue;
      const e = extent(h), a = ANCHOR[h], t = tpl[h];
      const cx = a[0] + t.dx, cy = a[1] + t.dy;
      outside += Math.max(0, -(cx + e.l)) + Math.max(0, (cx + e.r) - 1)
               + Math.max(0, -(cy + e.t)) + Math.max(0, (cy + e.b) - 1);
    }
    const pct = Math.round(outside * 100);
    const cs = m.querySelector(".clipstat");
    cs.textContent = pct <= 0 ? "✓ all pixels inside the border" : `${pct}% outside`;
    cs.className = "clipstat" + (pct > 0 ? " bad" : " ok");
    for (const h of ["left", "right"]) {
      const t = tpl[h];
      m.querySelector(`[data-p="rot"][data-h="${h}"]`).value = t.rot;
      m.querySelector(`[data-p="scale"][data-h="${h}"]`).value = t.scale;
      m.querySelector(`[data-o="rot"][data-h="${h}"]`).textContent = `${Math.round(t.rot)}°`;
      m.querySelector(`[data-o="scale"][data-h="${h}"]`).textContent = `${t.scale.toFixed(2)}x`;
      m.querySelector(`[data-o="off"][data-h="${h}"]`).textContent =
        `offset ${t.dx >= 0 ? "+" : ""}${t.dx.toFixed(2)}, ${t.dy >= 0 ? "+" : ""}${t.dy.toFixed(2)}`;
      m.querySelector(`.ctlrow[data-ctl="${h}"]`).classList.toggle("sel", sel.hand === h);
    }
  }
  ghost.onload = layout;
  m.querySelectorAll(".hand").forEach((el) => { el.onload = layout; });
  layout();

  let drag = null;
  stage.addEventListener("mousedown", (e) => {
    const el = e.target.closest(".hand"); if (!el) return;
    sel.hand = el.dataset.hand;
    drag = { el, x: e.clientX, y: e.clientY, t: Object.assign({}, tpl[el.dataset.hand]) };
    e.preventDefault(); layout();
  });
  window.addEventListener("mousemove", (e) => {
    if (!drag) return;
    const W = ghost.clientWidth || 1, H = ghost.clientHeight || 1;
    const t = tpl[drag.el.dataset.hand];
    t.dx = drag.t.dx + (e.clientX - drag.x) / W;
    t.dy = drag.t.dy + (e.clientY - drag.y) / H;
    layout();
  });
  window.addEventListener("mouseup", () => { drag = null; });
  stage.addEventListener("wheel", (e) => {
    const el = e.target.closest(".hand"); if (!el) return;
    e.preventDefault();
    sel.hand = el.dataset.hand;
    const t = tpl[el.dataset.hand];
    t.scale = Math.max(0.1, Math.min(6, t.scale * (e.deltaY < 0 ? 1.05 : 0.952)));
    layout();
  }, { passive: false });
  const keys = (e) => {
    if (e.key !== "[" && e.key !== "]") return;
    tpl[sel.hand].rot += (e.key === "[" ? -3 : 3);
    layout();
  };
  window.addEventListener("keydown", keys);
  m.querySelectorAll("input[type=range]").forEach((r) => {
    r.oninput = () => {
      sel.hand = r.dataset.h;
      tpl[r.dataset.h][r.dataset.p] = parseFloat(r.value);
      layout();
    };
  });
  m.querySelectorAll("[data-zero]").forEach((b) => {
    b.onclick = () => {
      Object.assign(tpl[b.dataset.zero], { dx: 0, dy: 0, scale: 1, rot: 0 });
      sel.hand = b.dataset.zero; layout();
    };
  });
  m.querySelector("#tAutofit").onclick = async () => {
    const b = m.querySelector("#tAutofit");
    b.disabled = true; b.textContent = "fitting…";
    try {
      const r = await (await fetch("/api/pair/autofit", {
        method: "POST", headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ invfile, left_task: pick.left || null,
                               right_task: pick.right || null }),
      })).json();
      if (!r.ok) return toast("auto-fit failed: " + (r.error || ""), true);
      Object.assign(tpl, r.template);
      Object.assign(authored, JSON.parse(JSON.stringify(r.template)));
      layout();
      const flipped = Object.entries(r.notes || {})
        .filter(([, n]) => n && n.flipped_to_match_name).map(([h]) => h);
      toast(flipped.length
        ? `auto-fitted — ${flipped.join(" + ")} art was oriented the other way, flipped to match`
        : "auto-fitted — pinky edges squared to the border");
    } finally {
      b.disabled = false; b.textContent = "✦ Auto-fit";
    }
  };
  m.querySelector("#tZeroAll").onclick = () => {
    for (const h of ["left", "right"]) Object.assign(tpl[h], { dx: 0, dy: 0, scale: 1, rot: 0 });
    layout();
  };
  m.querySelector("#frontSel").onchange = (e) => { tpl.front = e.target.value; layout(); };

  const close = () => { window.removeEventListener("keydown", keys); m.remove(); };
  m.querySelector("#tCancel").onclick = close;
  m.querySelector("#tReset").onclick = () => {
    Object.assign(tpl, JSON.parse(JSON.stringify(p.template || {})));
    m.querySelector("#frontSel").value = tpl.front || "right"; layout();
  };
  m.querySelector("#tSave").onclick = async () => {
    const r = await (await fetch(`/api/pair/template/${encodeURIComponent(invfile)}`, {
      method: "POST", headers: { "Content-Type": "application/json" },
      body: JSON.stringify(tpl),
    })).json();
    if (!r.ok) return toast("save failed", true);
    p.template = r.template;
    toast(`layout saved — every ${invfile} variant pair uses it`);
    close();
  };
}
