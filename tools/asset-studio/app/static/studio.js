import * as THREE from 'three';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';

const $ = (s) => document.querySelector(s);
let SEL = null;        // selected item
let TASK = null;       // current task id
let PHASE = null;      // 'draft' | 'texture'
let ALT = null;        // alt id after Accept (for live refit)
let ITEMS = [];

function toast(msg, err) {
  const t = $("#toast"); t.textContent = msg; t.className = "toast" + (err ? " err" : "");
  clearTimeout(t._t); t._t = setTimeout(() => t.classList.add("hidden"), 4200);
}

/* ---------- three.js viewer ---------- */
let renderer, scene, camera, controls, model;
function initViewer() {
  const el = $("#viewer");
  renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
  renderer.setPixelRatio(devicePixelRatio);
  renderer.setSize(el.clientWidth, el.clientHeight);
  el.appendChild(renderer.domElement);
  scene = new THREE.Scene();
  camera = new THREE.PerspectiveCamera(35, el.clientWidth / el.clientHeight, 0.01, 100);
  camera.position.set(0, 0.4, 3);
  controls = new OrbitControls(camera, renderer.domElement);
  controls.enableDamping = true; controls.autoRotate = false;
  scene.add(new THREE.HemisphereLight(0xffffff, 0x404050, 1.1));
  const key = new THREE.DirectionalLight(0xffffff, 2.2); key.position.set(2, 3, 2); scene.add(key);
  const fill = new THREE.DirectionalLight(0xffffff, 0.9); fill.position.set(-2, 1, -1.5); scene.add(fill);
  addEventListener("resize", onResize);
  (function loop() { requestAnimationFrame(loop); controls.update(); renderer.render(scene, camera); })();
}
function onResize() {
  const el = $("#viewer");
  camera.aspect = el.clientWidth / el.clientHeight; camera.updateProjectionMatrix();
  renderer.setSize(el.clientWidth, el.clientHeight);
}
function clearModel() { if (model) { scene.remove(model); model.traverse(o => { o.geometry?.dispose?.(); }); model = null; } }
async function loadGLB(url) {
  return new Promise((res, rej) => {
    new GLTFLoader().load(url, (g) => {
      clearModel(); model = g.scene;
      // center + fit
      const box = new THREE.Box3().setFromObject(model);
      const c = box.getCenter(new THREE.Vector3()), s = box.getSize(new THREE.Vector3());
      model.position.sub(c);
      const r = Math.max(s.x, s.y, s.z) || 1;
      model.scale.setScalar(1.4 / r);
      scene.add(model);
      controls.target.set(0, 0, 0); camera.position.set(0, 0.3, 3); controls.update();
      res();
    }, undefined, rej);
  });
}

/* ---------- session ---------- */
async function refreshSession() {
  const s = await (await fetch("/api/studio/session")).json();
  const el = $("#sess");
  if (s.loggedIn) { el.textContent = `session: ${s.tier || "ok"} ✓`; el.className = "pill ok"; $("#loginBtn").classList.add("hidden"); }
  else { el.textContent = "session: " + (s.note || "not ready"); el.className = "pill bad"; $("#loginBtn").classList.remove("hidden"); }
  $("#genBtn").disabled = !(s.loggedIn && SEL);
  return s.loggedIn;
}
$("#loginBtn").onclick = async () => {
  toast("launching the Meshy login browser…");
  await fetch("/api/studio/session/launch", { method: "POST" });
  toast("log in to Meshy in the window that opened, then it'll connect.");
  const t = setInterval(async () => { if (await refreshSession()) { clearInterval(t); toast("session connected ✓"); } }, 3000);
};
async function refreshCredits() {
  try { const s = await (await fetch("/api/studio/session")).json();
    $("#cred").textContent = s.loggedIn ? `free retries: ${s.freeMonthlyCredits ?? "?"}/mo` : "credits: —";
  } catch (e) {}
}

