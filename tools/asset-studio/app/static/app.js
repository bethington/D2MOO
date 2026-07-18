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
      <label>Generate with Meshy.ai — turns this item's art into a 3D model (~2 min, costs credits)</label>
      <button id="meshyGenBtn">✦ Generate 3D from Meshy</button>
      <div id="meshyProgress" class="meshyprog"></div>
      <div id="meshyRender" class="hidden">
        <label style="margin-top:8px">1. Review the 3D model:</label>
        <div class="thumb checker" style="height:120px"><img id="meshyPreview" style="max-height:120px;image-rendering:auto"></div>
        <label style="margin-top:8px">2. Texture it with Meshy (optional — describe the look):</label>
        <div class="anglerow">
          <input type="text" id="texPrompt" style="width:200px" placeholder="e.g. golden crown, green gems, worn leather">
          <button id="textureBtn" title="Generate a texture for the model from your description (Meshy, costs credits)">Texture (Meshy)</button>
        </div>
        <label style="margin-top:8px">3. Render to a sprite:</label>
        <div class="anglerow">
          azim <input type="number" id="azim" value="25" min="0" max="359" step="5">
          elev <input type="number" id="elev" value="20" min="-10" max="80" step="5">
          <button id="renderBtn" title="Render the (textured) 3D model at this angle with Blender">Render (Blender)</button>
          <button id="usePreviewBtn" title="Use Meshy's preview render (no angle control)">Use preview</button>
        </div>
        ${it.flippyfile ? `<div class="anglerow">
          <button id="renderFlippyBtn" title="Blender-turntable the 3D model into a full ground-drop animation (one render per flippy frame)">Render flippy (Blender)</button>
        </div>` : ""}
      </div>
    </div>`;
  d.querySelectorAll(".variant:not(.fv)").forEach((v) => {
    v.onclick = () => activate(it, v.dataset.choice);
  });
  d.querySelectorAll(".variant.fv").forEach((v) => {
    v.onclick = () => activateFlippy(it, v.dataset.fchoice);
  });
  $("#pngFile").onchange = (e) => importPng(it, e.target.files[0]);
  $("#meshyGenBtn").onclick = () => meshyGenerate(it);
  $("#dropBtn").onclick = () => dropInGame(it);
  if (it.category === "unique") wireTxtSection(it);
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

let CUR_TID = null;  // current Meshy task for the selected item (updates after texturing)

async function pollTask(tid, prog, label) {
  for (let i = 0; i < 90; i++) {
    await new Promise((res) => setTimeout(res, 4000));
    const t = await (await fetch(`/api/meshy/task/${tid}`)).json();
    prog.textContent = `${label}: ${t.status} ${t.progress || 0}%`;
    if (t.status === "SUCCEEDED") return true;
    if (t.status === "FAILED" || t.status === "CANCELED") { prog.textContent = `${label} ${t.status}: ${t.error || ""}`; return false; }
  }
  prog.textContent = label + ": timed out";
  return false;
}

function showModelStage(it, tid) {
  CUR_TID = tid;
  $("#meshyRender").classList.remove("hidden");
  $("#meshyPreview").src = `/api/meshy/preview/${tid}.png?t=${Date.now()}`;
  $("#renderBtn").disabled = !HAS_BLENDER;
  $("#renderBtn").textContent = HAS_BLENDER ? "Render (Blender)" : "Blender not installed";
  $("#renderBtn").onclick = () => meshyRender(it);
  $("#usePreviewBtn").onclick = () => meshyUsePreview(it, CUR_TID);
  $("#textureBtn").onclick = () => meshyTexture(it);
  const fb = $("#renderFlippyBtn");
  if (fb) {
    fb.disabled = !HAS_BLENDER;
    fb.onclick = () => meshyRenderFlippy(it);
  }
}

async function meshyRenderFlippy(it) {
  const prog = $("#meshyProgress");
  const elev = +($("#elev") ? $("#elev").value : 15);
  $("#renderFlippyBtn").disabled = true;
  prog.textContent = "rendering flippy turntable with Blender (one frame per flippy frame, ~1-3 min)…";
  try {
    const r = await fetch(`/api/item/${encodeURIComponent(it.id)}/meshy/render-flippy/${CUR_TID}`, {
      method: "POST", headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ elev }),
    });
    const u = await r.json();
    if (u.ok) {
      it.flippy_alts = u.flippy_alts;
      prog.textContent = "flippy rendered.";
      toast(`flippy alternate "${u.alt_id}" added`);
      await activateFlippy(it, u.alt_id);
    } else prog.textContent = "flippy render failed: " + u.error;
  } finally {
    if ($("#renderFlippyBtn")) $("#renderFlippyBtn").disabled = false;
  }
}

async function meshyGenerate(it) {
  const btn = $("#meshyGenBtn"), prog = $("#meshyProgress");
  btn.disabled = true;
  prog.textContent = "submitting to Meshy…";
  try {
    const r = await fetch(`/api/item/${encodeURIComponent(it.id)}/meshy/generate`, { method: "POST" });
    const data = await r.json();
    if (!data.ok) { prog.textContent = "error: " + data.error; return; }
    if (await pollTask(data.task_id, prog, "Meshy 3D")) {
      prog.textContent = "3D model ready — review the shape, then texture and/or render.";
      showModelStage(it, data.task_id);
      if (!HAS_BLENDER) await meshyUsePreview(it, data.task_id);
    }
  } finally {
    btn.disabled = false;
    pollMeshy();
  }
}

async function meshyTexture(it) {
  const prog = $("#meshyProgress");
  const prompt = $("#texPrompt").value.trim();
  $("#textureBtn").disabled = true;
  prog.textContent = prompt ? "texturing (from image + prompt)…" : "texturing from the original image…";
  try {
    const r = await fetch(`/api/item/${encodeURIComponent(it.id)}/meshy/texture/${CUR_TID}`, {
      method: "POST", headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ prompt, use_image: true }),
    });
    const d = await r.json();
    if (!d.ok) { prog.textContent = "texture failed: " + d.error; return; }
    if (await pollTask(d.task_id, prog, "Meshy texture")) {
      prog.textContent = "textured — review, then render.";
      showModelStage(it, d.task_id);  // CUR_TID now points at the textured model
      toast("model retextured");
    }
  } finally {
    $("#textureBtn").disabled = false;
    pollMeshy();
  }
}

async function meshyRender(it) {
  const prog = $("#meshyProgress");
  const azim = +$("#azim").value, elev = +$("#elev").value;
  $("#renderBtn").disabled = true;
  prog.textContent = `rendering with Blender (azim ${azim}, elev ${elev})…`;
  try {
    const r = await fetch(`/api/item/${encodeURIComponent(it.id)}/meshy/render/${CUR_TID}`, {
      method: "POST", headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ azim, elev }),
    });
    const u = await r.json();
    if (u.ok) { it.alts = u.alts; prog.textContent = "rendered."; toast(`Blender alternate "${u.alt_id}" added`); await activate(it, u.alt_id); }
    else prog.textContent = "render failed: " + u.error;
  } finally {
    $("#renderBtn").disabled = false;
  }
}

async function meshyUsePreview(it, tid) {
  const prog = $("#meshyProgress");
  prog.textContent = "importing Meshy preview…";
  const u = await (await fetch(`/api/item/${encodeURIComponent(it.id)}/meshy/use/${tid}`, { method: "POST" })).json();
  if (u.ok) { it.alts = u.alts; prog.textContent = "3D model ready."; toast(`Meshy preview alternate "${u.alt_id}" added`); await activate(it, u.alt_id); }
  else prog.textContent = "import failed: " + u.error;
}

async function pollMeshy() {
  try {
    const r = await fetch("/api/meshy/status");
    const d = await r.json();
    const el = $("#meshyStatus");
    HAS_BLENDER = !!d.blender;
    const bl = d.blender ? " · blender ✓" : " · no blender";
    if (d.hasKey && d.ok) { el.textContent = `meshy: ${d.balance} credits${bl}`; el.className = "game ok"; }
    else if (!d.hasKey) { el.textContent = "meshy: no key" + bl; el.className = "game bad"; }
    else { el.textContent = "meshy: error" + bl; el.className = "game bad"; }
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
function debounce(fn, ms) { let t; return (...a) => { clearTimeout(t); t = setTimeout(() => fn(...a), ms); }; }

loadItems();
pollGame();
pollMeshy();
setInterval(pollGame, 5000);
setInterval(pollMeshy, 15000);
