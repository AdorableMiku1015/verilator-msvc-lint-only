// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Command line options
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

#ifndef VERILATOR_V3OPTIONS_H_
#define VERILATOR_V3OPTIONS_H_

#include "config_build.h"
#include "verilatedos.h"

#include "V3Error.h"
#include "V3LangCode.h"

#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

class V3OptionsImp;
class FileLine;

//######################################################################

class VOptionBool final {
    // Class to track options that are either not specified (and default
    // true/false), versus user setting the option to true or false
public:
    enum en : uint8_t { OPT_DEFAULT_FALSE = 0, OPT_DEFAULT_TRUE, OPT_TRUE, OPT_FALSE };
    enum en m_e;
    VOptionBool()
        : m_e{OPT_DEFAULT_FALSE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VOptionBool(en _e)
        : m_e{_e} {}
    explicit VOptionBool(int _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { return m_e; }
    const char* ascii() const {
        static const char* const names[]
            = {"DEFAULT_FALSE", "DEFAULT_TRUE", "OPT_TRUE", "OPT_FALSE"};
        return names[m_e];
    }
    bool isDefault() const { return m_e == OPT_DEFAULT_FALSE || m_e == OPT_DEFAULT_TRUE; }
    bool isTrue() const { return m_e == OPT_TRUE || m_e == OPT_DEFAULT_TRUE; }
    bool isSetTrue() const { return m_e == OPT_TRUE; }
    bool isSetFalse() const { return m_e == OPT_FALSE; }
    void setTrueOrFalse(bool flag) { m_e = flag ? OPT_TRUE : OPT_FALSE; }
};
constexpr bool operator==(const VOptionBool& lhs, const VOptionBool& rhs) {
    return lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VOptionBool& lhs, VOptionBool::en rhs) { return lhs.m_e == rhs; }
constexpr bool operator==(VOptionBool::en lhs, const VOptionBool& rhs) { return lhs == rhs.m_e; }

//######################################################################

class VFileLibName final {
    // Filename and libname pair
    const string m_filename;  // Filename
    const string m_libname;  // Libname
public:
    VFileLibName(const string& filename, const string& libname)
        : m_filename{filename}
        , m_libname{libname} {}
    VFileLibName(const VFileLibName& rhs) = default;
    string filename() const { return m_filename; }
    string libname() const { return m_libname; }
    bool operator==(const VFileLibName& rhs) const {
        return m_filename == rhs.m_filename && m_libname == rhs.m_libname;
    }
    bool operator<(const VFileLibName& rhs) const {
        if (m_filename < rhs.m_filename) return true;
        if (m_filename > rhs.m_filename) return false;
        return m_libname < rhs.m_libname;
    }
};

using VFileLibList = std::vector<VFileLibName>;
using VFileLibSet = std::set<VFileLibName>;

// ######################################################################

class VTimescale final {
public:
    enum en : uint8_t {
        // clang-format off
        TS_100S = 0, TS_10S = 1, TS_1S = 2,
        TS_100MS = 3, TS_10MS = 4, TS_1MS = 5,
        TS_100US = 6, TS_10US = 7, TS_1US = 8,
        TS_100NS = 9, TS_10NS = 10, TS_1NS = 11,
        TS_100PS = 12, TS_10PS = 13, TS_1PS = 14,
        TS_100FS = 15, TS_10FS = 16, TS_1FS = 17,
        // clang-format on
        NONE = 18,
        _ENUM_END
    };
    enum : uint8_t { TS_DEFAULT = TS_1PS };
    enum en m_e;
    // CONSTRUCTOR
    VTimescale()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTimescale(en _e)
        : m_e{_e} {}
    explicit VTimescale(int _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    // Construct from string
    VTimescale(const string& value, bool& badr);
    VTimescale(double value, bool& badr) {
        badr = false;
        for (int i = TS_100S; i < _ENUM_END; ++i) {
            m_e = static_cast<en>(i);
            if (multiplier() == value) break;
        }
        if (multiplier() != value) {
            m_e = NONE;
            badr = true;
        }
    }
    bool isNone() const { return m_e == NONE; }
    // Parse a "unit/precision" string into two VTimescales, with error checking
    static void parseSlashed(FileLine* fl, const char* textp, VTimescale& unitr, VTimescale& precr,
                             bool allowEmpty = false);
    const char* ascii() const {
        static const char* const names[]
            = {"100s", "10s", "1s",    "100ms", "10ms", "1ms",   "100us", "10us", "1us", "100ns",
               "10ns", "1ns", "100ps", "10ps",  "1ps",  "100fs", "10fs",  "1fs",  "NONE"};
        return names[m_e];
    }
    int powerOfTen() const { return 2 - static_cast<int>(m_e); }
    double multiplier() const {
        static const double values[]
            = {100,  10,   1,     1e-1,  1e-2,  1e-3,  1e-4,  1e-5,  1e-6, 1e-7,
               1e-8, 1e-9, 1e-10, 1e-11, 1e-12, 1e-13, 1e-14, 1e-15, 0};
        return values[m_e];
    }
};
constexpr bool operator==(const VTimescale& lhs, const VTimescale& rhs) {
    return lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VTimescale& lhs, VTimescale::en rhs) { return lhs.m_e == rhs; }
constexpr bool operator==(VTimescale::en lhs, const VTimescale& rhs) { return lhs == rhs.m_e; }
// Comparisons are based on time, not enum values, so seconds > milliseconds
constexpr bool operator<(const VTimescale& lhs, const VTimescale& rhs) {
    return lhs.m_e > rhs.m_e;
}
inline std::ostream& operator<<(std::ostream& os, const VTimescale& rhs) {
    return os << rhs.ascii();
}

// ######################################################################

// Information given by --hierarchical-block option
class V3HierarchicalBlockOption final {
public:
    // key:parameter name, value:value (as string)
    using ParamStrMap = std::map<const std::string, std::string>;

private:
    string m_origName;  // module name
    // module name after uniquified
    // same as m_origName for non-parameterized module
    string m_mangledName;
    // overriding parameter values specified by -G option
    ParamStrMap m_parameters;

public:
    explicit V3HierarchicalBlockOption(const string& optstring);
    const string& origName() const { return m_origName; }
    const string& mangledName() const { return m_mangledName; }
    const ParamStrMap params() const { return m_parameters; }
};

using V3HierBlockOptSet = std::map<const std::string, V3HierarchicalBlockOption>;

//######################################################################
// V3Options - Command line options

class V3Options final {
public:
private:
    // TYPES
    using DebugLevelMap = std::map<const std::string, unsigned>;

    // MEMBERS (general options)
    V3OptionsImp* m_impp;  // Slow hidden options

    // clang-format off
    VStringSet m_futures;      // argument: -Wfuture- list
    VStringSet m_future0s;     // argument: -future list
    VStringSet m_future1s;     // argument: -future1 list
    VFileLibSet m_libraryFiles; // argument: Verilog -v files
    VFileLibList m_vFiles;      // argument: Verilog files to read
    VFileLibSet m_vltFiles;     // argument: Verilator config files to read
    VStringSet m_libmapFiles;  // argument: --libmap files to read
    VStringList m_forceIncs;   // argument: -FI
    DebugLevelMap m_debugLevel; // argument: --debugi-<srcfile/tag> <level>
    DebugLevelMap m_dumpLevel;  // argument: --dumpi-<srcfile/tag> <level>
    std::map<const string, string> m_parameters;  // Parameters
    std::map<const string, V3HierarchicalBlockOption> m_hierBlocks;  // main switch: --hierarchical-block
    VStringSet m_fDfgPeepholeDisabled; // argument: -f[no-]dfg-peephole-<name>

    bool m_preprocOnly = false;     // main switch: -E
    bool m_preprocResolve = false;  // main switch: --preproc-resolve
    bool m_makePhony = false;       // main switch: -MP
    bool m_preprocNoLine = false;   // main switch: -P
    bool m_assert = true;           // main switch: --assert
    bool m_assertCase = true;       // main switch: --assert-case
    bool m_autoflush = false;       // main switch: --autoflush
    bool m_bboxSys = false;         // main switch: --bbox-sys
    bool m_bboxUnsup = false;       // main switch: --bbox-unsup
    bool m_binary = false;          // main switch: --binary
    bool m_build = false;           // main switch: --build
    bool m_context = true;          // main switch: --Wcontext
    bool m_coverageExpr = false;    // main switch: --coverage-expr
    bool m_coverageFsm = false;     // main switch: --coverage-fsm
    bool m_coverageLine = false;    // main switch: --coverage-block
    bool m_coverageToggle = false;  // main switch: --coverage-toggle
    bool m_coverageUnderscore = false;  // main switch: --coverage-underscore
    bool m_coverageUser = false;    // main switch: --coverage-func
    bool m_debugCheck = false;      // main switch: --debug-check
    bool m_debugCollision = false;  // main switch: --debug-collision
    bool m_debugEmitV = false;      // main switch: --debug-emitv
    bool m_debugExitElab = false;   // main switch: --debug-exit-elab
    bool m_debugExitParse = false;  // main switch: --debug-exit-parse
    bool m_debugLeak = true;        // main switch: --debug-leak
    bool m_debugNondeterminism = false;  // main switch: --debug-nondeterminism
    bool m_debugOptions = false;    // main switch: --debug-options
    bool m_debugPreprocPassthru = false;  // main switch: --debug-preproc-passthru
    bool m_debugProtect = false;    // main switch: --debug-protect
    bool m_debugSelfTest = false;   // main switch: --debug-self-test
    bool m_debugWidth = false;      // main switch: --debug-width
    bool m_decoration = true;       // main switch: --decoration
    bool m_decorationNodes = false;  // main switch: --decoration=nodes
    bool m_diagnosticsSarif = false;  // main switch: --diagnostics-sarif
    bool m_dpiHdrOnly = false;      // main switch: --dpi-hdr-only
    bool m_emitAccessors = false;   // main switch: --emit-accessors
    bool m_exe = false;             // main switch: --exe
    bool m_flatten = false;         // main switch: --flatten
    bool m_fourstate = false;       // main switch: --fourstate
    bool m_hierarchical = false;    // main switch: --hierarchical
    bool m_ignc = false;            // main switch: --ignc
    bool m_jsonOnly = false;        // main switch: --json-only
    bool m_lintFast = false;        // main switch: --lint-fast
    bool m_lintOnly = false;        // main switch: --lint-only
    bool m_main = false;            // main switch: --main
    bool m_outFormatOk = false;     // main switch: --cc, --sc or --sp was specified
    bool m_pedantic = false;        // main switch: --Wpedantic
    bool m_pinsInoutEnables = false;// main switch: --pins-inout-enables
    bool m_pinsScUint = false;      // main switch: --pins-sc-uint
    bool m_pinsScUintBool = false;  // main switch: --pins-sc-uint-bool
    bool m_pinsScBigUint = false;   // main switch: --pins-sc-biguint
    bool m_pinsUint8 = false;       // main switch: --pins-uint8
    bool m_preprocComments = false;  // main switch: --preproc-comments
    bool m_preprocDefines = false;  // main switch: --preproc-defines
    bool m_profExec = false;        // main switch: --prof-exec
    bool m_protectIds = false;      // main switch: --protect-ids
    bool m_public = false;          // main switch: --public
    bool m_publicFlatRW = false;    // main switch: --public-flat-rw
    bool m_publicIgnore = false;    // main switch: --public-ignore
    bool m_publicParams = false;    // main switch: --public-params
    bool m_quietExit = false;       // main switch: --quiet-exit
    bool m_quietStats = false;      // main switch: --quiet-stats
    bool m_relativeIncludes = false;  // main switch: --relative-includes
    bool m_savable = false;         // main switch: --savable
    VOptionBool m_schedZeroDelay;  // main switch: --sched-zero-delay
    bool m_stdPackage = true;       // main switch: --std-package
    bool m_stdWaiver = true;        // main switch: --std-waiver
    bool m_systemC = false;         // main switch: --sc: System C instead of simple C++
    bool m_stats = false;           // main switch: --stats
    bool m_statsVars = false;       // main switch: --stats-vars
    VOptionBool m_timing;           // main switch: --timing
    bool m_trace = false;           // main switch: --trace
    bool m_traceCoverage = false;   // main switch: --trace-coverage
    bool m_traceParams = true;      // main switch: --trace-params
    bool m_noTraceTop = false;      // main switch: --no-trace-top
    bool m_verilate = true;         // main switch: --verilate
    VOptionBool m_vpi;              // main switch: --vpi
    bool m_waiverMultiline = false;  // main switch: --waiver-multiline

    int         m_buildJobs = -1;    // main switch: --build-jobs, -j
    int         m_coverageExprMax = 32;    // main switch: --coverage-expr-max
    uint32_t    m_coverageMaxBins = 1024;  // main switch: --coverage-max-bins
    uint32_t    m_coverageMaxRealBins = 1024;  // main switch: --coverage-max-real-bins
    int         m_coverageMaxWidth = 256; // main switch: --coverage-max-width
    int         m_debugAllocRandom = 0;  // main switch: --debug-alloc-random <seed>
    int         m_expandLimit = 256;  // main switch: --expand-limit
    int         m_gateStmts = 100;    // main switch: --gate-stmts
    int         m_hierChild = 0;      // main switch: --hierarchical-child
    int         m_hierThreads = 0;      // main switch: --hierarchical-threads
    int         m_ifDepth = 0;      // main switch: --if-depth
    int         m_inlineMult = 2000;   // main switch: --inline-mult
    int         m_instrCountDpi = 200;   // main switch: --instr-count-dpi
    bool        m_jsonEditNums = true; // main switch: --no-json-edit-nums
    bool        m_jsonIds = true; // main switch: --no-json-ids
    int         m_maxNumWidth = 65536;  // main switch: --max-num-width
    int         m_funcRecursion = 1000;  // main switch: --func-recursion-depth
    int         m_moduleRecursion = 100;  // main switch: --module-recursion-depth
    int         m_outputSplit = 20000;  // main switch: --output-split
    int         m_pinsBv = 65;       // main switch: --pins-bv
    int         m_preprocTokenLimit = 40000; // main switch: --preproc-token-limit
    int         m_publicDepth = 0;   // main switch: --public-depth
    int         m_replicationLimit = 8192; // main switch: --replication-limit
    VOptionBool m_skipIdentical;  // main switch: --skip-identical
    bool        m_stopFail = true;  // main switch: --stop-fail
    int         m_threads = 1;      // main switch: --threads
    VTimescale  m_timeDefaultPrec;  // main switch: --timescale
    VTimescale  m_timeDefaultUnit;  // main switch: --timescale
    VTimescale  m_timeOverridePrec;  // main switch: --timescale-override
    VTimescale  m_timeOverrideUnit;  // main switch: --timescale-override
    int         m_traceDepth = 0;   // main switch: --trace-depth
    int         m_unrollCount = 64;  // main switch: --unroll-count
    int         m_unrollLimit = 16384;  // main switch: --unroll-limit
    int         m_unrollStmts = 30000;  // main switch: --unroll-stmts
    int         m_constraintArrayLimit = 64;  // main switch: --constraint-array-limit
    int         m_verilateJobs = -1;  // main switch: --verilate-jobs

    int         m_compLimitMembers = 64;  // compiler selection; number of members in struct before make anon array

    string      m_buildDepBin;  // main switch: --build-dep-bin {filename}
    string      m_diagnosticsSarifOutput;  // main switch: --diagnostics-sarif-output
    string      m_exeName;      // main switch: -o {name}
    VFileLibList m_hierParamsFile; // main switch: --hierarchical-params-file
    string      m_jsonOnlyOutput;    // main switch: --json-only-output
    string      m_jsonOnlyMetaOutput;    // main switch: --json-only-meta-output
    string      m_l2Name;       // main switch: --l2name; "" for top-module's name
    string      m_libCreate;    // main switch: --lib-create {lib_name}
    string      m_makeDir;      // main switch: -Mdir
    string      m_modPrefix;    // main switch: --mod-prefix
    string      m_pipeFilter;   // main switch: --pipe-filter
    string      m_prefix;       // main switch: --prefix
    string      m_protectKey;   // main switch: --protect-key
    string      m_topModule;    // main switch: --top-module
    string      m_unusedRegexp; // main switch: --unused-regexp
    string      m_waiverOutput;  // main switch: --waiver-output {filename}
    string      m_work = "work";  // main switch: --work {libname}
    string      m_xAssign;      // main switch: --x-assign

    // Language is now held in FileLine, on a per-node basis. However we still
    // have a concept of the default language at a global level.
    V3LangCode  m_defaultLanguage;      // main switch: --language

    // MEMBERS (optimizations)
    bool m_fAcycSimp;    // main switch: -fno-acyc-simp: acyclic pre-optimizations
    bool m_fAssemble;    // main switch: -fno-assemble: assign assemble
    bool m_fBitScanLoops;  // main switch: -fno-bit-scan-loops: convert bit scan loops to builtins
    bool m_fCaseDecoder; // main switch: -fno-case-decoder: case decoder conversion
    bool m_fCaseTable;   // main switch: -fno-case-table: case table conversion
    bool m_fCaseTree;    // main switch: -fno-case-tree: case tree conversion
    bool m_fConst;       // main switch: -fno-const: constant folding
    bool m_fConstBeforeDfg = true;  // main switch: -fno-const-before-dfg for testing only!
    bool m_fConstBitOpTree;  // main switch: -fno-const-bit-op-tree constant bit op tree
    bool m_fDedupe;      // main switch: -fno-dedupe: logic deduplication
    bool m_fDfgPeephole = true; // main switch: -fno-dfg-peephole
    bool m_fDfgPushDownSels = true; // main switch: -fno-dfg-push-down-sels
    bool m_fDfg;         // main switch: -fno-dfg
    bool m_fDfgSynthesizeAll = false;  // main switch: -fdfg-synthesize-all
    bool m_fDeadAssigns;     // main switch: -fno-dead-assigns: remove dead assigns
    bool m_fDeadCells;   // main switch: -fno-dead-cells: remove dead cells
    bool m_fDeadMethods;   // main switch: -fno-dead-methods: remove dead methods
    bool m_fGate;        // main switch: -fno-gate: gate wire elimination
    // main switch: -fno-ico-change-detect: input change detection optimization
    VOptionBool m_fIcoChangeDetect{VOptionBool::OPT_DEFAULT_TRUE};
    bool m_fInline;      // main switch: -fno-inline: module inlining
    bool m_fInlineFuncs = true;  // main switch: -fno-inline-funcs: function inlining
    bool m_fInlineFuncsEager = true;  // main switch: -fno-inline-funcs-eager: don't inline eagerly
    bool m_fLife;        // main switch: -fno-life: variable lifetime
    bool m_fReorder;     // main switch: -fno-reorder: reorder assignments in blocks
    bool m_fSlice = true;  // main switch: -fno-slice: array assignment slicing
    int  m_fSliceElementLimit = 256;  // main switch: --fslice-element-limit
    bool m_fSplit;       // main switch: -fno-split: always assignment splitting
    bool m_fTaskifyAll = false;  // main switch: --ftaskify-all-forked
    bool m_fVarSplit;    // main switch: -fno-var-split: automatic variable splitting
    // clang-format on

    bool m_available = false;  // Set to true at the end of option parsing

    // METHODS
    void addArg(char** argv, size_t count, bool isForRerun);
    void addArg(const std::string& arg, bool isForRerun);
    void addLineArg(const string& arg);
    void addDefine(const string& defline, bool allowPlus) VL_MT_DISABLED;
    void addFuture(const string& flag);
    void addFuture0(const string& flag);
    void addFuture1(const string& flag);
    void addIncDirUser(const string& incdir);  // User requested
    void addIncDirUserRecursive(const string& incdir);  // User requested, with subdirs
    void addIncDirFallback(const string& incdir);  // Low priority if not found otherwise
    void addParameter(const string& paramline, bool allowPlus);
    void addLangExt(const string& langext, const V3LangCode& lc);
    void addLibExtV(const string& libext);
    void optimize(int level);
    void showVersion(bool verbose);
    void coverage(bool flag) {
        m_coverageLine = m_coverageToggle = m_coverageExpr = m_coverageFsm = m_coverageUser = flag;
    }
    static bool suffixed(const string& sw, const char* arg);
    static string parseFileArg(const string& optdir, const string& relfilename);
    string filePathCheckOneDir(const string& modname, const string& dirname);
    static int stripOptionsForChildRun(const string& opt, bool forTop);
    void validateIdentifier(FileLine* fl, const string& arg, const string& opt);

    // CONSTRUCTORS
    VL_UNCOPYABLE(V3Options);

public:
    V3Options();
    ~V3Options();
    void setDebugMode(int level);
    unsigned debugLevel(const string& tag) const VL_MT_SAFE;
    unsigned debugSrcLevel(const string& srcfile_path) const VL_MT_SAFE;
    unsigned dumpLevel(const string& tag) const VL_MT_SAFE;
    unsigned dumpSrcLevel(const string& srcfile_path) const VL_MT_SAFE;

    // METHODS
    void addLibMapFile(const string& filename);
    void addLibraryFile(const string& filename, const string& libname);
    void addVFile(const string& filename, const string& libname);
    void addVltFile(const string& filename, const string& libname);
    void addForceInc(const string& filename);
    bool available() const VL_MT_SAFE { return m_available; }
    void ccSet();
    void decorations(FileLine* fl, const string& filename);
    void notify() VL_MT_DISABLED;

    // ACCESSORS (options)
    bool preprocOnly() const { return m_preprocOnly; }
    bool makePhony() const { return m_makePhony; }
    bool preprocNoLine() const { return m_preprocNoLine; }
    bool preprocResolve() const { return m_preprocResolve; }
    int preprocTokenLimit() const { return m_preprocTokenLimit; }
    bool systemC() const VL_MT_SAFE { return m_systemC; }
    bool savable() const VL_MT_SAFE { return m_savable; }
    VOptionBool schedZeroDelay() const { return m_schedZeroDelay; }
    bool stats() const { return m_stats; }
    bool statsVars() const { return m_statsVars; }
    bool stdPackage() const { return m_stdPackage; }
    bool stdWaiver() const { return m_stdWaiver; }
    bool assertOn() const { return m_assert; }  // assertOn as __FILE__ may be defined
    bool assertCase() const { return m_assertCase; }
    bool autoflush() const { return m_autoflush; }
    bool bboxSys() const { return m_bboxSys; }
    bool bboxUnsup() const { return m_bboxUnsup; }
    bool build() const { return m_build; }
    string buildDepBin() const { return m_buildDepBin; }
    void buildDepBin(const string& flag) { m_buildDepBin = flag; }
    bool context() const VL_MT_SAFE { return m_context; }
    bool coverage() const VL_MT_SAFE {
        // Any enabled coverage kind, including FSM coverage. Code generation
        // and runtime support should generally query this accessor.
        return m_coverageLine || m_coverageToggle || m_coverageExpr || m_coverageUser
               || m_coverageFsm;
    }
    bool coverageNonFsm() const VL_MT_SAFE {
        // The broad line/toggle/expr/user coverage transforms use this
        // accessor. FSM coverage shares the overall coverage umbrella, but its
        // extraction still happens through a separate early-recognition path.
        return m_coverageLine || m_coverageToggle || m_coverageExpr || m_coverageUser;
    }
    bool coverageExpr() const { return m_coverageExpr; }
    bool coverageFsm() const { return m_coverageFsm; }
    bool coverageLine() const { return m_coverageLine; }
    bool coverageToggle() const { return m_coverageToggle; }
    bool coverageUnderscore() const { return m_coverageUnderscore; }
    bool coverageUser() const { return m_coverageUser; }
    bool debugCheck() const VL_MT_SAFE { return m_debugCheck; }
    bool debugCollision() const { return m_debugCollision; }
    bool debugEmitV() const VL_MT_SAFE { return m_debugEmitV; }
    bool debugExitElab() const { return m_debugExitElab; }
    bool debugExitParse() const { return m_debugExitParse; }
    bool debugLeak() const { return m_debugLeak; }
    bool debugNondeterminism() const { return m_debugNondeterminism; }
    bool debugPreprocPassthru() const VL_MT_SAFE { return m_debugPreprocPassthru; }
    bool debugProtect() const VL_MT_SAFE { return m_debugProtect; }
    bool debugSelfTest() const { return m_debugSelfTest; }
    bool debugWidth() const VL_PURE { return m_debugWidth; }
    bool decoration() const VL_MT_SAFE { return m_decoration; }
    bool decorationNodes() const VL_MT_SAFE { return m_decorationNodes; }
    bool diagnosticsSarif() const VL_MT_SAFE { return m_diagnosticsSarif; }
    bool dumpAstPatterns() const {
        return m_dumpLevel.count("ast-patterns") && m_dumpLevel.at("ast-patterns");
    }
    bool dumpDefines() const { return m_dumpLevel.count("defines") && m_dumpLevel.at("defines"); }
    bool dumpDfgPatterns() const {
        return m_dumpLevel.count("dfg-patterns") && m_dumpLevel.at("dfg-patterns");
    }
    bool dumpTreeDot() const {
        return m_dumpLevel.count("tree-dot") && m_dumpLevel.at("tree-dot");
    }
    bool emitAccessors() const { return m_emitAccessors; }
    bool exe() const { return m_exe; }
    bool dpiHdrOnly() const { return m_dpiHdrOnly; }
    bool flatten() const { return m_flatten; }
    bool fourstate() const { return m_fourstate; }
    VOptionBool timing() const { return m_timing; }
    bool trace() const { return m_trace; }
    bool traceCoverage() const { return m_traceCoverage; }
    bool traceParams() const { return m_traceParams; }
    bool main() const { return m_main; }
    bool outFormatOk() const { return m_outFormatOk; }
    bool jsonOnly() const { return m_jsonOnly; }
    bool keepTempFiles() const { return (V3Error::debugDefault() != 0); }
    bool pedantic() const { return m_pedantic; }
    bool pinsInoutEnables() const { return m_pinsInoutEnables; }
    bool pinsScUint() const { return m_pinsScUint; }
    bool pinsScUintBool() const { return m_pinsScUintBool; }
    bool pinsScBigUint() const VL_MT_SAFE { return m_pinsScBigUint; }
    bool pinsUint8() const { return m_pinsUint8; }
    bool preprocComments() const { return m_preprocComments; }
    bool preprocDefines() const { return m_preprocDefines; }
    bool profExec() const { return m_profExec; }
    bool protectIds() const VL_MT_SAFE { return m_protectIds; }
    bool allPublic() const { return m_public; }
    bool publicParams() const { return m_publicParams; }
    bool publicOff() const { return m_publicIgnore; }
    bool publicFlatRW() const { return m_publicFlatRW; }
    int publicDepth() const { return m_publicDepth; }
    bool anyPublicFlat() const { return m_publicParams || m_publicFlatRW || m_publicDepth; }
    bool lintFast() const VL_MT_SAFE { return m_lintFast; }
    bool lintOnly() const VL_MT_SAFE { return m_lintOnly; }
    bool ignc() const { return m_ignc; }
    bool quietExit() const VL_MT_SAFE { return m_quietExit; }
    bool quietStats() const VL_MT_SAFE { return m_quietStats; }
    bool verilate() const { return m_verilate; }
    bool vpi() const { return m_vpi.isTrue(); }
    bool waiverMultiline() const { return m_waiverMultiline; }
    bool serializeOnly() const { return m_jsonOnly; }
    bool topIfacesSupported() const { return lintOnly() && !hierarchical(); }

    int buildJobs() const VL_MT_SAFE { return m_buildJobs; }
    int coverageExprMax() const { return m_coverageExprMax; }
    uint32_t coverageMaxBins() const { return m_coverageMaxBins; }
    uint32_t coverageMaxRealBins() const { return m_coverageMaxRealBins; }
    int coverageMaxWidth() const { return m_coverageMaxWidth; }
    int debugAllocRandom() const { return m_debugAllocRandom; }
    bool dumpTreeAddrids() const VL_MT_SAFE;
    int expandLimit() const { return m_expandLimit; }
    int gateStmts() const { return m_gateStmts; }
    int ifDepth() const { return m_ifDepth; }
    int inlineMult() const { return m_inlineMult; }
    int instrCountDpi() const { return m_instrCountDpi; }
    bool jsonEditNums() const { return m_jsonEditNums; }
    bool jsonIds() const { return m_jsonIds; }
    int maxNumWidth() const { return m_maxNumWidth; }
    int funcRecursionDepth() const { return m_funcRecursion; }
    int moduleRecursionDepth() const { return m_moduleRecursion; }
    int outputSplit() const { return m_outputSplit; }
    int pinsBv() const VL_MT_SAFE { return m_pinsBv; }
    int replicationLimit() const { return m_replicationLimit; }
    VOptionBool skipIdentical() const { return m_skipIdentical; }
    bool stopFail() const { return m_stopFail; }
    int threads() const VL_MT_SAFE { return m_threads; }
    bool mtasks() const VL_MT_SAFE { return (m_threads > 1); }
    VTimescale timeDefaultPrec() const { return m_timeDefaultPrec; }
    VTimescale timeDefaultUnit() const { return m_timeDefaultUnit; }
    VTimescale timeOverridePrec() const { return m_timeOverridePrec; }
    VTimescale timeOverrideUnit() const { return m_timeOverrideUnit; }
    VTimescale timeComputePrec(const VTimescale& flag) const;
    VTimescale timeComputeUnit(const VTimescale& flag) const;
    int traceDepth() const { return m_traceDepth; }
    int unrollCount() const { return m_unrollCount; }
    int unrollLimit() const { return m_unrollLimit; }
    int unrollStmts() const { return m_unrollStmts; }
    int constraintArrayLimit() const { return m_constraintArrayLimit; }
    int verilateJobs() const { return m_verilateJobs; }

    int compLimitMembers() const VL_MT_SAFE { return m_compLimitMembers; }

    string diagnosticsSarifOutput() const VL_MT_SAFE {
        return m_diagnosticsSarifOutput.empty() ? makeDir() + "/" + prefix() + ".sarif"
                                                : m_diagnosticsSarifOutput;
    }
    string exeName() const { return m_exeName != "" ? m_exeName : prefix(); }
    VFileLibList hierParamFile() const { return m_hierParamsFile; }
    string jsonOnlyOutput() const { return m_jsonOnlyOutput; }
    string jsonOnlyMetaOutput() const { return m_jsonOnlyMetaOutput; }
    string l2Name() const { return m_l2Name; }
    string libCreate() const { return m_libCreate; }
    string makeDir() const VL_MT_SAFE { return m_makeDir; }
    string modPrefix() const VL_MT_SAFE { return m_modPrefix; }
    string pipeFilter() const { return m_pipeFilter; }
    string prefix() const VL_MT_SAFE { return m_prefix; }
    // Not just called protectKey() to avoid bugs of not using protectKeyDefaulted()
    bool protectKeyProvided() const { return !m_protectKey.empty(); }
    string protectKeyDefaulted() VL_MT_SAFE;  // Set default key if not set by user
    string topModule() const { return m_topModule; }  // As AstNode::encodeName()
    bool noTraceTop() const { return m_noTraceTop; }
    string unusedRegexp() const { return m_unusedRegexp; }
    string waiverOutput() const { return m_waiverOutput; }
    string work() const { return m_work; }
    bool isWaiverOutput() const { return !m_waiverOutput.empty(); }
    string xAssign() const { return m_xAssign; }

    const VFileLibSet& libraryFiles() const { return m_libraryFiles; }
    const VStringSet& libmapFiles() const { return m_libmapFiles; }
    const VFileLibList& vFiles() const { return m_vFiles; }
    const VFileLibSet& vltFiles() const { return m_vltFiles; }
    const VStringList& forceIncs() const { return m_forceIncs; }

    bool hasParameter(const string& name);
    string parameter(const string& name);
    void checkParameters();

    bool isFuture(const string& flag) const;
    bool isFuture0(const string& flag) const;
    bool isFuture1(const string& flag) const;
    bool isLibraryFile(const string& filename, const string& libname) const;

    // ACCESSORS (optimization options)
    bool fAcycSimp() const { return m_fAcycSimp; }
    bool fAssemble() const { return m_fAssemble; }
    bool fBitScanLoops() const { return m_fBitScanLoops; }
    bool fCaseDecoder() const { return m_fCaseDecoder; }
    bool fCaseTable() const { return m_fCaseTable; }
    bool fCaseTree() const { return m_fCaseTree; }
    bool fConst() const { return m_fConst; }
    bool fConstBeforeDfg() const { return m_fConstBeforeDfg; }
    bool fConstBitOpTree() const { return m_fConstBitOpTree; }
    bool fDedupe() const { return m_fDedupe; }
    bool fDfg() const { return m_fDfg; }
    bool fDfgPeephole() const { return m_fDfgPeephole; }
    bool fDfgPushDownSels() const { return m_fDfgPushDownSels; }
    bool fDfgSynthesizeAll() const { return m_fDfgSynthesizeAll; }
    bool fDfgPeepholeEnabled(const std::string& name) const {
        return !m_fDfgPeepholeDisabled.count(name);
    }
    bool fDeadAssigns() const { return m_fDeadAssigns; }
    bool fDeadCells() const { return m_fDeadCells; }
    bool fDeadMethods() const { return m_fDeadMethods; }
    bool fGate() const { return m_fGate; }
    bool fInline() const { return m_fInline; }
    bool fInlineFuncs() const { return m_fInlineFuncs; }
    bool fInlineFuncsEager() const { return m_fInlineFuncsEager; }
    bool fLife() const { return m_fLife; }
    bool fReorder() const { return m_fReorder; }
    bool fSlice() const { return m_fSlice; }
    int fSliceElementLimit() const { return m_fSliceElementLimit; }
    bool fSplit() const { return m_fSplit; }
    bool fTaskifyAll() const { return m_fTaskifyAll; }
    bool fVarSplit() const { return m_fVarSplit; }

    bool hierarchical() const { return m_hierarchical; }
    int hierChild() const VL_MT_SAFE { return m_hierChild; }
    int hierThreads() const VL_MT_SAFE { return m_hierThreads == 0 ? m_threads : m_hierThreads; }
    bool hierTop() const VL_MT_SAFE { return !m_hierChild && !m_hierBlocks.empty(); }
    const V3HierBlockOptSet& hierBlocks() const { return m_hierBlocks; }
    // Directory to save .tree, .dot, .dat, .vpp for hierarchical block top
    // Returns makeDir() unless top module of hierarchical Verilation.
    string hierTopDataDir() const VL_MT_SAFE {
        return hierTop() ? (makeDir() + '/' + prefix() + "__hier.dir") : makeDir();
    }

    // METHODS (from main)
    static string version() VL_PURE;
    static string argString(int argc, char** argv);  ///< Return list of arguments as simple string
    string allArgsString() const VL_MT_SAFE;  ///< Return all passed arguments as simple string
    const std::list<std::pair<std::list<std::string>, bool>>& allArgs() const;
    // Return options for child hierarchical blocks when forTop==false, otherwise returns args for
    // the top module.
    string allArgsStringForHierBlock(bool forTop) const;
    void parseOpts(FileLine* fl, int argc, char** argv) VL_MT_DISABLED;
    void parseOptsList(FileLine* fl, const string& optdir, int argc, char** argv) VL_MT_DISABLED;
    void parseOptsFile(FileLine* fl, const string& filename, bool rel) VL_MT_DISABLED;

    // METHODS (environment)
    // Most of these may be built into the executable with --enable-defenv,
    // see the README.  If adding new variables, also see src/Makefile_obj.in
    // Also add to V3Options::showVersion()
    static string getenvBuiltins(const string& var);
    static string getenvMAKE();
    static string getenvMAKEFLAGS();
    static string getenvPERL();
    static string getenvPYTHON3();
    static string getenvSYSTEMC();
    static string getenvSYSTEMC_ARCH();
    static string getenvSYSTEMC_INCLUDE();
    static string getenvSYSTEMC_LIBDIR();
    static string getenvVERILATOR_ROOT();
    static string getenvVERILATOR_SOLVER();
    static string getStdPackagePath();
    static string getStdWaiverPath();
    // The built-in std package and lint waivers are compiled into the binary;
    // these return their contents.
    static const char* getStdPackageText();
    static const char* getStdWaiverText();
    static string getSupported(const string& var);
    static bool systemCSystemWide();
    static bool systemCFound();  // SystemC installed, or environment points to it
    static bool coroutineSupport();  // Configured C++ compiler supports coroutines
    static bool devAsan();  // 'verilator_bin' built with AddressSanitizer
    static bool devGcov();  // 'verilator_bin' built with code coverage for gcov
    static bool tsanSupport();  // Configured C++ compiler supports ThreadSanitizer

    // METHODS (file utilities using these options)
    string fileExists(const string& filename);
    string filePath(FileLine* fl, const string& modname, const string& lastpath,
                    const string& errmsg);
    string filePathLookedMsg(FileLine* fl, const string& modname);
    V3LangCode fileLanguage(const string& filename);
    static bool fileStatNormal(const string& filename);
    static bool fileIsAbsent(const string& filename);

    // METHODS (other OS)
    static void throwSigsegv();
};

//######################################################################

#endif  // guard
