// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// Names the linker cannot resolve are errors, but they must not stop the later
// lint passes: the case and latch checks run after them, so their warnings
// still have to be reported in the same run.  The reference that cannot be
// resolved is replaced by a stand-in value, and the assignments whose target
// does not resolve are dropped, so the later passes can still walk the tree.
// Any error that is not recoverable still stops those passes, which the mixed
// variant below checks.
module t (
    input logic a,
    output logic b
);
  always_comb begin
    case (a)
      1'b0: b = 1'b0;
    endcase
  end

  sub sub_inst (.nonexistent(1'b0));

  // A name that does not resolve in an expression, in a block with other work
  initial begin
    if (missing_scope.value != 9) $stop;
  end

  // The only statement of its block, so removing it empties the block
  initial begin
    missing_task;
  end

  // A write target that does not resolve, again the only statement of its block
  always_comb begin
    absent_scope.field = 1'b1;
  end

`ifdef T_LINT_NOTFOUND_CONTINUE_MIXED
  logic [3:0] o;
  assign o = {0{1'b0}};
`endif
endmodule

module sub (
    input logic present
);
endmodule
