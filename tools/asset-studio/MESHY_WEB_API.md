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

### Candidates are DC6 ART FILES, not items (2026-07-19 correction)

The first cut ranked per catalog ITEM. Wrong model, two ways:
- **Arbitrary label.** 1406 items resolve to only **577 distinct DC6s**. `invtow.dc6` is
  Tower Shield *and* Pavise *and* Aegis *and* Sigon's Guard, so an exact d0 match on that
  file was reported as whichever item happened to sort first ("Tower Shield") when the
  user knows the same art as the Aegis.
- **Wasted slots.** `invne4.dc6` backs Gargoyle Head, Cantor Trophy, Succubae Skull,
  Trang-Oul's Wing and Boneflame — so a 5-candidate list could show the SAME picture five
  times. That is what made the suggestions look useless.

The art file is also the true unit of work: an override replaces `invtow.dc6` for
everything that uses it. So `dc6_hashes()` groups items by `invfile`, hashes each file
once, and ranks files. A candidate carries `{invfile, item_id (representative), items[],
item_count}`; the card shows the sprite, `invXXX.dc6`, the match reason, and "N items:
…". `_rep_item()` picks the canonical owner (base-category, then shortest name) for the
representative that the Studio needs for cell dims + activation.

Side effects: the scan decodes 577 files instead of 1406 items (265s vs 991s cold), and
`item_dhash_cache.json` is now keyed by invfile. Links carry an `invfile` field; existing
links were migrated in place. `SUGGEST_MAX_DISTANCE=16` is deliberately permissive since
real hits land as far out as d13 (the scale-armor upload matched Templar Coat there) —
tighten it if the tail feels noisy.

### The re-imagined art library is the pairing key (2026-07-19, decisive)

The reference images fed to Meshy are NOT the DC6 sprites — they are AI-redrawn versions
of them (`D:\d2\DC6\Data\global\items`, override with `PD2_REIMAGINED_ART`). The redraw
keeps the subject and none of the pixels, and even recomposes: the game's paired-gauntlet
`invlgl.dc6` was split into single left/right hands. So no pixel, silhouette or shape
metric can connect a hand-made generation to its DC6 — measured, not assumed.

What makes it exact is that the library is **named by DC6 file**: `armors/invplt.png`,
`gloves/invtgl-l4.png`, `boots/invhbt-1.png`. Hash the library, match a task's input
image against it, and the filename names the DC6 outright. `dc6_name_from_art_path()`
strips the `-L/-R/-l3/-lj4` variant suffixes (split hands / numbered redraws) and
validates the result against the known DC6 set so a strip can never invent a file.

Matching order: re-imagined library first (hand-made tasks), then the DC6 sprites
themselves (Studio-created tasks upload those directly). Both hit at d0.

Result on the live workspace: **3 auto-pairs → 23**, all distance 0, e.g. `invbrnz.dc6`
(brain) ↔ "Cerebral Nut", `invcar.dc6` (orbital globe) ↔ "Golden Orbital Globe Emblem",
`invhbt.dc6` (plate boots) ↔ "Greaves of the Golden Sentinel". 909 PNGs cover 509 of the
577 DC6s; the ~200 unresolved names are PD2 customs (`invch1`, `invbonr2`) no catalog
item references — still valid DC6 targets, which is why `/api/dc6/<name>.png` renders by
file name rather than through an item.

The page (`/pairing`) is anchored on art files: LEFT the original DC6 artwork, RIGHT the
generations matched to it. Several generations can target one file (four re-imagined
variants of `invtgl`), so a radio picks which one that file uses (`POST
/api/meshy/primary`, exclusive per invfile). Generations with no library match land in an
"Unplaced" strip with per-item search.

### "None" — declining a pairing (2026-07-19)

Three levels, because a wrong match must FREE the generation rather than strand it:
- **Per art file** — the `none` card in a row (`POST /api/meshy/none {invfile}`) unlinks
  every generation on that DC6; they return to Unplaced, reassignable.
