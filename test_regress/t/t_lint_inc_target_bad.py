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

for mode in ('post', 'pre', 'postdec', 'predec', 'expr', 'index'):
    test.lint(v_flags2=['+define+TEST_' + mode.upper()],
              fails=True,
              expect_filename='t/t_lint_inc_target_bad_' + mode + '.out')
    log = test.compile_log_filename
    test.file_grep(log, r"Can't find definition of variable: 'absent'")
    test.file_grep(log, r'%Warning-CASEINCOMPLETE:')
    test.file_grep(log, r'%Warning-LATCH:')
    test.file_grep(log, r'Exiting due to (\d+) error\(s\)', 1)
    test.file_grep_not(log, r'Internal Error|%Warning-WIDTH')

test.passes()
