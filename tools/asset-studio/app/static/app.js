const $ = (s) => document.querySelector(s);
let ITEMS = [];
let SELECTED = null;
let HAS_BLENDER = false;

function toast(msg, isErr) {
  const t = $("#toast");
  t.textContent = msg;
  t.className = "toast" + (isErr ? " err" : "");
  clearTimeout(t._t);
  t._t = setTimeout(() => t.classList.add("hidden"), 4200);
}

async function loadItems() {
  const q = encodeURIComponent($("#search").value.trim());
  const cat = $("#category").value;
  const r = await fetch(`/api/items?q=${q}&category=${cat}`);
  const data = await r.json();
  ITEMS = data.items;
  $("#count").textContent = `${data.count} items`;
  renderGrid();
}

function renderGrid() {
  const g = $("#grid");
  g.innerHTML = "";
  for (const it of ITEMS) {
    const modded = it.active && it.active !== "original";
    const card = document.createElement("div");
    card.className = "card" + (modded ? " modded" : "");
    card.innerHTML = `
      <div class="thumb checker"><img loading="lazy" src="/api/item/${encodeURIComponent(it.id)}/original.png" onerror="this.style.opacity=.15"></div>
      <div class="nm" title="${it.name}">${it.name}</div>
      <div class="cd">${it.code}${modded ? ' <span class="badge">● mod</span>' : ""}</div>`;
    card.onclick = () => selectItem(it);
    g.appendChild(card);
  }
}

async function selectItem(it) {
  SELECTED = it;
  const d = $("#detail");
  d.classList.remove("hidden");
  const variants = [
    `<div class="variant ${it.active === "original" ? "active" : ""}" data-choice="original">
       <div class="thumb checker"><img src="/api/item/${encodeURIComponent(it.id)}/original.png"></div>
       <div class="lbl">original</div></div>`,
    ...it.alts.map((a) => `
      <div class="variant ${it.active === a ? "active" : ""}" data-choice="${a}">
        <div class="thumb checker"><img src="/api/item/${encodeURIComponent(it.id)}/alt/${a}.png?t=${Date.now()}"></div>
        <div class="lbl">${a}</div></div>`),
  ].join("");
  const flippyVariants = it.flippyfile ? [
    `<div class="variant fv ${it.flippy_active === "original" ? "active" : ""}" data-fchoice="original">
       <div class="thumb checker"><img src="/api/item/${encodeURIComponent(it.id)}/flippy/original.gif"></div>
       <div class="lbl">original</div></div>`,
    ...(it.flippy_alts || []).map((a) => `
      <div class="variant fv ${it.flippy_active === a ? "active" : ""}" data-fchoice="${a}">
        <div class="thumb checker"><img src="/api/item/${encodeURIComponent(it.id)}/flippy/alt/${a}.gif?t=${Date.now()}"></div>
        <div class="lbl">${a}</div></div>`),
  ].join("") : "";
  const txtSection = it.category === "unique" ? `
    <div class="uploader" id="txtSection">
      <label>Own art file — give this unique its own invfile in uniqueitems.bin
        (it currently ${it.invtransform ? "inherits + tints" : "uses"} <b>${it.invfile}</b>)</label>
      <div class="anglerow">
        <input type="text" id="ownInvfile" style="width:180px" maxlength="31"
               placeholder="e.g. inv${it.code.trim()}u" spellcheck="false">
        <button id="setInvfileBtn" title="Patch this unique's invfile cell in uniqueitems.bin (goes into the patch.mpq on push; needs Full reload)">Set invfile</button>
        <button id="revertInvfileBtn" title="Revert to the inherited base art file">Revert</button>
      </div>
      <div class="meta" id="txtState"></div>
    </div>` : "";
  d.innerHTML = `
    <h2>${it.name}</h2>
    <div class="meta">${it.category} · code <b>${it.code}</b> · ${it.invwidth}×${it.invheight} cells · ${it.invfile}.dc6${it.invtransform ? " · tint " + it.invtransform : ""}</div>
    <div class="anglerow"><button id="dropBtn" title="Spawn this item on the ground at your feet (be in a game) to test its art — pick it up to see the inventory sprite">⤓ Drop in game</button></div>
    <div class="variants">${variants}</div>
    ${txtSection}
    ${it.flippyfile ? `<div class="uploader">
      <label>Ground-drop animation (flippy — ${it.flippyfile}.dc6)</label>
      <div class="variants">${flippyVariants}</div>
    </div>` : ""}
    <div class="uploader">
      <label>Import a PNG as a new alternate (auto-fit to ${it.invwidth}×${it.invheight} cells &amp; quantized to the D2 palette)</label>
      <input type="file" id="pngFile" accept="image/png,image/*">
    </div>
    <div class="uploader">
      <label>Generate a 3D model — open the <b>Studio</b>: web-app flow with a rotatable 3D preview, shape re-rolls, texture step &amp; live tone controls (uses your Meshy login, free retries)</label>
      <a href="/studio?item=${encodeURIComponent(it.id)}"><button class="gold">⚒ Open ${it.name} in Studio →</button></a>
    </div>
    <div class="uploader">
      <label>Meshy pairing — link a generation you already made in the Meshy web app to this item, then open it in the Studio to re-roll / texture / accept</label>
      <div class="anglerow"><button id="linkMeshyBtn">🔗 Link a Meshy task…</button></div>
      <div class="meta" id="linkState"></div>
      <div class="variants" id="taskPicker" style="display:none"></div>
    </div>`;
  d.querySelectorAll(".variant:not(.fv)").forEach((v) => {
    v.onclick = () => activate(it, v.dataset.choice);
  });
  d.querySelectorAll(".variant.fv").forEach((v) => {
    v.onclick = () => activateFlippy(it, v.dataset.fchoice);
  });
  $("#pngFile").onchange = (e) => importPng(it, e.target.files[0]);
  $("#dropBtn").onclick = () => dropInGame(it);
  if (it.category === "unique") wireTxtSection(it);
  wireMeshyLinks(it);
}

