# DESCRIPTION: Verilator: Verilog Test bootstrap loader
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of either the GNU Lesser General Public License Version 3
# or the Perl Artistic License Version 2.0.
# SPDX-FileCopyrightText: 2024 Wilson Snyder
# SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0

import os
import re
import sys

os.chdir(os.path.dirname(os.path.realpath(__file__)) + "/..")
# Avoid chdir leaving the .. which confuses later commands
os.environ['PWD'] = os.getcwd()
args = list(map(lambda arg: re.sub(r'.*/test_regress/', '', arg), sys.argv))
if os.name == "nt":
    # Windows cannot exec a script through its shebang, so re-enter the
    # interpreter explicitly instead of relying on os.execl("./driver.py").
    os.execv(sys.executable, [sys.executable, "./driver.py", *args])
else:
    os.execl("./driver.py", "driver.py", *args)
