<!-- DESCRIPTION: Verilator: Feature document for lint continuation past unresolved names
     SPDX-FileCopyrightText: 2026-2026 Wilson Snyder
     SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0 -->

---
feature: lint-continue-notfound
status: delivered
updated: 2026-09-30
branch: msvc-lint-only
commits: 9258db075..c96a13b26
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
`V3LinkDot::repairUnresolved()` removes or replaces those marked nodes by role: an
assignment whose write target did not resolve is removed, a task call that is the
whole of a statement is dropped, and a marked reference used as an expression
becomes a marked false constant. A marked node the repair cannot place stops the
run where it would have stopped before, so a name error can never turn into
`%Error: Internal Error`. Message text, severities, suppression rules, error
accounting and the exit status are unchanged, and a structural error (a parse
error) or a named error (`E_UNSUPPORTED`, ...) still stops the run as before.

The mark is one bit on the node itself (`AstNode`'s attribute word, next to
`didWidth`, `doingWidth` and `protect`), not a copied `FileLine` carrying
`erroringOn`. Because no pass can inherit the node flag from a fileline, the repair
classifies instead of guessing: a marked assignment is removed, a marked reference
is replaced or dropped, and any other marked node means the mark was placed
wrongly and stops the run. The write-target scan, the `AstConst` exemption and the
"only a marked expression is unrepairable" rule are gone with the guessing, and
`markUnresolved()` now walks to the innermost enclosing assignment and asks
whether the reference sits under its left side, instead of stopping at a node that
is not a select, member selection or dot.

Repairing belongs to the pass that marks: `repairUnresolved()` runs at the end of
`linkDotPrimary()`, `linkDotParamed()` and `linkDotScope()`, the only three callers
of the link guts, so a mark's lifetime is one link invocation and no other pass
ever meets a marked node. The four pipeline-level calls in `Verilator.cpp` are
gone; the tolerant gates stay where they are.

Two consequences beyond the refactor. The corrected walk fixes shapes the old one
got wrong: an assignment to a name that does not resolve, blocking or nonblocking,
used to be kept and then stopped the run on a cascade `PROCASSWIRE` error, and a
write target whose index does not resolve used to be kept and warned
`WIDTHEXPAND` about the stand-in index; all of them now drop the assignment and
continue to the later warnings. Both are covered in `t_lint_notfound_continue`.
Also, deleting the `visit(AstParseRef*)` repair branch first came back green from
the whole suite and still broke the `disable` statement, which leaves a parse
reference in the tree because its target names a block; the branch is back, with
the two `disable` targets covered in the same test.

`test_regress/t/t_lint_notfound_continue` covers both directions: the default run
proves that a missing scope/variable name, a missing task call, an assignment to
an unresolved target and a missing pin still let the later `CASEINCOMPLETE` and
`LATCH` checks report, and its `T_LINT_NOTFOUND_CONTINUE_MIXED` variant mixes in
`ZEROREPL` and proves those checks are skipped again. The lint-only note in
`docs/guide/exe_verilator.rst` and the `PINNOTFOUND` entry in
`docs/guide/warnings.rst` document the behavior.

**Verification** -- commands run from the repository root:

- `cmd /c C:\Users\Miku\AppData\Local\Temp\build-verilator.bat` (MSVC, preset
  `win64-release`) -- PASS.
- `nodist/test-regress.ps1` (whole `--vlt` suite) -- PASSED 1006 / Failed 0 /
  Skipped 48, the same counts as the delivered head; the only goldens that changed
  are the two of `t_lint_notfound_continue`, which this amendment extends.
- `nodist/test-regress.ps1 --dist` -- Passed 22 / Failed 3 / Skipped 3, the three
  PRE-EXISTING: `t_dist_contributors` (a human must edit `docs/CONTRIBUTORS`),
  `t_driver_timeout` (bash unavailable), `t_flag_version` (this branch's version
  stamp omits the word "Verilator").
