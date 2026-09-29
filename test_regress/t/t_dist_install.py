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

if not os.path.exists(test.root + "/.git"):
    test.skip("Not in a git repository")

# The autoconf 'make install-all'/'uninstall' pair went away with the GNU make
# build; install with CMake instead and undo it from the manifest CMake wrote.
build_dir = test.root + "/build"
if not os.path.exists(build_dir + "/CMakeCache.txt"):
    test.skip("No configured CMake build directory to install from")

cwd = os.getcwd()
destdir = cwd + "/" + test.obj_dir

# Start clean
if os.path.exists(destdir):
    shutil.rmtree(destdir)
os.makedirs(destdir)

# Install into temp area
print("Install...")
test.run(cmd=["cmake --install " + build_dir + " --prefix " + destdir], check_finished=False)

# The lint tool, its include helper and the runtime headers must all be there
for want in ("bin/verilator", "bin/verilator_includer", "include/verilated.h",
             "include/verilated_config.h"):
    if not os.path.exists(destdir + "/" + want):
        test.error("Install missed " + want)

# Uninstall from the manifest
print("Uninstall...\n")
manifest = build_dir + "/install_manifest.txt"
if not os.path.exists(manifest):
    test.error("Install wrote no " + manifest)
else:
    with open(manifest, encoding="utf8") as fh:
        installed = [line.rstrip("\n") for line in fh if line.strip()]
    for filename in installed:
        if os.path.exists(filename):
            os.unlink(filename)
    # Drop the directories the install created, deepest first. Keep destdir
    # itself; the harness writes its status file there when the test exits.
    for root, dirs, files in os.walk(destdir, topdown=False):
        if root != destdir and not os.listdir(root):
            os.rmdir(root)

# Check empty
files = []
for root, dirs, found in os.walk(destdir):
    for filename in found:
        if re.search(r'\.status', filename):  # Made by driver.py, not Verilator
            continue
        print("\tLEFT:  " + filename)
        files.append(os.path.join(root, filename))

if len(files) > 0:
    test.error("Uninstall missed files: " + ' '.join(files))

test.passes()
