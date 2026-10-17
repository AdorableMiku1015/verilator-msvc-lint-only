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
  // These late diagnostics must survive the unrelated unresolved targets.
  always_comb begin
    case (a)
      1'b0: b = 1'b0;
    endcase
  end

  logic [6:0] known;
`ifdef TEST_DELAYED_FIRE
  event present;
  initial begin
    known = '0;
    ->> absent_event;
    ->> absent_scope.triggered;
  end
`elsif TEST_FIRE
  // No event declaration: the recovered trigger must not reach V3Delayed.
  initial begin
    known = '0;
    -> absent_event;
    -> absent_scope.triggered;
  end
`else
  initial begin
    // Previous sibling assignments are not parents of the release operand.
    known = '0;
    release absent;
    release absent_scope.value;
    release {known, absent_concat};
    release known[absent_index];
    deassign absent_deassign;
    force absent_force = a;
  end
`endif
endmodule
`default_nettype wire
