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
3. re-roll ×8 (**free** — `POST /v2/tasks/{id}/retry`, adopt the new id) until the shape is
right → 4. **texture** the chosen draft → poll → preview textured → 5. accept → download GLB →
Blender render → DC6 → push to game.

## Pairing existing workspace tasks with catalog items (2026-07-19)

`app/meshy_links.py` + `/api/meshy/*`. Links persist in `<workspace>/meshy_links.json`
(`{task_id: {item_id, image_id, phase, source, name, linked_at}}`); the server seeds its
in-memory `_STUDIO` map from it at boot, so pairings survive restarts (previously they
died with the process).

**There is no server-side image listing** — `POST /web/v1/files/images` is upload-only and
every `GET` variant 404s — so a task's origin item can't be recovered by filename. Pairing
uses the task's **input image** (`args.draft.imageUrl`) instead:
- **dHash (64-bit) vs each catalog sprite** run through the same `prep_image_for_meshy`
  used at upload. Studio-created tasks re-hash to **d=0**.
- **Name similarity** as a weak secondary — unreliable alone because Meshy auto-names by
  *appearance*: the Plate Mail sprite came back named "Chainmail hauberk".

**Threshold calibration (measured, not guessed).** A real scan of 60 tasks × 1406 items was
rendered as a side-by-side contact sheet: all `d==0` pairs correct; `d` in 5..8 mostly WRONG
(gauntlet↔skeleton key at d8, glove↔potion at d7, leather↔metal boots at d8). Small dark
sprites with similar silhouettes collide. So **auto-link only at d≤2**; d≤16 and name hits
become one-click suggestions shown next to the input thumbnail. Result on the live
workspace: 3 auto (all correct), 24 suggestions, 5 unmatched — the unmatched/loose ones are
mostly hand-uploaded hi-res renders that were never catalog sprites.

First scan hashes the whole catalog (~16 min, mostly DC6 decode) and caches to
`item_dhash_cache.json`; later scans take ~10s.

### Pairing review page (`/pairing`, 2026-07-19)

Filmstrip rows: the task's **input image + generated preview** pinned left, candidate item
sprites as radio cards right, ordered strongest→weakest with a reason badge (`image d12`,
`name 59%`). Radio + "Link this" per row, or "Link all picked" for a batch
(`POST /api/meshy/links/batch`).

Two hard-won requirements, both from looking at real data:
1. **Per-row "search all items"** — the auto-candidates frequently contain NO correct
   answer, because much of the workspace is hand-uploaded hi-res art that was never a
   catalog sprite. dHash then returns five near-identical wrong sprites (a gauntlet
   upload offered five skull/key icons at d6). The search box is the escape hatch and
   is the primary path for those tasks.
2. **Low-confidence links are re-offered, not hidden.** `is_high_confidence()` gates on
   the link's `source`: only `auto-image d≤2`, `studio-*`, `manual*`, `reviewed*` are
   trusted. Anything else (notably legacy `fuzzy-confirmed` picks made through the old
   dropdown, before candidates showed sprites) comes back with a red "was linked to X —
   confirm or change" badge and the current pick offered first. Without this, a blind
   guess silently became permanent and vanished from review (21 such links existed; one
   had put a scale-armor generation on `herb`).
