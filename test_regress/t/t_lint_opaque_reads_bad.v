// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

`default_nettype none
module t (
    input logic a,
    input logic [6:0] data,
    output logic b,
    output logic r,
    output logic observed
);
  logic [6:0] x;
`ifndef TEST_DIRECT
  logic [6:0] opaque_signal;
`endif

  function automatic bit known(input logic [6:0] value);
    return ^value;
  endfunction

`ifdef TEST_TWO
  // A possible opaque write must not replace this definite writer's origin.
  always_comb x = data;
`ifndef TEST_DIRECT
  always_comb opaque_signal = data;
`endif
`endif

`ifdef TEST_DIRECT
  always_comb r = a ^ known(x);
`else
  // The known input is a real read even though its result goes to a black box.
  // The next bare actual still has unknown direction after that nested call.
  always_comb r = a ^ absent_fn(known(x), opaque_signal);
`endif

`ifndef TEST_READ
  always_comb x = ~data;
`ifndef TEST_DIRECT
  always_comb opaque_signal = ~data;
`endif
`endif

  assign observed = ^data;
  always_comb begin
    case (a)
      1'b0: b = 1'b0;
    endcase
  end
endmodule
`default_nettype wire
