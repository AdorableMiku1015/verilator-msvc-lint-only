// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// A diagnosed, repaired width error must not hide independent later checks.
module t (
    input logic a,
    input logic [2:0] data_in,
    output logic b,
    output logic [1:0] data_out
);
  always_comb begin
    case (a)
      1'b0: b = 1'b0;
    endcase
  end

`ifdef TEST_ZERO
  assign data_out = {0{data_in}};
`elsif TEST_ENUM_WIDTH
  typedef enum logic [1:0] { TOO_WIDE = 3'b100 } state_t;
  state_t state;
  assign state = state_t'(data_in);
  assign data_out = state;
`elsif TEST_ENUM_VALUE
  typedef enum logic [1:0] { ZERO, ONE } state_t;
  state_t state;
  assign state = data_in[1:0];
  assign data_out = state;
`else
  // Also run with WIDTHTRUNC explicitly promoted to an error.
  assign data_out = data_in;
`endif

`ifdef TEST_STRICT
  // Unlike an enum conversion, this leaves incompatible object types.
  class Cls;
  endclass
  Cls c;
  initial c = 1;
`endif
endmodule
