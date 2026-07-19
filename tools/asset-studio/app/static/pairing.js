/* Pairing view, anchored on PD2 DC6 ART FILES.
   LEFT  = the original game artwork (rendered from the DC6).
   RIGHT = the Meshy generations matched to it — several when you re-imagined the same
           sprite more than once (invtgl has four), so you choose which one to use. */
const $ = (s) => document.querySelector(s);
let PAIRS = [], UNPAIRED = [], IGNORED = [], ALL_ITEMS = [];

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
