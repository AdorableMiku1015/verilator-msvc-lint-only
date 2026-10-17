// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// An ordinary warning promoted to an error must still collect later lint.
// The unavailable IP also exercises the library-load error gate.
// verilog_format: off
`ifdef TEST_PROTECTED
`pragma protect begin_protected
`pragma protect encoding = (enctype = "BASE64", line_length = 76, bytes = 3)
`pragma protect data_block
aW5p
`pragma protect end_protected
`endif
// verilog_format: on

module t (
    input logic a,
    output logic b
);
  missing_ip u_ip ();
  always_comb begin
    case (a)
      1'b0: b = 1'b0;
    endcase
  end
endmodule

// Dead parameterized templates must stay dead even after an early error.
class Unused #(int N);
  logic [N-1:0] unused_template_member;
endclass
