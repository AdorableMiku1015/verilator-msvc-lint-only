<!-- SPDX-FileCopyrightText: 2026 Wilson Snyder -->
<!-- SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0 -->

# Parser recovery design audit

This records the design audit behind the parser recovery implementation.
See `lint-recovery-progress.md` for current delivery and verification evidence.
The staged same-file recovery recommendations below are now implemented (see the
follow-up delivery section of that file). Full orphan-allocation reclamation
remains unimplemented and is a documented bounded-leak compromise; consumer
closure for rejected declarations is not part of the initial guarantee.

## Entry points and transaction

- `V3Global::readFiles` reuses one V3Parse for builtin/config/libmap/user/library
  files. `V3LinkCells::resolveModule` creates another parser for lazy libraries.
  Both must receive the same safety guarantee.
- `V3ParseImp::parseFile` preprocesses the entire file before `parsePreprocessed`
  and `lexFile`. Replace lexFile's fatal-on-bison-return boundary, not the global
  classification of all syntax EC_ERROR diagnostics.
- `V3ParseGrammar.cpp` bisonParse wrapper can expose return status and explicit
  syntax-error delta. Return zero is not necessarily clean: error productions
  may recover and accept. Return two (memory exhaustion) must stay fatal.
- Begin a transaction before preprocessing/parsing. Snapshot real-root modules
  and misc append tails, existing $unit statement tail, root metadata/std pointers,
  and parse/global semantic state. On a certified recoverable syntax failure,
  unlink all additions from that input and restore state before continuing.
  FileLine filenames are not ownership boundaries (includes, macros and `line).
- Module/package/interface/program headers enter the live AST before their body
  completes, so an incomplete header cannot be retained as an empty valid unit.
- Record rejected declaration names/kinds/libnames to prevent repeated lazy
  autoload attempts. If all units are quarantined, stop with original diagnostics
  rather than issuing secondary no-top/internal errors.
- Keep builtin std, .vlt/control, and library-map failures strict initially:
  control mutations are not just AST suffixes. A temporary AstNetlist alone does
  not isolate parsing because many actions directly call v3Global.rootp().

## State and ownership requirements

This checkout has no active V3ParseSym symbol table to roll back; identifier
classification is lexical, and most names remain string-based references until
linking. Remaining stale upstream grammar comments must not mislead the design.

- Recreating Flex is insufficient. Clear token lookahead, last Bison token,
  previous-token/colon-colon state, preprocessor buffers/byte counters, lex string,
  tag aliases, lint-state stack, and per-input FileLines. Keyword mode/depth must
  agree with the new lexer. Preserve legitimate cross-file directives according
  to an explicit policy, rather than accidentally inheriting a partial prefix.
- V3ParseGrammar is a process singleton. Recreating V3Parse does not reset it.
  parserClear currently cleans only dtype/netDelay/scopedSigAttr. Reset all pin
  stacks/counters, ANSI/decl/IO/lifetime/property/modport/UDP/tracing modes too.
  Distinguish owned scratch (dtype/delay/instantiation parameter/strength/prototype
  templates) from aliases (attributes, case attributes, gate ranges): null aliases
  before deleting transaction ASTs, and do not separately delete aliased nodes.
- Bison uses fresh local stacks and yyerrstatus per call, but yynerrs and yylval
  are global here. Snapshot/reset counters and clear token aliases before freeing
  pools. Generated yydestruct is a no-op: discarded semantic AST values may be
  orphaned even after removing live-root suffixes.
- Do not add a generic semantic-value deleteTree destructor: reductions alias and
  transfer nodes, so it would double-delete. An allocation transaction registry,
  ownership-aware destructors, or parser-session ownership is the longer-term
  solution. Bounded failed-parse leaks in a short-lived lint process could be an
  explicitly documented first-stage compromise, not a claim of clean ownership.
- Restore/recompute $unit imports/statements, misc nodes, std package/process
  pointers, usesStdPackage, timeunit/timeprecision metadata, and global feature
  gates (classes/events/force/covergroup/randomize). Stale true feature gates are
  not a safety proof even if they often merely enable extra passes.

## Preprocessor boundary

V3PreShell retains one persistent preprocessor and drains the whole source before
Bison. Retain valid define/undef/include effects on an SV syntax-only failure:
recreating the preprocessor or preprocessing the failed file again can break later
files' macros/include guards or duplicate effects. Unterminated ifdefs and include
failures are separate: transient ifdef/off/include-error state currently persists,
and m_incError can prevent all future opens. Keep those failures strict until a
reset preserving m_defines has been implemented and tested.

For an aborted syntax parse, restoring pre-file compiler-directive state is a
conservative initial policy. Later, lex-to-EOF with semantic actions disabled can
establish final directive state more precisely.

## Same-file recovery and dependencies

Existing description/item/block/class error productions only synchronize tokens;
they are not AST/state transactions. There is no general module-error-to-endmodule
rollback. A semicolon can be inside the broken item; lookahead can pass a design
boundary. Do not split raw source with regex module/endmodule. Use explicit
begin/complete/error epochs and checkpoints at design-unit grammar actions.
A staged guarantee could first retain complete units before the first error,
then retain later clean units only after explicit end-token synchronization.

A bad package's consumers may still issue strict unresolved-type errors. Initial
independent-file recovery must not promise dependent-package/consumer closure
without separate symbol-aware quarantine.

## Required regression matrix

- Bad input before/after a clean external width/case/latch input.
- Accepted parser error recovery versus aborted EOF; incomplete ANSI header, UDP,
  parameter list and property followed by clean external source.
- Macro defined in a syntactically bad file remains usable in the next file.
- Broken ifdef/include remains deliberately fail-fast until separately supported.
- Earlier valid $unit typedef retained; failed-input typedef additions removed.
- Explicit top quarantined, unrelated top retained; all-inputs-invalid exit cleanly.
- Failed lazy-library parsing cached once, preserving original source attribution.
- Includes/macros/`line cannot defeat transaction ownership.
- Original error counts, independent late diagnostics, and --debug-check invariants.

Audit was read-only; none of this should be represented as delivered parser
recovery behavior until implemented and verified.
