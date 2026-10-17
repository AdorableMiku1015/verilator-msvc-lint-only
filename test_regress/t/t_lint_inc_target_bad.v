// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

`default_nettype none
module t (
    input logic a,
    output logic b
);
  // Independent later diagnostics must survive the unusable increment target.
  always_comb begin
    case (a)
      1'b0: b = 1'b0;
    endcase
  end

  logic result_bit;
  logic [6:0] vec;
`ifdef TEST_PRE
  initial ++absent;
`elsif TEST_POSTDEC
  initial absent--;
`elsif TEST_PREDEC
  initial --absent;
`elsif TEST_EXPR
  // The stand-in value has the same width, so no genuine width warning applies.
  initial result_bit = absent++;
`elsif TEST_INDEX
  // The unresolved index must not become a bogus one-bit writable target.
  initial vec[absent]++;
`else
  initial absent++;
`endif
endmodule
`default_nettype wire
