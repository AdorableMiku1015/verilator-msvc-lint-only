#!/usr/bin/env python3
# DESCRIPTION: Verilator: Verilog Test driver/expect definition
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of either the GNU Lesser General Public License Version 3
# or the Perl Artistic License Version 2.0.
# SPDX-FileCopyrightText: 2026 Wilson Snyder
# SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0

import vltest_bootstrap

test.scenarios('vlt')

# Names that do not resolve only: the later case/latch checks still run.
test.lint(fails=True, expect_filename=test.golden_filename)
# Those names plus a repaired width error: the later case/latch checks
# must still run.
test.lint(v_flags2=['+define+T_LINT_NOTFOUND_CONTINUE_MIXED'],
          fails=True,
          expect_filename='t/t_lint_notfound_continue_mixed.out')

test.passes()