- Delivered binary against the built one over 45 probe files, each run with
  `--error-limit 0` and with the default limit (90 runs): 82 byte-identical, and
  the 8 differences are 4 probes where the new code continues further -- a
  blocking or nonblocking assignment to an undeclared name now reaches the later
  `CASEINCOMPLETE`/`LATCH` warnings instead of stopping on a cascade
  `PROCASSWIRE` error, and a write target with an unresolved index, plain or
  nested, now drops the assignment instead of leaving it and warning
  `WIDTHEXPAND` about the 1-bit stand-in index. The two `disable` probes are
  identical again once the parse-reference branch is restored.
- Scan of every fresh `test_regress/obj_vlt/*/vlt_compile.log` for an
  `Internal Error`/`internal fault` line that the test's own golden does not
  contain, with the compiler paths normalized for the comparison -- 0 unexpected.
  The 10 tests that contain one expect it in their golden, including the
  intentional `t_debug_sigsegv_bad`.
- The fail-closed path was observed rather than argued: while the parse-reference
  branch was deleted, the `disable` probe stopped with
  `%Error: Exiting due to N error(s)` and no internal error, which is exactly the
  stop the delivered code produced for an unrepairable tree.
- `git diff --check` -- clean, and every changed file is ASCII-only.
  `nodist/verilog_format` could not run (it is a shell script needing bash and
  verible, the same missing bash that fails `t_driver_timeout`) -- PRE-EXISTING
  environment gap.
- Only three other goldens changed in the delivered round, each verified additive
  (no line lost, by set-diff of message heads): `t_param_implicit_local_bad.out`
  (+4 `USERERROR` from elaboration-time `$error` checks now reached),
  `t_interface_mismodport_bad.out` (+1 `WIDTHEXPAND` on a sibling statement) and
  `t_vlt_match_error_2.out` (+1 `UNSUPPORTED` for `$readmemh`, which the earlier
  stop hid).
- The mark is visible where it matters for debugging: `--dumpi-tree 7` on a probe
  with an unresolved name produces a `linkdot` tree whose reference carries
  `[UNRESOLVED]`, which is a dump taken before the repair runs.
- Independent review by a fresh subagent over `9258db075..5caa8bc49` found no
  correctness bug: every T7/T8/T10/T11 criterion met, no shape where the exact
  classification stops a run the delivered code continued, no unmarked hole (the
  only `replaceWith` reachable after a mark is the implicit net, which copies it),
  the walk cannot mark an assignment for a reference outside its write target, and
  the style matches the neighbouring flags. Two notes came out of it, and both are
  settled: the mark is now printed by `AstNode::dump()` (the pass dumps its tree
  before the repair runs), and the fail-closed guard stays unreachable-by-
  construction, which `src/AGENTS.md` allows when the code justifies it. A bounded
  delta re-review of `5caa8bc49..c96a13b26` confirmed both, found nothing to fix,
  and agreed that a `dumpJson` entry would be the inconsistent choice for
  pass-internal scratch state. Two earlier review attempts stalled without
  producing a report and were stopped; the reports above come from the two that
  finished.

**Journey log**:

- Blanket continuation was measured before it was rejected, and so was the narrow
  family: classifying every code-less error recoverable changed 123 of 1005 tests,
  added `%Error: Internal Error` in 67 and hung `t_class_extends_rec_bad`, while
  the missing-name family plus `PINNOTFOUND` alone still gained internal errors in
  10 of the 43 affected tests, because the link left the unresolved node in the
  tree. The answer was a repair step with a fail-closed rule, not a wider
  recoverable set.
- The fileline mark was load-bearing in a way the code did not say: a pass that
  builds a statement around a placeholder inherits it, so `V3Inst` gave a port
  connection's assignment and its wrapping `AstAlways` the placeholder's fileline
  and the repair deleted a usable assignment. The node flag removes that class of
  false positive, but it also removes inheritance, so a reference that stands in
  for an unresolved name has to copy the flag explicitly -- without it the
  `generate if (missing_var)` shape stopped on a cascade const-evaluation error
  that the delivered code hid.
