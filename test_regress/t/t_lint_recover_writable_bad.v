// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// Unresolved writable targets must keep their original name errors without
// cascading into electrical-short/cast/readmem errors, and independent lint
// checks after them must still run.
`default_nettype none

module sub(input logic a, output logic b);
  assign b = a;
endmodule

module t;
  logic a, b, c;
  int src;

  sub u_sub (.a(a), .b(absent_pin));

  initial begin
    $readmemh("t/t_lint_recover_writable_missing.hex", mem_missing);
    $cast(dst_missing, src);
  end

  always_comb begin
    if (c) c = 1'b1;
  end
endmodule
