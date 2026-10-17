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

for mode in ('zero', 'enum_width', 'enum_value', 'promoted', 'strict', 'limit'):
    flags = ['+define+TEST_' + mode.upper()]
    if mode == 'promoted':
        flags += ['-Werror-WIDTHTRUNC']
    elif mode == 'strict':
        flags += ['+define+TEST_ENUM_VALUE']
    elif mode == 'limit':
        flags += ['+define+TEST_ZERO', '--error-limit', '1']
    test.lint(v_flags2=flags,
              fails=True,
              expect_filename='t/t_lint_width_error_bad_' + mode + '.out')
    log = test.obj_dir + '/vlt_compile.log'
    test.file_grep_not(log, r'Internal Error')
    if mode in ('strict', 'limit'):
        test.file_grep_not(log, r'%Warning-(CASEINCOMPLETE|LATCH):')
    else:
        test.file_grep(log, r'%Warning-CASEINCOMPLETE:')
        test.file_grep(log, r'%Warning-LATCH:')
        test.file_grep(log, r'Exiting due to (\d+) error\(s\)', 1)

test.passes()
