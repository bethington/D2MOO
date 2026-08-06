"""Offline guards for gen_shadow_dispatch.py's startup-mode declaration.

MEASURED ORIGIN, 2026-08-05. A dispatcher's mode is process-local state that
resets to Original on every launch, and the only way to change it is to POST
/dispatcher/N/mode to the running game. That is fine for a function called
during play, and structurally useless for one that is not.

Sgd2fr_D2Client_SetTileCullingBound (SGD2FreeRes.dll) fires EXACTLY TWICE during process
startup and never again -- hits stayed at 2 through a full world load and
46,000 rendered frames. Both of its calls were therefore over before anything
could arm it, and it sat at `hits=2, divergences=0` looking like a clean pass
while its reimpl had never once been executed. `divergences: 0` means "agreed"
and "never ran" alike, which is the failure mode this file exists to prevent.

Two mechanisms together fixed it, and both are load-bearing:

  1. LAZY ARMING -- the thunk calls LiveDispatchGen::EnsureArmed() on its first
     dispatch, which loads the reimpl provider from inside the call rather than
     waiting for an HTTP request that arrives too late. It cannot be done in
     DllPreLoadHook because that runs under the Windows loader lock.

  2. STARTUP MODE -- this file. An entry may declare the mode it STARTS in, so
     the shadow comparison is live before the first call rather than after the
     last one.

Opt-in per entry, never a global default: shadow mode runs the reimpl on every
real call, so defaulting it corpus-wide would change the startup path of all
196 dispatched functions at once. After the fix the specimen reported
`mode=shadow, hits=2, distinct_inputs=1, divergences=0` -- and distinct_inputs
is the load-bearing number, because the thunk returns early on `!rfn` BEFORE
NoteInputs(), so a non-zero value can only be written after the reimpl actually
ran.
"""

from __future__ import annotations

import importlib.util
import sys
from pathlib import Path

import pytest

_TOOL = Path(__file__).resolve().parent / "gen_shadow_dispatch.py"


def _load():
    spec = importlib.util.spec_from_file_location("gen_shadow_dispatch", _TOOL)
    mod = importlib.util.module_from_spec(spec)
    sys.modules["gen_shadow_dispatch"] = mod
    spec.loader.exec_module(mod)
    return mod


gen = _load()


# --- the declaration itself --------------------------------------------------

def test_absent_startup_mode_means_original():
    """Every existing entry omits the key, and none of their behaviour may move."""
    assert gen.startup_mode_for({"name": "F"}) == "Original"


@pytest.mark.parametrize("declared,expected", [
    ("shadow", "Shadow"),
    ("original", "Original"),
    ("reimpl", "Reimpl"),
    ("SHADOW", "Shadow"),      # case is not the operator's problem
    ("  shadow  ", "Shadow"),
])
def test_declared_modes_map_to_enumerators(declared, expected):
    assert gen.startup_mode_for({"name": "F", "startup_mode": declared}) == expected


def test_an_unknown_mode_refuses_rather_than_defaulting():
    """A typo'd `startup_mode: "shadowed"` must not silently generate Original.

    That is exactly the shape of the bug this feature fixes: a dispatcher that
    looks armed, reports divergences=0, and never ran anything.
    """
    with pytest.raises(SystemExit) as e:
        gen.startup_mode_for({"name": "Sgd2fr_D2Client_SetTileCullingBound", "startup_mode": "shadowed"})
    msg = str(e.value)
    assert "Sgd2fr_D2Client_SetTileCullingBound" in msg and "shadowed" in msg


def test_the_refusal_names_the_valid_set():
    with pytest.raises(SystemExit) as e:
        gen.startup_mode_for({"name": "F", "startup_mode": "on"})
    assert "shadow" in str(e.value) and "original" in str(e.value)


# --- what actually reaches the generated header ------------------------------

_GEN_DIR = Path(__file__).resolve().parents[2] / "D2.Detours.patches" / "1.13c"


def _header(module):
    p = _GEN_DIR / f"{module}_ShadowDispatch.gen.h"
    if not p.exists():
        pytest.skip(f"{p.name} not generated in this tree")
    return p.read_text(encoding="utf-8", errors="replace")


def test_the_specimen_starts_in_shadow():
    assert "Mode::Shadow };" in _header("SGD2FreeRes")


def test_no_other_module_was_switched_on():
    """The measured guarantee: 195 of 196 dispatchers still start in Original.

    A blanket default would run every reimpl on the startup path at once.
    """
    for module in ("D2Common", "D2Client"):
        assert "Mode::Shadow };" not in _header(module), module


def test_lazy_arming_is_emitted_into_the_thunks():
    h = _header("SGD2FreeRes")
    assert "void EnsureArmed()" in h                       # the definition
    assert "LiveDispatchGen::EnsureArmed();" in h          # and the call site


def test_arming_precedes_the_reimpl_read():
    """EnsureArmed must run before `void* rfn = reimpl;` or the first call --
    the only call, for a startup-fired function -- still sees an unbound slot."""
    h = _header("SGD2FreeRes")
    assert h.index("LiveDispatchGen::EnsureArmed();") < h.index("void* rfn = reimpl;")


def test_class_d_is_documented_as_unarmed_rather_than_silently_missing():
    """Class D thunks are __declspec(naked), so a C++ call cannot be injected
    without hand-writing the register save/restore. That gap is recorded in the
    emitter rather than left for someone to rediscover from a stuck hit count.
    """
    src = _TOOL.read_text(encoding="utf-8")
    d = src.index("def emit_dispatcher_d(")
    body = src[d:d + 1200]
    assert "naked" in body and "EnsureArmed" in body
