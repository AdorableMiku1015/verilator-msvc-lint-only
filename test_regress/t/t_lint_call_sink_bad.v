// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

`default_nettype none
function automatic int known(output int value);
  value = 32'd1;
  return 32'd2;
endfunction

// Both initializer calls need a private target in their shared $unit owner.
// Restoring a per-class name counter must not give those targets one name.
class First;
  int field = known(absent_first);
endclass
class Second;
  int field = known(absent_second);
endclass

module t (
    input logic a,
    input logic [6:0] data,
    output logic b,
    output logic [1:0] narrow
);
  First first;
  Second second;
  initial begin
    first = new;
    second = new;
  end
  assign narrow = data;
  always_comb begin
    case (a)
      1'b0: b = 1'b0;
    endcase
  end
endmodule
`default_nettype wire
