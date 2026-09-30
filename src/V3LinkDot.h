// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Link XREF signals/functions together
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

#ifndef VERILATOR_V3LINKDOT_H_
#define VERILATOR_V3LINKDOT_H_

#include "config_build.h"
#include "verilatedos.h"

#include "V3Ast.h"
#include "V3Error.h"

//============================================================================

enum VLinkDotStep : uint8_t { LDS_PRIMARY, LDS_PARAMED, LDS_SCOPED };

class V3LinkDot final {
    static void dumpSubstep(const string& name) VL_MT_DISABLED;
    static void linkDotGuts(AstNetlist* rootp, VLinkDotStep step) VL_MT_DISABLED;

public:
    static void linkDotPrimary(AstNetlist* nodep) VL_MT_DISABLED;
    static void linkDotParamed(AstNetlist* nodep) VL_MT_DISABLED;
    static void linkDotScope(AstNetlist* nodep) VL_MT_DISABLED;
    // Mark a name this pass could not resolve, so repairUnresolved() can remove
    // the reference before the later passes read the tree
    static void markUnresolved(AstNode* nodep) VL_MT_DISABLED;
    // Replace or remove what markUnresolved() marked, and stop the run when a
    // marked node cannot be placed, so the later passes never see a tree they
    // cannot traverse
    static void repairUnresolved(AstNetlist* nodep) VL_MT_DISABLED;};

#endif  // Guard