/* ---------- items ---------- */
async function loadItems() {
  const q = encodeURIComponent($("#search").value.trim());
  const d = await (await fetch(`/api/items?q=${q}`)).json();
  ITEMS = d.items;
  // float a pre-selected item (?item=<id> from the gallery's "Open in Studio") to the top so it
  // renders inside the 300-row cap and can be highlighted, no matter where it sorts naturally.
  const want = new URLSearchParams(location.search).get("item");
  if (want && !SEL) { const i = ITEMS.findIndex(x => x.id === want); if (i > 0) ITEMS.unshift(ITEMS.splice(i, 1)[0]); }
  const l = $("#ilist"); l.innerHTML = "";
  for (const it of ITEMS.slice(0, 300)) {
    const row = document.createElement("div"); row.className = "irow" + (SEL?.id === it.id ? " sel" : "");
    row.innerHTML = `<img loading="lazy" src="/api/item/${encodeURIComponent(it.id)}/original.png" onerror="this.style.opacity=.15"><span>${it.name}</span>`;
    row.onclick = () => selectItem(it);
    l.appendChild(row);
  }
  if (want && !SEL) { const it = ITEMS.find(x => x.id === want); if (it) { selectItem(it); document.querySelector(".irow.sel")?.scrollIntoView({ block: "center" }); } }
}
function selectItem(it) {
  SEL = it; TASK = null; PHASE = null; ALT = null;
  document.querySelectorAll(".irow").forEach(r => r.classList.remove("sel"));
  [...document.querySelectorAll(".irow")].find(r => r.textContent.trim() === it.name)?.classList.add("sel");
  $("#prog").textContent = `${it.name} (${it.invwidth}×${it.invheight}) — Generate a draft.`;
  $("#genBtn").disabled = false;
  setStage("start");
}
function setStage(stage) {
  $("#draftActions").style.opacity = (stage === "draft" || stage === "textured") ? 1 : .4;
  $("#rerollBtn").disabled = stage !== "draft" && stage !== "textured";
  $("#texActions").style.opacity = (stage === "draft") ? 1 : .4;
  $("#texBtn").disabled = stage !== "draft";
  $("#acceptGrp").style.opacity = (stage === "draft" || stage === "textured") ? 1 : .4;
  $("#acceptBtn").disabled = !(stage === "draft" || stage === "textured");
}

/* ---------- poll ---------- */
async function pollTask(tid, label) {
  for (let i = 0; i < 90; i++) {
    const t = await (await fetch(`/api/studio/task/${tid}`)).json();
    $("#prog").textContent = `${label}: ${t.status} ${t.progress || 0}%`;
    if (t.status === "SUCCEEDED") return true;
    if (t.status === "FAILED" || t.status === "CANCELED") { toast(`${label} ${t.status}`, true); return false; }
    await new Promise(r => setTimeout(r, 3500));
  }
  return false;
}

/* ---------- flow ---------- */
$("#genBtn").onclick = async () => {
  if (!SEL) return;
  $("#genBtn").disabled = true; $("#prog").textContent = "submitting draft…";
  const opts = { aiModel: $("#aiModel").value, topology: $("#topology").value, symmetry: $("#symmetry").value, seed: +$("#seed").value };
  const r = await (await fetch("/api/studio/generate", { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify({ item_id: SEL.id, opts }) })).json();
  if (!r.ok) { toast("generate failed: " + r.error, true); $("#genBtn").disabled = false; return; }
  TASK = r.task_id; PHASE = "draft";
  if (await pollTask(TASK, "draft")) { await showModel(TASK, "draft"); }
  $("#genBtn").disabled = false;
  refreshCredits();
};
$("#rerollBtn").onclick = async () => {
  if (!TASK) return;
  $("#rerollBtn").disabled = true; $("#prog").textContent = "re-rolling shape…";
  const r = await (await fetch("/api/studio/reroll", { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify({ task_id: TASK }) })).json();
  if (!r.ok) { toast("re-roll failed: " + r.error, true); $("#rerollBtn").disabled = false; return; }
  TASK = r.task_id; PHASE = "draft";
  $("#rerollNote").textContent = r.free ? "free re-roll ✓" : (r.note || "fresh draft (~20 credits)");
  if (await pollTask(TASK, "re-roll")) { await showModel(TASK, "draft"); }
  refreshCredits();
};
$("#texBtn").onclick = async () => {
  if (!TASK) return;
  $("#texBtn").disabled = true; $("#prog").textContent = "texturing…";
  const opts = { artStyle: $("#artStyle").value, enablePBR: $("#enablePBR").checked, prompt: $("#texPrompt").value.trim() };
  const r = await (await fetch("/api/studio/texture", { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify({ task_id: TASK, opts }) })).json();
  if (!r.ok) { toast("texture failed: " + r.error, true); $("#texBtn").disabled = false; return; }
  TASK = r.task_id; PHASE = "texture";
  if (await pollTask(TASK, "texture")) { await showModel(TASK, "textured"); }
  refreshCredits();
};
async function showModel(tid, stage) {
  $("#prog").textContent = "loading 3D model…";
  try { await loadGLB(`/api/studio/glb/${tid}.glb`); $("#prog").textContent = stage === "textured" ? "textured — rotate to inspect, then Accept." : "draft — rotate to check the shape. Re-roll or Texture."; }
  catch (e) { toast("model load failed: " + e, true); }
  const pp = $("#phasePill"); pp.textContent = stage; pp.classList.remove("hidden");
  setStage(stage);
}

