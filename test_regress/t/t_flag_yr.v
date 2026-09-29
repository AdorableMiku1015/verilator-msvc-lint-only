// -*- Verilog -*-
// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

module t;
  // Both of these live in subdirectories of the -yr search directory, and the
  // second one is two levels down, so neither is found unless the search
  // recurses.
  t_flag_yr_sub sub ();
  t_flag_yr_deep deep ();
endmodule
