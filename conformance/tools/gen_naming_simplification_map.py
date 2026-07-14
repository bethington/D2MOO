#!/usr/bin/env python3
"""Generate the project-wide naming-simplification map.

Rules (Fortification style is the ruling standard):
  - Types: bare PascalCase, no D2 prefix, no Strc suffix.
  - Adopt the exact Fortification/BH community name where a counterpart exists
    (seeded from conformance/fort_rename_map.json name-correspondence pairs).
  - `...Txt` suffix stays for data-table rows, `...Data` for per-unit payloads.
  - Enums: strip D2C_ / D2 prefix to bare PascalCase.
  - Unk*/address-derived/numeric-suffix names are flagged NEEDS_NAME (they need
    a real descriptive name, not a mechanical strip).

Output:
  conformance/naming_simplification_map.json  (machine-readable)
  stdout summary
"""
import json
import re
import sys
from collections import defaultdict
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
SRC = REPO / "source"

# ---------------------------------------------------------------- extraction

# Name capture; whether it's a definition is decided by scanning ahead for
# '{' vs ';' while skipping comments and base-clauses.
DECL_RE = re.compile(r"\b(struct|union|class|enum)\s+(?:class\s+|struct\s+)?([A-Za-z_][A-Za-z_0-9]*)")
COMMENT_RE = re.compile(r"//[^\n]*|/\*.*?\*/", re.S)


def is_definition(text: str, end: int) -> bool:
    """True if the decl whose name ends at `end` is followed by '{' before ';'."""
    tail = COMMENT_RE.sub(" ", text[end:end + 400])
    for ch in tail:
        if ch == "{":
            return True
        if ch in ";)>,*&=":  # fwd decl, param type, template arg, member type...
            return False
    return False

EXCLUDE_PARTS = {"external", ".git", "out"}


def harvest():
    types = {}   # name -> {kind, file}
    enums = {}
    files = []
    for scope in (SRC, REPO / "D2.Detours.patches", REPO / "conformance"):
        if scope.exists():
            files += list(scope.rglob("*.h")) + list(scope.rglob("*.cpp"))
    for h in files:
        if EXCLUDE_PARTS & set(p.lower() for p in h.parts):
            continue
        try:
            text = h.read_text(encoding="utf-8", errors="ignore")
        except OSError:
            continue
        rel = h.relative_to(REPO).as_posix()
        for m in DECL_RE.finditer(text):
            kind, name = m.group(1), m.group(2)
            end = m.end()
            # `struct D2LANG_DLL_DECL Unicode {` — an ALL_CAPS macro between
            # keyword and name; the real name is the next identifier.
            if re.fullmatch(r"[A-Z0-9_]+", name) and "_" in name:
                nxt = re.match(r"\s+([A-Za-z_]\w*)", text[end:])
                if nxt:
                    name = nxt.group(1)
                    end += nxt.end()
            if not is_definition(text, end):
                continue
            if kind == "enum":
                enums.setdefault(name, {"kind": "enum", "file": rel})
            else:
                types.setdefault(name, {"kind": kind, "file": rel})
    return types, enums


# ---------------------------------------------------------------- overrides

# Fortification-canonical adoptions (community/BH names). Seeded from
# fort_rename_map.json name-correspondence pairs + judgment calls where the
# community vocabulary clearly applies.
FORT_CANONICAL = {
    "D2UnitStrc": "UnitAny",
    "D2ItemDataStrc": "ItemData",
    "D2PlayerDataStrc": "PlayerData",
    "D2MonsterDataStrc": "MonsterData",
    "D2ObjectDataStrc": "ObjectData",
    "D2StatListStrc": "StatList",
    "D2StatStrc": "Stat",
    "D2SkillStrc": "Skill",
    "D2InventoryStrc": "Inventory",
    "D2InventoryNodeStrc": "InventoryNode",
    "D2InactiveUnitNodeStrc": "InactiveUnitNode",
    "D2DynamicPathStrc": "Path",
    "D2StaticPathStrc": "StaticPath",
    "D2DrlgActStrc": "Act",
    "D2DrlgStrc": "ActMisc",
    "D2DrlgLevelStrc": "Level",
    "D2ActiveRoomStrc": "Room1",
    "D2DrlgRoomStrc": "Room2",
    "D2RoomTileStrc": "RoomTile",
    "D2PresetUnitStrc": "PresetUnit",
    "D2AutomapCellStrc": "AutomapCell",
    "D2AutomapLayerStrc": "AutomapLayer",
    "D2CellFileStrc": "CellFile",
    "D2GfxCellStrc": "GfxCell",
    "D2RosterUnitStrc": "RosterUnit",
    "D2UnitEventStrc": "UnitEvent",
    "D2QuestInfoStrc": "QuestInfo",
    "D2WaypointDataStrc": "WaypointData",
    "D2WinControlStrc": "Control",
}

