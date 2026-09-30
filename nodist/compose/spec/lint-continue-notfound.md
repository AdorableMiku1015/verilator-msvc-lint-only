<!-- DESCRIPTION: Verilator: Feature document for lint continuation past unresolved names
     SPDX-FileCopyrightText: 2026-2026 Wilson Snyder
     SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0 -->

---
feature: lint-continue-notfound
status: delivered
updated: 2026-09-30
branch: msvc-lint-only
commits: 820348384..e2fd4fa0d
---

# Unresolved names do not stop lint

## Report

**What was built** -- Lint no longer stops at the first name the linker cannot
resolve. `V3ErrorCode::recoverableError()` now covers `PINNOTFOUND` as well as
`MODMISSING`, and a new `v3errorRecoverable()` macro marks an individual
code-less error as one the later passes tolerate, through a
`VErrorMessage::recoverable()` flag that `v3errorEndGuts()` counts next to
`incErrors()`. `V3LinkDot` uses it at the sites that report a reference it could
not resolve and marks what it could not resolve with `V3LinkDot::markUnresolved()`.
Before each tolerant gate, `V3LinkDot::repairUnresolved()` removes or replaces
those marked nodes by role: an assignment whose write target still holds the
unresolved reference is removed, a task call that is the whole of a statement is
dropped, and a marked reference used as an expression becomes a marked false
constant. A marked expression the repair cannot place stops the run where it
would have stopped before, so a name error can never turn into
`%Error: Internal Error`. Message text, severities, suppression rules, error
accounting and the exit status are unchanged, and a structural error (a parse
error) or a named error (`E_UNSUPPORTED`, ...) still stops the run as before.

`test_regress/t/t_lint_notfound_continue` covers both directions: the default run
proves that a missing scope/variable name, a missing task call, an assignment to
an unresolved target and a missing pin still let the later `CASEINCOMPLETE` and
`LATCH` checks report, and its `T_LINT_NOTFOUND_CONTINUE_MIXED` variant mixes in
`ZEROREPL` and proves those checks are skipped again. The lint-only note in
`docs/guide/exe_verilator.rst` and the `PINNOTFOUND` entry in
`docs/guide/warnings.rst` document the behavior.

**Verification** -- commands run from the repository root:

- `cmake --build --preset win64-release -j` (MSVC) -- PASS.
- `nodist/test-regress.ps1` (whole `--vlt` suite) -- PASSED 1006 / Failed 0 /
  Skipped 48.
- `nodist/test-regress.ps1 --dist` -- Passed 22 / Failed 3 / Skipped 3, the three
  PRE-EXISTING: `t_dist_contributors` (a human must edit `docs/CONTRIBUTORS`),
  `t_driver_timeout` (bash unavailable), `t_flag_version` (this branch's version
  stamp omits the word "Verilator").
- Scan of every fresh `test_regress/obj_vlt/*/vlt_compile.log` for an
  `Internal Error`/`internal fault` line that the test's own golden does not
  contain -- only `t_debug_sigsegv_bad`, the intentional crash test.
- Only three existing goldens changed, each verified additive (no line lost, by
  set-diff of message heads): `t_param_implicit_local_bad.out` (+4 `USERERROR`
  from elaboration-time `$error` checks now reached),
  `t_interface_mismodport_bad.out` (+1 `WIDTHEXPAND` on a sibling statement) and
  `t_vlt_match_error_2.out` (+1 `UNSUPPORTED` for `$readmemh`, which the earlier
  stop hid).
- Spot checks by direct invocation: unresolved names used in an expression, a
  port connection, a sensitivity list, a system-task argument, a generate-if
  condition and a hierarchical write all report and continue; blocks emptied by
  the repair still elaborate; 60 unresolved names end cleanly at
  `--error-limit=50`; a syntax error and an `E_UNSUPPORTED` error still stop the
  run.
- Independent review by a fresh subagent over `820348384..6d4279fb9` found no
  correctness bug; its five findings (one unverified criterion, one unverified
  branch, and three comment/naming nits) are all addressed in `5b84dc3f5`, and a
  separate re-review of that delta found no new correctness problem and only the
  two documentation-polish items fixed here, one of which is a comment rewording
  in `src/V3Error.cpp` with no behavior change.

