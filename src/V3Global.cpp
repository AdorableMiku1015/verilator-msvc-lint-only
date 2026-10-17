// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Common implementations
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2004-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#include "V3PchAstMT.h"

#include "V3Global.h"

#include "V3ConstPool.h"
#include "V3EmitV.h"
#include "V3Error.h"
#include "V3File.h"
#include "V3LibMap.h"
#include "V3LinkCells.h"
#include "V3Parse.h"
#include "V3ParseImp.h"
#include "V3PreShell.h"
#include "V3Stats.h"
#include "V3ThreadPool.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// AddressSanitizer default options

#ifdef VL_ASAN
extern "C" const char* __asan_default_options() {
    // See https://github.com/google/sanitizers/wiki/SanitizerCommonFlags
    // or 'env ASAN_OPTIONS=help=1 ./verilator_bin'
    return ""
#ifndef VL_LEAK_CHECKS
           ":detect_leaks=0"
#endif
        ;
}
#endif

//######################################################################
// V3Global

void V3Global::boot() {
    UASSERT(!m_rootp, "call once");
    m_rootp = new AstNetlist;
    m_constPoolp = new V3ConstPool;
    m_libMapp = new V3LibMap;
}

void V3Global::shutdown() {
    V3PreShell::shutdown();
    VL_DO_CLEAR(delete m_threadPoolp, m_threadPoolp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_libMapp, m_libMapp = nullptr);  // delete nullptr is safe
    VL_DO_CLEAR(delete m_constPoolp, m_constPoolp = nullptr);  // delete nullptr is safe
#ifdef VL_LEAK_CHECKS
    if (m_rootp) VL_DO_CLEAR(m_rootp->deleteTree(), m_rootp = nullptr);
#endif
    FileLine::deleteAllRemaining();
}

void V3Global::vlExit(int status) {
    shutdown();
    std::exit(status);
}

void V3Global::checkTree() const { rootp()->checkTree(); }

V3Global::ParseState V3Global::parseState() const {
    return {m_assertDTypesResolved,
            m_assertScoped,
            m_assignsEvents,
            m_constRemoveXs,
            m_needTraceDumper,
            m_dpi,
            m_hasEvents,
            m_hasClasses,
            m_hasSampled,
            m_hasTable,
            m_hasVirtIfaces,
            m_usesProbDist,
            m_usesStdPackage,
            m_usesTiming,
            m_usesForce,
            m_usesZeroDelay,
            m_hasForceableSignals,
            m_hasAssignDeassign,
            m_hasSystemCSections,
            m_useParallelBuild,
            m_useRandSequence,
            m_useCovergroup,
            m_useRandomizeMethods,
            m_hasPrintedObjects};
}

void V3Global::parseState(const ParseState& state) {
    m_assertDTypesResolved = state.assertDTypesResolved;
    m_assertScoped = state.assertScoped;
    m_assignsEvents = state.assignsEvents;
    m_constRemoveXs = state.constRemoveXs;
    m_needTraceDumper = state.needTraceDumper;
    m_dpi = state.dpi;
    m_hasEvents = state.hasEvents;
    m_hasClasses = state.hasClasses;
    m_hasSampled = state.hasSampled;
    m_hasTable = state.hasTable;
    m_hasVirtIfaces = state.hasVirtIfaces;
    m_usesProbDist = state.usesProbDist;
    m_usesStdPackage = state.usesStdPackage;
    m_usesTiming = state.usesTiming;
    m_usesForce = state.usesForce;
    m_usesZeroDelay = state.usesZeroDelay;
    m_hasForceableSignals = state.hasForceableSignals;
    m_hasAssignDeassign = state.hasAssignDeassign;
    m_hasSystemCSections = state.hasSystemCSections;
    m_useParallelBuild = state.useParallelBuild;
    m_useRandSequence = state.useRandSequence;
    m_useCovergroup = state.useCovergroup;
    m_useRandomizeMethods = state.useRandomizeMethods;
    m_hasPrintedObjects = state.hasPrintedObjects;
}