# Judgment overrides where a mechanical strip is wrong/ambiguous.
CURATED = {
    "D2pSpellTblStrc": "SpellTbl",              # stray 'p' in the type name
    "D2DrlgRoomTilesStrc": "RoomTileList",      # avoid clash with RoomTile
    "D2CharStrc": "FontGlyph",                  # 'Char' alone is unusable
    "D2FontStrc": "FontFile",                   # 'Font' enum already exists in D2Win
    "D2CoordStrc": "Coord",
    "D2SeedStrc": "Seed",
    "D2DataTablesStrc": "DataTables",           # the sgptDataTables master table
    "D2GameStrc": "Game",
    "D2ClientStrc": "GameClient",               # 'Client' too generic server-side
    "D2CompositStrc": "Composit",               # keep D2 community spelling
}

# Names that are placeholders for unknown/unreversed structures: renaming
# D2UnkFooStrc -> UnkFoo is churn without value; they need REAL names when the
# structure is understood. Flag, propose the strip as interim.
UNK_RE = re.compile(r"(Unk|_6F[0-9A-F]{6}_|Strc\d+$|\d+Strc$)")

ENUM_CURATED = {
    "D2Jungle": "JungleIds",  # avoid clash with D2JungleStrc -> Jungle
}

# Known identifiers that would shadow/collide with Win32 or C++ vocabulary.
RISKY_TARGETS = {
    "Char", "Font", "Control", "Task", "Link", "Text", "Property", "Message",
    "Field", "Tile", "Effect", "Mode", "Direction", "Point", "Rect",
}


def strip_type(name: str) -> str:
    n = name
    if n.startswith("D2"):
        n = n[2:]
    n = re.sub(r"Strc(\d*)$", r"\1", n)
    n = n.strip("_")
    if not n or not (n[0].isalpha() or n[0] == "_"):
        n = "Unk" + n.rstrip("_")
    return n or name


def strip_enum(name: str) -> str:
    for pre in ("D2C_", "D2_", "D2"):
        if name.startswith(pre):
            return name[len(pre):] or name
    return name


def main():
    types, enums = harvest()
    fort_map = json.loads((REPO / "conformance" / "fort_rename_map.json").read_text())
    fort_pairs = {p["d2moo"]: p for p in fort_map["pairs"] if "fort" in p}

    entries = []
    targets = defaultdict(list)
    existing = set(types) | set(enums)

    for name, info in sorted(types.items()):
        if not (name.startswith("D2") or name.endswith("Strc")):
            # already clean (or third-party style); record as keep
            entries.append({**info, "old": name, "new": name, "tier": "keep"})
            continue
        if name in FORT_CANONICAL:
            new, tier = FORT_CANONICAL[name], "fort_canonical"
        elif name in CURATED:
            new, tier = CURATED[name], "curated"
        elif UNK_RE.search(name):
            new, tier = strip_type(name), "needs_name"
        else:
            new, tier = strip_type(name), "mechanical"
        e = {**info, "old": name, "new": new, "tier": tier}
        fp = fort_pairs.get(name)
        if fp:
            e["fort"] = fp["fort"]
            e["fort_overlap"] = fp.get("overlap")
        flags = []
        if new in RISKY_TARGETS:
            flags.append("risky_generic")
        if new in existing and new != name:
            flags.append("collides_existing")
        if flags:
            e["flags"] = flags
        entries.append(e)
        targets[new].append(name)

    for name, info in sorted(enums.items()):
        if not name.startswith("D2"):
            entries.append({**info, "old": name, "new": name, "tier": "keep"})
            continue
        new = ENUM_CURATED.get(name) or strip_enum(name)
        e = {**info, "old": name, "new": new, "tier": "enum_strip"}
        flags = []
        if new in RISKY_TARGETS:
            flags.append("risky_generic")
        if new in existing:
            flags.append("collides_existing")
        if flags:
            e["flags"] = flags
        entries.append(e)
        targets[new].append(name)

    # duplicate-target collisions
    for new, olds in targets.items():
        if len(olds) > 1:
            for e in entries:
                if e["new"] == new and e["old"] in olds and e["old"] != e["new"]:
                    e.setdefault("flags", []).append("duplicate_target")

    # Merge: keep rename pairs from a previous map whose old name no longer
    # appears in the tree (already executed) — the map is a cumulative ledger.
    dst = REPO / "conformance" / "naming_simplification_map.json"
    if dst.exists():
        prev = json.loads(dst.read_text())
        have = {e["old"] for e in entries}
        for e in prev.get("entries", []):
            if e["old"] != e["new"] and e["old"] not in have:
                entries.append(e)

    tiers = defaultdict(int)
    for e in entries:
        tiers[e["tier"]] += 1
    flagged = [e for e in entries if e.get("flags")]

    out = {
        "generated_by": "gen_naming_simplification_map.py",
        "standard": "Fortification (BH/slashdiablo) naming style",
        "summary": {
            "total": len(entries),
            "tiers": dict(sorted(tiers.items())),
            "flagged": len(flagged),
        },
        "entries": entries,
    }
    dst.write_text(json.dumps(out, indent=1), encoding="utf-8")
    print(f"wrote {dst}")
    print("tiers:", dict(sorted(tiers.items())))
    print(f"flagged: {len(flagged)}")
    for e in flagged[:40]:
        print("  ", e["old"], "->", e["new"], e["flags"])
    if len(flagged) > 40:
        print(f"   ... and {len(flagged) - 40} more")


if __name__ == "__main__":
    sys.exit(main())
