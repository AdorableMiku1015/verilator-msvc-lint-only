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

for mode, errors in (('', 6), ('fire', 2), ('delayed', 2)):
    flags = []
    golden = test.golden_filename
    if mode:
        flags = ['+define+TEST_DELAYED_FIRE' if mode == 'delayed' else '+define+TEST_FIRE']
        golden = 't/t_lint_unresolved_target_bad_' + mode + '.out'
    test.lint(v_flags2=flags, fails=True, expect_filename=golden)
    log = test.compile_log_filename
    test.file_grep(log, r'%Warning-CASEINCOMPLETE:')
    test.file_grep(log, r'%Warning-LATCH:')
    test.file_grep(log, r'Exiting due to (\d+) error\(s\)', errors)
    test.file_grep_not(log, r'Internal Error')
    test.file_grep_not(log, r'%Warning-WIDTHEXPAND:')

test.passes()
