# Note :
# Tests in static libraries might not get registered, see https://github.com/onqtam/doctest/blob/master/doc/markdown/faq.md#why-are-my-tests-in-a-static-library-not-getting-registered
# For this reason, and because it is interesting to have individual
# test executables for each library, it is suggested not to put tests directly in the libraries (even though doctest advocates this usage)
# Creating multiple executables is of course not mandatory, and one could use the same executable with various command lines to filter what tests to run.

# PD2S12Conformance.gen.cpp is GENERATED from the frozen CONF_REGRESSION corpora
# (conformance/regression/*.cases.json) by conformance/tools/gen_offline_tests.py.
# It replays {input -> real-game-output} cases -- captured live from PD2 -- against
# D2MOO's real compiled D2Common, offline, so `ctest` is a pre-merge regression gate.
# Checked in (build needs no Python); re-run the generator after editing a corpus.
add_executable(D2CommonTests D2CommonTests.cpp PD2S12Conformance.cpp PD2S12Conformance.gen.cpp)
target_link_libraries(D2CommonTests PRIVATE doctest::doctest ${D2CommonImplName})
target_compile_features(D2CommonTests PRIVATE cxx_std_17)

set_target_properties(D2CommonTests PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/workingDirectory
)

add_test(
    # Use some per-module/project prefix so that it is easier to run only tests for this module
    NAME D2CommonTests.unittests
    COMMAND D2CommonTests ${TEST_RUNNER_PARAMS}
    WORKING_DIRECTORY $<TARGET_PROPERTY:D2CommonTests,VS_DEBUGGER_WORKING_DIRECTORY>
)

# Tier-2 behavioral-fingerprint generator (the D2MOO/game-free half of the
# automated ordinal matcher — see conformance/behavioral/). Links the real
# compiled D2Common and dumps (input -> output) fingerprints for the coord family.
add_executable(d2moo_fingerprint ${CMAKE_CURRENT_LIST_DIR}/../../../conformance/behavioral/d2moo_fingerprint.cpp)
target_link_libraries(d2moo_fingerprint PRIVATE ${D2CommonImplName})
target_compile_features(d2moo_fingerprint PRIVATE cxx_std_17)
# The *_DLL_DECL macros are applied by D2MOO's module helper to targets it creates,
# not propagated as INTERFACE defs — replicate what D2CommonTests gets (static link
# to D2Common => empty; sibling modules => dllimport) so the headers parse.
target_compile_definitions(d2moo_fingerprint PRIVATE
    D2COMMON_DLL_DECL=
    "D2CMP_DLL_DECL=__declspec(dllimport)"
    "FOG_DLL_DECL=__declspec(dllimport)"
    "STORM_DLL_DECL=__declspec(dllimport)"
    "D2LANG_DLL_DECL=__declspec(dllimport)")

# Milestone 3 of ../../../conformance/DATATABLE_CONFORMANCE_PLAN.md: opens
# PD2's real MPQ archives via D2MOO's Storm import and drives D2Common's real
# DATATBLS_LoadAllTxts loader, so D2MOO acts as its own oracle for data tables
# (no game process needed). Emits the Experience table for diffing against the
# live-game golden capture (pd2_experience.json).
add_executable(d2moo_loader_harness ${CMAKE_CURRENT_LIST_DIR}/../../../conformance/behavioral/d2moo_loader_harness.cpp)
target_link_libraries(d2moo_loader_harness PRIVATE ${D2CommonImplName})
target_compile_features(d2moo_loader_harness PRIVATE cxx_std_17)
target_compile_definitions(d2moo_loader_harness PRIVATE
    D2COMMON_DLL_DECL=
    "D2CMP_DLL_DECL=__declspec(dllimport)"
    "FOG_DLL_DECL=__declspec(dllimport)"
    "STORM_DLL_DECL=__declspec(dllimport)"
    "D2LANG_DLL_DECL=__declspec(dllimport)")

# Phase 2 of ../../../conformance/LIVE_DISPATCH_FRAMEWORK_PLAN.md: links the
# REAL compiled D2Common so d2moo_conform.cpp's coord-family dispatch cases
# call the actual shipping functions (not just header-only RNG, which
# conformance/build.sh already covers via g++). Closes the loop for any live
# SHADOW divergence captured in conformance/behavioral/live_shadow_divergences.jsonl.
add_executable(d2moo_conform ${CMAKE_CURRENT_LIST_DIR}/../../../conformance/d2moo_conform.cpp)
target_link_libraries(d2moo_conform PRIVATE ${D2CommonImplName})
target_compile_features(d2moo_conform PRIVATE cxx_std_17)
target_compile_definitions(d2moo_conform PRIVATE
    D2MOO_CONFORM_LINK_D2COMMON=1
    D2COMMON_DLL_DECL=
    "D2CMP_DLL_DECL=__declspec(dllimport)"
    "FOG_DLL_DECL=__declspec(dllimport)"
    "STORM_DLL_DECL=__declspec(dllimport)"
    "D2LANG_DLL_DECL=__declspec(dllimport)")
set_target_properties(d2moo_conform PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../conformance)

