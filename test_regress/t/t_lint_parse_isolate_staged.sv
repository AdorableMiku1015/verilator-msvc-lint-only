// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// The first design unit completes before the damage, so it is retained and
// still linted. The declaration and module after it are part of the rejected
// suffix and must be rolled back. The last unit follows the damage and is
// retained again once bison has resynchronized at a design boundary.
module staged_good(input logic a, output logic b);
  assign b = 2'b11;
endmodule

typedef logic [1:0] staged_rolled_back_t;

module damaged;
  assign = ;
endmodule

module staged_after(input logic a, output logic b);
  assign b = 3'b101;
endmodule
