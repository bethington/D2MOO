/* Pairing review — one filmstrip row per unlinked Meshy task: what you fed Meshy on
   the left, candidate item sprites as radio cards on the right, strongest -> weakest. */
const $ = (s) => document.querySelector(s);
let SUGS = [];
const PICKS = new Map();   // task_id -> {item_id, why}
const DONE = new Set();    // task_ids linked this session

function toast(msg, bad) {
  const t = $("#toast");
  t.textContent = msg; t.className = "toast" + (bad ? " bad" : "");
  setTimeout(() => (t.className = "toast hidden"), 3800);
}

let ALL_ITEMS = [];  // for the per-row "search all items" escape hatch

const spriteUrl = (id) => `/api/item/${encodeURIComponent(id)}/original.png`;
const esc = (s) => String(s).replace(/[&<>"]/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]));

async function loadAllItems() {
  try {
    const d = await (await fetch("/api/items")).json();
    ALL_ITEMS = d.items || [];
  } catch (e) { ALL_ITEMS = []; }
}

async function scan() {
  $("#scanState").textContent = "scanning…";
  $("#rows").innerHTML = '<div class="empty">Scanning your Meshy workspace…</div>';
  let d;
  try {
    d = await (await fetch("/api/meshy/links/scan", {
      method: "POST", headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ pages: 2 }),
    })).json();
  } catch (e) { d = { ok: false, error: String(e) }; }
  if (!d.ok) {
    $("#scanState").textContent = "scan failed";
    $("#rows").innerHTML = `<div class="empty">Scan failed: ${esc(d.error || "")}<br>
      Is the Meshy session logged in? Check the Studio page.</div>`;
    return;
  }
  SUGS = d.suggestions || [];
  $("#scanState").textContent =
    `${d.scanned} tasks scanned · ${(d.auto || []).length} auto-linked (exact image match)`;
  $("#counts").textContent =
    `${SUGS.length} need review · ${(d.unmatched || []).length} had no plausible candidate`;
  render();
}

function render() {
  const hide = $("#hideDone").checked;
  const rows = $("#rows");
  const list = SUGS.filter((s) => !(hide && DONE.has(s.task_id)));
  if (!list.length) {
    rows.innerHTML = `<div class="empty">${SUGS.length ? "All reviewed 🎉" : "Nothing needs review."}</div>`;
    return updateFoot();
  }
  rows.innerHTML = list.map((s) => {
    const title = esc(s.task_name || s.task_id.slice(0, 8));
    const cands = s.candidates.map((c, i) => {
      const id = `c_${s.task_id}_${i}`;
      const picked = PICKS.get(s.task_id)?.invfile === c.invfile;
      const also = c.item_count > 1
        ? `<div class="also" title="${esc(c.items.join(", "))}${c.item_count > 8 ? ", …" : ""}">${c.item_count} items: ${esc(c.items.slice(0, 2).join(", "))}${c.item_count > 2 ? "…" : ""}</div>`
        : `<div class="also">${esc(c.item_name)}</div>`;
      return `<div class="cand${i === 0 ? " best" : ""}">
        <input type="radio" name="t_${s.task_id}" id="${id}" value="${esc(c.item_id)}"
               data-task="${esc(s.task_id)}" data-why="${esc(c.why)}" data-invfile="${esc(c.invfile)}"
               ${picked ? "checked" : ""}>
        <label for="${id}">
          <div class="ph checker"><img loading="lazy" src="${spriteUrl(c.item_id)}"
               onerror="this.style.opacity=.15"></div>
          <div class="nm" title="${esc(c.invfile)}.dc6">${esc(c.invfile)}.dc6</div>
          <div class="why">${esc(c.why)}</div>
          ${also}
        </label></div>`;
    }).join("");
    const noneId = `c_${s.task_id}_none`;
    const warn = s.current_item_id
      ? `<span class="warn">was linked to ${esc(s.current_invfile ? s.current_invfile + ".dc6" : s.current_item_name || "?")} (${esc(s.current_source || "")}) — confirm or change</span>`
      : "";
    return `<div class="prow${DONE.has(s.task_id) ? " done" : ""}${s.current_item_id ? " needsconfirm" : ""}" data-row="${esc(s.task_id)}">
      <div class="head">
        <span class="tname">${title}</span>
        <span class="tmeta">${s.best_distance != null ? "closest image d" + s.best_distance : "name match only"}
          · ${s.candidates.length} options</span>
        ${warn}
        <span class="rowact">
          <button data-link="${esc(s.task_id)}">Link this</button>
        </span>
      </div>
      <div class="body">
        <div class="anchor">
          ${s.input_image ? `<figure><img src="${esc(s.input_image)}" class="checker">
            <figcaption>you fed Meshy</figcaption></figure>` : ""}
          ${s.preview ? `<figure><img src="${esc(s.preview)}" class="checker">
            <figcaption>it generated</figcaption></figure>` : ""}
        </div>
        <div class="cands" data-cands="${esc(s.task_id)}">${cands}
          <div class="cand none">
            <input type="radio" name="t_${s.task_id}" id="${noneId}" value=""
                   data-task="${esc(s.task_id)}" data-why="none">
            <label for="${noneId}">none of these</label>
          </div>
          <div class="searchcell">
            <input type="search" class="itemsearch" data-search="${esc(s.task_id)}"
                   placeholder="none right? search all items…" spellcheck="false">
            <div class="hits" data-hits="${esc(s.task_id)}"></div>
          </div>
        </div>
      </div></div>`;
  }).join("");

  rows.querySelectorAll("input[type=radio]").forEach((r) => {
    r.onchange = () => {
      if (r.value) PICKS.set(r.dataset.task, { item_id: r.value, why: r.dataset.why, invfile: r.dataset.invfile });
      else PICKS.delete(r.dataset.task);
      updateFoot();
    };
  });
  rows.querySelectorAll("[data-link]").forEach((b) => {
    b.onclick = () => linkOne(b.dataset.link, b);
  });
  rows.querySelectorAll("[data-search]").forEach((inp) => {
    inp.oninput = () => renderHits(inp.dataset.search, inp.value.trim());
  });
  updateFoot();
}