void V3Global::noteQuarantinedModule(const string& name, const string& libname, FileLine* flp) {
    const auto inserted = m_quarantinedModules.emplace(std::make_pair(libname, name), nullptr);
    if (!inserted.second) return;
    FileLine* const errorp = new FileLine{flp};
    errorp->erroringOn(true);
    inserted.first->second = errorp;
}

FileLine* V3Global::quarantinedModuleFileline(const string& name, const string& libname) const {
    const auto it = m_quarantinedModules.find({libname, name});
    return it == m_quarantinedModules.end() ? nullptr : it->second;
}

void V3Global::readFiles() {
    // NODE STATE
    //   AstNode::user4p()      // VSymEnt*    Package and typedef symbol names
    const VNUser4InUse inuser4;

    VInFilter filter{v3Global.opt.pipeFilter()};

    {
        V3Parse parser{v3Global.rootp(), &filter};

        // Parse the std waivers
        if (v3Global.opt.stdWaiver()) {
            parser.parseText(new FileLine{V3Options::getStdWaiverPath()},
                             V3Options::getStdWaiverText(), V3Options::getStdWaiverPath());
        }
        // Read .vlt files
        for (const VFileLibName& filelib : v3Global.opt.vltFiles()) {
            parser.parseFile(new FileLine{FileLine::commandLineFilename()}, filelib.filename(),
                             false, false, filelib.libname(),
                             "Cannot find file containing .vlt file: ");
        }

        // Parse the std package
        if (v3Global.opt.stdPackage()) {
            parser.parseText(new FileLine{V3Options::getStdPackagePath()},
                             V3Options::getStdPackageText(), V3Options::getStdPackagePath());
        }

        // Parse libmap files
        for (const string& filename : v3Global.opt.libmapFiles()) {
            parser.parseFile(new FileLine{FileLine::commandLineFilename()}, filename, false, true,
                             "work", "Cannot find file containing libmap definitions: ");
        }
        // Create library mapping
        V3LibMap::map(v3Global.rootp());

        // Read top module
        for (const auto& filelib : v3Global.opt.vFiles()) {
            const string& libname = filelib.libname() == "work"
                                        ? v3Global.libMapp()->matchMapping(filelib.filename())
                                        : filelib.libname();
            parser.parseFile(new FileLine{FileLine::commandLineFilename()}, filelib.filename(),
                             false, false, libname, "Cannot find file containing module: ");
        }

        // Read libraries
        // To be compatible with other simulators,
        // this needs to be done after the top file is read
        for (const auto& filelib : v3Global.opt.libraryFiles()) {
            parser.parseFile(new FileLine{FileLine::commandLineFilename()}, filelib.filename(),
                             true, false, filelib.libname(),
                             "Cannot find file containing library module: ");
        }

        // Read hierarchical type parameter file
        for (const auto& filelib : v3Global.opt.hierParamFile()) {
            parser.parseFile(new FileLine{FileLine::commandLineFilename()}, filelib.filename(),
                             false, false, filelib.libname(),
                             "Cannot open file containing hierarchical parameter declarations: ");
        }
    }

    // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("parse.tree"));
    V3Error::abortIfUnrecoverableErrors();

    if (quarantinedSources()) {
        // Package-only and compilation-unit functions/classes are lintable too.
        bool hasRetainedInput = rootp()->dollarUnitPkgp()->stmtsp();
        bool hasRequestedTop = opt.topModule().empty();
        for (AstNodeModule* modp = rootp()->modulesp(); modp;
             modp = VN_AS(modp->nextp(), NodeModule)) {
            if (!VN_IS(modp, NotFoundModule) && !modp->isDollarUnit() && !modp->isConstPool()
                && modp != rootp()->stdPackagep()) {
                hasRetainedInput = true;
            }
            if ((VN_IS(modp, Module) || VN_IS(modp, Iface) || VN_IS(modp, Primitive))
                && modp->name() == opt.topModule()) {
                hasRequestedTop = true;
            }
        }
        // Only a truly empty user tree should stop with the original errors.
        if (!hasRetainedInput) V3Error::abortIfErrors();
        // The requested top may have been in a rejected input. Its syntax error
        // still makes this run fail; lint the remaining independent roots rather
        // than selecting an unavailable placeholder or dropping healthy modules.
        if (!hasRequestedTop) opt.clearTopModule();
    }

    if (!v3Global.opt.preprocOnly() || v3Global.opt.preprocResolve()) {
        // Resolve all modules cells refer to
        V3LinkCells::link(v3Global.rootp(), &filter);
    }

    V3Global::dumpCheckGlobalTree("cells", false, dumpTreeEitherLevel() >= 9);
}

