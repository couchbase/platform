#!/usr/bin/python

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

print sys.argv[1]
proc = subprocess.Popen(sys.argv[1],stderr=subprocess.PIPE)

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
