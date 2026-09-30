<!-- DESCRIPTION: Verilator: Feature document for MODMISSING lint continuation
     SPDX-FileCopyrightText: 2026-2026 Wilson Snyder
     SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0 -->

---
feature: modmissing-lint-continue
status: delivered
updated: 2026-09-30
branch: msvc-lint-only
commits: 0572ed9f5..cfc1d227f
---

# Missing module does not stop lint

## Report

**What was built** -- `MODMISSING` is now classified by
`V3ErrorCode::recoverableError()` (`src/V3Error.h`), and the recoverable errors
are counted separately in `V3ErrorGuarded::m_recoverableErrCount`, incremented
in `v3errorEndGuts()` next to `incErrors()`. `V3Error::hasUnrecoverableErrors()`
reads both counts under one lock, and `V3Error::abortIfUnrecoverableErrors()`
uses it at the three post-elaboration abort points in `src/Verilator.cpp`
(after `V3Width::width()`, after `V3Dead::deadifyDTypes()` and after
`V3LinkDot::linkDotScope()`). A missing module therefore no longer cuts lint
short: the later passes still report `BLKSEQ`, `UNDRIVEN`, `CASEINCOMPLETE`,
`LATCH` and the rest, while `MODMISSING` stays an error, `errorCount()` still
includes it, the final `abortIfWarnings()` still exits 1, `-Wno-MODMISSING` is
unchanged, and any error the later passes cannot tolerate still stops the
pipeline at the first tolerant gate.

`test_regress/t/t_lint_modmissing_continue` covers both directions: the default
run proves the case and latch warnings are emitted after `%Error-MODMISSING`,
and its `T_LINT_MODMISSING_CONTINUE_MIXED` variant mixes in `ZEROREPL` and
proves those warnings are still skipped. `docs/guide/warnings.rst` records that
linting continues after the error. A separate commit fixes
`test_regress/driver.py`, which could not enumerate any test on Windows because
`glob.glob()` returns backslash separators.

**Verification** -- commands run from the repository root:

- `cmake --build --preset win64-release -j` (MSVC) -- PASS, 118 targets, exit 0.
- `nodist/test-regress.ps1 t/t_lint_modmissing_continue.py t/t_gen_missing_bad.py
  t/t_gen_nonconst_bad.py t/t_inst_long_bad.py t/t_package_dup_bad2.py` --
  PASS 5, FAIL 0; the four existing MODMISSING goldens pass unmodified.
- Same new test against a saved copy of the pre-fix binary -- FAIL
  (`Line 15 miscompares`), so the test does catch the bug.
- `nodist/test-regress.ps1` (whole `--vlt` suite, 1100 tests) -- post-fix
  Passed 986 / Failed 66 / Skipped 48, pre-fix baseline Passed 985 / Failed 67
  / Skipped 48; the failure sets differ by exactly the new test and nothing
  else. The 66 remaining failures are PRE-EXISTING (`t_order_wireloop` needs
  UNOPTFLAT from after `V3Delayed`; the rest need `--cc`/`--sc`, SystemC or
  coroutines this lint-only build does not provide).
- `nodist/test-regress.ps1 --dist` -- Passed 22 / Failed 3 / Skipped 3,
  identical with the pre-fix binary. PRE-EXISTING failures:
  `t_dist_contributors` (asks the human author to certify in
  `docs/CONTRIBUTORS`), `t_flag_version` (this branch's version stamp omits the
  word "Verilator"), `t_driver_timeout` (bash exec fails here).
- Independent review by a fresh subagent: no critical findings; it reproduced
  the continuation and the non-recoverable abort, checked `-Wno-MODMISSING`,
  `--error-limit`, the strict gates, `--verilate-jobs 4` and twelve design
  shapes under `--debug-check`.

**Journey log**:

- The first version compared the two counts through two separate locked
  accessors; it was refactored before review to a single-lock
  `hasUnrecoverableErrors()` plus `abortIfUnrecoverableErrors()`, so the two
  counts can never be read inconsistently.
- Running the whole suite was impossible at first: `driver.py` enumerated tests
  with `glob.glob()` and Windows backslashes broke the derived test name, so
  every enumerated test failed to find its directory. The one-line
  normalization fix is its own commit; it then exposed 66 pre-existing
  failures in the branch that no-arg runs had never reached.
- Baselines were taken by swapping a saved copy of the pre-fix binary into
  `bin/`, which gives an exact before/after comparison for both suites without
  a second full build.
- `-Wno-MODMISSING` was the cheap oracle for the intended behaviour: it already
  let the pipeline run past the placeholder cell, so it showed what full lint
  should print.
- One `--dist` failure needs a human: `t_dist_contributors` asks the author to
  sorted-insert themselves into `docs/CONTRIBUTORS`, which agents must not edit.

## [S1] Problem

An instance of a module Verilator cannot find is reported by
`V3Width::visit(AstCell)` as `v3warn(MODMISSING, ...)`, but `MODMISSING` is a
"pretend error" (`V3ErrorCode::pretendError()`), so the diagnostic is printed as
`%Error-MODMISSING:` and increments the error counter.