/* ---------- Meshy pairing ---------- */
async function wireMeshyLinks(it) {
  $("#linkMeshyBtn").onclick = () => openTaskPicker(it);
  refreshLinkState(it);
}

async function refreshLinkState(it) {
  const state = $("#linkState");
  try {
    const d = await (await fetch("/api/meshy/links")).json();
    const mine = (d.links || []).filter((l) => l.item_id === it.id);
    if (!mine.length) { state.textContent = "no Meshy task linked yet"; return; }
    state.innerHTML = mine.map((l) => `linked: <b>${l.name || l.task_id.slice(0, 8)}</b>
      (${l.phase || "draft"}, ${l.source || "manual"})
      <a href="/studio?item=${encodeURIComponent(it.id)}&task=${l.task_id}"><button>⚒ Open in Studio</button></a>
      <button data-unlink="${l.task_id}">✕ unlink</button>`).join("<br>");
    state.querySelectorAll("[data-unlink]").forEach((b) => {
      b.onclick = async () => {
        await fetch(`/api/meshy/links/${b.dataset.unlink}`, { method: "DELETE" });
        refreshLinkState(it);
      };
    });
  } catch (e) { state.textContent = ""; }
}

async function openTaskPicker(it) {
  const p = $("#taskPicker");
  if (p.style.display !== "none") { p.style.display = "none"; return; }
  p.style.display = ""; p.innerHTML = "<div class='meta'>loading your Meshy workspace…</div>";
  const d = await (await fetch("/api/meshy/tasks")).json();
  if (!d.ok) { p.innerHTML = `<div class='meta'>${d.error || "failed"}</div>`; return; }
  const rows = d.tasks.filter((t) => t.status === "SUCCEEDED");
  p.innerHTML = rows.map((t) => `
    <div class="variant" data-task="${t.id}" title="${t.phase} · ${8 - (t.retryCount || 0)} free re-rolls left${t.linked_item_name ? " · already linked to " + t.linked_item_name : ""}">
      <div class="thumb checker">${t.preview ? `<img src="${t.preview}" loading="lazy">` : ""}</div>
      <div class="lbl">${t.name || t.id.slice(0, 8)}${t.linked_item_name ? " 🔗" : ""}</div>
    </div>`).join("") || "<div class='meta'>no finished tasks found</div>";
  p.querySelectorAll("[data-task]").forEach((v) => {
    v.onclick = async () => {
      const r = await (await fetch("/api/meshy/links", {
        method: "POST", headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ task_id: v.dataset.task, item_id: it.id }),
      })).json();
      if (!r.ok) return toast("link failed: " + (r.error || ""), true);
      toast(`linked to ${it.name} — open it in the Studio to continue`);
      p.style.display = "none";
      refreshLinkState(it);
    };
  });
}