- The suite is not the arbiter of reachability. Deleting the parse-reference
  repair branch left all 1006 tests green, yet the branch was needed: the target
  of a `disable` names a block, so the link reports it with implicit net creation
  off and leaves the parse reference in the tree for the repair. A 45-file probe
  set found it; the test now covers it.
- Marking a hole is not enough: the write-target walk must reach the assignment
  through any node. Stopping at a node that is not a select, member selection or
  dot meant `x = a;` on an undeclared name kept the assignment, got an implicit
  net, and stopped the run on a cascade `PROCASSWIRE` error instead of continuing.
- The harness creates a missing golden by copying the raw log, so a golden needs a
  second `HARNESS_UPDATE_GOLDEN` pass to be normalized (`?v=latest`); without it
  the test fails with a `Line N miscompares` that looks like a code bug.

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

## [S4] The mark belongs to the node, and the pass that marks repairs

Amendment: the delivered design works, but its marker and its ownership are
imprecise in ways that were papered over with positional guesses. This section
replaces the marker and moves the repair. One user-visible behavior change comes
with it, the write-target walk below, and T10 covers it.

Contracts:

- **The mark is a node flag, not a fileline flag.** One bit in `AstNode`'s
  existing attribute word (the reserved bits of `m_flags`, next to `didWidth`,
  `doingWidth` and `protect`) carries it, with `unresolved()`/`unresolved(bool)`
  accessors; the implicit copy constructor already copies the word, so a clone
  keeps the mark. The attribute word needs no `dumpJson` entry, following the
  existing flags; `AstNode::dump()` prints the mark as `[UNRESOLVED]` beside the
  existing `[ERRORING]`, because the pass dumps its tree before the repair runs
  and a marked node would otherwise look ordinary in that dump. `markUnresolved()`
  sets the flag on the node it picked -- the enclosing assignment when the
  reference is part of a write target, the reference otherwise -- and no longer
  copies a `FileLine`. The `erroringOn` copy survives only where it does a
  different job: on the `BitFalseErroring` placeholder that replaces a reference,
  where it is what suppresses the cascade complaints of the later passes.
- **Why.** `erroringOn` is a warning-suppression flag with fourteen other
  producers (`AstConst::BitFalseErroring`) and two consumers, and any pass that
  builds a statement around a placeholder copies the placeholder's `FileLine`, so
  a mark arrives on nodes that are not holes. The repair then had to guess by
  position whether a marked node was a hole (`writeTargetUnresolved()`, the
  `AstConst` exemption, "only a marked expression is unrepairable"), and that
  guess failed closed on a usable assignment built around a placeholder for a
  port connection -- the shape this feature exists for. The node flag is set only
  by the mark sites, or copied onto a node that stands in for one of them, so the
  repair classifies instead of guessing: a marked assignment is removed, a marked
  reference is replaced or dropped by role, and any other marked node is a
  violated contract that stops the run. (A clone copies the flag, which is what
  makes a cloned hole repairable too.)
- **Mark lifetime is one link invocation.** A mark is set only by the
  name-resolution sites inside `linkDotGuts()`, which is called only by
  `linkDotPrimary()`, `linkDotParamed()` and `linkDotScope()`. The repair runs at
  the end of those three functions, so no other pass ever sees a marked node and
  the gap in which a marked node could be transformed is gone. The four
  pipeline-level calls in `Verilator.cpp` are removed; the tolerant gates remain
  where they are, because which errors stop a run is pipeline policy, not a link
  concern.
