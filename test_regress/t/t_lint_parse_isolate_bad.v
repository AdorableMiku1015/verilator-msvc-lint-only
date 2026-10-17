// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// Preprocessing succeeds even when the subsequent SV parse does not. The
// following independent file must still see this macro after quarantine.
// verilog_format: off
`define T_LINT_PARSE_WIDTH 7
// verilog_format: on

// This declaration must be removed with the failed file, not retained in $unit.
typedef logic [1:0] parse_rolled_back_t;

`ifdef TEST_RECOVER
module damaged;
  assign = ;
endmodule
`elsif TEST_ANSI
module damaged (
    input logic [6:0] unfinished_port
`elsif TEST_PROPERTY
module damaged;
  property unfinished_property;
    @(posedge clk) 1'b1 |->
`elsif TEST_INSTANCE
module damaged;
  unavailable_ip #(.WIDTH(7)) u_ip (.a(
`elsif TEST_UDP
primitive damaged_udp (output o, input a);
  table
    0 : 1;
`elsif TEST_PARAM
module damaged #(parameter WIDTH = 7, parameter DEPTH =
`elsif TEST_INC
`include "t/t_lint_parse_isolate_inc.vh"
module damaged;
  assign = ;
endmodule
`elsif TEST_INCMISSING
`include "t/t_lint_parse_isolate_missing.vh"
module damaged;
endmodule
`else
module damaged;
  initial begin
`endif
