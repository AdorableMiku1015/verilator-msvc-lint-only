// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

`default_nettype none
module t (
    input logic a,
    input logic [6:0] bus,
    output logic b,
    output logic [31:0] result
);
  always_comb begin
    case (a)
      1'b0: b = 1'b0;
    endcase
  end

  task automatic out_int(output int target, input logic [1:0] narrow);
    target = int'(narrow);
  endtask
  task automatic ref_int(ref int target, input logic [1:0] narrow);
    target = int'(narrow);
  endtask
  task automatic out_bit(output bit target, input logic [1:0] narrow);
    target = ^narrow;
  endtask
  task automatic ref_bit(ref bit target, input logic [1:0] narrow);
    target = ^narrow;
  endtask
  function automatic int out_func(output int target, input logic [1:0] narrow);
    target = int'(narrow);
    return target;
  endfunction

`ifdef TEST_FUNCTION
  assign result = out_func(missing_target, bus);
`else
  assign result = 32'(bus);
  initial begin
`ifdef TEST_OUT_INT
    out_int(missing_target, bus);
`elsif TEST_REF_INT
    ref_int(missing_target, bus);
`elsif TEST_OUT_BIT
    out_bit(missing_target, bus);
`elsif TEST_REF_BIT
    ref_bit(missing_target, bus);
`else
    // This is a real user constant, not an unresolved-name recovery value.
    out_int(32'sd0, bus);
`endif
  end
`endif
endmodule
`default_nettype wire
