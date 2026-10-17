// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: main()
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#define VL_MT_CONTROL_CODE_UNIT 1

#include "V3Active.h"
#include "V3Assert.h"
#include "V3AssertNfa.h"
#include "V3AssertPre.h"
#include "V3Ast.h"
#include "V3Begin.h"
#include "V3Broken.h"
#include "V3Case.h"
#include "V3Class.h"
#include "V3Const.h"
#include "V3Control.h"
#include "V3Coverage.h"
#include "V3CoverageJoin.h"
#include "V3Covergroup.h"
#include "V3Dead.h"
#include "V3Delayed.h"
#include "V3DfgOptimizer.h"
#include "V3DiagSarif.h"
#include "V3File.h"
#include "V3Force.h"
#include "V3Fork.h"
#include "V3FsmDetect.h"
#include "V3Gate.h"
#include "V3Global.h"
#include "V3Graph.h"
#include "V3HashTable.h"
#include "V3Inline.h"
#include "V3Inst.h"
#include "V3Interface.h"
#include "V3Life.h"
#include "V3LiftExpr.h"
#include "V3LinkDot.h"
#include "V3LinkDotIfaceCapture.h"
#include "V3LinkInc.h"
#include "V3LinkJump.h"
#include "V3LinkLValue.h"
#include "V3LinkLevel.h"
#include "V3LinkParse.h"
#include "V3LinkResolve.h"
#include "V3LinkWith.h"
#include "V3Name.h"
#include "V3Os.h"
#include "V3Param.h"
#include "V3PreShell.h"
#include "V3RandSequence.h"
#include "V3Randomize.h"
#include "V3Reorder.h"
#include "V3Scope.h"
#include "V3Slice.h"
#include "V3Split.h"
#include "V3SplitVar.h"
#include "V3Stats.h"
#include "V3String.h"
#include "V3Task.h"
#include "V3ThreadPool.h"
#include "V3Timing.h"
#include "V3Tristate.h"
#include "V3Udp.h"
#include "V3Undriven.h"
#include "V3Unknown.h"
#include "V3Unroll.h"
#include "V3Waiver.h"
#include "V3Width.h"
#include "V3WidthCommit.h"

#include <ctime>
#include <iomanip>

VL_DEFINE_DEBUG_FUNCTIONS;

V3Global v3Global;

static void lintStagePrint(const char* namep, double seconds) {
    if (!v3Global.opt.lintFast()) return;
    std::cout << "- Verilator: Lint stage " << std::left << std::setw(10) << namep
              << std::setprecision(3) << std::fixed << seconds << " s\n"
              << std::flush;
}

static void reportStatsIfEnabled() {
    if (v3Global.opt.stats()) {
        FileLine::stats();
        V3Stats::statsReport();
    }
}

static void emitJson() VL_MT_DISABLED {
    const string filename
        = (v3Global.opt.jsonOnlyOutput().empty()
               ? v3Global.opt.makeDir() + "/" + v3Global.opt.prefix() + ".tree.json"
               : v3Global.opt.jsonOnlyOutput());
    v3Global.rootp()->dumpTreeJsonFile(filename);
}

static void emitSerialized() VL_MT_DISABLED {
    if (v3Global.opt.jsonOnly()) emitJson();
}

