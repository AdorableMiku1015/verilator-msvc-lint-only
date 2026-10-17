// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// Valid package and $unit-only inputs are lintable without a module top. A
// later quarantined module must not hide these independent width diagnostics.
`ifdef TEST_UNIT_PACKAGE
package kept_pkg;
  function automatic logic [1:0] narrow(input logic [6:0] value);
    return value;
  endfunction
endpackage
`elsif TEST_UNIT_CLASS
class KeptClass;
  function logic [1:0] narrow(input logic [6:0] value);
    return value;
  endfunction
endclass
`else
function automatic logic [1:0] narrow(input logic [6:0] value);
  return value;
endfunction
`endif
