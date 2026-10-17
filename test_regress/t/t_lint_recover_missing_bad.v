// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// Missing declarations must keep the tree traversable so unrelated late checks
// still run.
`default_nettype none

module t;
  import nopkg::*;

  missing_type_t typed_missing;
  missing_iface iface_missing;

  logic c;

  always_comb begin
    if (c) c = 1'b1;
  end
endmodule
