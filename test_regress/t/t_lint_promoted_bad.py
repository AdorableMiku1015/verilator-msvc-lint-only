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

for mode in ('protected', 'filename', 'parse'):
    flags = ['-Wwarn-DECLFILENAME', '-Werror-DECLFILENAME']
    if mode in ('protected', 'parse'):
        flags = ['+define+TEST_PROTECTED', '-Werror-PROTECTED']
    if mode == 'parse':
        flags += ['--debug-exit-parse']
    flags += ['-Wwarn-UNUSEDSIGNAL']
    test.lint(v_flags2=flags, fails=True, expect_filename='t/t_lint_promoted_bad_' + mode + '.out')
    log = test.obj_dir + '/vlt_compile.log'
    if mode == 'parse':
        test.file_grep(log, r'Exiting due to (\d+) error\(s\)', 1)
        test.file_grep_not(log, r'%Warning-(CASEINCOMPLETE|LATCH):')
    else:
        test.file_grep(log, r'%Error-MODMISSING:')
        test.file_grep(log, r'%Warning-CASEINCOMPLETE:')
        test.file_grep(log, r'%Warning-LATCH:')
        test.file_grep(log, r'Exiting due to (\d+) error\(s\)', 2)
    test.file_grep_not(log, r'Internal Error|unused_template_member')

test.passes()
