// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// The include recursion fails preprocessing. The next input must still be
// opened and report its own syntax error.
`include "t/t_lint_parse_isolate_recinc.vh"
