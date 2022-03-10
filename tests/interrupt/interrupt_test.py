#!/usr/bin/env python3

"""
Copyright 2017-Present Couchbase, Inc.

Use of this software is governed by the Business Source License included in
the file licenses/BSL-Couchbase.txt.  As of the Change Date specified in that
file, in accordance with the Business Source License, use of this software will
be governed by the Apache License, Version 2.0, included in the file
licenses/APL2.txt.
"""

import os
import signal
import subprocess
import sys
import time

"""
This test runs the underlying 'platform-interrupt-test' binary,
sends it a SIGINT/CTRL_C_EVENT, and then propogates the return
code that the underlying binary gave.
"""

if len(sys.argv) != 2:
    exit(1)

print(sys.argv[1])
proc = subprocess.Popen(sys.argv[1], stderr=subprocess.PIPE,
                        universal_newlines=True)

# Let the underlying process register the signal handler
while True:
    line = proc.stderr.readline()
    if 'Busy waiting for signal' in line:
        break

if os.name == 'nt':
    # The most consistent way to detect ctrl+c on Windows
    # is via the specific CTRL_C_EVENT rather than SIGINT
    os.kill(proc.pid, signal.CTRL_C_EVENT)
else:
    os.kill(proc.pid, signal.SIGINT)

# Propogate the returncode of the underlying test
exit(proc.wait())