/* Search the whole catalog for a row — the auto-candidates often contain no correct
   answer (hand-uploaded art that was never a catalog sprite), so this is the way out. */
function renderHits(taskId, q) {
  const box = document.querySelector(`[data-hits="${CSS.escape(taskId)}"]`);
  if (!box) return;
  if (q.length < 2) { box.innerHTML = ""; return; }
  const ql = q.toLowerCase();
  const hits = ALL_ITEMS
    .filter((i) => i.name.toLowerCase().includes(ql) || i.code.toLowerCase().includes(ql))
    .slice(0, 8);
  if (!hits.length) { box.innerHTML = "<div class='why'>no match</div>"; return; }
  box.innerHTML = hits.map((i, n) => {
    const id = `s_${taskId}_${n}`;
    return `<div class="cand">
      <input type="radio" name="t_${taskId}" id="${id}" value="${esc(i.id)}"
             data-task="${esc(taskId)}" data-why="searched" data-invfile="${esc((i.invfile||"").toLowerCase())}">
      <label for="${id}">
        <div class="ph checker"><img src="${spriteUrl(i.id)}" onerror="this.style.opacity=.15"></div>
        <div class="nm" title="${esc(i.name)}">${esc(i.name)}</div>
        <div class="why">${esc((i.invfile||"").toLowerCase())}.dc6</div>
      </label></div>`;
  }).join("");
  box.querySelectorAll("input[type=radio]").forEach((r) => {
    r.onchange = () => {
      PICKS.set(r.dataset.task, { item_id: r.value, why: r.dataset.why, invfile: r.dataset.invfile });
      updateFoot();
    };
  });
}

function updateFoot() {
  const n = [...PICKS.keys()].filter((t) => !DONE.has(t)).length;
  $("#linkAllBtn").disabled = !n;
  $("#pickCount").textContent = n ? `${n} picked, ready to link` : "nothing picked yet";
}

async function linkOne(taskId, btn) {
  const p = PICKS.get(taskId);
  if (!p) return toast("pick a sprite for this row first", true);
  if (btn) { btn.disabled = true; btn.textContent = "linking…"; }
  const r = await (await fetch("/api/meshy/links", {
    method: "POST", headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ task_id: taskId, item_id: p.item_id, invfile: p.invfile, source: `reviewed ${p.why}` }),
  })).json();
  if (!r.ok) {
    if (btn) { btn.disabled = false; btn.textContent = "Link this"; }
    return toast("link failed: " + (r.error || ""), true);
  }
  DONE.add(taskId);
  toast(`linked to ${r.item_name}`);
  const row = document.querySelector(`[data-row="${CSS.escape(taskId)}"]`);
  if (row) row.classList.add("done");
  if (btn) btn.textContent = "✓ linked";
  if ($("#hideDone").checked) render();
  updateFoot();
}

async function linkAll() {
  const picks = [...PICKS.entries()]
    .filter(([t]) => !DONE.has(t))
    .map(([task_id, p]) => ({ task_id, item_id: p.item_id, invfile: p.invfile, source: `reviewed ${p.why}` }));
  if (!picks.length) return;
  $("#linkAllBtn").disabled = true; $("#linkAllBtn").textContent = "linking…";
  const r = await (await fetch("/api/meshy/links/batch", {
    method: "POST", headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ picks }),
  })).json();
  $("#linkAllBtn").textContent = "Link all picked";
  if (!r.ok) return toast("batch link failed", true);
  r.linked.forEach((l) => DONE.add(l.task_id));
  toast(`linked ${r.linked.length}${r.failed.length ? `, ${r.failed.length} failed` : ""}`,
        r.failed.length > 0);
  render();
}

$("#rescanBtn").onclick = scan;
$("#linkAllBtn").onclick = linkAll;
$("#hideDone").onchange = render;
loadAllItems().then(scan);