**Journey log**:

- Blanket continuation was measured before it was rejected: classifying every
  code-less error recoverable changed 123 of 1005 tests, added
  `%Error: Internal Error` in 67 and hung `t_class_extends_rec_bad`. The scope
  decision comes from that measurement, not from taste.
- The narrow family was not automatically safe either: 10 of the 43 affected
  tests still gained internal errors, because the link pass left the unresolved
  node in the tree. The answer was a repair step with a fail-closed rule, not a
  wider recoverable set.
- The marker lives on a `FileLine`, so a pass that builds a statement around a
  placeholder inherits it: `V3Inst` gave the connection assignment and its
  wrapping `AstAlways` the placeholder's fileline, so the repair deleted a
  usable assignment and then stopped the run on the `AstAlways`. An unresolved
  name in a port connection is exactly the legacy shape this feature exists for;
  checking the write target itself, and treating a mark on a statement as
  inherited, fixed it.
- The harness creates a missing golden by copying the raw log, so a brand-new
  golden needs one more `HARNESS_UPDATE_GOLDEN` pass to be normalized
  (`?v=latest`); without it the test fails with a `Line 6 miscompares` that looks
  like a code bug.
- `t_dist_untracked` fails until the new test files are committed, so the
  `--dist` suite has to be re-run after committing, not before.

## [S1] Problem

Linting a legacy design stops at the first name the linker cannot resolve. Two
messages do this in practice: `Can't find definition of scope/variable: 'x'`
(reported by `V3LinkDot` as a code-less error) and `Pin not found: 'x'`
(`PINNOTFOUND`, a pretend error). Both are reported by the link stage, which is
followed by the strict `V3Error::abortIfErrors()` gates, so `V3WidthCommit`,
`V3Undriven`, `V3Case` and `V3Active` never run and their diagnostics are lost
even though the rest of the design resolved cleanly.

Extending continuation naively does not work; this was measured on the current
test suite by temporarily classifying errors as recoverable:

- Marking every code-less error (`EC_ERROR`) recoverable: 123 of 1005 tests
  changed, **67 of them gained a new `%Error: Internal Error`**, and
  `t_class_extends_rec_bad` never finished (infinite loop).
- Marking only the `Can't find definition of ...` family plus `PINNOTFOUND`:
  17 of the 43 affected tests changed, **10 of them gained a new
  `%Error: Internal Error`** (`V3Width.cpp:8111 Unlinked`,
  `V3Broken.cpp:280 No write refs on LHS of assignment`,
  `V3Task.cpp:249 Unlinked task`).

The internal errors have one cause: `V3LinkDot` reports that it cannot resolve a
name but leaves the unresolved node in the tree. Later passes then assert on it.
Continuing therefore requires the error site to leave a placeholder the later
passes can consume, which is also what the repository guidelines require of
error paths.

## [S2] Design

Lint continues past the name-resolution errors that can be made safe, and stops
exactly as today everywhere else.

Contracts:

- **Continuation set.** `V3ErrorCode::recoverableError()` gains `PINNOTFOUND`
  next to the existing `MODMISSING`. Name-resolution messages, which carry no
  error code, are marked individually at the site that reports them (below).
- **Per-message marking.** `VErrorMessage` carries a `recoverable` flag, reset
  by `VErrorMessage::init()`. `V3Error::v3errorPrepRecoverable(code)` prepares a
  message with the flag set, and a new `v3errorRecoverable(msg)` macro reports an
  `EC_ERROR` through it. `v3errorEndGuts()` counts a reported error as
  recoverable when `code.recoverableError() || message.recoverable()`, in the
  existing single place next to `incErrors()`. The flag travels with the message
  under the existing `V3ErrorGuarded` mutex and needs no other locking.
