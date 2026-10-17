// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// An unresolved name inside a rejected writable target must not hide the
// independent range checks of the remaining selector expressions.
`default_nettype none

module t;
  logic [7:0] data;

  initial begin
    data[absent_a + 20] = 1'b0;
    data[absent_b + 21]++;
    release data[absent_c + 22];
  end
endmodule
