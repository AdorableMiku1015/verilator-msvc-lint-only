// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// Included by a syntactically damaged input. Both the typedef and the module
// must be rolled back with that input, or the replacement in the clean file
// would be a duplicate declaration.
typedef logic [1:0] inc_rolled_back_t;

module included_damaged;
  logic [1:0] width_probe;
  assign width_probe = 2'b11;
endmodule
