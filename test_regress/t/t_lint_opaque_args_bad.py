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

for mode in ('module', 'task', 'dtask', 'function', 'dfunc', 'write'):
    flags = ['+define+TEST_' + mode.upper(), '-Wall', '-Wno-DECLFILENAME', '-Wno-UNUSEDPARAM']
    test.lint(v_flags2=flags,
              verilator_flags2=['--error-limit', '0'],
              fails=True,
              expect_filename='t/t_lint_opaque_args_bad_' + mode + '.out')
    log = test.compile_log_filename
    test.file_grep(log, r'%Warning-SELRANGE:')
    test.file_grep(log, r'%Warning-CASEINCOMPLETE:')
    test.file_grep(log, r'%Warning-LATCH:')
    test.file_grep(
        log, r"(?:Can't find definition of variable|Signal definition not found, and implicit "
        r"disabled with `default_nettype): 'missing_arg'")
    if mode == 'module':
        test.file_grep(
            log, r"(?:Can't find definition of variable|Signal definition not found, and implicit "
            r"disabled with `default_nettype): 'missing_param'")
        test.file_grep(log, r'%Error-MODMISSING:')
        test.file_grep_count(log, r'%Warning-SELRANGE:', 2)
    else:
        test.file_grep_count(log, r'%Warning-SELRANGE:', 1)
    if mode == 'write':
        test.file_grep(log, r"%Warning-UNDRIVEN:[^\n]*'rhs_only'")
    test.file_grep(log, r'Exiting due to (\d+) error\(s\)', 3 if mode == 'module' else 2)
    test.file_grep_not(log, r'Internal Error|%Error-PINNOTFOUND:|%Error-PORTSHORT:')
    test.file_grep_not(log, r'%(?:Error|Warning)-(?:WIDTH|WIDTHEXPAND|WIDTHTRUNC|WIDTHCONCAT):')
    test.file_grep_not(log, r'%Warning-(?:UNDRIVEN|UNUSEDSIGNAL):[^\n]*opaque_(?:in|out)')
    test.file_grep_not(log,
                       r'%Warning-(?:UNDRIVEN|UNUSEDSIGNAL):[^\n]*missing_(?:arg|param|write)')

test.passes()
