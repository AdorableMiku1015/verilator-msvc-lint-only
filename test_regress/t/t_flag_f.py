#!/usr/bin/env python3
# DESCRIPTION: Verilator: Verilog Test driver/expect definition
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of either the GNU Lesser General Public License Version 3
# or the Perl Artistic License Version 2.0.
# SPDX-FileCopyrightText: 2024 Wilson Snyder
# SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0

import vltest_bootstrap

test.scenarios('vlt')

test.lint(v_flags2=[
    "-f t/t_flag_f.vc",
    # -Fs reads its file like -F, but ignores it when it does not exist;
    # t_flag_fs_absent.vc is intentionally not present in the tree
    "-Fs t/t_flag_fs.vc",
    "-Fs t/t_flag_fs_absent.vc",
])

test.passes()
