"""Item catalog — parse PD2's item tables (read live from the MPQs) into a browsable list.

Base items come from weapons/armor/misc.txt (they carry invfile + invwidth/invheight).
Uniques/sets come from uniqueitems/setitems.txt; when their own invfile is blank they
inherit the base item's invfile (resolved via the base 'code').
"""

from __future__ import annotations

import csv
import io
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from pyd2.mpq import read_effective  # noqa: E402

EXCEL = "data\\global\\excel\\{}.txt"
BASE_TABLES = ("weapons", "armor", "misc")


def _read_table(name: str):
	data, _src = read_effective(EXCEL.format(name))
	text = data.decode("latin-1")
	return list(csv.DictReader(io.StringIO(text), delimiter="\t"))


def _clean(s: str) -> str:
	return (s or "").strip()


def set_pieces(set_query: str):
	"""Return [{row, index, base, set}] for every setitems.txt row whose set name (or piece
	index) matches set_query (case-insensitive substring). `row` is the 0-based setitems.txt
	row, `base` is the piece's base item code -- both needed to force the exact set item via
	the /showcase/item {code, setRow} verb (setRow is the row; base must be the piece's base).
	"""
	rows = _read_table("setitems")
	q = set_query.strip().lower()
	out = []
	# The game's setitems array (from the compiled .bin) is 0-based over the rows the
	# compiler KEEPS -- it drops the "Expansion" section separator (a row with no index),
	# so the raw CSV row number is off. Use a running counter of kept rows = the game index.
	game_idx = 0
	for r in rows:
		index = _clean(r.get("index"))
		base = _clean(r.get("item"))
		if not index or index.lower() == "expansion" or not base:
			continue  # a dropped row -- does NOT advance the game index
		if q in _clean(r.get("set")).lower() or q in index.lower():
			out.append({"row": game_idx, "index": index, "set": _clean(r.get("set")),
			            "base": base, "lvl": _clean(r.get("lvl"))})
		game_idx += 1
	return out


def build_catalog():
	"""Return (items, by_code). items: list of dicts; by_code: base code -> base item."""
	items = []
	by_code = {}

	# --- base items ---
	for table in BASE_TABLES:
		try:
			rows = _read_table(table)
		except FileNotFoundError:
			continue
		for r in rows:
			code = _clean(r.get("code"))
			invfile = _clean(r.get("invfile"))
			name = _clean(r.get("name"))
			if not code or not invfile or not name or name.lower() in ("expansion", "name"):
				continue
			item = {
				"id": f"base/{table}/{code}",
				"category": "base",
				"table": table,
				"name": name,
				"code": code,
				"invfile": invfile,
				"flippyfile": _clean(r.get("flippyfile")),
				"invwidth": int(r.get("invwidth") or 1),
				"invheight": int(r.get("invheight") or 1),
				"invtransform": "",
				"type": _clean(r.get("type")),
			}
			items.append(item)
			by_code.setdefault(code, item)

	# --- uniques ---
	try:
		for r in _read_table("uniqueitems"):
			index = _clean(r.get("index"))
			base = _clean(r.get("code"))
			if not index or not base or index.lower() in ("expansion", "index"):
				continue
			binfo = by_code.get(base)
			invfile = _clean(r.get("invfile")) or (binfo["invfile"] if binfo else "")
			if not invfile:
				continue
			items.append({
				"id": f"unique/{index}",
				"category": "unique",
				"table": "uniqueitems",
				"name": index,
				"code": base,
				"invfile": invfile,
				"flippyfile": binfo["flippyfile"] if binfo else "",
				"invwidth": binfo["invwidth"] if binfo else 2,
				"invheight": binfo["invheight"] if binfo else 2,
				"invtransform": _clean(r.get("invtransform")),
				"type": binfo["type"] if binfo else "",
			})
	except FileNotFoundError:
		pass

	# --- sets ---
	try:
		for r in _read_table("setitems"):
			index = _clean(r.get("index"))
			base = _clean(r.get("item"))
			if not index or not base:
				continue
			binfo = by_code.get(base)
			invfile = _clean(r.get("invfile")) or (binfo["invfile"] if binfo else "")
			if not invfile:
				continue
			items.append({
				"id": f"set/{index}",
				"category": "set",
				"table": "setitems",
				"name": index,
				"code": base,
				"invfile": invfile,
				"flippyfile": binfo["flippyfile"] if binfo else "",
				"invwidth": binfo["invwidth"] if binfo else 2,
				"invheight": binfo["invheight"] if binfo else 2,
				"invtransform": _clean(r.get("invtransform")),
				"type": binfo["type"] if binfo else "",
			})
	except FileNotFoundError:
		pass

	return items, by_code
