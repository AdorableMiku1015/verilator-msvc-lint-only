// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// A `line directive changes the reported filename of later tokens, not which
// input owns them. The $unit declaration and the module below must still be
// rolled back with this file.
typedef logic [1:0] line_rolled_back_t;

`line 100 "t/t_lint_parse_isolate_fake.sv" 0
module damaged;
  assign = ;
endmodule
