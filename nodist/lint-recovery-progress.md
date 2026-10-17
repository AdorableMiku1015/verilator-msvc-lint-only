<!-- SPDX-FileCopyrightText: 2026 Wilson Snyder -->
<!-- SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0 -->

# Lint recovery hardening progress

## Objective and scope

This private Windows/MSVC lint-only fork targets incomplete SystemVerilog projects,
including omitted VHDL modules and encrypted IP. `origin/master` is a sync branch,
not an upstream merge target. Recovery must retain original errors and nonzero exit
status, collect independent diagnostics, and avoid cascades or malformed ASTs.

The user explicitly froze further expansion and requested a prompt local commit.
The delivered scope is the three implementations below, with their regression
contracts; it is not a claim that arbitrary invalid SystemVerilog can safely pass
all pipeline gates. Confirmed deferred boundaries are listed at the end.

## First implementation

- Count ordinary warnings promoted with `-Werror-*` as recoverable: severity does
  not change their AST invariants. Early read/module-loading gates use the same
  recoverability accounting as later passes. Default hard/named-error semantics
  remain strict unless a reporting site explicitly guarantees recovery.
- Add `v3warnRecoverable` and mark the `V3Width` sites for ZEROREPL,
  ENUMITEMWIDTH, and ENUMVALUE, which already normalize widths/types. Do not
  blanket-classify all width errors as recoverable.
- Remove unresolved release/deassign/event operations instead of replacing their
  writable/event target with a constant. Correct ancestor walking across previous
  siblings. A baseline `release absent` reproducer crashed in `V3Force.cpp:278`.
- Remove dead generic class templates after recoverable errors too. The previous
  `errorCount()` guard assumed immediate abort; retaining an unused template with
  a required integer parameter created bogus constant-range errors and stopped
  independent lint. This was found in independent review.
- Invalidate the harness log-content cache when truncating/reusing a command log.
  Otherwise multi-run assertions silently inspect the first run's output.
- Add width/promoted-warning/write-target regression families; update the two
  existing mixed continuation tests so ZEROREPL no longer blesses lost case/latch
  diagnostics. The historical compose specs' ZEROREPL-strict examples are
  superseded by these new expectations, not by a blanket removal of error gates.

## Second implementation: whole-file parser isolation

- An external SystemVerilog input with Bison syntax errors is removed as a whole,
  including additions to modules, root misc nodes, and existing $unit statements.
  Root std pointers, time metadata and semantic feature flags are restored.
- Explicit per-input syntax tracking catches both aborted parses and successful
  Bison returns after error productions. Grammar scratch ownership, pin/mode
  stacks, token lookahead, lexer state and aliases are cleared before continuing.
- Failed declaration identities live in a separate lookup registry. They cannot
  become phantom tops or shadow valid replacement/stub definitions. Only a real
  reference without a concrete definition creates an unavailable-module node.
- All-invalid inputs stop with the original errors. Retained user packages and
  $unit functions/classes count as healthy input too, even without a module top.
  If a requested top was in a rejected source, remaining independent roots are
  linted and the invocation still fails. Valid preprocessor macro effects survive
  ordinary SV syntax failure.
- Preprocessor hard errors do not enable parser quarantine. Their existing
  lexer/parser diagnostics are still collected; this avoids introducing a new
  premature abort that review found would hide existing messages.
- Lexer/control/libmap/resource failures and unavailable-declaration consumers
  remain deliberately outside the independent-file guarantee.
- Bison's discarded semantic AST values can remain orphaned until process exit;
  reachable AST is rolled back, but complete parser-allocation reclamation is
  not claimed. Same-file clean design-unit salvage is not implemented.

## Third implementation: caller operands and writable error values

- Preserve unavailable module pin/value/type parameter actuals, unknown call
  arguments, and known-read RHS expressions instead of deleting them with their
  destination. Reuse explicit `AstSysIgnore` diagnostic carriers with `readOnly`
  and `lintOnly` state, both dumped/serialized/compared.
- Diagnostic operands stay alive through local parameter/generate constant passes,
  full width checking and usage analysis, then are removed unconditionally before
  executable lowering, including with `-fno-const-before-dfg`.
- Retain ordinary call operands beside their owning statement/declaration within
  Begin/FTask/module scope. Keep lambda operands under AstWith and do not insert
  siblings into singular function-return variable slots. Constant simulation skips
  these non-executable diagnostic statements.
- Normalize diagnostic-only type operands through the existing child-dtype path
  before deleting their carrier. Fold retained select indices before Width narrows
  them, so generated `data[0+20]`/`data[1+20]` cannot silently become indices 4/5.
- Unknown directions conservatively count as possible usage/drive, not definite
  writer origins. Known nested function formals retain their known accesses;
  known-read RHS/parameter operands do not invent drivers or hide true UNDRIVEN.