The first `V3Error::abortIfErrors()` after that point is the gate immediately
following `V3Width::width()` in `Verilator.cpp`; it calls `abortIfWarnings()`
and terminates the process. Every later pass is skipped, and in this lint-only
tree most lint checks live there, for example `V3WidthCommit`
(`BLKSEQ`, `ALWNEVER`, `FUNCTIMECTL`, `ENCAPSULATED`), `V3Undriven`
(`UNDRIVEN`, `UNUSEDSIGNAL`, `UNUSEDPARAM`, `UNUSEDGENVAR`, `MULTIDRIVEN`),
`V3Case` (`CASEINCOMPLETE`, `CASEOVERLAP`) and `V3Active` (`LATCH`, `NOLATCH`).

Measured before the fix on a design whose top has an incomplete `always_comb`
case plus one unknown module instance: without the unknown module the run
reports `CASEINCOMPLETE` and `LATCH`; with it the run reports only
`PINMISSING`/`MODMISSING` and stops right after `V3Width`.

## [S2] Design

An error that leaves the AST usable must not stop the remaining passes; every
other error keeps the existing immediate-abort behavior.

Contracts:

- `V3ErrorCode::recoverableError()` is a semantic predicate over error codes:
  true when the reported error leaves a tree the later passes can still
  traverse. Only `MODMISSING` qualifies today, because the unresolved cell keeps
  a well-formed `AstNotFoundModule` placeholder, which was verified by running
  the pipeline with the diagnostic suppressed (`-Wno-MODMISSING`).
- `V3ErrorGuarded` counts recoverable errors in a member separate from
  `m_errCount`. The counter is incremented in `v3errorEndGuts()` next to
  `incErrors()`, so it covers only errors actually reported as diagnostics;
  direct `V3Error::incErrors()` callers (`V3Broken`, `V3Slice`) are untouched.
- `V3ErrorGuarded::hasUnrecoverableErrors()`, reached through
  `V3Error::hasUnrecoverableErrors()`, reports whether any counted error is not
  recoverable; both counts are read under one lock so the two cannot be
  observed inconsistently.
- `V3Error::abortIfUnrecoverableErrors()` runs the same `abortIfWarnings()`
  abort, but only when `hasUnrecoverableErrors()` is true.
- The three post-elaboration gates in `Verilator.cpp` -- after
  `V3Width::width()`, after `V3Dead::deadifyDTypes()` and after
  `V3LinkDot::linkDotScope()` -- switch to that tolerant variant so lint can
  reach `V3Case`/`V3Active`. All other `abortIfErrors()` sites stay strict.
- Error accounting and exit status do not change: `MODMISSING` is still an
  error, `errorCount()` still includes it, and the final
  `abortIfErrors()`/`abortIfWarnings()` pair in `verilate()` still exits 1 with
  `%Error: Exiting due to <n> error(s)`. Suppressing the code with
  `-Wno-MODMISSING` behaves exactly as before.
- An unusable tree still stops early: any non-recoverable `v3error` aborts at
  the first tolerant gate exactly as it aborts today.

Data flow: report -> `v3errorEndGuts()` -> `incErrors()` plus, for a recoverable
code, `++m_recoverableErrCount` -> each tolerant gate asks
`hasUnrecoverableErrors()` before deciding to abort.

Testing boundary: one regression test proves that `V3Case` and `V3Active`
diagnostics are emitted after `%Error-MODMISSING` in a single lint run and that
the run still fails with a non-zero exit status; a second lint run of the same
source mixes the missing module with `ZEROREPL` and proves that a
non-recoverable error still stops those passes.

## [S3] Out of Scope

- Changing the severity, text, or suppression semantics of `MODMISSING`.
- Changing which errors `V3Width` reports, or when it reports them.
- Changing the strict `abortIfErrors()` at the parse/link gates
  (`V3Global::readFiles`, `V3LinkCells`) or the internal consistency gate in
  `V3Covergroup`; `MODMISSING` cannot be counted before `V3Width` runs, so those
  gates are unaffected either way.
- Extending the pass pipeline beyond `V3Delayed`, the endpoint of this
  lint-only build.

## Tasks

- [x] T1: Add the recoverable-error contract to `src/V3Error.{h,cpp}`:
      `recoverableError()`, the separate counter, `hasUnrecoverableErrors()` and
      `abortIfUnrecoverableErrors()` -- acceptance: the CMake MSVC build
      succeeds and no strict `abortIfErrors()` call site changes (covers: S2)
- [x] T2: Use the tolerant gate at the three post-elaboration abort points in
      `src/Verilator.cpp` -- acceptance: a lint run of a design with an unknown
      module plus an incomplete `always_comb` case prints `%Error-MODMISSING`,
      then `CASEINCOMPLETE`/`LATCH`, and exits 1 (covers: S2; depends: T1)
- [x] T3: Add regression test `test_regress/t/t_lint_modmissing_continue`
      (`.v`, `.py`, golden `.out` regenerated with `HARNESS_UPDATE_GOLDEN=1`)
      -- acceptance: the test fails on the pre-fix binary because `LATCH` is
      absent, passes after the fix, and its mixed variant (missing module plus
      `ZEROREPL`) shows `CASEINCOMPLETE`/`LATCH` are still skipped (covers: S2;
      depends: T2)
- [x] T4: Re-run and, where output legitimately changed, regenerate the existing
      `MODMISSING` goldens (`t_gen_missing_bad`, `t_gen_nonconst_bad`,
      `t_inst_long_bad`, `t_package_dup_bad2`) -- acceptance: those tests pass
      with the fixed binary and no golden records a crash or internal error
      (covers: S2; depends: T2)
- [x] T5: Run the lint regression suite and the relevant `--dist` tests --
      acceptance: recorded command output with pass counts and no new failures
      (covers: S2; depends: T3, T4)