/* ---------- accept + live grade/framing ---------- */
function gradeVals() { return { brightness: +$("#brightness").value, warmth: +$("#warmth").value, saturation: +$("#saturation").value, contrast: +$("#contrast").value }; }
$("#acceptBtn").onclick = async () => {
  if (!TASK) return;
  $("#acceptBtn").disabled = true; $("#prog").textContent = "rendering → DC6…";
  const body = { task_id: TASK, azim: +$("#azim").value, elev: +$("#elev").value, fill: +$("#fill").value, grade: gradeVals() };
  const r = await (await fetch("/api/studio/accept", { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify(body) })).json();
  $("#acceptBtn").disabled = false;
  if (!r.ok) { toast("accept failed: " + r.error, true); return; }
  ALT = r.alt_id; $("#pushBtn").disabled = false; $("#prog").textContent = "accepted — tweak tone/framing (instant), then Push.";
  toast(`created "${r.alt_id}" — activated`);
  updateCellPreview();
};
function updateCellPreview() {
  if (!SEL || !ALT) return;
  const g = gradeVals();
  const qs = `fill=${$("#fill").value}&dx=0&dy=0&brightness=${g.brightness}&warmth=${g.warmth}&saturation=${g.saturation}&contrast=${g.contrast}&t=${Date.now()}`;
  $("#cellPrev").src = `/api/item/${encodeURIComponent(SEL.id)}/alt/${ALT}/cell.png?${qs}`;
}
let refitTimer;
function onGradeChange() {
  ["brightness", "warmth", "saturation", "contrast", "fill", "azim", "elev"].forEach(id => { const v = $("#" + id + "V"); if (v) v.textContent = (+$("#" + id).value).toFixed(2).replace(/0$/, "").replace(/\.$/, ""); });
  updateCellPreview();
  if (ALT) { clearTimeout(refitTimer); refitTimer = setTimeout(applyRefit, 400); }
}
async function applyRefit() {
  if (!SEL || !ALT) return;
  const g = gradeVals();
  await fetch(`/api/item/${encodeURIComponent(SEL.id)}/alt/${ALT}/refit`, { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify({ fill: +$("#fill").value, dx: 0, dy: 0, grade: g }) });
}
["brightness", "warmth", "saturation", "contrast", "fill", "azim", "elev"].forEach(id => $("#" + id).oninput = onGradeChange);
$("#pushBtn").onclick = async () => {
  $("#pushBtn").disabled = true; toast("building patch.mpq & pushing…");
  const r = await (await fetch("/api/push", { method: "POST" })).json();
  toast(r.ok ? `pushed → reload/enter game to see it` : "push failed: " + (r.error || ""), !r.ok);
  $("#pushBtn").disabled = false;
};

/* ---------- boot ---------- */
window._loadGLB = loadGLB;  // debug hook: load an arbitrary model into the viewer
$("#search").oninput = () => { clearTimeout($("#search")._t); $("#search")._t = setTimeout(loadItems, 250); };
initViewer(); loadItems(); refreshSession(); refreshCredits();
setInterval(refreshSession, 8000); setInterval(refreshCredits, 20000);
