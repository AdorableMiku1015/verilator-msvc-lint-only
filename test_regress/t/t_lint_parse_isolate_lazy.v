// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// Lazy loading fails before a module identity is reduced. The requested module
// must be cached as unavailable, and this $unit addition must be rolled back.
typedef logic [1:0] parse_rolled_back_t;
module
