/* AI upscale -> 3D workflow panel.
   Slides over the item detail rail. Opened by openWorkflow(item) from app.js.

   Top-to-bottom flow:
     1 Source        — the original art.
     2 Generate      — two tabs sharing one history strip (badges: ↑ upscale / ✎ desc):
                       "Upscale" = img2img detail-fill; "From description" = Florence-2 GPU
                       caption -> editable text -> txt2img with a Reference slider (tile CN).
     3 3D model      — Meshy draft from the selected variant's hi-res master; free ×8 re-rolls;
                       live three.js viewer (workflow3d.js). Gloves feed the saved single-hand
                       mask instead (pair is one merged blob).
     4 Texture       — texture the approved draft; the GLB downloads (proxied) once it's done.
     5 Final render  — Blender render at the inventory angle -> DC6 alternate (downsized to the
                       original art size) -> manual Activate. Gloves route through the
                       mirrored-pair Blender build.

   Meshy chain state persists server-side in the item's upscale index (survives reloads). */
(function () {
  const $ = (s, r = document) => r.querySelector(s);
  const enc = encodeURIComponent;
  let ITEM = null, STATE = null, capPoll = null, GENTAB = "enhance";
  let MESHY = {};                       // {draft_tid, texture_tid, phase, source_vid, alt_id}
  // Sticky Generate controls — survive render() (which rebuilds innerHTML) so a
  // Generate click doesn't reset the sliders/prompt back to their defaults.
  let GEN = { steps: 4, engine: "qwen", shape: 0.7, prompt: "", restyle: "", neg: "", gan: true };
  function snapGen() {
    const g = (id) => $(id);
    if (g("#wfSteps")) GEN.steps = +g("#wfSteps").value;
    if (g("#wfEngine")) GEN.engine = g("#wfEngine").value;
    if (g("#wfShape")) GEN.shape = +g("#wfShape").value;
    if (g("#wfPrompt")) GEN.prompt = g("#wfPrompt").value;   // enhance 'extra'
    if (g("#wfDesc")) GEN.restyle = g("#wfDesc").value;       // restyle 'new look'
    if (g("#wfNeg")) GEN.neg = g("#wfNeg").value;
    if (g("#wfGan")) GEN.gan = g("#wfGan").checked;
  }
  let TASK = { draft: null, texture: null };  // last poll snapshots
  let taskPoll = null, BUSY = null;     // BUSY: "gen3d" | "texture" | "ship" | null

  function ensurePanel() {
    let p = $("#workflow");
    if (p) return p;
    p = document.createElement("aside");
    p.id = "workflow";
    p.className = "workflow-panel";
    document.body.appendChild(p);
    return p;
  }

  async function api(method, url, body) {
    const opt = { method, headers: {} };
    if (body !== undefined) { opt.headers["Content-Type"] = "application/json"; opt.body = JSON.stringify(body); }
    const r = await fetch(url, opt);
    return r.json();
  }

  function randSeed() { return Math.floor(Math.random() * 2 ** 31); }
  function fmt(x) { return (x == null ? "" : Number(x).toFixed(2)); }
  function esc(s) { return (s || "").replace(/[&<>]/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;" }[c])); }

  function chip(state, label) {
    if (state === "busy") return `<span class="wf-chip busy">${label || "working…"}</span>`;
    if (state === "done") return `<span class="wf-chip done">${label || "done"}</span>`;
    return `<span class="wf-chip">${label || "empty"}</span>`;
  }

  // Meshy textures from EITHER the reference image OR a text prompt (mutually exclusive).
  // Default = empty prompt = guided by your selected art image, which is what the flow wants.

  // ---------- section markup ----------

  function section3() {
    const it = ITEM, up = STATE.upscale || { variants: [] };
    const hasSel = !!up.selected;
    const glove = it.is_glove, boot = it.is_boot;
    const enabled = glove ? it.has_mask : hasSel;
    const d = TASK.draft;
    const running = MESHY.draft_tid && d && (d.status === "PENDING" || d.status === "IN_PROGRESS");
    const done = d && d.status === "SUCCEEDED";
    const rr = d ? d.rerollsLeft : 8;
    let src;
    if (glove) {
      src = it.has_mask
        ? `<div class="wf-sub">Gloves: the 3D model is built from your saved <b>single-hand mask</b>
           (the pair is one merged blob). The §2 art is still used for the DC6 look.</div>`
        : `<div class="wf-sub">⚠ Gloves need a single-hand mask first — brush it in the
           <a href="/studio?item=${enc(it.id)}" target="_blank">Studio</a>, then reopen this panel.</div>`;
    } else if (boot) {
      const bs = STATE.boots || {};
      const mode = MESHY.boots_mode || "multi";
      src = `
        <div class="wf-sub">Boots: the art holds both boots — they're auto-split and the model is
          built from the split sides.</div>
        <div class="wf-gen" style="margin-top:6px">
          <button id="wfSplit" ${BUSY ? "disabled" : ""}>🥾 Split preview</button>
          <span id="wfSplitNote" class="wf-sub" style="margin:0">
            ${bs.method ? `${bs.method} split · confidence ${bs.confidence} · ${esc(bs.source || "")}` : "not split yet"}</span>
        </div>
        <div id="wfSplitRow" class="wf-src" style="margin-top:6px; ${bs.method ? "" : "display:none"}">
          <div class="thumb checker"><img id="wfBootL" src="/api/upscale/${enc(it.id)}/boot/left.png?t=${Date.now()}"></div>
          <div class="thumb checker"><img id="wfBootR" src="/api/upscale/${enc(it.id)}/boot/right.png?t=${Date.now()}"></div>
        </div>
        <div class="wf-row" style="margin-top:6px"><label>Meshy input</label>
          <label style="width:auto"><input type="radio" name="wfBootsMode" value="multi" ${mode === "multi" ? "checked" : ""}>
            both boots (2-image)</label>
          <label style="width:auto"><input type="radio" name="wfBootsMode" value="single" ${mode === "single" ? "checked" : ""}>
            one boot + mirror</label>
        </div>`;
    } else {
      src = hasSel
        ? `<div class="wf-sub">Feeds the selected variant's hi-res master (${esc(up.selected)}).</div>`
        : `<div class="wf-sub">Select or generate a variant in §2 first.</div>`;
    }
    return `
      <div class="wf-section ${enabled ? "" : "disabled"}">
        <div class="wf-shead"><span class="num">3</span>3D model (Meshy)
          ${done ? chip("done") : running ? chip("busy", (d && d.progress != null ? d.progress + "%" : "…")) : chip("empty")}</div>
        <div class="wf-body">
          ${src}
          <div class="wf-gen">
            <button id="wfGen3d" class="gold" ${running || BUSY ? "disabled" : ""}>
              ${MESHY.draft_tid ? "⟳ New draft" : "🧊 Generate 3D"}</button>
            ${done ? `<button id="wfReroll" ${BUSY || !rr ? "disabled" : ""}
                title="Free in-place re-roll (Meshy replaces the draft)">🎲 Re-roll (${rr} left)</button>` : ""}
            <span id="wf3dNote" class="wf-sub" style="margin:0">${running ? `<span class="wf-spinner"></span> generating… ${d && d.progress != null ? d.progress + "%" : ""}` : ""}</span>
          </div>
          <div id="wf3dHost" class="wf-3dhost" ${MESHY.draft_tid && done ? "" : 'style="display:none"'}></div>
        </div>
      </div>`;
  }

  function section4() {
    const d = TASK.draft, t = TASK.texture;
    const draftDone = d && d.status === "SUCCEEDED";
    const running = MESHY.texture_tid && t && (t.status === "PENDING" || t.status === "IN_PROGRESS");
    const done = t && t.status === "SUCCEEDED";
    return `
      <div class="wf-section ${draftDone ? "" : "disabled"}">
        <div class="wf-shead"><span class="num">4</span>Texture
          ${done ? chip("done", "textured + downloaded") : running ? chip("busy", (t && t.progress != null ? t.progress + "%" : "…")) : chip("empty")}</div>
        <div class="wf-body">
          <div class="wf-sub" style="margin-bottom:6px">Leave the prompt empty to texture from
            your selected art image (recommended). Typing a prompt switches Meshy to text-guided
            texturing instead — they're mutually exclusive.</div>
          <textarea id="wfTexPrompt" class="wf-panel-full" style="min-height:44px"
            placeholder="Optional text override — empty = guided by the art image"></textarea>
          <div class="wf-gen">
            <button id="wfTex" class="gold" ${running || BUSY ? "disabled" : ""}>
              ${MESHY.texture_tid ? "⟳ Re-texture" : "🎨 Generate texture"}</button>
            <span id="wfTexNote" class="wf-sub" style="margin:0">${running ? `<span class="wf-spinner"></span> texturing… ${t && t.progress != null ? t.progress + "%" : ""}` : done ? "GLB ready — preview above shows the textured model" : ""}</span>
          </div>
        </div>
      </div>`;
  }

  function section5() {
    const it = ITEM, t = TASK.texture;
    const texDone = t && t.status === "SUCCEEDED";
    const alt = MESHY.alt_id;
    const isActive = alt && it.active === alt;
    const paired = it.is_glove || it.is_boot;
    const bootPairRow = it.is_boot ? `
      <div class="wf-row"><label title="Mirror the model into a left+right pair. Proven 2026-07-22: the 2-image draft also yields ONE boot (Meshy reads the two images as two views of one object), so mirroring is right for both modes">Pair</label>
        <label style="width:auto"><input type="checkbox" id="wfMirror" checked>
          mirror into a pair</label></div>` : "";
    const engineRow = `
      <div class="wf-row"><label title="Browser = instant three.js capture (same camera/light rig as Blender, no install needed). Blender = Cycles final quality with real mirrored-geometry shadows.">Engine</label>
        <label style="width:auto"><input type="radio" name="wfEngine" value="browser"
          ${!STATE.blender || MESHY.engine === "browser" ? "checked" : ""}> browser (fast)</label>
        <label style="width:auto"><input type="radio" name="wfEngine" value="blender"
          ${STATE.blender && MESHY.engine !== "browser" ? "checked" : ""} ${STATE.blender ? "" : "disabled"}>
          Blender${STATE.blender ? "" : " (not installed)"}</label></div>`;
    const shipBtn = paired
      ? `<button id="wfShip" class="gold" ${BUSY ? "disabled" : ""}
           title="Render the pair, build the DC6 at the original art size">🎬 Render pair → DC6</button>`
      : `<button id="wfShip" class="gold" ${BUSY ? "disabled" : ""}
           title="Render at this angle, build the DC6 at the original art size">🎬 Render → DC6</button>`;
    const angles = it.is_glove ? "" : `
      <div class="wf-row"><label>Angle</label>
        <input type="range" id="wfAzim" min="-90" max="90" step="1" value="25">
        <span class="val" id="wfAzimV">25</span></div>
      <div class="wf-row"><label>Elevation</label>
        <input type="range" id="wfElev" min="-30" max="60" step="1" value="15">
        <span class="val" id="wfElevV">15</span></div>`;
    return `
      <div class="wf-section ${texDone ? "" : "disabled"}">
        <div class="wf-shead"><span class="num">5</span>Final render & ship
          ${isActive ? chip("done", "ACTIVE") : alt ? chip("done", "saved") : chip("empty")}</div>
        <div class="wf-body">
          ${engineRow}
          ${angles}
          ${bootPairRow}
          <div class="wf-gen">${shipBtn}
            <span id="wfShipNote" class="wf-sub" style="margin:0"></span></div>
          ${alt ? `
          <div class="wf-src" style="margin-top:10px">
            <div class="thumb checker" title="original"><img src="/api/item/${enc(it.id)}/original.png"></div>
            <div class="thumb checker" title="new alternate (framed like the original)">
              <img src="/api/item/${enc(MESHY.alt_item || it.id)}/alt/${enc(alt)}.png?t=${Date.now()}"></div>
            <div class="wf-desc wf-sub" style="justify-content:center">
              saved as <b>${esc(alt)}</b>${isActive ? " · ACTIVE" : ""}<br>
              ${isActive
                ? `<span>use <b>Push to game</b> in the header to see it in game</span>`
                : `<button id="wfActivate" class="gold" style="margin-top:6px">✔ Activate</button>`}
            </div>
          </div>` : ""}
        </div>
      </div>`;
  }

  // ---------- render ----------

  function render() {
    snapGen();
    const p = ensurePanel();
    const it = ITEM, up = STATE.upscale || { variants: [], selected: null };
    const desc = STATE.description;
    const hasVariants = up.variants.length > 0;
    const sel = up.selected;
    p.innerHTML = `
      <div class="wf-head">
        <h2>${esc(it.name)}</h2>
        <button class="wf-close" id="wfClose" title="Back to item panel">✕</button>
      </div>
      <div class="wf-sub">${esc(it.code)} · ${it.cells[0]}×${it.cells[1]} cells · AI upscale → 3D</div>

      <div class="wf-section">
        <div class="wf-shead"><span class="num">1</span>Source ${chip("done")}</div>
        <div class="wf-body">
          <div class="wf-src">
            <div class="thumb checker"><img src="/api/item/${enc(it.id)}/original.png"></div>
            <div class="wf-desc wf-sub" style="justify-content:center">
              ${it.cells[0]}×${it.cells[1]} cells · the original inventory art.<br>
              Generate below: <b>Enhance</b> repaints this exact item, crisper; <b>Restyle</b>
              changes the look while keeping the shape.
            </div>
          </div>
        </div>
      </div>

      <div class="wf-section">
        <div class="wf-shead"><span class="num">2</span>Generate
          ${chip(hasVariants ? "done" : "empty")}</div>
        <div class="wf-body">
          <div class="wf-tabs">
            <button class="wf-tab ${GENTAB === "enhance" ? "active" : ""}" data-gentab="enhance"
              title="Qwen repaints the exact item in the house style, sharper — keeps shape, materials, colours">Enhance</button>
            <button class="wf-tab ${GENTAB === "restyle" ? "active" : ""}" data-gentab="restyle"
              title="Change the look (material/colour) while keeping the item's shape">Restyle</button>
          </div>

          <div class="wf-tabbody" data-gentab="enhance" ${GENTAB !== "enhance" ? 'style="display:none"' : ""}>
            <div class="wf-sub">Faithful clean-up — Qwen repaints the exact item in the house dark-fantasy style,
              crisper and detailed. Keeps shape, materials and colours. No prompt needed.</div>
            <div class="wf-row" style="align-items:flex-start;margin-top:8px"><label>Extra</label>
              <textarea id="wfPrompt" class="wf-panel-full" style="min-height:40px"
                placeholder="Optional: nudge the style — e.g. 'more ornate', 'brighter metal'.">${esc(GEN.prompt)}</textarea></div>
          </div>

          <div class="wf-tabbody" data-gentab="restyle" ${GENTAB !== "restyle" ? 'style="display:none"' : ""}>
            <div class="wf-row" style="align-items:flex-start"><label>New look</label>
              <textarea id="wfDesc" class="wf-panel-full" style="min-height:64px"
                placeholder="Describe the new look — e.g. 'molten fiery iron with glowing embers', 'royal gold with blue sapphires', 'frostbitten ice'.">${esc(GEN.restyle || "")}</textarea></div>
            <div class="wf-row" style="margin-top:8px">
              <label title="Qwen edits the real sprite (best quality); Flux-lock uses a ControlNet outline to hold thin/complex shapes">Engine</label>
              <select id="wfEngine" class="wf-panel-full">
                <option value="qwen" ${GEN.engine !== "flux-lock" ? "selected" : ""}>Qwen edit — best quality (default)</option>
                <option value="flux-lock" ${GEN.engine === "flux-lock" ? "selected" : ""}>Flux + ControlNet — thin shapes (sword/staff)</option>
              </select></div>
            <div class="wf-row" style="margin-top:8px" data-eng="flux-lock">
              <label title="ControlNet strength. Higher locks the original outline harder while the prompt restyles the material/colour.">Shape lock</label>
              <input type="range" id="wfShape" min="0.2" max="1" step="0.05" value="${GEN.shape}">
              <span class="val" id="wfShapeV">${GEN.shape.toFixed(2)}</span></div>
            <div class="wf-row" style="margin-top:8px" data-eng="flux-lock">
              <label title="Flux sampling steps. 4 is fast; higher adds refinement, slower.">Steps</label>
              <input type="range" id="wfSteps" min="4" max="8" step="1" value="${GEN.steps}">
              <span class="val" id="wfStepsV">${GEN.steps}</span></div>
            <div class="wf-sub" data-eng="qwen" style="margin:6px 0 0">Qwen edits the real sprite — richest detail, stays true to the shape.</div>
          </div>

          <div class="wf-row" data-eng="qwen" style="margin-top:8px">
            <label title="4x-UltraSharp pre-upscale before Qwen. On (default) = crisp, smooth silhouette. Off = plain LANCZOS backup: try it if the GAN result looks wrong for this item.">Sharp edges</label>
            <label style="width:auto;display:flex;align-items:center;gap:6px">
              <input type="checkbox" id="wfGan" ${GEN.gan ? "checked" : ""}>
              <span class="wf-sub" style="margin:0">GAN pre-upscale (default; uncheck for the plain backup)</span></label>
          </div>
          <details><summary class="wf-sub">Advanced: negative prompt</summary>
            <textarea id="wfNeg" class="wf-panel-full" style="min-height:40px;margin-top:6px">${esc(GEN.neg)}</textarea></details>
          <div class="wf-gen">
            <label class="wf-sub" style="margin:0">seed</label>
            <input type="number" id="wfSeed" class="seedbox wf-panel-full" value="${randSeed()}">
            <button id="wfDice" title="Randomize seed">🎲</button>
            <button id="wfGen" class="gold" title="Generate one image with the active tab's settings">✨ Generate</button>
            <button id="wfGenCfg" title="View / edit the model, style prompt and every parameter used to generate images">⚙ Settings</button>
            <span id="wfGenNote" class="wf-sub" style="margin:0"></span>
          </div>
          <div class="wf-genset hidden" id="wfGenSet"></div>
          <div class="wf-strip" id="wfStrip">${stripHTML(up)}</div>
          <div class="wf-selbig checker" id="wfSelBig">${sel ? `<img src="/api/upscale/${enc(it.id)}/variant/${sel}.png?master=1&t=${Date.now()}" title="selected variant ${sel} — full resolution">` : ""}</div>
          <div class="wf-prov" id="wfProv">${provenanceHTML((up.variants || []).find((v) => v.id === sel))}</div>
          <div class="wf-2d" id="wf2dWrap" ${sel ? "" : 'style="display:none"'}>
            <div class="wf-gen" style="margin-top:2px">
              <button id="wfAccept2d" class="gold"
                title="Skip 3D — ship the selected image straight to this item's inventory art. Background is already cut; you confirm a fitted preview first.">✓ Accept image as item art → DC6</button>
              <span class="wf-sub" style="margin:0">No 3D model needed — the selected §2 image becomes the item's art.</span>
            </div>
            <div class="wf-2dconfirm checker hidden" id="wf2dConfirm"></div>
          </div>
        </div>
      </div>

      ${section3()}
      ${section4()}
      ${section5()}
    `;
    wire();
    p.classList.add("open");
    mountViewerIfReady();
  }

  function stripHTML(up) {
    if (!up.variants.length) return `<div class="wf-sub" style="padding:6px">Nothing generated yet — press Generate.</div>`;
    return up.variants.map((v) => {
      const isDesc = v.mode === "desc";
      const badge = isDesc ? "✎" : "↑";
      const badgeTitle = isDesc ? `from description · ref ${fmt(v.ref_strength)}` : "direct upscale";
      const capBits = isDesc ? `ref ${fmt(v.ref_strength)}` : `crea ${fmt(v.creativity)}`;
      return `
      <div class="wf-thumb ${v.id === up.selected ? "sel" : ""}" data-vid="${v.id}">
        <div class="t"><img src="/api/upscale/${enc(ITEM.id)}/variant/${v.id}.png?t=${v.ts || 0}"></div>
        <span class="badge" title="${badgeTitle}">${badge}</span>
        <button class="x" data-del="${v.id}" title="Delete this variant">✕</button>
        <div class="cap" title="seed ${v.seed} · ${badgeTitle}">${v.id} · ${capBits}</div>
      </div>`;
    }).join("");
  }

  // Provenance: EXACTLY what produced the selected image. New variants carry a full snapshot
  // (model/steps/gan/px + the assembled instruction, style and negative); older ones show what
  // was recorded at the time.
  function provenanceHTML(v) {
    if (!v) return "";
    const row = (k, val) => val === undefined || val === "" || val === null ? "" :
      `<div class="wf-provrow"><span class="wf-provk">${k}</span><span class="wf-provv">${esc(val)}</span></div>`;
    const params = [
      v.model ? `model ${v.model}` : "", v.steps ? `${v.steps} steps` : "",
      (v.gan !== undefined) ? (v.gan ? "GAN 4×" : "LANCZOS") : "", v.px ? `${v.px}px` : "",
      `seed ${v.seed}`, v.preset ? v.preset : "", v.engine ? v.engine : "",
    ].filter(Boolean).join(" · ");
    const full = v.instruction || (v.prompt || "");
    return `
      <details class="wf-provbox">
        <summary>ⓘ How ${esc(v.id)} was generated</summary>
        ${row("params", params)}
        ${row("your prompt", v.prompt)}
        ${v.style ? row("style", v.style) : ""}
        ${full ? row("full instruction", full) : ""}
        ${v.negative ? row("negative", v.negative) : ""}
        ${!v.instruction && !v.style ? `<div class="wf-sub" style="padding:4px 0">(generated before provenance capture — only basic params recorded)</div>` : ""}
      </details>`;
  }

  // ---------- generation settings (transparent + editable) ----------
  const NUMF = { steps: 1, px: 1, accept_brightness: 0.05 };
  async function openGenSettings() {
    const box = $("#wfGenSet");
    if (!box.classList.contains("hidden")) { box.classList.add("hidden"); box.innerHTML = ""; return; }
    const r = await api("GET", "/api/gen/settings");
    const s = r.settings, d = r.defaults;
    const ta = (k, rows, hint) =>
      `<label class="wf-sub">${k}${hint ? ` — <span style="opacity:.7">${hint}</span>` : ""}</label>
       <textarea class="wf-panel-full" data-gk="${k}" rows="${rows}">${esc(s[k])}</textarea>`;
    const num = (k, hint) =>
      `<label class="wf-sub">${k}${hint ? ` — <span style="opacity:.7">${hint}</span>` : ""}</label>
       <input type="number" step="${NUMF[k] || 1}" class="seedbox" data-gk="${k}" value="${s[k]}">`;
    box.innerHTML = `
      <div class="wf-setgrid">
        ${ta("style", 3, "the house look prepended to every image — remove 'dark/gritty/muted' words to brighten")}
        ${ta("enhance_tail", 2, "appended on the Enhance tab")}
        ${ta("restyle_tail", 2, "appended on the Restyle tab")}
        ${ta("negative", 2, "what to avoid")}
        <div class="wf-setnums">
          ${num("steps", "sampler steps")}
          ${num("px", "resolution")}
          ${num("accept_brightness", "1.0=off; >1 brightens at accept (fixes existing art, no re-gen)")}
          <label class="wf-sub">gan<input type="checkbox" data-gk="gan" ${s.gan ? "checked" : ""}></label>
          <label class="wf-sub">model</label>
          <input type="text" class="wf-panel-full" data-gk="model" value="${esc(s.model)}">
        </div>
        <div class="wf-setprev"><b>Enhance sends:</b> <span id="wfSetPrevE">${esc(r.preview.enhance)}</span></div>
        <div class="wf-gen">
          <button id="wfSetSave" class="gold">Save</button>
          <button id="wfSetReset" title="Back to the shipped defaults">Reset to defaults</button>
          <span id="wfSetNote" class="wf-sub"></span>
        </div>
      </div>`;
    box.classList.remove("hidden");
    const collect = () => {
      const o = {};
      box.querySelectorAll("[data-gk]").forEach((el) => {
        o[el.dataset.gk] = el.type === "checkbox" ? el.checked
          : (el.type === "number" ? parseFloat(el.value) : el.value);
      });
      return o;
    };
    box.querySelector("#wfSetSave").onclick = async () => {
      const rr = await api("PUT", "/api/gen/settings", collect());
      $("#wfSetPrevE").textContent = rr.preview.enhance;
      $("#wfSetNote").textContent = "saved — applies to the next image";
    };
    box.querySelector("#wfSetReset").onclick = async () => {
      await api("POST", "/api/gen/settings/reset", {});
      $("#wfGenSet").innerHTML = ""; $("#wfGenSet").classList.add("hidden"); openGenSettings();
    };
  }

  // ---------- wiring ----------

  function wire() {
    $("#wfClose").onclick = close;
    document.querySelectorAll(".wf-tab").forEach((t) => {
      t.onclick = () => {
        GENTAB = t.dataset.gentab;
        document.querySelectorAll(".wf-tab").forEach((x) => x.classList.toggle("active", x === t));
        document.querySelectorAll(".wf-tabbody").forEach((b) =>
          (b.style.display = b.dataset.gentab === GENTAB ? "" : "none"));
        toggleEngine();  // enhance is always qwen; keep the qwen-only controls (Sharp edges) in sync
      };
    });
    const steps = $("#wfSteps"); if (steps) steps.oninput = () => ($("#wfStepsV").textContent = steps.value);
    const shape = $("#wfShape"); if (shape) shape.oninput = () => ($("#wfShapeV").textContent = (+shape.value).toFixed(2));
    const eng = $("#wfEngine");
    const toggleEngine = () => {
      // the enhance tab has no engine selector and is always qwen; only restyle can pick flux-lock
      const e = GENTAB === "enhance" ? "qwen" : (eng ? eng.value : "qwen");
      document.querySelectorAll("[data-eng]").forEach((el) =>
        (el.style.display = el.dataset.eng.split(" ").includes(e) ? "" : "none"));
    };
    if (eng) eng.onchange = toggleEngine;
    toggleEngine();
    $("#wfDice").onclick = () => ($("#wfSeed").value = randSeed());
    const cap = $("#wfCaption"); if (cap) cap.onclick = caption;
    const dsave = $("#wfDescSave"); if (dsave) dsave.onclick = saveDesc;
    $("#wfGen").onclick = generate;
    { const c = $("#wfGenCfg"); if (c) c.onclick = openGenSettings; }
    $("#wfStrip").querySelectorAll(".wf-thumb .t").forEach((t) => {
      t.onclick = () => selectVariant(t.parentElement.dataset.vid);
    });
    $("#wfStrip").querySelectorAll("[data-del]").forEach((b) => {
      b.onclick = (e) => { e.stopPropagation(); delVariant(b.dataset.del); };
    });
    const a2 = $("#wfAccept2d"); if (a2) a2.onclick = accept2dPreview;
    const g3 = $("#wfGen3d"); if (g3) g3.onclick = gen3d;
    const rr = $("#wfReroll"); if (rr) rr.onclick = reroll3d;
    const sp = $("#wfSplit"); if (sp) sp.onclick = splitBoots;
    document.querySelectorAll("input[name=wfBootsMode]").forEach((r) => {
      r.onchange = () => {
        MESHY.boots_mode = r.value;
        api("PUT", `/api/upscale/${enc(ITEM.id)}/meshy`, { boots_mode: r.value });
      };
    });
    const tx = $("#wfTex"); if (tx) tx.onclick = genTexture;
    const az = $("#wfAzim"), el = $("#wfElev");
    if (az) az.oninput = () => ($("#wfAzimV").textContent = az.value);
    if (el) el.oninput = () => ($("#wfElevV").textContent = el.value);
    const sh = $("#wfShip"); if (sh) sh.onclick = ship;
    const act = $("#wfActivate"); if (act) act.onclick = activateAlt;
  }

  // ---------- section 2 actions ----------

  async function reloadState() {
    const r = await api("GET", `/api/upscale/${enc(ITEM.id)}/state`);
    if (r.ok) { STATE = r; ITEM = r.item; MESHY = r.meshy || {}; }
    return r;
  }

  async function caption() {
    if ($("#wfCapNote")) $("#wfCapNote").innerHTML = `<span class="wf-spinner"></span> captioning…`;
    await api("POST", `/api/describe/${enc(ITEM.id)}?force=1`);
    startCapPoll();
  }
  function startCapPollIfNeeded() { if (STATE.caption_status === "running") startCapPoll(); }
  function startCapPoll() {
    clearInterval(capPoll);
    capPoll = setInterval(async () => {
      const r = await api("GET", `/api/upscale/${enc(ITEM.id)}/state`);
      if (!r.ok) return;
      if (r.caption_status === "running") return;
      clearInterval(capPoll); capPoll = null;
      STATE = r; ITEM = r.item; MESHY = r.meshy || {};
      if (String(r.caption_status || "").startsWith("error")) {
        $("#wfCapNote") && ($("#wfCapNote").textContent = r.caption_status);
      } else { render(); }
    }, 3000);
  }

  async function saveDesc() {
    const text = $("#wfDesc").value;
    const r = await api("PUT", `/api/describe/${enc(ITEM.id)}`, { text });
    if (r.ok) { STATE.description = r.description; $("#wfCapNote").textContent = "saved"; }
  }

  async function generate() {
    const btn = $("#wfGen"); btn.disabled = true;
    $("#wfGenNote").innerHTML = `<span class="wf-spinner"></span> generating…`;
    let body;
    if (GENTAB === "restyle") {
      const text = $("#wfDesc").value.trim();
      if (!text) {
        $("#wfGenNote").textContent = "describe the new look first";
        btn.disabled = false;
        return;
      }
      body = { mode: "desc", preset: "restyle", prompt: text, engine: $("#wfEngine").value,
               shape_strength: +$("#wfShape").value, steps: +$("#wfSteps").value,
               gan: $("#wfGan") ? $("#wfGan").checked : true, seed: +$("#wfSeed").value };
    } else {  // enhance
      body = { mode: "desc", preset: "enhance", engine: "qwen",
               gan: $("#wfGan") ? $("#wfGan").checked : true,
               prompt: $("#wfPrompt").value.trim(), seed: +$("#wfSeed").value };
    }
    try {
      const r = await api("POST", `/api/upscale/${enc(ITEM.id)}/generate`, body);
      if (r.ok) { STATE.upscale = r.upscale; render(); }
      else { $("#wfGenNote").textContent = "error: " + (r.error || "failed"); btn.disabled = false; }
    } catch (e) { $("#wfGenNote").textContent = "error: " + e; btn.disabled = false; }
  }

  async function selectVariant(vid) {
    const r = await api("POST", `/api/upscale/${enc(ITEM.id)}/select`, { vid });
    if (r.ok) { STATE.upscale = r.upscale; render(); }
  }

  // ---------- accept a §2 image straight to item art (skip 3D) ----------
  // Kept entirely inside §2 so it never touches MESHY / lights up the 3D "Final render & ship"
  // section. Shows the true fitted DC6 preview, then commits as a new alternate on confirm.
  function accept2dPreview() {
    const sel = (STATE.upscale || {}).selected;
    if (!sel) return;
    const box = $("#wf2dConfirm");
    if (!box) return;
    box.classList.remove("hidden");
    box.innerHTML = `
      <div class="wf-sub" style="margin-bottom:6px">Exactly how the art ships in-game —
        ${ITEM.cells[0]}×${ITEM.cells[1]} cells, palette + framing applied. Accept?</div>
      <div class="wf-2dprev checker"><img src="/api/upscale/${enc(ITEM.id)}/accept-2d/preview.png?vid=${enc(sel)}&t=${Date.now()}"></div>
      <div class="wf-gen" style="margin-top:8px">
        <button id="wf2dGo" class="gold">✓ Accept → DC6</button>
        <button id="wf2dCancel">Cancel</button>
        <span id="wf2dNote" class="wf-sub" style="margin:0"></span>
      </div>`;
    $("#wf2dCancel").onclick = () => { box.classList.add("hidden"); box.innerHTML = ""; };
    $("#wf2dGo").onclick = () => accept2dCommit(sel);
  }

  async function accept2dCommit(vid) {
    const note = $("#wf2dNote");
    if (note) note.innerHTML = `<span class="wf-spinner"></span> saving…`;
    const r = await api("POST", `/api/upscale/${enc(ITEM.id)}/accept-2d`, { vid });
    if (!r.ok) { if (note) note.textContent = "error: " + (r.error || "failed"); return; }
    // Landed as a new alternate (not auto-activated) — same manual-Activate pattern as the 3D ship.
    const box = $("#wf2dConfirm");
    box.innerHTML = `
      <div class="wf-src">
        <div class="thumb checker" title="original"><img src="/api/item/${enc(r.item_id)}/original.png"></div>
        <div class="thumb checker" title="new alternate">
          <img src="/api/item/${enc(r.item_id)}/alt/${enc(r.alt_id)}.png?t=${Date.now()}"></div>
        <div class="wf-desc wf-sub" style="justify-content:center">
          saved as <b>${esc(r.alt_id)}</b><br>
          <button id="wf2dActivate" class="gold" style="margin-top:6px">✔ Activate</button>
        </div>
      </div>`;
    $("#wf2dActivate").onclick = async () => {
      const a = await api("POST", `/api/item/${enc(r.item_id)}/activate`, { choice: r.alt_id });
      if (a.ok) {
        ITEM.active = r.alt_id;
        $("#wf2dActivate").outerHTML = `<span class="wf-sub">✔ active — use <b>Push to game</b> to see it</span>`;
        if (window.refreshView) window.refreshView();
      }
    };
    if (window.refreshView) window.refreshView();  // the item panel/gallery behind lists alternates
  }
  async function delVariant(vid) {
    const r = await api("DELETE", `/api/upscale/${enc(ITEM.id)}/variant/${vid}`);
    if (r.ok) { STATE.upscale = r.upscale; render(); }
  }

  // ---------- section 3: Meshy draft ----------

  async function splitBoots() {
    const note = $("#wfSplitNote");
    note.innerHTML = `<span class="wf-spinner"></span> splitting…`;
    const r = await api("POST", `/api/boots/split/${enc(ITEM.id)}`, {});
    if (!r.ok) { note.textContent = "split failed: " + (r.error || "?") + " — mask by hand in the Studio"; return; }
    STATE.boots = r.boots;
    render();
  }

  async function gen3d() {
    BUSY = "gen3d";
    const note = $("#wf3dNote");
    note.innerHTML = `<span class="wf-spinner"></span> creating draft…`;
    try {
      let r;
      if (ITEM.is_glove) {
        r = await api("POST", `/api/studio/generate-masked`, { item_id: ITEM.id, opts: {} });
        if (r.ok) await api("PUT", `/api/upscale/${enc(ITEM.id)}/meshy`,
                            { draft_tid: r.task_id, phase: "draft" });
      } else {
        const body = { item_id: ITEM.id, opts: {} };
        if (ITEM.is_boot) {
          const mode = document.querySelector("input[name=wfBootsMode]:checked");
          body.boots_mode = mode ? mode.value : "multi";
        }
        r = await api("POST", `/api/studio/generate-upscale`, body);
        if (r.ok && r.boots) STATE.boots = r.boots;
      }
      if (!r.ok) { note.textContent = "error: " + (r.error || "failed"); BUSY = null; return; }
      MESHY = { ...MESHY, draft_tid: r.task_id, phase: "draft", texture_tid: null, alt_id: null,
                ...(ITEM.is_boot ? { boots_mode: (document.querySelector("input[name=wfBootsMode]:checked") || {}).value || "multi" } : {}) };
      TASK = { draft: { status: "PENDING", progress: 0, rerollsLeft: 8 }, texture: null };
      BUSY = null;
      render();
      startTaskPoll();
    } catch (e) { note.textContent = "error: " + e; BUSY = null; }
  }

  async function reroll3d() {
    BUSY = "gen3d";
    $("#wf3dNote").innerHTML = `<span class="wf-spinner"></span> re-rolling…`;
    try {
      const r = await api("POST", `/api/studio/reroll`, { task_id: MESHY.draft_tid });
      if (!r.ok) { $("#wf3dNote").textContent = "error: " + (r.error || "failed"); BUSY = null; return; }
      await api("PUT", `/api/upscale/${enc(ITEM.id)}/meshy`, { draft_tid: r.task_id, phase: "draft" });
      MESHY = { ...MESHY, draft_tid: r.task_id, texture_tid: null, alt_id: null };
      TASK = { draft: { status: "PENDING", progress: 0 }, texture: null };
      BUSY = null;
      render();
      startTaskPoll();
    } catch (e) { $("#wf3dNote").textContent = "error: " + e; BUSY = null; }
  }

  // ---------- section 4: texture ----------

  async function genTexture() {
    BUSY = "texture";
    $("#wfTexNote").innerHTML = `<span class="wf-spinner"></span> creating texture task…`;
    try {
      const r = await api("POST", `/api/studio/texture`, {
        task_id: MESHY.draft_tid,
        opts: { prompt: $("#wfTexPrompt").value, artStyle: "realistic", enablePBR: true },
      });
      if (!r.ok) { $("#wfTexNote").textContent = "error: " + (r.error || "failed"); BUSY = null; return; }
      await api("PUT", `/api/upscale/${enc(ITEM.id)}/meshy`, { texture_tid: r.task_id, phase: "texture" });
      MESHY = { ...MESHY, texture_tid: r.task_id, phase: "texture", alt_id: null };
      TASK.texture = { status: "PENDING", progress: 0 };
      BUSY = null;
      render();
      startTaskPoll();
    } catch (e) { $("#wfTexNote").textContent = "error: " + e; BUSY = null; }
  }

  // ---------- section 5: render & ship ----------

  async function ship() {
    BUSY = "ship";
    const note = $("#wfShipNote");
    const engineSel = document.querySelector("input[name=wfEngine]:checked");
    const engine = engineSel ? engineSel.value : (STATE.blender ? "blender" : "browser");
    MESHY.engine = engine;
    note.innerHTML = engine === "browser"
      ? `<span class="wf-spinner"></span> rendering in browser…`
      : `<span class="wf-spinner"></span> downloading GLB + Blender rendering… (1-4 min)`;
    try {
      let r;
      const tid = MESHY.texture_tid || MESHY.draft_tid;
      const mirror = ITEM.is_glove || (ITEM.is_boot && $("#wfMirror") && $("#wfMirror").checked);
      const azim = +($("#wfAzim") ? $("#wfAzim").value : 25);
      const elev = +($("#wfElev") ? $("#wfElev").value : 15);
      if (engine === "browser") {
        // three.js capture with the Blender-matched camera/light rig (pair3d.js) — no Blender
        const K = 8;
        const w = ITEM.cells[0] * 29 * K, h = ITEM.cells[1] * 29 * K;
        const png = await window.WF3D.captureSprite({
          glbUrl: `/api/studio/glb/${tid}.glb`, single: !mirror,
          frame: { azim: ITEM.is_glove ? 0 : azim, elev: ITEM.is_glove ? 0 : elev },
          w, h,
        });
        r = mirror
          ? await api("POST", `/api/pair/build3d`, {
              invfile: ITEM.invfile, png, engine: "browser", activate: false,
              pose: { azim, elev } })
          : await api("POST", `/api/pair/build-single`, {
              task_id: tid, azim, elev, fill: 0.94, png, activate: false });
      } else if (mirror) {
        const pose = ITEM.is_boot ? { azim, elev } : undefined;  // gloves use their template pose
        r = await api("POST", `/api/pair/build3d/blender`,
                      { invfile: ITEM.invfile, task_id: tid, activate: false,
                        ...(pose ? { pose } : {}) });
      } else {
        r = await api("POST", `/api/studio/accept`, {
          task_id: tid, azim, elev, fill: 0.94, dx: 0, dy: 0, activate: false,
        });
      }
      if (!r.ok) { note.textContent = "error: " + (r.error || "failed"); BUSY = null; return; }
      // pair alternates land on the item that OWNS the invfile (art files are shared across an
      // item family: Battle Boots xtb art lives on tbt's invtbt) — track it for preview/activate
      const owner = r.item_id || ITEM.id;
      await api("PUT", `/api/upscale/${enc(ITEM.id)}/meshy`,
                { alt_id: r.alt_id, phase: "shipped", alt_item: owner, engine });
      MESHY = { ...MESHY, alt_id: r.alt_id, alt_item: owner, phase: "shipped", engine };
      BUSY = null;
      await reloadState();  // pick up the fresh alt list
      MESHY = { ...MESHY, alt_id: r.alt_id, alt_item: owner, engine };
      render();
      // the gallery + detail rail behind the panel also list alternates — refresh them so a
      // newly saved/updated alt shows up without a manual page reload
      if (window.refreshView) window.refreshView();
    } catch (e) { note.textContent = "error: " + e; BUSY = null; }
  }

  async function activateAlt() {
    const owner = MESHY.alt_item || ITEM.id;
    const r = await api("POST", `/api/item/${enc(owner)}/activate`, { choice: MESHY.alt_id });
    if (r.ok) {
      ITEM.active = MESHY.alt_id;
      render();
      if (window.refreshView) window.refreshView();  // let the gallery reflect the change
    }
  }

  // ---------- task polling + viewer ----------

  function startTaskPoll() {
    clearInterval(taskPoll);
    taskPoll = setInterval(pollTasks, 5000);
    pollTasks();
  }

  async function pollTasks() {
    const jobs = [];
    if (MESHY.draft_tid && (!TASK.draft || ["PENDING", "IN_PROGRESS"].includes(TASK.draft.status)))
      jobs.push(["draft", MESHY.draft_tid]);
    if (MESHY.texture_tid && (!TASK.texture || ["PENDING", "IN_PROGRESS"].includes(TASK.texture.status)))
      jobs.push(["texture", MESHY.texture_tid]);
    if (!jobs.length) { clearInterval(taskPoll); taskPoll = null; return; }
    for (const [kind, tid] of jobs) {
      try {
        const r = await api("GET", `/api/studio/task/${tid}`);
        if (!r.ok) continue;
        const prev = TASK[kind];
        TASK[kind] = r;
        const transitioned = !prev || prev.status !== r.status;
        if (transitioned) render();
        else {
          const el = kind === "draft" ? $("#wf3dNote") : $("#wfTexNote");
          if (el && ["PENDING", "IN_PROGRESS"].includes(r.status))
            el.innerHTML = `<span class="wf-spinner"></span> ${kind === "draft" ? "generating" : "texturing"}… ${r.progress != null ? r.progress + "%" : ""}`;
        }
      } catch (e) { /* transient poll error — keep trying */ }
    }
  }

  function mountViewerIfReady() {
    const host = $("#wf3dHost");
    if (!host || !window.WF3D) return;
    const d = TASK.draft, t = TASK.texture;
    const tid = (t && t.status === "SUCCEEDED" && t.hasGlb) ? MESHY.texture_tid
      : (d && d.status === "SUCCEEDED" && d.hasGlb) ? MESHY.draft_tid : null;
    if (!tid) return;
    host.style.display = "";
    window.WF3D.mount(host);
    if (host.dataset.loaded !== tid) {
      host.dataset.loaded = tid;
      window.WF3D.load(`/api/studio/glb/${tid}.glb`).catch(() => { host.dataset.loaded = ""; });
    }
  }

  // ---------- open/close ----------

  function close() {
    const p = $("#workflow");
    if (p) p.classList.remove("open");
    clearInterval(capPoll); capPoll = null;
    clearInterval(taskPoll); taskPoll = null;
    if (window.WF3D) window.WF3D.unmount();
  }

  window.openWorkflow = async function (item) {
    ITEM = { id: item.id, name: item.name, code: item.code,
             cells: [item.invwidth, item.invheight], type: item.type };
    ensurePanel();
    const r = await reloadState();
    if (!r.ok) { alert("workflow: " + (r.error || "failed to load")); return; }
    TASK = { draft: null, texture: null };
    render();
    startCapPollIfNeeded();
    if (MESHY.draft_tid || MESHY.texture_tid) startTaskPoll();
  };
})();