async function scanMeshy() {
  const btn = $("#pairMeshyBtn");
  btn.disabled = true; btn.textContent = "⇄ scanning…";
  try {
    const r = await (await fetch("/api/meshy/links/scan", {
      method: "POST", headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ pages: 2 }),
    })).json();
    if (!r.ok) return toast("scan failed: " + (r.error || ""), true);
    toast(`scanned ${r.scanned} tasks — ${r.auto.length} auto-paired by image, ${r.suggestions.length} name suggestions, ${r.unmatched.length} unmatched`);
    if (r.suggestions.length) showSuggestions(r.suggestions);
  } finally {
    btn.disabled = false; btn.textContent = "⇄ Pair Meshy";
  }
}

function showSuggestions(sugs) {
  let m = document.getElementById("sugModal");
  if (m) m.remove();
  m = document.createElement("div");
  m.id = "sugModal";
  m.style.cssText = "position:fixed;inset:10% 20%;background:#1c1c22;border:1px solid #444;" +
    "border-radius:8px;padding:16px;overflow:auto;z-index:50;box-shadow:0 8px 40px #000";
  m.innerHTML = "<h3>Pair suggestions — check the picture before linking</h3>" +
    "<div class='meta'>Left = what you fed Meshy. These are guesses (image-similarity or name), " +
    "not exact matches — pick the right item or skip.</div>" + sugs.map((s, i) => `
    <div class="anglerow" style="margin:8px 0;align-items:center">
      ${s.input_image ? `<img src="${s.input_image}" style="width:64px;height:64px;object-fit:contain;background:#111" title="input image">` : ""}
      ${s.preview ? `<img src="${s.preview}" style="width:64px;height:64px;object-fit:contain;background:#111" title="generated model">` : ""}
      <b>${s.task_name || s.task_id.slice(0, 8)}</b> →
      <select id="sug${i}">${s.candidates.map((c) => `<option value="${c.item_id}">${c.item_name} — ${c.why}</option>`).join("")}</select>
      <button data-i="${i}" data-task="${s.task_id}">Link</button>
    </div>`).join("") + "<div class='anglerow'><button id='sugClose'>Close</button></div>";
  document.body.appendChild(m);
  m.querySelector("#sugClose").onclick = () => m.remove();
  m.querySelectorAll("[data-task]").forEach((b) => {
    b.onclick = async () => {
      const item_id = m.querySelector(`#sug${b.dataset.i}`).value;
      const r = await (await fetch("/api/meshy/links", {
        method: "POST", headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ task_id: b.dataset.task, item_id, source: "fuzzy-confirmed" }),
      })).json();
      if (!r.ok) return toast("link failed: " + (r.error || ""), true);
      b.textContent = "✓ linked"; b.disabled = true;
    };
  });
}

async function wireTxtSection(it) {
  const state = $("#txtState");
  try {
    const u = await (await fetch(`/api/item/${encodeURIComponent(it.id)}/txt`)).json();
    if (u.ok) {
      $("#ownInvfile").value = u.invfile || "";
      state.textContent = u.invfile
        ? `own invfile set: ${u.invfile}.dc6 (stock: ${u.stock_invfile || "inherited"})`
        : `no own invfile — inherits ${u.effective_invfile}.dc6`;
    } else state.textContent = u.error || "";
  } catch (e) { state.textContent = ""; }
  $("#setInvfileBtn").onclick = () => setOwnInvfile(it, $("#ownInvfile").value.trim());
  $("#revertInvfileBtn").onclick = () => setOwnInvfile(it, "");
}

async function setOwnInvfile(it, value) {
  $("#setInvfileBtn").disabled = true;
  try {
    const r = await fetch(`/api/item/${encodeURIComponent(it.id)}/txt`, {
      method: "POST", headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ field: "invfile", value }),
    });
    const u = await r.json();
    if (!u.ok) return toast("invfile edit failed: " + (u.error || ""), true);
    toast(value
      ? `${it.name} now has its own art file "${value}.dc6"${u.seeded ? " (seeded with current art)" : ""} — Push + Full reload to apply`
      : `${it.name} reverted to inherited art file`);
    await loadItems();
    const fresh = ITEMS.find((x) => x.id === it.id);
    if (fresh) selectItem(fresh);
  } finally {
    if ($("#setInvfileBtn")) $("#setInvfileBtn").disabled = false;
  }
}

async function activateFlippy(it, choice) {
  const r = await fetch(`/api/item/${encodeURIComponent(it.id)}/activate-flippy`, {
    method: "POST", headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ choice }),
  });
  const data = await r.json();
  if (!data.ok) return toast("flippy activate failed: " + (data.error || ""), true);
  it.flippy_active = choice;
  toast(`${it.name} flippy: ${choice === "original" ? "reverted to original" : "using " + choice} (Push to game to apply)`);
  selectItem(it);
}