void V3Global::removeStd() {
    // Delete the std package if unused
    if (!usesStdPackage()) {
        UINFO(3, "Removing unused std:: package");
        if (AstNodeModule* stdp = v3Global.rootp()->stdPackagep()) {
            v3Global.rootp()->stdPackagep(nullptr);
            v3Global.rootp()->stdPackageProcessp(nullptr);
            VL_DO_DANGLING(stdp->unlinkFrBack()->deleteTree(), stdp);
        }
    }
}

string V3Global::debugFilename(const string& nameComment, int newNumber) {
    ++m_debugFileNumber;
    if (newNumber) m_debugFileNumber = newNumber;
    return opt.hierTopDataDir() + "/" + opt.prefix() + "_" + digitsFilename(m_debugFileNumber)
           + "_" + nameComment;
}
string V3Global::digitsFilename(int number) {
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(3) << number;
    return ss.str();
}

void V3Global::dumpCheckGlobalTree(const string& stagename, int newNumber, bool doDump,
                                   bool doCheck) {
    const string treeFilename = v3Global.debugFilename(stagename + ".tree", newNumber);
    if (dumpTreeLevel()) v3Global.rootp()->dumpTreeFile(treeFilename, doDump);
    if (dumpTreeJsonLevel()) {
        v3Global.rootp()->dumpTreeJsonFile(treeFilename + ".json", doDump);
    }
    if (v3Global.opt.dumpTreeDot()) {
        v3Global.rootp()->dumpTreeDotFile(treeFilename + ".dot", doDump);
    }
    if (v3Global.opt.stats()) V3Stats::statsStage(stagename);

    if (doDump && v3Global.opt.debugEmitV()) V3EmitV::debugEmitV(treeFilename + ".v");
    if (doCheck && (v3Global.opt.debugCheck() || dumpTreeEitherLevel())) {
        // Error check
        v3Global.rootp()->checkTree();
        // Broken isn't part of check tree because it can munge iterp's
        // set by other steps if it is called in the middle of other operations
        V3Broken::brokenAll(v3Global.rootp());
    }
}

void V3Global::idPtrMapDumpJson(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"pointers\": {";
    for (const auto& itr : m_ptrToId) {
        os << sep << '"' << itr.second << "\": \"" << cvtToHex(itr.first) << '"';
        sep = ",\n  ";
    }
    os << "\n }";
}

void V3Global::saveJsonPtrFieldName(const std::string& fieldName) {
    m_jsonPtrNames.insert(fieldName);
}

void V3Global::ptrNamesDumpJson(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"ptrFieldNames\": [";
    for (const auto& itr : m_jsonPtrNames) {
        os << sep << '"' << itr << '"';
        sep = ",\n  ";
    }
    os << "\n ]";
}

const std::string& V3Global::ptrToId(const void* p) {
    const auto pair = m_ptrToId.emplace(p, "");
    if (pair.second) {
        std::ostringstream os;
        if (p) {
            os << "(";
            unsigned id = m_ptrToId.size();
            do { os << static_cast<char>('A' + id % 26); } while (id /= 26);
            os << ")";
        } else {
            os << "0";
        }
        pair.first->second = os.str();
    }
    return pair.first->second;
}
