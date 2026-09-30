<!-- DESCRIPTION: Verilator: Feature document for completing the lint-only test pruning
     SPDX-FileCopyrightText: 2026-2026 Wilson Snyder
     SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0 -->

---
feature: test-suite-pruning
status: delivered
updated: 2026-09-30
branch: msvc-lint-only
commits: 74f976237..90df41329
---

# Complete the lint-only test-suite pruning

## Report

**What was built** -- The default suite is clean: `nodist/test-regress.ps1`
reports `Passed 1005 / Failed 0 / Skipped 48` instead of `Passed 986 /
Failed 66 / Skipped 48`, and `--dist` keeps exactly its three pre-existing
failures.

Of the 66 failing tests, 18 now exercise the lint pipeline instead of building a
model: each driver is `test.scenarios('vlt')` plus `test.lint(...)`, with
`test.execute()` and model-only flags removed, so the language-extension,
lint-cleanliness, preprocessor, slice-statistic, `-f`/`-Fs`, virtual-interface,
package-export, generate-instantiation, `INSIDETRUE`-suppression, `--top-module`
and NOTIMING-golden checks survive. `t_flag_hier0_bad` was already a lint test
with a drifted golden: it now uses the surviving `t/t_EXAMPLE.v` as its top, its
two options that no longer error were dropped, and its golden was regenerated.
The other 47 tests -- 120 files -- were deleted with the fixtures only they
owned: model build/simulation behaviour, waves and VPI/DPI, coverage data, the
profiling tools, removed options (`--CFLAGS`, `-no-MMD`) and the passes this
build stops before (`UNOPTFLAT`, the internal width error). Sources another test
still uses are kept (`t_castdyn.v`, `t_delay.v`, `t_flag_main.v`, the
`_bad`-sibling fixtures, ...). Five stale references were updated, and the spec
records the accepted coverage losses.

**Verification** -- run from the repository root:

- `nodist/test-regress.ps1` -- Passed 1005 / Failed 0 / Skipped 48 (baseline
  before the change: Passed 986 / Failed 66 / Skipped 48).
- `nodist/test-regress.ps1 --dist` -- Passed 22 / Failed 3 / Skipped 3,
  identical to the baseline; PRE-EXISTING failures are `t_dist_contributors`
  (needs a human signature in `docs/CONTRIBUTORS`), `t_flag_version` (the branch
  version stamp omits "Verilator") and `t_driver_timeout` (bash execution here).
- Targeted runs of the 18 converted tests, the restored `t_flag_hier0_bad` and
  `t_lint_width_bad` -- all pass.
- Bidirectional orphan sweep and a basename scan of all 120 deleted files over
  the surviving tracked tree -- zero orphans; the only mentions of deleted files
  are in this document, which records their removal. The two orphaned goldens
  and the `t_config_work.map`/`t_uvm_hello.v` files still unreferenced were
  already unreferenced at the base commit.
- Three independent review rounds; the final one confirmed every earlier finding
  resolved, no remaining orphan, all four tasks met, and no new correctness
  problem.

**Journey log**:

- The first sweep decided deletions from text mentions by *surviving* files.
  Two review rounds showed that misses two couplings the scanner cannot see:
  fixtures mentioned only by files deleted in the same change, and sources tied
  to a driver by the harness's default `top_filename` derivation. The sweep now
  runs in both directions.
- `t/t_config_libmap/` was kept at first on a "proven by experiment" claim that
  came from running `driver.py` without the `nodist/test-regress.ps1` wrapper and
  reading a stale log. Re-running under the wrapper showed the directory is dead
  weight, so it was deleted and the spec corrected; the same wrapper mistake is
  the reason the earlier claim looked reproducible.