- **Per generation** — the `✕` on a card (`DELETE /api/meshy/links/<task_id>`) frees just
  that one, leaving the rest of the row intact.
- **Never pair this** — `none` on an Unplaced card (`POST /api/meshy/ignore`) for
  generations that aren't game art at all. Stored as an `ignored` link entry rather than
  deleted, and `auto_pair()` skips ignored tasks, so **a rescan cannot silently re-link
  it**. Reversible via the "Marked none" strip.

Verified round-trip: none on `invbsc` → 14→13 pairs, generation back in Unplaced; ignore
→ survives a full rescan unlinked; restore → 14 pairs again.

### Glove pair compositor (2026-07-19)

D2 draws gloves as a PAIR in one sprite, but the re-imagined library splits them into
single hands (`invtgl-l4.png` / `invtgl-r4.png`) because one hand is what Meshy can
usefully model. `app/glove_pairs.py` puts them back together.

**The hand is known, not guessed** — `hand_of()` reads it off the matched library
filename (`-l4` left, `-rj2` right; the `j` series is a second set of redraws). Boots
are numbered (`invhbt-1`) with no `-l`/`-r` and their reference art is already a pair,
so they are correctly not pairable.

**Why a hand-tuned template.** Connected-component analysis of all five glove DC6s
returns exactly ONE region each — the two gloves overlap, so there is no way to split
the original and learn each hand's position from it. Instead each art file stores a
layout (`pair_templates.json`), tuned once against the original shown as a ghost and
reused by every variant pair of that glove (l1+r1, l4+r4, lj2+rj2 …). Placement is
normalised (`cx`/`cy`/`scale` as fractions, `rot` degrees) so it survives any change of
resolution.

**Always a pair.** A hand with no generation is mirrored from the other, flagged in the
row and in the tuner (which previews it mirrored, so you never position a picture the
build won't produce). It upgrades automatically once the real second hand is generated.

**Output sizing.** The composite is built at the ORIGINAL sprite's pixel size (invtgl is
56x56, not the 2*29=58 the cell grid implies) and encoded by `canvas_to_dc6()` —
deliberately NOT `assets.png_to_item_dc6()`, which crops to the alpha bbox and re-fills
the cell, undoing the placement.

Routes: `/api/pair/template/<invfile>` (GET/POST), `/api/pair/ghost/<invfile>.png`,
`/api/pair/hand/<task>.png` (source art, flat background cut), `/api/pair/preview`,
`/api/pair/build`. Build renders each hand's GLB in Blender at the inventory angle,
composites, saves the alternate and activates it — so Push to game is unchanged.

Verified live: `invlgl` built from two genuine 3D models in 23s to a valid 56x56 DC6
pair; `invtgl` (left hands only) built with the right mirrored.

### The number is the VARIANT, and it pairs the hands (2026-07-19)

`invtgl-l4` / `invtgl-r4` are not merely "a left and a right" — they are **one redraw's
two hands**, and the trailing number is what says so. `-lj2` / `-rj2` likewise for the
`j` series; `-L` / `-R` are the unnumbered base pair.

This matters because selecting hands independently silently mixes designs. `invvgl` has
`l3`, `l5` and `r5`: a first-left + first-right pick pairs **l3's left with r5's right**
— two different gloves on one pair of hands, and nothing in the output announces it.

So the VARIANT is the unit of selection. `variant_of()` parses it, the server groups a
row's generations into variant sets (`{variant, left, right, complete}`) sorted
complete-first, and the row offers one "pair set" dropdown instead of two hand
dropdowns. The default lands on a complete set so nothing is mirrored unnecessarily.
A set missing a hand mirrors the other and names the exact file to generate
(`invtgl-r4`) to make it real.

Live grouping: invlgl 2 of 2 sets complete; invvgl 1 of 2 (variant 5 complete, 3 is
left-only); invtgl 0 of 4 and invmgl 0 of 2 — left hands only, all mirroring for now.
