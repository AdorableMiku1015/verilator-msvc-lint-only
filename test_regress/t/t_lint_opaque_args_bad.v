// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

`default_nettype none
module t (
    input logic a,
    input logic [6:0] bus,
    input logic opaque_in,
    output logic b,
    output logic result,
    output logic bus_parity,
    output wire opaque_out
);
  // These independent late checks must survive every unresolved operation.
  always_comb begin
    case (a)
      1'b0: b = 1'b0;
    endcase
  end
  assign bus_parity = ^bus;

`ifdef TEST_MODULE
  localparam logic [6:0] PARAM_BUS = 7'h35;
  absent_ip #(
      .WIDTH(PARAM_BUS[20]),
      .EXTRA(missing_param)
  ) u_ip (
      .bad_select(bus[20]),
      .extra(missing_arg),
      .source(opaque_in),
      .sink(opaque_out)
  );
  assign result = a;
`elsif TEST_TASK
  initial absent_task(bus[20], missing_arg, opaque_in, opaque_out);
  assign result = a;
`elsif TEST_DTASK
  initial absent_scope.task_name(bus[20], missing_arg, opaque_in, opaque_out);
  assign result = a;
`elsif TEST_FUNCTION
  assign result = absent_function(bus[20], missing_arg, opaque_in, opaque_out);
`elsif TEST_DFUNC
  assign result = absent_scope.function_name(bus[20], missing_arg, opaque_in, opaque_out);
`else
  // The LHS is unusable, but independent RHS errors must still be checked.
  wire rhs_only;
  initial missing_write = {bus[20], missing_arg, opaque_in, opaque_out, rhs_only};
  assign result = a;
  // Unlike unknown calls and pins, this RHS has a known read-only role.
  assign opaque_out = opaque_in;
`endif
endmodule
`default_nettype wire