- `--hierarchical-block`'s two backslash error paths are unreachable on this
  platform because the harness rewrites `\` to `/` before running Verilator, so
  the two `V3Options` error paths lose their only test; recorded as an accepted
  loss rather than papered over with a platform-dependent golden.
- Six of the tests the first pass deleted were convertible after all, and
  restoring them also kept their fixtures used (notably `t_flag_f`'s
  `-f`/`-Fs` command-file chain).
- The delete-or-convert line was drawn by asking whether an assertion can still
  fail: removing the load-bearing input (the define, the flag, the directory)
  must break the test, which was checked for the conversions that would
  otherwise have been vacuous.

## [S1] Problem

`nodist/test-regress.ps1` runs 1100 tests and reports `Failed 66`. The branch's
prune commit deleted 6760 `test_regress` files ("tests and tools that only build
or run a C++ model"), but the remaining suite was never run end to end on
Windows, so the residue was never seen. The 66 failures have two causes:

- 53 tests still call `test.compile()`, so the harness passes `--cc` (or
  `--exe`, `--build`, `--main`, `--CFLAGS`, `-no-MMD`), and the lint-only binary
  rejects the option: `%Error: Unsupported: this is a lint-only Verilator`.
- 13 tests fail for their own reasons: `t_flag_hier0_bad` (golden no longer
  matches the changed `--hierarchical-block` validation), `t_debug_width`
  (its golden expects an internal width error that no longer occurs),
  `t_order_wireloop` (greps `UNOPTFLAT`, produced by passes this build stops
  before), `t_stop_winos_bad` (`-no-MMD` removed), `t_hier_block` and
  `t_covergroup_inst_retire_asan` (`--CFLAGS` removed), and the five
  `t_sys_readmem_bad_*`/`t_sys_fread`-style tests whose goldens are runtime
  `$readmem`/`$fread` aborts that only a built model can produce.

The pruning is therefore incomplete in both directions the branch already
committed to: tests that only build or run a model are still present, and the
tests that remain were not adjusted for the options and passes this build no
longer has.

## [S2] Design

A failing test is **converted** to a lint test when its purpose is
verilation-side and the lint-only build still exercises it; otherwise it is
**deleted** together with the fixtures only it owns.

Converted tests (18). Each driver keeps its flags, goldens and `file_grep`
assertions but calls `test.lint(...)` instead of `test.compile(...)`, loses its
`test.execute()` and model-only flags, and is normalized to
`test.scenarios('vlt')`. Each was verified to lint cleanly (exit 0) except
`t_property_s_until_nfa`, whose two `test.lint()` runs still produce their
existing goldens:

- `t_EXAMPLE` (documented template; its `.v` is also a fixture for eleven other
  tests, seven of which this build skips for missing coroutines or tools),
  `t_gen_forif` (genvar for-if elaboration; referenced by comments in
  `src/V3LinkDot.cpp` and `t_lint_block_redecl_bad.v`), `t_lint_implicit`,
  `t_lint_noreturn`, `t_lint_paramnodefault`, `t_langext_1`, `t_langext_2`,
  `t_langext_3`, `t_opt_slice_element_limit` (its
  `Optimizations, Slice array skips due to size limit` statistic is 4 under
  lint), `t_pp_dupdef`, `t_property_s_until_nfa` (keep the two lint runs, drop
  the three compile runs and the simulation run), `t_var_rsvd_port`.
- `t_flag_f` (keeps `-f`/`-Fs` command-file parsing, including the missing-file
  `-Fs` case, which otherwise loses its only test), `t_interface_virtual` (keeps
  the positive half of the virtual-interface pair with
  `t_interface_virtual_bad`), `t_package_export` (keeps the positive
  `export` case of the package pair), `t_gen_missing` (keeps the
  generate-instantiation half of the `MODMISSING` pair), and
  `t_inside_unbounded_both` (keeps the `-Wno-INSIDETRUE` suppression check).
- `t_mod_topmodule__underunder` (keeps the only check that `--top-module`
  disambiguates between two top-level modules; without the flag the run fails
  with `MULTITOP`).

Restored and repaired (1): `t_flag_hier0_bad` was already a `test.lint()` test
whose golden had drifted; its `top_filename` now points at the surviving
`t/t_EXAMPLE.v`, its two option values that no longer produce an error were
dropped, and its golden was regenerated with `HARNESS_UPDATE_GOLDEN=1`. This
keeps the branch's only coverage of `--hierarchical-block` argument validation.

Deleted tests (47) and files (120). The driver, its golden and its `.v`/`.sv`/
`.cpp`/`.h`/`.mem`/`.dat`/`.vc` fixtures are removed, except where a surviving
tracked file mentions the basename. The sweep is run in both directions: a
source is also orphaned when the only files that mentioned it were deleted by
the same change, or when it was coupled to a driver by the default
`top_filename` naming convention rather than by text. That second direction
also removed the `t/t_config_libmap/` fixture directory and the
`TestSimulator.h`/`TestVpi.h` headers, whose only consumers were deleted.

That rule keeps these shared fixtures: `t_castdyn.v`,
`t_cover_fsm_policy_accept_multi.v`, `t_covergroup_clocked_sample.v`,
`t_covergroup_with.v`, `t_delay.v`, `t_flag_decoration.v`, `t_flag_main.v`,
`t_flag_parameter.v`, `t_flag_topmodule.v`, `t_foreach_noivar.v`,
`t_func_wide_out.v`, `t_gate_basic.v`, `t_gen_missing.v`,
`t_inside_unbounded_both.v`, `t_interface_virtual.v`, `t_multitop_sig.v`,
`t_package_export.v`, `t_param_concat.v`, `t_struct_init.v` and
`t_sys_psprintf.v`.

Deleting the `t/t_config_libmap/` directory does not disturb its last surviving
mention: `t_config_libmap_inc.map` still contains
`include ./t_config_libmap/lib.map;`, but `include` is an unimplemented reserved
config keyword, so the diagnostic comes from the lexer and the included file is
never opened. With the directory absent, `t_config_libmap_inc` passes
byte-identically.

Stale references created by the pruning are updated in the same change:

- `nodist/test-regress.ps1` usage example (`t/t_flag_f.py` -> a surviving test).
- `t_dist_copyright.py` exemption for the deleted `t/t_randsequence_svtests.v`.
- `t_dist_whitespace.py` exemption for the deleted
  `t/t_sys_file_basic_uz.dat`.
- `t_lint_width_bad.v` cross-reference to the deleted `t_math_width` points at
  the surviving `t_lint_width_genfor_bad` instead, keeping the line count so its
  golden stays valid.
- `t_interface_typo_bad.py`'s historical comment no longer quotes a deleted
  file's path.

Contracts: no `.v` is left without a driver unless a surviving test uses it; no
surviving test references a deleted file; `nodist/test-regress.ps1` finishes
with `Failed 0` and the same skip set; `--dist` keeps exactly the three
pre-existing failures.

## [S3] Out of Scope

- The ~30 tests that call `test.compile()` but are skipped here for missing
  coroutines, SystemC or a constraint solver (their scenario check runs first).
- The three `--dist` failures: `t_dist_contributors` (needs a human signature),
  `t_flag_version` (the branch version stamp omits "Verilator"), and
  `t_driver_timeout` (bash execution in this environment).
- Any production-code change: no `src/` behavior, option or pass pipeline
  changes, and no new driver flags.
- Replacing the runtime coverage the deleted simulation tests provided; that
  coverage cannot exist in a build that never emits a model.
- Accepted coverage losses, recorded rather than hidden: `t_debug_width`'s
  `--debug-width` invocation (its golden pinned an internal width error that no
  longer occurs, and the option itself is a debug aid); the two
  `--hierarchical-block` backslash cases that `t_flag_hier0_bad` used to feed to
  `V3Options` (the Windows harness rewrites `\` to `/`, so those values never
  reached the parser here, and the two `V3Options` error paths are left without
  a test); and the positive NFA/simulation half of `t_property_s_until_nfa` (it
  needs `--timing` and coroutines). The `-Fs` command-file case that `t_flag_f`
  used to cover at runtime is kept by its conversion, and the three orphaned
  goldens this change does not touch (`t_cover_line.out`, `t_sarif.sarif.out`,
  `t_sarif_clean.sarif.out`) were already orphaned before it.

## Tasks

- [x] T1: Convert the 18 listed tests to `test.lint()` with model-only parts
      removed, and restore `t_flag_hier0_bad` with a regenerated golden --
      acceptance: each passes individually under `nodist/test-regress.ps1`
      (covers: S2)
- [x] T2: Delete the 47 tests and their exclusively-owned fixtures, using the
      bidirectional orphan sweep -- acceptance: `git ls-files` no longer lists
      them, and no surviving file references a deleted file or leaves a source
      orphaned (covers: S2; depends: T1)
- [x] T3: Update the five stale references (ps1 example, two dist exemptions,
      two in-test cross-references) -- acceptance: none of them names a deleted
      file, and `t_dist_copyright`, `t_dist_whitespace` and
      `t_lint_width_bad` still pass (covers: S2; depends: T2)
- [x] T4: Run the full `--vlt` suite and the `--dist` suite -- acceptance:
      `Failed 0` in the suite with the skip set unchanged, and `--dist` shows
      the same three pre-existing failures (covers: S2; depends: T2, T3)
