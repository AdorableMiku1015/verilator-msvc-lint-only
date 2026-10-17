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

prior = 't/t_lint_parse_isolate_prior.sv'
bad = 't/t_lint_parse_isolate_bad.v'
good = 't/t_lint_parse_isolate_good.sv'

for mode in ('before', 'after', 'rec', 'ansi', 'prop', 'inst', 'all', 'pp', 'top', 'lib', 'stub',
             'package', 'function', 'class'):
    files = [prior, bad, good]
    flags = ['--error-limit', '0']
    defines = []
    if mode == 'after':
        files = [prior, good, bad]
    elif mode == 'all':
        files = [bad]
    elif mode == 'pp':
        files = [prior, 't/t_lint_parse_isolate_pp.sv', good]
    elif mode == 'rec':
        defines = ['+define+TEST_RECOVER']
    elif mode == 'ansi':
        defines = ['+define+TEST_ANSI']
    elif mode == 'prop':
        defines = ['+define+TEST_PROPERTY']
    elif mode == 'inst':
        defines = ['+define+TEST_INSTANCE']
    elif mode == 'lib':
        files = [prior, good]
        flags += ['-y', 't']
        defines = ['+define+TEST_LAZY']
    elif mode == 'stub':
        files = [prior, bad, 't/t_lint_parse_isolate_stub.sv', good]
        defines = ['+define+TEST_STUB']
    elif mode in ('package', 'function', 'class'):
        files = ['t/t_lint_parse_isolate_unit.sv', bad]
        defines = ['+define+TEST_UNIT_' + mode.upper()]
    if mode == 'top':
        flags += ['--top-module', 'damaged']
    elif mode not in ('all', 'pp', 'package', 'function', 'class'):
        flags += ['--top-module', 't']
    if mode not in ('after', 'all', 'pp', 'lib', 'package', 'function', 'class'):
        defines += ['+define+TEST_REQUIRE_BAD_MACRO']
    test.lint(top_filename=files[0],
              v_other_filenames=files[1:],
              verilator_flags2=flags,
              v_flags2=defines,
              fails=True,
              expect_filename='t/t_lint_parse_isolate_bad_' + mode + '.out')
    log = test.compile_log_filename
    test.file_grep_not(
        log,
        r'Internal Error|No top level module|Duplicate declaration|%Warning-MULTITOP:|%Error-MODMISSING:'
    )
    if mode in ('all', 'pp'):
        test.file_grep_not(log, r'%Warning-(WIDTHTRUNC|CASEINCOMPLETE|LATCH):')
    elif mode in ('package', 'function', 'class'):
        test.file_grep(log, r'%Warning-WIDTHTRUNC: t/t_lint_parse_isolate_unit.sv:')
        test.file_grep_not(log, r'%Warning-(CASEINCOMPLETE|LATCH):|Cannot continue')
    else:
        test.file_grep(log, r'%Warning-WIDTHTRUNC:')
        test.file_grep(log, r'%Warning-CASEINCOMPLETE:')
        test.file_grep(log, r'%Warning-LATCH:')
        test.file_grep_not(log,
                           r'Cannot continue|Define or directive not defined|Can.t find typedef')
    if mode == 'stub':
        test.file_grep(log, r'%Warning-WIDTHTRUNC: t/t_lint_parse_isolate_stub.sv:')
    if mode != 'pp':
        test.file_grep(log, r'Exiting due to (\d+) error\(s\)', 1)

test.passes()
