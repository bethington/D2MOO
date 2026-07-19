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

### Free ×8 RE-ROLL of a draft  ✅ captured live (2026-07-19)
```
POST /v2/tasks/{taskId}/retry     (EMPTY body)  → 200
```
Captured by CDP-driving the workspace viewer's ⟳ ×8 on draft `019f7901…` (the UI shows a
"Confirm retry? The current version will be replaced" dialog first). Semantics, all verified:
- **In-place REPLACE with a new id**: a NEW task appears (same name/params, `retryCount`+1,
  status IN_PROGRESS) and the OLD id starts **404ing** immediately — always adopt the new id.
- **Free**: main credit balance unchanged (4,928 → 4,928). Remaining free re-rolls per draft
  = `8 - retryCount` (the grid's ×8/×7/×6 badges are exactly this).
- Only shown for UNTEXTURED drafts (the ×8 button vanishes once a draft is textured).
- Earlier guess of PATCH was wrong — the preflight `OPTIONS /v2/tasks/{id}` belonged to this
  POST on the `/retry` subpath.
Wired: `meshy_web.retry_task()` → `/api/studio/reroll` now uses this (returns `free: true` and
the NEW task id; the old parent-linked-draft fallback that cost ~20cr is gone).
Historical dead-ends kept for the record: `parent`-linked draft create is NOT free (cost 20cr);
`POST /v2/tasks/{id}/regenerate` (retryTaskV2) is failed-tasks-only.

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
