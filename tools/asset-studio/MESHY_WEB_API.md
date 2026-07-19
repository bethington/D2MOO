# Meshy WEB API (internal) — reverse-engineered for free-retry generation

Meshy runs **two separate auth systems**. The documented **openapi** key (`Bearer msy_…`) hits
`api.meshy.ai/openapi/v1/*`, is billed per call, and has **no free-retry**. The **web app** uses a
**browser login session** (a Supabase JWT, ~1h expiry) sent as `Authorization: Bearer <jwt>` to the
internal `api.meshy.ai/web/*` API — this is where the plan's free **×8 retries** live. The openapi
key is rejected (401) on `/web`. Tier confirmed **studio** (8 free retries) via `GET /web/v1/me/tier`.

## Getting the session token (no manual paste)

Launch the user's browser (Brave/Chrome) with `--remote-debugging-port=<p> --remote-allow-origins=*`
on their real profile (login persists). The JWT is **not** in localStorage/cookies — it's held in
memory and attached as a header. Capture it via CDP **Network**: connect to a `meshy.ai` page target,
`Network.enable`, and read the `Authorization: Bearer …` header off any `api.meshy.ai/web` request
(trigger one with `fetch('/web/v1/me/tier',{credentials:'include'})`). Re-read on demand (expires ~1h).
Gotcha: CDP `Target.setAutoAttach` only catches NEW tabs — connect to each existing page target
directly. The app uses **axios over XHR**, so a `window.fetch` hook misses calls — use CDP Network.

## Endpoints (all `https://api.meshy.ai/web`, `Bearer <jwt>`, `Origin: https://app.meshy.ai`)

### Register an input image  ✅ working
`POST /v1/files/images` — multipart form field `file` = PNG bytes; query `?removeBackground=false`.
→ `{result:{id, url, name}}`. The `id` (e.g. `86ac…f90feacf.png`) is what the create references.
(Meshy even auto-names it, e.g. "Armored Dome".) IMPORTANT: prep the sprite **aspect-preserved**
(pad to square) — a `resize((512,512))` distorts non-square sprites and Meshy builds the distortion.

### Create the DRAFT (geometry, untextured)  ✅ working — costs ~20 credits
```
POST /v2/tasks
{ "phase":"draft", "batchId":"<uuid>",
  "args":{"draft":{ "aiModel":"avocado" (=Meshy6), "modelType":"standard",
    "topology":"triangle", "imageIds":["<image id>"], "shouldTransferImageStyle":true,
    "symmetryMode":0, "seed":0, "license":"private", "prompt":"" }} }
→ {"result":"<draftTaskId>"}
```
Poll `GET /v2/tasks/{id}` → `{status, phase, mode:"draft", result:{...}}`. status SUCCEEDED ≈ 40s.

### TEXTURE the approved draft  ✅ captured live
```
POST /v2/tasks
{ "phase":"texture", "parent":"<draftTaskId>",
  "args":{"texture":{ "imageId":"<image id>", "artStyle":"realistic", "aiModel":"avocado",
    "enablePBR":true, "srMode":"weak", "textureSize":0, "prompt":"" }} }
→ {"result":"<textureTaskId>"}
```

### Free ×8 RE-ROLL of a draft  ⚠️ NOT fully captured
Only the CORS preflight was seen: `OPTIONS /v2/tasks/{draftId}` — i.e. a non-GET (likely **PATCH**)
to the specific draft URL that re-generates it **in place** (free, deducts a retry). The body is
still unknown. RULED OUT: a `parent`-linked draft create (`POST /v2/tasks {phase:draft, parent}`)
is **NOT free** — tested, cost 20 credits. `POST /v2/tasks/{id}/regenerate` (retryTaskV2) is
**failed-tasks-only** ("Only failed tasks can be regenerated") — not the ×8.
**To capture it:** open an UNTEXTURED draft in the workspace viewer and click the ⟳ ×8 (circular
arrows, leftmost in the toolbar — NOT the green Texture) while a CDP capture watches all tabs for a
non-GET to `/web/v2/tasks/{id}`. The ×8 button vanishes once a draft is textured.

### Other
- `GET /v1/me/tier` → `{tier:"studio", freeMonthlyCredits, …}`.
- `GET /v2/tasks?pageNum=&pageSize=` → task list; `GET /v2/tasks/{id}` → full task.
- `GET /v1/files/upload-url-for-public {prefixes:["image"], filename}` → presigned S3 PUT (alt upload
  path; the CDN url is `https://cdn.meshy.ai/{key}`) — but the create wants the **registered** image
  id from `/v1/files/images`, not a raw S3 key ("Image not found").
- Task `mode`: `api-image-to-3d` (from the openapi key/API console) = **not retryable**; `draft`/
  `generate` (from the web workspace) = retryable. `canRegenerate = !isExample && mode!="upload" &&
  phase ∈ {draft,texture,image-to-3d-texture,generate}`.

## Two-phase flow (what the studio implements)
1. register image (aspect-preserved) → 2. create **draft** → poll → **3D preview** →
3. re-roll ×8 (free, once the PATCH body is captured; else a fresh draft = 20cr) until the shape is
right → 4. **texture** the chosen draft → poll → preview textured → 5. accept → download GLB →
Blender render → DC6 → push to game.
