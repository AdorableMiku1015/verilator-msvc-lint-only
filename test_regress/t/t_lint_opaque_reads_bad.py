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

for mode in ('read', 'one', 'two'):
    for direct in (False, True):
        flags = ['+define+TEST_' + mode.upper(), '-Wall', '-Wno-DECLFILENAME']
        name = mode
        if direct:
            flags += ['+define+TEST_DIRECT']
            name += '_ok'
        test.lint(v_flags2=flags,
                  verilator_flags2=['--error-limit', '0'],
                  fails=True,
                  expect_filename='t/t_lint_opaque_reads_bad_' + name + '.out')
        log = test.compile_log_filename
        test.file_grep(log, r'%Warning-CASEINCOMPLETE:')
        test.file_grep(log, r'%Warning-LATCH:')
        test.file_grep_not(log, r'Internal Error|%Error-PINNOTFOUND:|%Error-PORTSHORT:')
        test.file_grep_not(log,
                           r'%(?:Error|Warning)-(?:WIDTH|WIDTHEXPAND|WIDTHTRUNC|WIDTHCONCAT):')
        test.file_grep_not(log, r"%Warning-UNUSEDSIGNAL:[^\n]*'(?:x|opaque_signal)'")
        test.file_grep_not(log, r"%Warning-UNDRIVEN:[^\n]*'opaque_signal'")
        if direct:
            test.file_grep_not(log, r"Can't find definition of task/function")
            test.file_grep(log, r'Exiting due to \d+ warning\(s\)')
        else:
            test.file_grep(log, r"Can't find definition of task/function: 'absent_fn'")
            test.file_grep(log, r'Exiting due to (\d+) error\(s\)', 1)
        if mode == 'read':
            test.file_grep(log, r"%Warning-UNDRIVEN:[^\n]*'x'")
        else:
            test.file_grep_not(log, r"%Warning-UNDRIVEN:[^\n]*'x'")
        if mode == 'two':
            test.file_grep(log, r"%Warning-MULTIDRIVEN:[^\n]*'x'")
            if not direct:
                test.file_grep(log, r"%Warning-MULTIDRIVEN:[^\n]*'opaque_signal'")
        else:
            test.file_grep_not(log, r'%Warning-MULTIDRIVEN(?:PROC)?:')

test.passes()
