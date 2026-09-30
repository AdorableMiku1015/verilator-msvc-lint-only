<!-- DESCRIPTION: Verilator: Feature document for MODMISSING lint continuation
     SPDX-FileCopyrightText: 2026-2026 Wilson Snyder
     SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0 -->

---
feature: modmissing-lint-continue
status: in-progress
updated: 2026-09-30
branch: msvc-lint-only
commits:
---

# Missing module does not stop lint

## Report

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

- [ ] T1: Add the recoverable-error contract to `src/V3Error.{h,cpp}`:
      `recoverableError()`, the separate counter, `recoverableErrorCount()` and
      `abortIfErrorsExceptRecoverable()` -- acceptance: the CMake MSVC build
      succeeds and no strict `abortIfErrors()` call site changes (covers: S2)
- [ ] T2: Use the tolerant gate at the three post-elaboration abort points in
      `src/Verilator.cpp` -- acceptance: a lint run of a design with an unknown
      module plus an incomplete `always_comb` case prints `%Error-MODMISSING`,
      then `CASEINCOMPLETE`/`LATCH`, and exits 1 (covers: S2; depends: T1)
- [ ] T3: Add regression test `test_regress/t/t_lint_modmissing_continue`
      (`.v`, `.py`, golden `.out` regenerated with `HARNESS_UPDATE_GOLDEN=1`)
      -- acceptance: the test fails on the pre-fix binary because `LATCH` is
      absent, passes after the fix, and its mixed variant (missing module plus
      `ZEROREPL`) shows `CASEINCOMPLETE`/`LATCH` are still skipped (covers: S2;
      depends: T2)
- [ ] T4: Re-run and, where output legitimately changed, regenerate the existing
      `MODMISSING` goldens (`t_gen_missing_bad`, `t_gen_nonconst_bad`,
      `t_inst_long_bad`, `t_package_dup_bad2`) -- acceptance: those tests pass
      with the fixed binary and no golden records a crash or internal error
      (covers: S2; depends: T2)
- [ ] T5: Run the lint regression suite and the relevant `--dist` tests --
      acceptance: recorded command output with pass counts and no new failures
      (covers: S2; depends: T3, T4)