async function dropInGame(it) {
  const btn = $("#dropBtn");
  btn.disabled = true;
  toast(`dropping ${it.name} at your feet…`);
  try {
    const r = await fetch(`/api/item/${encodeURIComponent(it.id)}/drop`, { method: "POST" });
    const d = await r.json();
    toast(d.ok ? `${it.name}: ${d.note}` : "drop failed: " + (d.error || ""), !d.ok);
  } finally {
    btn.disabled = false;
  }
}

async function pollMeshy() {
  try {
    const s = await (await fetch("/api/studio/session")).json();
    const el = $("#meshyStatus");
    HAS_BLENDER = !!s.blender;
    const bl = s.blender ? " · blender ✓" : " · no blender";
    if (s.loggedIn) { el.textContent = `meshy: ${s.tier || "session"} ✓${bl}`; el.className = "game ok"; }
    else { el.textContent = "meshy: log in via Studio" + bl; el.className = "game bad"; }
  } catch (e) { /* ignore */ }
}

async function activate(it, choice) {
  const r = await fetch(`/api/item/${encodeURIComponent(it.id)}/activate`, {
    method: "POST", headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ choice }),
  });
  const data = await r.json();
  if (!data.ok) return toast("activate failed", true);
  it.active = choice;
  toast(`${it.name}: ${choice === "original" ? "reverted to original" : "using " + choice} (Push to game to apply)`);
  selectItem(it);
  renderGrid();
}

async function importPng(it, file) {
  if (!file) return;
  const fd = new FormData();
  fd.append("file", file);
  const r = await fetch(`/api/item/${encodeURIComponent(it.id)}/import`, { method: "POST", body: fd });
  const data = await r.json();
  if (!data.ok) return toast("import failed: " + (data.error || ""), true);
  it.alts = data.alts;
  toast(`imported alternate "${data.alt_id}"`);
  await activate(it, data.alt_id);
}

async function push() {
  $("#pushBtn").disabled = true;
  toast("building patch.mpq & registering…");
  try {
    const r = await fetch("/api/push", { method: "POST" });
    const data = await r.json();
    if (data.ok) toast(`pushed ${data.built} file(s) → registered live. Reload game (or re-enter) to see it.`);
    else toast(`push failed (${data.stage}): ${data.error}`, true);
  } finally {
    $("#pushBtn").disabled = false;
  }
}

async function reload() {
  $("#reloadBtn").disabled = true;
  toast("soft reload: exiting to menu & re-entering…");
  try {
    const r = await fetch("/api/reload", { method: "POST" });
    const data = await r.json();
    toast(data.ok ? "soft reload done — " + (data.note || "") : "reload issue: " + (data.error || ""), !data.ok);
  } finally {
    $("#reloadBtn").disabled = false;
  }
}

async function fullReload() {
  $("#fullReloadBtn").disabled = true;
  toast("full reload: relaunching a fresh game (accept the UAC prompt)… ~60-90s");
  try {
    const r = await fetch("/api/full-reload", { method: "POST" });
    const data = await r.json();
    toast(data.ok ? "full reload done — art loaded fresh (" + (data.note || "") + ")" : "full reload issue: " + (data.note || ""), !data.ok);
  } finally {
    $("#fullReloadBtn").disabled = false;
  }
}

async function pollGame() {
  try {
    const r = await fetch("/api/game/status");
    const data = await r.json();
    const el = $("#gameStatus");
    if (data.reachable) {
      const a = data.asset || {};
      el.textContent = `game: connected${a.registered ? " · overlay registered @" + a.priority : ""}`;
      el.className = "game ok";
    } else {
      el.textContent = "game: not running (start PD2 with the debugger)";
      el.className = "game bad";
    }
  } catch (e) { /* ignore */ }
}

$("#search").oninput = debounce(loadItems, 250);
$("#category").onchange = loadItems;
$("#pushBtn").onclick = push;
$("#reloadBtn").onclick = reload;
$("#fullReloadBtn").onclick = fullReload;
$("#pairMeshyBtn").onclick = scanMeshy;
function debounce(fn, ms) { let t; return (...a) => { clearTimeout(t); t = setTimeout(() => fn(...a), ms); }; }

loadItems();
pollGame();
pollMeshy();
setInterval(pollGame, 5000);
setInterval(pollMeshy, 15000);