- **Marked sites.** `V3LinkDot` marks and reports, at one place per message, the
  name-resolution failures whose leftovers the repair step can place: `Can't find
  definition of <what>` (both forms of the `expectWhat` block, undotted and
  dotted), `Can't find definition of ... in dotted signal`, `Can't find varpin
  scope of ...`, `Can't find definition of task/function` and `Can't find
  definition of ... in dotted task/function`. Each of these sites replaces
  `v3error(...)` with `v3errorRecoverable(...)` and marks the node it failed to
  resolve with `markUnresolved(nodep)`, which gives that node -- or, when the
  reference is part of a write target, the enclosing assignment -- a copy of its
  `FileLine` with `erroringOn(true)`, the existing flag that means "created
  because of an earlier error". A marked parse reference passes the mark on:
  the resolved node that replaces it is built with `nodep->fileline()`, so the
  repair step sees the reference that is actually left in the tree.
  The messages that leave a node the repair cannot place stay fatal, because
  marking them would only move the abort earlier: `Cannot find file containing
  interface`, `Can't find typedef/interface`, `Can't find definition of
  clocking`, the unknown-PLI message, and the `findDotted()` failure in
  `visit(AstVarXRef)` -- the last one because the marked sites report a prefix
  that does not resolve before it is reached, so the shape of a tree repaired
  there is unproven.
- **Repair step.** `V3LinkDot::repairUnresolved(nodep)` walks the tree and fixes
  every marked node by role:
  - a marked assignment whose write target still holds the unresolved reference
    is removed, because it has nothing to assign to; an assignment that only
    inherited the mark -- a later pass building a statement around a placeholder
    copies its fileline -- keeps its usable target and is left alone;
  - a marked task call that is the whole of a statement (the `AstStmtExpr` that
    holds it, or the call itself when no statement expression wraps it) is
    removed from its statement list;
  - a marked parse reference, variable reference or call used as an expression is
    replaced by `AstConst{fl, AstConst::BitFalseErroring{}}`.
  Whether a marked reference is part of a write target is decided by walking up
  from it through dotted references and select or member selections -- the parts
  of one reference expression -- and asking whether the top of that expression is
  the left-hand side of an assignment.
  The dot-error placeholder in `V3LinkDot` switches from `BitFalse` to
  `BitFalseErroring`, so every placeholder this feature creates is marked and
  suppresses the cascade warnings the later passes would otherwise report about
  the fake value.
- **When the repair runs.** It is a no-op unless the run is actually going to
  continue: it runs only when at least one recoverable error was counted and no
  unrecoverable error was counted, so a run that is going to stop produces
  byte-identical output to today. It is called after `V3LinkDot::linkDotPrimary()`
  (so the passes between it and the next gate see a repaired tree), just before
  the tolerant gate after `V3LinkInc::linkIncrements()`, just before the tolerant
  gate after `V3LinkWith::linkWith()` and just before the tolerant gate after
  `V3LinkDot::linkDotScope()`.
- **Fail closed.** If the repair step meets a marked expression it cannot place,
  it aborts the run at that point, which is the same clean `%Error: Exiting due
  to ...` stop as today. A run that cannot be repaired never reaches the later
  passes, so this feature cannot turn a name error into `%Error: Internal
  Error`; the cost is that such a run stops before the passes that would have
  reported further diagnostics, not that it reports a wrong one. A mark that
  arrives on a statement is not a hole the repair has to fill, so it does not
  make the run stop.
- **Gates.** The link-stage gates that currently call `abortIfErrors()` after
  `V3LinkInc::linkIncrements()` and after `V3LinkWith::linkWith()` switch to
  `abortIfUnrecoverableErrors()`, joining the three tolerant gates that already
  exist after `V3Width::width()`, after `V3Dead::deadifyDTypes()` and after
  `V3LinkDot::linkDotScope()`. Every other gate stays strict, including the
  parse gate in `V3Global::readFiles()`, the link-cells gates, the covergroup
  gate and the final gates in `verilate()`.
- **Unchanged externals.** No message text, severity, code or suppression rule
  changes: marked messages still print the plain `%Error:` prefix and stay
  unsuppressible like every other code-less error, `PINNOTFOUND` stays
  waivable with `-Wno-PINNOTFOUND`, `errorCount()` still counts every error and
  the run still exits non-zero with `%Error: Exiting due to <n> error(s)`.
  `--error-limit` still ends a run whose error count reaches the limit.
- **Structural errors still stop the run.** A parse error aborts at the strict
  parse gate before the link stage, and any named error (`E_UNSUPPORTED`,
  `E_NOTIMING`, `E_PORTSHORT`, ...) keeps `hasUnrecoverableErrors()` true, so the
  next tolerant gate aborts it exactly as today.