static void process() {
    {
        const VlOs::DeltaWallTime elabWallTime{true};
        VlOs::DeltaWallTime stageWallTime{true};
        const bool doLintProfile = v3Global.opt.lintFast();

        // Sort modules by level so later algorithms don't need to care
        V3LinkLevel::modSortByLevel();
        V3Error::abortIfUnrecoverableErrors();
        if (v3Global.opt.debugExitParse()) {
            V3Error::abortIfErrors();
            cout << "--debug-exit-parse: Exiting after parse\n";
            v3Global.vlExit(0);
        }

        // Convert parseref's to varrefs, and other directly post parsing fixups
        V3LinkParse::linkParse(v3Global.rootp());
        // Cross-link signal names
        // Cross-link dotted hierarchical references
        // The link repairs or removes the names it could not resolve itself
        V3LinkDot::linkDotPrimary(v3Global.rootp());
        v3Global.checkTree();  // Force a check, as link is most likely place for problems
        // Check if all parameters have been found
        v3Global.opt.checkParameters();
        // Correct state we couldn't know at parse time, repair SEL's
        V3LinkResolve::linkResolve(v3Global.rootp());
        // Set Lvalue's in variable refs
        V3LinkLValue::linkLValue(v3Global.rootp());
        // Convert return/continue/disable to jumps
        V3LinkJump::linkJump(v3Global.rootp());
        // Convert --/++ to normal operations. Must be after LinkJump.
        V3LinkInc::linkIncrements(v3Global.rootp());
        // Continue past repaired names and promoted ordinary warnings. Errors
        // that leave an unusable tree still stop at this tolerant gate.
        V3Error::abortIfUnrecoverableErrors();

        if (doLintProfile) {
            lintStagePrint("link", stageWallTime.deltaTime());
            stageWallTime.start();
        }

        if (v3Global.opt.stats()) V3Stats::statsStageAll(v3Global.rootp(), "Link");

        // Remove parameters by cloning modules to de-parameterized versions
        //   This requires some width calculations and constant propagation
        // No more AstGenCase/AstGenFor/AstGenIf after this
        V3Param::param(v3Global.rootp());

        V3LinkDot::linkDotParamed(v3Global.rootp());  // Cleanup as made new modules
        V3Param::finalizeDeferredParams(v3Global.rootp());
        V3LinkLValue::linkLValue(v3Global.rootp());  // Resolve new VarRefs

        // Link cleanup of 'with' as final link phase before V3Width
        // (called only once, not when width a single params)
        V3LinkWith::linkWith(v3Global.rootp());

        V3Error::abortIfUnrecoverableErrors();

        if (doLintProfile) {
            lintStagePrint("param", stageWallTime.deltaTime());
            stageWallTime.start();
        }

        // Fix any remaining cross-interface refs created during V3Width::widthParamsEdit
        // that weren't captured earlier. Must run before V3Dead deletes template modules.
        V3LinkDotIfaceCapture::finalizeIfaceCapture();

        // Remove any modules that were parameterized and are no longer referenced.
        V3Dead::deadifyModules(v3Global.rootp());

        v3Global.checkTree();
        if (v3Global.hasTable()) V3Udp::udpResolve(v3Global.rootp());

        // Calculate and check widths, edit tree to TRUNC/EXTRACT any width mismatches
        V3Width::width(v3Global.rootp());

        // Missing-module placeholders and explicitly repaired width errors
        // leave a usable tree. Keep collecting diagnostics after those errors,
        // but stop here if any error has not been made safe for later passes.
        V3Error::abortIfUnrecoverableErrors();

        // Commit to the widths we've chosen; Make widthMin==width
        V3WidthCommit::widthCommit(v3Global.rootp());
        v3Global.assertDTypesResolved(true);
        v3Global.widthMinUsage(VWidthMinUsage::MATCHES_WIDTH);

        if (doLintProfile) {
            lintStagePrint("width", stageWallTime.deltaTime());
            stageWallTime.start();
        }

        // End of elaboration
        V3Stats::addStatPerf(V3Stats::STAT_WALLTIME_ELAB, elabWallTime.deltaTime());
        const VlOs::DeltaWallTime cvtWallTime{true};
        if (v3Global.opt.debugExitElab()) {
            V3Error::abortIfErrors();
            if (v3Global.opt.serializeOnly()) emitSerialized();
            cout << "--debug-exit-elab: Exiting after elaboration pass\n";
            v3Global.vlExit(0);
        }

        if (!v3Global.opt.serializeOnly() || v3Global.opt.flatten()) {
            // Add top level wrapper with instance pointing to old top
            // Move packages to under new top
            // Must do this after we know parameters and dtypes (as don't clone dtype decls)
            V3LinkLevel::wrapTop(v3Global.rootp());
        } else {
            V3LinkLevel::nonWrapTop(v3Global.rootp());
        }

        if (!v3Global.opt.serializeOnly()) {
            // Insert generic code coverage before dead code elimination and
            // inlining, or those opportunities may be optimized away. FSM
            // coverage is handled later in V3FsmDetect, after scoping has created
            // the AST context needed to recover and lower FSMs reliably.
            if (v3Global.opt.coverageNonFsm()) V3Coverage::coverage(v3Global.rootp());
        }

        // Functional coverage code generation
        //    Generate code for covergroups/coverpoints
        if (v3Global.useCovergroup()) V3Covergroup::covergroup(v3Global.rootp());

        // Resolve randsequence if they are used by the design
        if (v3Global.useRandSequence()) V3RandSequence::randSequenceNetlist(v3Global.rootp());

        // Add randomize() class methods if they are used by the design
        if (v3Global.useRandomizeMethods()) V3Randomize::randomizeNetlist(v3Global.rootp());

        // Push constants, but only true constants preserving liveness
        // so V3Undriven sees variables to be eliminated, ie "if (0 && foo) ..."
        if (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAllLive(v3Global.rootp());

        // Signal based lint checks, no change to structures
        // Must be before first constification pass drops dead code
        V3Undriven::undrivenAll(v3Global.rootp());
        // Opaque operands are for diagnostics and liveness only. Remove them
        // even when const-before-dfg is disabled, before executable lowering.
        V3LinkDot::removeIgnored(v3Global.rootp());

        // Assertion insertion
        //    After we've added block coverage, but before other nasty transforms
        V3AssertCommon::collectDefaultDisable(v3Global.rootp());
        V3AssertCommon::lowerSequenceEvents(v3Global.rootp());
        V3AssertNfa::assertNfaAll(v3Global.rootp());
        // V3AssertProp removed: NFA subsumes multi-cycle property lowering.
        // Unsupported constructs fall through to V3AssertPre.
        V3AssertPre::assertPreAll(v3Global.rootp());
        //
        V3Assert::assertAll(v3Global.rootp());

        // Propagate constants into expressions
        if (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAllLint(v3Global.rootp());

        if (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            // Split packed variables into multiple pieces to resolve UNOPTFLAT.
            // should be after constifyAllLint() which flattens to 1D bit vector
            V3SplitVar::splitVariable(v3Global.rootp());

            if (v3Global.opt.timing().isSetTrue()) {
                // Generate classes and tasks required to maintain proper lifetimes for references
                // in forks
                V3Fork::makeDynamicScopes(v3Global.rootp());
                V3Fork::makeTasks(v3Global.rootp());
            }

            // Task inlining & pushing BEGINs names to variables/cells
            // Begin processing must be after Param, before module inlining
            // No more AstGenBlocks after this
            V3Begin::debeginAll(v3Global.rootp());  // Flatten cell names, before inliner

            // Expand inouts, stage 2
            // Also simplify pin connections to always be AssignWs in prep for V3Unknown
            V3Tristate::tristateAll(v3Global.rootp());
        }

        if (!v3Global.opt.serializeOnly()) {
            // Lift expressions out of statements.
            V3LiftExpr::liftExprAll(v3Global.rootp());

            // Move assignments from X into MODULE temps.
            // (Before flattening, so each new X variable is shared between all scopes of that
            // module.)
            V3Unknown::unknownAll(v3Global.rootp());
            v3Global.constRemoveXs(true);
        }

        // Interface references feed trace file aliases and VPI name resolution
        if (v3Global.opt.trace() || v3Global.opt.vpi()) {
            V3Interface::interfaceAll(v3Global.rootp());
        }

        // --PRE-FLAT OPTIMIZATIONS------------------

        // Initial const/dead to reduce work for ordering code
        if (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());
        v3Global.checkTree();

        V3Dead::deadifyDTypes(v3Global.rootp());
        v3Global.checkTree();

        V3Error::abortIfUnrecoverableErrors();

        // --FLATTENING---------------

        if (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            // We're going to flatten the hierarchy, so as many optimizations that
            // can be done as possible should be before this....

            // Convert instantiations to wassigns and always blocks
            V3Inst::instAll(v3Global.rootp());

            // Inst may have made lots of concats; fix them
            if (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());

            // Flatten hierarchy, creating a SCOPE for each module's usage as a cell
            // No more AstAlias after linkDotScope
            V3Scope::scopeAll(v3Global.rootp());
            V3LinkDot::linkDotScope(v3Global.rootp());
            V3Error::abortIfUnrecoverableErrors();

            // FSM coverage needs scopes, but should otherwise run as early as possible before
            // later lowering rewrites user-visible clocked case structure.
            if (v3Global.opt.coverageFsm()) V3FsmDetect::detect(v3Global.rootp());

            // Module inlining
            if (v3Global.opt.fInline()) V3Inline::inlineAll(v3Global.rootp());

            // Relocate classes (after linkDot)
            V3Class::classAll(v3Global.rootp());
        }

        // --SCOPE BASED OPTIMIZATIONS--------------

        if (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            // Cleanup
            if (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyDTypesScoped(v3Global.rootp());
            v3Global.checkTree();
        }

        if (!v3Global.opt.serializeOnly()) {
            // Convert case statements to if() blocks.  Must be after V3Unknown
            // Must be before V3Task so don't need to deal with task in case value compares
            V3Case::caseAll(v3Global.rootp());
        }

        if (!(v3Global.opt.serializeOnly() && !v3Global.opt.flatten())) {
            // Inline all tasks
            V3Task::taskAll(v3Global.rootp());
        }

        if (!v3Global.opt.serializeOnly()) {
            // Add __PVT's
            // After V3Task so task internal variables will get renamed
            V3Name::nameAll(v3Global.rootp());

            // Loop unrolling
            V3Unroll::unrollAll(v3Global.rootp());

            // Expand slices of arrays
            V3Slice::sliceAll(v3Global.rootp());

            // Push constants across variables and remove redundant assignments
            if (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());

            if (v3Global.opt.fLife()) V3Life::lifeAll(v3Global.rootp());

            // Cleanup
            if (v3Global.opt.fConstBeforeDfg()) V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyDTypesScoped(v3Global.rootp());
            v3Global.checkTree();

            // Move assignments/sensitives into a SBLOCK for each unique sensitivity list
            // (May convert some ALWAYS to combo blocks, so should be before V3Gate step.)
            V3Active::activeAll(v3Global.rootp());

            // Split single ALWAYS blocks into multiple blocks for better ordering chances
            if (v3Global.opt.fSplit()) V3Split::splitAll(v3Global.rootp());

            // Convert forceable signals, process force/release statements.
            // Convert forceable signals and assign/deassign statements in one combined pass set.
            // We reserve AST user slots across both sub-passes so helper pointers can be handed
            // directly from force discovery to assign/deassign lowering without rediscovery.
            V3Force::forceAndAssignAll(v3Global.rootp());

            // DFG optimization
            if (v3Global.opt.fDfg()) V3DfgOptimizer::optimize(v3Global.rootp());

            // Gate-based logic elimination; eliminate signals and push constant across cell
            // boundaries Instant propagation makes lots-o-constant reduction possibilities.
            if (v3Global.opt.fGate()) {
                V3Gate::gateAll(v3Global.rootp());
                // V3Gate calls constant propagation itself.
            } else {
                v3info("Command Line disabled gate optimization with -fno-gate.  "
                       "This may cause ordering problems.");
            }

            // Combine generic COVERINCs with duplicate terms. FSM coverage is
            // already lowered separately inside V3FsmDetect.
            if (v3Global.opt.coverageNonFsm()) V3CoverageJoin::coverageJoin(v3Global.rootp());

            // Remove unused vars
            V3Const::constifyAll(v3Global.rootp());
            V3Dead::deadifyAllScoped(v3Global.rootp());

            // Reorder assignments in pipelined blocks
            if (v3Global.opt.fReorder()) V3Reorder::reorderAll(v3Global.rootp());

            if (v3Global.opt.timing().isSetTrue()) {
                // Convert AST for timing if requested
                // Needs to be after V3Gate, as that step modifies sentrees
                // Needs to be before V3Delayed, as delayed assignments are handled differently in
                // suspendable processes
                V3Timing::timingAll(v3Global.rootp());
            }

            // Create delayed assignments
            // This creates lots of duplicate ACTIVES so ActiveTop needs to be after this step.
            V3Delayed::delayedAll(v3Global.rootp());

            // Everything past V3Delayed exists to build the schedulable C++ model,
            // which this lint-only build never emits, so the pipeline stops here.
            if (doLintProfile) lintStagePrint("postwidth", stageWallTime.deltaTime());
        }
        if (v3Global.opt.serializeOnly()) emitSerialized();
    }
}

static bool verilate(const string& argString) {
    // Run verilation, and return false if skipped
    UINFO(1, "Option --verilate: Start Verilation");

    // Can we skip doing everything if times are ok?
    V3File::addSrcDepend(v3Global.opt.buildDepBin());
    if (v3Global.opt.skipIdentical().isTrue()
        && V3File::checkTimes(v3Global.opt.hierTopDataDir() + "/" + v3Global.opt.prefix()
                                  + "__verFiles.dat",
                              argString)) {
        UINFO(1, "--skip-identical: No change to any source files, exiting");
        return false;
    }
    // Undocumented debugging - cannot be a switch as then command line
    // would mismatch forcing non-identicalness when we set it
    if (!V3Os::getenvStr("VERILATOR_DEBUG_SKIP_IDENTICAL", "").empty()) {  // LCOV_EXCL_START
        v3fatalSrc("VERILATOR_DEBUG_SKIP_IDENTICAL w/ --skip-identical: Changes found\n");
    }  // LCOV_EXCL_STOP

    // Disable mutexes in single-thread verilation
    V3MutexConfig::s().configure(v3Global.opt.verilateJobs() > 1 /*enable*/);

    // Initialize thread pool
    v3Global.threadPoolp(new V3ThreadPool{v3Global.opt.verilateJobs()});

    // --FRONTEND------------------

    // Cleanup
    // Ideally we'd do prefix + "_*.*", and prefix + ".*", but this seems
    // potentially disruptive to old behavior, and --skip-identical
    V3Os::unlinkRegexp(v3Global.opt.hierTopDataDir(), v3Global.opt.prefix() + "_*.dot");
    V3Os::unlinkRegexp(v3Global.opt.hierTopDataDir(), v3Global.opt.prefix() + "_*.tree");
    V3Os::unlinkRegexp(v3Global.opt.hierTopDataDir(), v3Global.opt.prefix() + "_*.txt");

    // Internal tests (after option parsing as need debug() setting,
    // and after removing files as may make debug output)
    VBasicDTypeKwd::selfTest();
    if (v3Global.opt.debugSelfTest()) {
        AstClassRefDType::selfTest();
        V3Os::selfTest();
        V3Number::selfTest();
        VString::selfTest();
        VHashSha512::selfTest();
        VSpellCheck::selfTest();
        V3Graph::selfTest();
        V3PreShell::selfTest();
        V3Broken::selfTest();
        V3Control::selfTest();
        V3HashTableInternals::selfTest();
        V3ThreadPool::selfTest();
        UINFO(2, "selfTest done");
    }

    // Read first filename
    const VlOs::DeltaWallTime lintTotalWallTime{true};
    const VlOs::DeltaWallTime parseWallTime{true};
    v3Global.readFiles();
    v3Global.removeStd();
    if (v3Global.opt.lintFast()) lintStagePrint("parse", parseWallTime.deltaTime());

    // Link, etc, if needed
    if (!v3Global.opt.preprocOnly()) {  //
        process();
    }
    if (v3Global.opt.lintFast()) lintStagePrint("total", lintTotalWallTime.deltaTime());

    // Final steps
    V3Global::dumpCheckGlobalTree("final", 990, dumpTreeEitherLevel() >= 3);
    if (v3Global.opt.jsonOnly() || dumpTreeJsonLevel()) {
        const string filename
            = (v3Global.opt.jsonOnlyMetaOutput().empty()
                   ? v3Global.opt.makeDir() + "/" + v3Global.opt.prefix() + ".tree.meta.json"
                   : v3Global.opt.jsonOnlyMetaOutput());
        v3Global.rootp()->dumpJsonMetaFile(filename);
    }

    V3Error::abortIfErrors();

    if (v3Global.opt.isWaiverOutput()) {
        // Create waiver output, must be just before we exit on warnings
        V3Waiver::write(v3Global.opt.waiverOutput());
    }

    V3Error::abortIfWarnings();

    // Linting is finished; there is no model to write, no makefile to emit and
    // no build to run.
    return true;
}

//######################################################################

int main(int argc, char** argv) {
    // General initialization
    std::ios::sync_with_stdio();
    const VlOs::DeltaWallTime wallTimeTotal{true};
    const VlOs::DeltaCpuTime cpuTimeTotal{true};

    time_t randseed;
    time(&randseed);
    srand(static_cast<int>(randseed));

    const string argString = V3Options::argString(argc - 1, argv + 1);

    // Post-constructor initialization of netlists
    v3Global.boot();

    // Preprocessor
    // Before command parsing so we can handle -Ds on command line.
    V3PreShell::boot();

    // Command option parsing
    v3Global.opt.buildDepBin(V3Os::filenameCleanup(argv[0]));
    v3Global.opt.parseOpts(new FileLine{FileLine::commandLineFilename()}, argc - 1, argv + 1);

    // Validate settings (aka Boost.Program_options)
    v3Global.opt.notify();
    v3Global.rootp()->timeInit();

    V3Error::abortIfErrors();

    bool didVerilate = false;
    if (v3Global.opt.verilate()) {
        didVerilate = verilate(argString);
    } else {
        UINFO(1, "Option --no-verilate: Skip Verilation");
    }

    if (didVerilate) reportStatsIfEnabled();
    V3DiagSarif::output(true);

    // Explicitly release resources
    v3Global.shutdown();

    if (!v3Global.opt.quietStats() && !v3Global.opt.preprocOnly()) {
        V3Stats::addStatPerf(V3Stats::STAT_CPUTIME, cpuTimeTotal.deltaTime());
        V3Stats::addStatPerf(V3Stats::STAT_WALLTIME, wallTimeTotal.deltaTime());
        V3Stats::summaryReport();
    }

    UINFO(1, "Done, Exiting...");
}
