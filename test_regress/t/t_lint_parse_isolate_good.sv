// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// Redeclaring the failed file's typedef is legal only after its $unit suffix
// has been quarantined. The distinct earlier typedef must remain available.
typedef logic parse_rolled_back_t;

module t (
    input logic a,
    input logic [6:0] data_in,
    output logic b,
    output logic [2:0] data_out
);
`ifdef TEST_REQUIRE_BAD_MACRO
  localparam int DATA_WIDTH = `T_LINT_PARSE_WIDTH;
`else
  localparam int DATA_WIDTH = 7;
`endif
  parse_kept_t retained;
  parse_rolled_back_t marker;
  logic [DATA_WIDTH-1:0] from_macro;
  assign retained = data_in;
  assign marker = a;
  assign from_macro = retained;
  assign data_out = from_macro;

  always_comb begin
    case (marker)
      1'b0: b = 1'b0;
    endcase
  end

`ifdef TEST_LAZY
  // Both references share one failed lazy-load result, not two parse attempts.
  t_lint_parse_isolate_lazy u_first ();
  t_lint_parse_isolate_lazy u_second ();
`endif
`ifdef TEST_STUB
  damaged u_replacement ();
`endif
endmodule

// Same check for the include and `line fixtures; a declaration that leaked out
// of the failed input would be a duplicate here. Kept after the module so the
// diagnostics above keep stable line numbers in the goldens.
typedef logic inc_rolled_back_t;
typedef logic line_rolled_back_t;