- Error-created implicit declarations retain exact unresolved-reference identity
  and do not generate ghost unused/undriven warnings. Default-nettype-none keeps
  its original error text and nonzero status while allowing repaired AST to proceed.
- Repair known output/ref/const-ref erroring-constant actuals with typed private
  variables before resize/type checks. Explicit user constants remain invalid.
  Private names are reserved against their real $unit owner across class visitors.
- Repair unresolved inc/dec targets before LinkInc can create constant-LHS
  assignments. Original name errors remain; debug-check invariants no longer fail.
- Fix CMake's missing AST-definition-header dependencies: changing an operand type
  must regenerate accessors, not reuse stale generated headers in Windows builds.

Third-batch formal non-update verification passed: **6 recovery drivers passed**
(45 compiler invocations) and the full suite reported **1016 passed, 0 failed,
48 skipped** (`obj_dir/lint_opaque_selected_final.log`,
`obj_dir/lint_opaque_full_final.log`). The final minimal with/inc binding fix adds
two compiler invocations; all six drivers passed again (47 invocations) in
`obj_dir/lint_wrap_selected.log`. Final full-suite evidence is recorded below.
Remaining role/poison boundaries are not represented as solved by these changes.

## Verification commands and evidence

Build from a Visual Studio x64 developer environment:

```text
cmake --build --preset win64-release -j 8
```

Run this branch's harness through `nodist/test-regress.ps1` (it sets
VERILATOR_ROOT and adds the MSYS2 Perl path). Do not use `--list-tests`: the
harness forwards unknown switches to Verilator instead of listing tests.

Baseline full vlt run with the original executable: **1006 passed, 48 skipped**;
its only failure was the newly added width driver whose golden files did not yet
exist. Log: `obj_dir/lint_baseline.log`. Two existing continuation drivers passed
unchanged before implementation. Native MSVC builds completed successfully;
logs `obj_dir/lint_build.log` and `obj_dir/lint_build_2.log`.

Golden generation uses HARNESS_UPDATE_GOLDEN=1 and is expected to fail while
creating/matching new output files. Do not treat those generation passes as test
success.

First-batch verification:

- Full Windows lint suite: **1009 passed, 0 failed, 48 skipped** in 88 seconds
  (`obj_dir/lint_full_final.log`).
- Final rebuilt binary, selected five recovery drivers: **5 passed, 0 failed**,
  exercising 16 compiler invocations, including exact error-count assertions,
  strict class-type mismatch, --error-limit 1, and explicit --debug-exit-parse
  failure status (`obj_dir/lint_selected_built_final.log`).
- Nine relevant distribution checks: **9 passed, 0 failed**
  (`obj_dir/lint_dist_final.log`). That distribution Python-lint check skipped
  optional programs not on PATH; new drivers were separately checked with the
  existing virtual environment's Ruff 0.14.8 and YAPF 0.43.0.
- clang-format 18.1.8 applied only to changed C++ lines with git-clang-format;
  subsequent changed-line format check and `git diff --check` passed.
- Final source build, including the explicit parse-debug error-status guard:
  `obj_dir/lint_build_3.log`. Complete-suite reconfirmation on that binary:
  **1009 passed, 0 failed, 48 skipped** in 85 seconds
  (`obj_dir/lint_full_built_final.log`).
- Direct SARIF check: ENUMVALUE retains `error` severity, CASEINCOMPLETE and
  LATCH retain `warning` severity, and executionSuccessful stays false
  (`obj_dir/lint_recovery.sarif`, `obj_dir/lint_sarif_final.log`).

Second-batch verification:

- New file-isolation driver covers 14 modes: both file orders, accepted parser
  recovery, ANSI/property/instance EOF, all-invalid input, strict preprocessor
  failure, rejected requested top, repeated lazy-library failure, valid stub
  replacement, and package/$unit function/$unit class-only healthy inputs.
- New and first-batch recovery drivers: **6 passed, 0 failed**
  (`obj_dir/lint_parse_selected.log`, before the three final retained-unit modes).
- Targeted diagnostic-preservation suite: **14 passed, 0 failed**
  (`obj_dir/lint_parse_preservation.log`). Review caught an early preprocessing
  gate that hid old lexer/parser messages; it was removed. The five affected
  preprocessor goldens and EOF fuzz golden remain unchanged.
- Only seven existing goldens changed, each preserving every primary diagnostic
  and replacing the old Cannot-continue/secondary-fatal trailer with a final
  error-count summary. Those differences were independently reviewed.
- Complete lint suite after those corrections: **1010 passed, 0 failed,
  48 skipped** (`obj_dir/lint_parse_full_final.log`). Reconfirmation including
  the final retained-package guard and all 14 modes also passed:
  **1010 passed, 0 failed, 48 skipped** in 85 seconds
  (`obj_dir/lint_parse_full_units_final.log`).
- Eight relevant distribution checks passed (`obj_dir/lint_parse_dist_final.log`);
  changed-line clang-format, Ruff/YAPF and whitespace checks passed.
