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

for mode in ('before', 'after', 'rec', 'ansi', 'prop', 'inst', 'udp', 'param', 'inc', 'line',
             'incmissing', 'staged', 'all', 'pp', 'ppnext', 'top', 'lib', 'stub', 'package',
             'function', 'class'):
    files = [prior, bad, good]
    flags = ['--error-limit', '0']
    defines = []
    if mode == 'after':
        files = [prior, good, bad]
    elif mode == 'all':
        files = [bad]
    elif mode == 'pp':
        files = [prior, 't/t_lint_parse_isolate_pp.sv', good]
    elif mode == 'ppnext':
        # The failed conditional in pp.sv must not swallow the next input, so the
        # following file's own syntax error is still reported.
        files = [prior, 't/t_lint_parse_isolate_pp.sv', bad, good]
    elif mode == 'rec':
        defines = ['+define+TEST_RECOVER']
    elif mode == 'ansi':
        defines = ['+define+TEST_ANSI']
    elif mode == 'prop':
        defines = ['+define+TEST_PROPERTY']
    elif mode == 'inst':
        defines = ['+define+TEST_INSTANCE']
    elif mode == 'line':
        # A `line directive must not let the input escape its own transaction,
        # so it uses its own file rather than a branch of the shared bad file.
        files = [prior, 't/t_lint_parse_isolate_line.sv', good]
    elif mode == 'staged':
        # A complete design unit before the damage is retained and still linted.
        files = [prior, 't/t_lint_parse_isolate_staged.sv', good]
        defines = ['+define+TEST_STAGED']
    elif mode in ('udp', 'param', 'inc', 'incmissing'):
        defines = ['+define+TEST_' + mode.upper()]
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
    elif mode not in ('all', 'pp', 'ppnext', 'package', 'function', 'class'):
        flags += ['--top-module', 't']
    if mode not in ('after', 'all', 'pp', 'ppnext', 'line', 'staged', 'lib', 'package', 'function',
                    'class'):
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
    if mode in ('all', 'pp', 'ppnext', 'incmissing'):
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
    if mode == 'staged':
        # The unit completed before the syntax error is linted in this run.
        test.file_grep(log, r'%Warning-WIDTHTRUNC: t/t_lint_parse_isolate_staged\.sv:')
    if mode == 'inc':
        # The included file belongs to the failed input, so none of its
        # declarations may survive or produce diagnostics.
        test.file_grep_not(log, r't_lint_parse_isolate_inc\.vh')
    elif mode == 'line':
        # A `line directive changes the reported filename, not ownership.
        test.file_grep(log, r'%Error: t/t_lint_parse_isolate_fake\.sv:\d+:\d+: syntax error')
    elif mode == 'incmissing':
        test.file_grep(log, r'Cannot find include file')
        test.file_grep_not(log, r'%Error: t/t_lint_parse_isolate_good\.sv')
    if mode == 'ppnext':
        # Only the damaged input reports the unterminated conditional; the next
        # input's own syntax error is reported instead of being swallowed.
        test.file_grep_count(log, r'ifdef not terminated at EOF', 1)
        test.file_grep(log, r'%Error: t/t_lint_parse_isolate_bad\.v:\d+:\d+: syntax error')
        test.file_grep_not(log, r'%Error: t/t_lint_parse_isolate_good\.sv')
        test.file_grep(log, r'Exiting due to (\d+) error\(s\)', 2)
    elif mode == 'udp':
        # The table lexer reports its own EOF error before the parser reports the
        # missing table field; both are confined to the damaged input.
        test.file_grep(log, r'Exiting due to (\d+) error\(s\)', 2)
    elif mode != 'pp':
        test.file_grep(log, r'Exiting due to (\d+) error\(s\)', 1)

test.passes()
