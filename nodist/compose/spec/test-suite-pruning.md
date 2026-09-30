<!-- DESCRIPTION: Verilator: Feature document for completing the lint-only test pruning
     SPDX-FileCopyrightText: 2026-2026 Wilson Snyder
     SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0 -->

---
feature: test-suite-pruning
status: in-progress
updated: 2026-09-30
branch: msvc-lint-only
commits:
---

# Complete the lint-only test-suite pruning

## Report

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

Converted tests (12). Each driver keeps its flags, goldens and `file_grep`
assertions but calls `test.lint(...)` instead of `test.compile(...)`, loses its
`test.execute()` and model-only flags, and is normalized to
`test.scenarios('vlt')`. Each was verified to lint cleanly (exit 0) except
`t_property_s_until_nfa`, whose two `test.lint()` runs still produce their
existing goldens:

- `t_EXAMPLE` (documented template; its `.v` is also a fixture for ten other
  tests), `t_gen_forif` (genvar for-if elaboration; referenced by comments in
  `src/V3LinkDot.cpp` and `t_lint_block_redecl_bad.v`), `t_lint_implicit`,
  `t_lint_noreturn`, `t_lint_paramnodefault`, `t_langext_1`, `t_langext_2`,
  `t_langext_3`, `t_opt_slice_element_limit` (its
  `Optimizations, Slice array skips due to size limit` statistic is 4 under
  lint), `t_pp_dupdef`, `t_property_s_until_nfa` (keep the two lint runs, drop
  the three compile runs and the simulation run), `t_var_rsvd_port`.

Deleted tests (54) and files (116). The driver, its golden and its `.v`/`.sv`/
`.cpp`/`.pli`/`.mem`/`.dat`/`.vc` fixtures are removed, except where a surviving
tracked file mentions the basename; that rule keeps these shared fixtures:
`t_castdyn.v`, `t_cover_fsm_policy_accept_multi.v`,
`t_covergroup_clocked_sample.v`, `t_covergroup_with.v`, `t_delay.v`,
`t_dpi_accessors.v`, `t_flag_decoration.v`, `t_flag_main.v`,
`t_flag_parameter.v`, `t_flag_topmodule.v`, `t_foreach_noivar.v`,
`t_func_wide_out.v`, `t_gate_basic.v`, `t_gen_missing.v`,
`t_inside_unbounded_both.v`, `t_interface_virtual.v`, `t_multitop_sig.v`,
`t_order_wireloop.v`, `t_package_export.v`, `t_param_concat.v`,
`t_struct_init.v`, `t_sys_psprintf.v`, and the whole `t/t_config_libmap/`
fixture directory, which the surviving `t_config_libmap_inc` test includes.

Stale references created by the pruning are updated in the same change:

- `nodist/test-regress.ps1` usage example (`t/t_flag_f.py` -> a surviving test).
- `t_dist_copyright.py` exemption for the deleted `t/t_randsequence_svtests.v`.
- `t_dist_whitespace.py` exemption for the deleted
  `t/t_sys_file_basic_uz.dat`.

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

## Tasks

- [ ] T1: Convert the 12 listed tests to `test.lint()` with model-only parts
      removed -- acceptance: each passes individually under
      `nodist/test-regress.ps1` (covers: S2)
- [ ] T2: Delete the 54 tests and their exclusively-owned fixtures -- acceptance:
      `git ls-files` no longer lists them and a word-boundary scan finds no
      reference from a surviving tracked file (covers: S2; depends: T1)
- [ ] T3: Update the three stale references (ps1 example, two dist exemptions)
      -- acceptance: none of them names a deleted file, and `t_dist_copyright`
      and `t_dist_whitespace` still pass (covers: S2; depends: T2)
- [ ] T4: Run the full `--vlt` suite and the `--dist` suite -- acceptance:
      `Failed 0` in the suite with the skip set unchanged, and `--dist` shows
      the same three pre-existing failures (covers: S2; depends: T2, T3)