- **The write-target walk reaches the assignment through any node.** Deciding
  whether a reference is part of a write target no longer stops at a node that is
  not a select, member selection or dot: it walks to the innermost enclosing
  assignment and asks whether the reference sits under its left side. That is what
  the delivered walk got wrong in the shapes a comparison over 45 probe files
  found: an assignment to a name that does not resolve, blocking or nonblocking,
  used to keep the assignment, after which the implicitly created net made the run
  stop on a cascade `PROCASSWIRE` error instead of continuing; a write target
  whose index does not resolve, `vec[i] = a;`, used to keep the assignment and
  report a spurious `WIDTHEXPAND` about the stand-in index. All of them now drop
  the assignment and continue, which is the one user-visible change of this
  amendment.
- **A reference that stands in for an unresolved name keeps the mark.** When the
  link replaces the reported reference with an implicitly created net
  (`checkImplicit`), the replacement is the same unresolved name in another
  shape, so it carries the mark; otherwise the repair would never see the hole
  and the run would stop later on a cascade error that the stand-in used to hide.
- **The `AstParseRef` branch is reachable, and stays.** Deleting it left the
  `--vlt` suite green, which turned out to be a false negative: the shape is the
  target of a `disable`, which names a block, so implicit net creation is off and
  the link reports the reference without replacing it. A marked parse reference
  that reaches the repair is then a real hole, and without the branch the repair
  stops the run on it where the delivered code continued to the later warnings.
  The branch is restored, and T11 covers the shape in the regression test so the
  suite cannot go green without it again.

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
- [x] T7: Replace the `FileLine` `erroringOn` mark with the node-level
      `unresolved` flag in `src/V3Ast.{h,cpp}` and use it in
      `V3LinkDot::markUnresolved()`/`repairUnresolved()`, removing the positional
      guesses the old mark forced (`writeTargetUnresolved()`'s inherited-mark
      clause, the `AstConst` exemption, "only a marked expression is
      unrepairable") -- acceptance: the MSVC build succeeds, the whole `--vlt`
      suite passes with no golden regenerated and no new `Internal Error`, and the
      only probe shapes whose output differs from the pre-change binary are the
      write-target shapes T10 covers (covers: S4)
- [x] T8: Call `repairUnresolved()` at the end of `linkDotPrimary()`,
      `linkDotParamed()` and `linkDotScope()`, and delete the four pipeline-level
      calls in `src/Verilator.cpp` -- acceptance: a lint run whose only errors are
      unresolved names still reports the later `CASEINCOMPLETE`/`LATCH` warnings
      and exits 1, `t_lint_notfound_continue` passes unchanged, and the whole
      `--vlt` suite passes with no golden regenerated (covers: S4; depends: T7)
- [x] T9: Delete the `visit(AstParseRef*)` repair branch and let the suite settle
      whether it was reachable -- outcome: the suite stayed green, but a probe of
      the `disable` shape showed the branch was reachable after all, so this is
      T9's red case and T11 restores it with the test that covers it (covers: S4;
      depends: T7)
- [x] T10: Extend `t_lint_notfound_continue` with the two write-target shapes the
      corrected walk changes (an undotted name and an unresolved index) and
      regenerate its two goldens with `HARNESS_UPDATE_GOLDEN=1` -- acceptance: the
      default golden shows the two errors and still the later
      `CASEINCOMPLETE`/`LATCH` warnings with no `PROCASSWIRE` or placeholder
      `WIDTHEXPAND`, the mixed golden shows the two errors and still no later
      warnings, and the whole `--vlt` suite passes (covers: S4; depends: T7)
- [x] T11: Restore the `visit(AstParseRef*)` repair branch and extend
      `t_lint_notfound_continue` with the two `disable` targets (undotted and
      dotted) that leave a parse reference for the repair -- acceptance: both
      targets report `Can't find definition of block/task` and the default run
      still reaches the later `CASEINCOMPLETE`/`LATCH` warnings while the mixed
      run still stops, the goldens are regenerated with `HARNESS_UPDATE_GOLDEN=1`,
      and the whole `--vlt` suite passes (covers: S4; depends: T9)
