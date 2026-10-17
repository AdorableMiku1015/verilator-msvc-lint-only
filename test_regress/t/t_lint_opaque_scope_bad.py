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

for mode in ('param', 'const', 'genfor', 'func', 'block', 'types', 'nested', 'with', 'fvarp',
             'constfun', 'withinc'):
    # The second run must match the same golden even when early const folding
    # cannot incidentally remove a diagnostic-only holder.
    for extra_flags in ([], ['-fno-const-before-dfg']):
        test.lint(v_flags2=['+define+TEST_' + mode.upper()],
                  verilator_flags2=['--error-limit', '0'] + extra_flags,
                  fails=True,
                  expect_filename='t/t_lint_opaque_scope_bad_' + mode + '.out')
        log = test.compile_log_filename
        test.file_grep(log, r'%Warning-SELRANGE:')
        test.file_grep(log, r'%Warning-CASEINCOMPLETE:')
        test.file_grep(log, r'%Warning-LATCH:')
        test.file_grep_count(log, r'%Warning-SELRANGE:', 2 if mode == 'genfor' else 1)
        test.file_grep(log, r'Exiting due to (\d+) error\(s\)', 2 if mode == 'nested' else 1)
        if mode == 'types':
            test.file_grep(log, r'%Error-MODMISSING:')
        else:
            test.file_grep(log, r"Can't find definition of task/function: 'absent_fn'")
        test.file_grep_not(log, r'Internal Error|%Error-PINNOTFOUND:|%Error-PORTSHORT:')
        test.file_grep_not(log,
                           r'%(?:Error|Warning)-(?:WIDTH|WIDTHEXPAND|WIDTHTRUNC|WIDTHCONCAT):')
        test.file_grep_not(log, r'%Error:[^\n]*(?:constant|local_data|arg|word_t|payload)')

test.passes()
