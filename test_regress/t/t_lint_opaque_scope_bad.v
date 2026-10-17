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
    output logic [1:0] bits,
    output logic observed
);
  always_comb begin
    case (a)
      1'b0: b = 1'b0;
    endcase
  end
  assign observed = ^data;

`ifdef TEST_PARAM
  // Unknown formals must not impose parameter constness on retained actuals.
  localparam int P = absent_fn(data[20]);
  assign bits = 2'(P);
`elsif TEST_CONST
  // Constant actuals must reach width checking before constant propagation.
  localparam logic [6:0] PARAM_BUS = 7'h35;
  localparam int P = absent_fn(PARAM_BUS[20]);
  assign bits = 2'(P);
`elsif TEST_GENFOR
  for (genvar g = 0; g < 2; ++g) begin : gen
    assign bits[g] = absent_fn(data[g+20]);
  end
`elsif TEST_FUNC
  // A retained argument cannot escape the automatic function's local scope.
  function automatic bit local_eval(input logic [6:0] arg);
    logic [6:0] local_data;
    local_data = arg;
    return absent_fn(local_data[20]);
  endfunction
  assign bits = {local_eval(data), 1'b0};
`elsif TEST_BLOCK
  // A block-local reference must remain within its declaring begin block.
  initial begin : local_blk
    logic [6:0] local_data;
    local_data = data;
    bits[0] = absent_fn(local_data[20]);
    bits[1] = 1'b0;
  end
`elsif TEST_NESTED
  // Replacing the outer task must not leave the inner function's holder orphaned.
  initial absent_task(absent_fn(data[20]));
  assign bits = 2'b0;
`elsif TEST_WITH
  // Retained actuals must keep the lambda context that supplies the item type.
  logic [6:0] q[$];
  initial bits = 2'(q.sum() with (absent_fn(item[20])));
`elsif TEST_FVARP
  // A function's return variable is not a statement-list insertion point.
  function logic [absent_fn(data[20]):0] return_eval();
    return 1'b0;
  endfunction
  assign bits = 2'(return_eval());
`elsif TEST_WITHINC
  // Invalid increments must keep item-bound call operands inside the lambda too.
  logic [6:0] q[$];
  logic [6:0] target;
  initial bits = 2'(q.sum() with (target[absent_fn(item[20])]++));
`elsif TEST_CONSTFUN
  // Diagnostic-only operands must not make an otherwise constant function impure.
  function automatic int known_const();
    int val;
    localparam logic [6:0] D = 7'h35;
    val = absent_fn(D[20]);
    return val;
  endfunction
  localparam int P = known_const();
  assign bits = 2'(P);
`else
  // Unknown IP parameter actuals can be types, not runtime expressions.
  typedef logic [6:0] word_t;
  absent_ip #(
      .LITERAL(logic [6:0]),
      .ALIAS(word_t),
      .PACKED(struct packed {
        logic [6:0] payload;
      })
  ) u_ip (
      .bad_select(data[20])
  );
  assign bits = 2'b0;
`endif
endmodule
`default_nettype wire
