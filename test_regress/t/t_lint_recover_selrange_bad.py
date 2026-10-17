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

test.lint(verilator_flags2=['--error-limit', '0'],
          fails=True,
          expect_filename=test.golden_filename)

log = test.compile_log_filename
# One name error and one independent SELRANGE for each rejected target.
test.file_grep(log, r'Exiting due to (\d+) error\(s\)', 3)
test.file_grep_count(log, r'%Warning-SELRANGE:', 3)
test.file_grep_not(log, r'Internal Error')

test.passes()
