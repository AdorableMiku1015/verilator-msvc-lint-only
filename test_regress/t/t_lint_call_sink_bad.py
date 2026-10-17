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
test.file_grep(log, r"Can't find definition of variable: 'absent_first'")
test.file_grep(log, r"Can't find definition of variable: 'absent_second'")
test.file_grep(log, r'%Warning-WIDTHTRUNC:')
test.file_grep(log, r'%Warning-CASEINCOMPLETE:')
test.file_grep(log, r'%Warning-LATCH:')
test.file_grep(log, r'Exiting due to (\d+) error\(s\)', 2)
test.file_grep_not(log, r'(?i)duplicate|internal error')
test.file_grep_not(log, r'connected to constant|Ref argument requires matching types')
test.file_grep_not(log, r'%Warning-WIDTHEXPAND:')

test.passes()
