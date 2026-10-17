// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// A concrete replacement must take precedence over an earlier rejected input's
// unavailable identity, regardless of the input ordering.
module damaged;
  logic [1:0] replacement_width;
  assign replacement_width = 7'h40;
endmodule