- Direct SARIF check retained the original syntax error plus WIDTHTRUNC,
  CASEINCOMPLETE and LATCH, with executionSuccessful false
  (`obj_dir/parse_isolate.sarif`, `obj_dir/lint_parse_sarif.log`).
- Independent direct checks exercised std imports in both orders, named work
  libraries, four-worker lint, reverse valid-stub ordering, member-type/net-
  strength/modport/UDP scratch reset, and package/$unit-only inputs. Logs are in
  `obj_dir/parse_registry_review_*.log`, `obj_dir/parser_scratch_audit/`, and
  `obj_dir/parse_guard_*.log`. No internal errors in the tested recovery shapes.
- Final native builds: `obj_dir/lint_parse_build_final.log` and
  `obj_dir/lint_parse_build_units.log`.

No runtime/model-generation tests are claimed: this fork only builds lint.
Verible formatting and cppcheck were not available in the inspected tool set.

Independent review: two focused audits examined custom changes since f2991c6ca;
one reviewer also exercised 15 corner cases directly on the rebuilt executable.
Direct probe sources/logs are under `obj_dir/audit_lint_diag_session6926/`.

## Final bounded delivery verification

- Final MSVC build succeeded (`obj_dir/lint_final_build.log`).
- Formal non-update six-driver verification, including the final with/inc
  regression: **6 passed, 0 failed**, covering **47 compiler invocations**
  (`obj_dir/lint_wrap_selected.log`).
- Full current Windows lint suite: **1016 passed, 0 failed, 48 skipped** in
  89 seconds (`obj_dir/lint_wrap_full.log`).
- Relevant distribution checks: **8 passed, 0 failed**
  (`obj_dir/lint_wrap_dist.log`). Changed-line clang-format, Ruff and
  source `git diff --check` passed. Three generated MULTIDRIVEN reference-location
  lines retain the diagnostic engine's trailing space; generated goldens were not
  hand-edited to change actual output.
- No runtime/model-generation results are claimed. No remote push or changes to
  `origin/master` are part of this delivery.
- Further work was explicitly stopped at the user's cost-control request. Known
  existing limitations below are recorded, not silently reclassified as fixed.

## Deferred limitations and follow-up options

The original parser audit and longer-term ownership/design-unit guidance are
preserved in `parser-recovery-design.md`. Whole-file isolation is implemented;
that document's finer-grained ownership recommendations are not all delivered.

1. **Parser granularity and ownership.** Whole-file isolation intentionally drops
   healthy units in a damaged input too. Same-file clean-unit salvage needs
   explicit grammar begin/complete/error checkpoints, not regex source splitting.
   Reclaim Bison-discarded orphan AST allocations without generic deleteTree
   destructors that would double-delete aliases. Preprocessor/lexer error-state
   recovery and unavailable package/type consumer closure remain separate work.
   Add directive-state and malformed include attribution coverage as these expand.
2. **Caller operands delivered.** The third implementation preserves unavailable
   module value/type actuals, unknown call arguments and bad-assignment RHS reads.
   Six dedicated regression families cover these contracts, including generated
   indices, lambdas and constant functions. This is not a promise to recover every
   selector expression inside an otherwise invalid write target.
3. **Deferred writable-role gaps (confirmed; no implementation included).** Known
   module output/inout pins with unresolved actuals still produce PORTSHORT or
   unsupported-width/tristate cascades; `$readmemh` with an unavailable memory still
   produces an unsupported-array error; `$cast` with an unresolved destination can
   still fail a write-reference invariant. Non-call read operands inside invalid
   selector targets (inc/dec, release/deassign or assignment LHS) can lose their
   independent SELRANGE. Scope was explicitly frozen by the user before these
   extensions; the interrupted module-pin prototype was fully withdrawn. Scratch
   evidence is under `obj_dir/write_context_review/` and `obj_dir/target_read_review/`.
   In contrast, task/function output/ref sinks, ordinary inc/dec, release/events,
   and the newly introduced with/inc lambda binding failure are addressed.
4. **Poison provenance and liveness.** BitFalseErroring placeholders are concrete
   constants; unknown generate/if conditions can prune independently lintable
   branches. Avoid treating recovery artifacts as authoritative user constants.
5. **Additional safe semantic recovery.** Missing types/interfaces/packages and
   missing packed-struct members still stop unrelated late checks. Quarantine or
   repair the invalid part before broadening recoverability. Keep a truly unsafe
   width-type mismatch negative control and a hard --error-limit control.
6. Expand policy/integration coverage: suppression, error-limit 0/1/default,
   --verilate-jobs 4, known valid siblings, and unused/multidriven/sequential lint
   sentinels alongside CASEINCOMPLETE/LATCH. Exact original error counts must be
   checked against raw logs because golden normalization strips final counts.

This bounded patch is prepared for the user's requested local commit. Do not
change the sync branch or push remotely as part of this work.