Data flow: a link site reports through `v3errorRecoverable()` and marks its node
-> `v3errorEndGuts()` counts it in both `m_errCount` and `m_recoverableErrCount`
-> `repairUnresolved()` replaces or removes the marked nodes and fails closed if
it cannot -> the tolerant gate asks `hasUnrecoverableErrors()` and continues past
recoverable-only errors.

Testing boundary: one regression test proves that a missing scope/variable, a
missing task and a missing pin, together with an assignment whose target does not
resolve, all report and still let the later case/latch checks run in the same
lint run, with a non-zero exit status; its mixed variant adds a non-recoverable
error and proves those later checks are skipped again. The whole `--vlt` suite
proves no test gains an internal error, a hang or a changed golden other than the
ones whose run now legitimately reports more.

## [S3] Out of Scope

- Continuing past named errors, in particular `E_UNSUPPORTED` ("Unsupported:"
  messages) and the `E_*` errors generally; the measured cost of blank
  continuation is internal errors and a hang.
- Continuing past every code-less error; that is the measured 67-internal-error
  case above.
- The name-resolution sites outside `V3LinkDot`, notably `V3Width`'s
  "Member ... not found in ..." family and `V3LinkCells`'s import/export
  failures; they can be marked in a follow-up once their tree shapes are
  repaired.
- Changing the pipeline endpoint (`V3Delayed`) of this lint-only build.
- Changing message wording, error codes, severities, suppression or exit status.
- Creating a linked worktree: this change is made in the current checkout on
  branch `msvc-lint-only`, per the workspace decision already taken.

## Tasks

- [x] T1: Add the per-message recoverable contract to `src/V3Error.{h,cpp}`:
      the `VErrorMessage::recoverable()` flag, `v3errorPrepRecoverable()`, the
      `v3errorRecoverable()` macro and the OR in `v3errorEndGuts()` --
      acceptance: the MSVC build succeeds and the `--vlt` suite is unchanged
      because nothing is marked yet (covers: S2)
- [x] T2: Add `PINNOTFOUND` to `recoverableError()` and switch the two
      link-stage gates in `src/Verilator.cpp` to
      `abortIfUnrecoverableErrors()` -- acceptance: a lint run whose only error
      is a missing pin still reports the later case/latch warnings and exits 1,
      and `t_lint_pinnotfound_bad`, `t_lint_pindup_bad`, `t_udp_bad` keep
      passing with no internal error (covers: S2; depends: T1)
- [x] T3: Mark the `V3LinkDot` name-resolution sites with
      `v3errorRecoverable()`/`markUnresolved()` and add
      `V3LinkDot::repairUnresolved()` with its role-based rules, the
      fail-closed check and the placeholder change to `BitFalseErroring` --
      acceptance: the 43 suite tests whose goldens contain the family show no
      `%Error: Internal Error` and no hang; sites the repair cannot place still
      stop the run exactly as before (covers: S2; depends: T2)
- [x] T4: Add regression test `test_regress/t/t_lint_notfound_continue`
      (`.v`, `.py`, goldens regenerated with `HARNESS_UPDATE_GOLDEN=1`) --
      acceptance: the default lint run reports the missing scope/variable, the
      missing task and the missing pin, then the later `CASEINCOMPLETE`/`LATCH`
      warnings, and exits non-zero; the mixed variant adds a non-recoverable
      error and shows those later warnings are absent (covers: S2; depends: T3)
- [x] T5: Reconcile the goldens the continuation legitimately changes and run
      the whole suite -- acceptance: `nodist/test-regress.ps1` passes with only
      the intended goldens regenerated, `nodist/test-regress.ps1 --dist` keeps
      its three PRE-EXISTING failures, and a scan of every `obj_vlt` log finds
      no new `Internal Error`, no `internal fault` and no hang (covers: S2;
      depends: T4)
- [x] T6: Document the behavior in `docs/guide/exe_verilator.rst` (lint-only
      note) and `docs/guide/warnings.rst` (`PINNOTFOUND`, `MODMISSING`) --
      acceptance: ASCII-only, rewrapped prose, `t_dist_docs_warnings` passes
      (covers: S2; depends: T3)
