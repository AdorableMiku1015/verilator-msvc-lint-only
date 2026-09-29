#!/usr/bin/env python3
# DESCRIPTION: Verilator: Verilog Test driver/expect definition
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of either the GNU Lesser General Public License Version 3
# or the Perl Artistic License Version 2.0.
# SPDX-FileCopyrightText: 2024 Wilson Snyder
# SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0

import shutil

import vltest_bootstrap

test.scenarios('dist')

if 'VERILATOR_TEST_NO_LINT_PY' in os.environ:
    test.skip("Skipping due to VERILATOR_TEST_NO_LINT_PY")
if not os.path.exists(test.root + "/.git"):
    test.skip("Not in a git repository")

# The make-based 'lint-py' target went away with the autoconf build, so run the
# same linters over the same file sets directly. Programs the old target listed
# that this fork no longer ships are skipped.
PY_PROGRAMS = [
    p for p in (
        "bin/verilator_includer",
        "docs/bin/vl_sphinx_extract",
        "docs/bin/vl_sphinx_fix",
        "docs/guide/conf.py",
        "nodist/clang_check_attributes",
        "nodist/dot_importer",
        "nodist/fuzzer/actual_fail",
        "nodist/fuzzer/generate_dictionary",
        "nodist/lint_py_test_filter",
        "nodist/log_changes",
        "nodist/uvm_pkg_packer",
        "nodist/verilator_bisect",
        "nodist/verilator_saif_diff",
        "src/.gdbinit.py",
        "src/astgen",
        "src/bisonpre",
        "src/config_rev",
        "src/flexfix",
    ) if os.path.exists(test.root + "/" + p)
]

PYLINT_FLAGS = "--score=n --disable=R0801 --disable=E0401"
PYLINT_TEST_FLAGS = PYLINT_FLAGS + (
    " --disable=C0103,C0114,C0116,C0209,C0411,C0413,C0301,R0801,"
    "R0912,R0915,R0916,R1702,W0511,W0621")
RUFF_FLAGS = "check --ignore=E402,E501,E701"

# Missing tools are not a failure: this is a fork with a small, separately
# installed tool set. E0401 (unresolvable import) is disabled because the
# optional documentation dependencies are not part of that set.
LANGS = [
    ("pylint", PYLINT_FLAGS, PY_PROGRAMS),
    ("pylint", PYLINT_TEST_FLAGS, test.glob_some("t/*.py")),
    ("ruff", RUFF_FLAGS, PY_PROGRAMS),
    ("mypy", "--strict --no-error-summary", []),
]
# mypy only checks the programs that ask for it
for filename in PY_PROGRAMS:
    with open(test.root + "/" + filename, encoding="utf8") as fh:
        if "# mypy" in fh.read():
            LANGS[3][2].append(filename)

for tool, flags, files in LANGS:
    if not files:
        continue
    if shutil.which(tool) is None:
        print("Skipping " + tool + ": not installed")
        continue
    cmd = "cd " + test.root + " && " + tool + " " + flags + " " + " ".join(files)
    print("\t" + cmd)
    out = test.run_capture(cmd, check=False)
    if re.search(r"\berror\b", out, re.I):
        test.error(tool + " reported problems:\n" + out)

test.passes()
