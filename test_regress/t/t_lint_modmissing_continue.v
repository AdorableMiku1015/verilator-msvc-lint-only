// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// An unknown module is reported as an error by the width pass, but that error
// must not stop the later lint passes: the case and latch checks run after it,
// so their warnings still have to be reported in the same run.  Any other
// error still stops those passes, which the mixed variant below checks.
module t (
    input logic a,
    output logic b
);
  always_comb begin
    case (a)
      1'b0: b = 1'b0;
    endcase
  end

  missing_mod u_missing ();

`ifdef T_LINT_MODMISSING_CONTINUE_MIXED
  logic [3:0] o;
  assign o = {0{1'b0}};
`endif
endmodule
