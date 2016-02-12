# run_kernel_test.py (Oclgrind)
# Copyright (c) 2013-2015, James Price and Simon McIntosh-Smith,
# University of Bristol. All rights reserved.
#
# This program is provided under a three-clause BSD license. For full
# license terms please see the LICENSE file distributed with this
# source code.

import errno
import os
import re
import subprocess
import sys

# Check arguments
if len(sys.argv) != 3:
  print 'Usage: python run_kernel_test.py EXE SIMFILE'
  sys.exit(1)
if not os.path.isfile(sys.argv[2]):
  print 'Test file not found'
  sys.exit(1)

# Construct paths to test inputs/outputs
test_exe    = sys.argv[1]
test_file   = sys.argv[2]
test_dir    = os.path.dirname(os.path.realpath(test_file))
test_file   = os.path.basename(test_file)
test_name   = os.path.splitext(test_file)[0]
test_ref    = test_dir + os.path.sep + test_name + '.ref'
current_dir = os.getcwd()

def fail(ret=1):
  print 'FAILED'
  sys.exit(ret)

def run(output_suffix):

  test_out = 'kernels' + os.path.sep + \
             test_dir.split(os.path.sep)[-1] + os.path.sep + \
             test_name + output_suffix + '.out'

  output_dir = os.path.dirname(test_out)
  try:
    os.makedirs(output_dir)
  except OSError as exc:
    if exc.errno == errno.EEXIST and os.path.isdir(output_dir):
      pass
    else:
      raise

  # Run oclgrind-kernel
  out = open(test_out, 'w')
  os.chdir(test_dir)
  retval = subprocess.call([test_exe,
                            '--data-races', '--uninitialized', test_file],
                           stdout=out, stderr=out)
  out.close()
  if retval != 0:
    print 'oclgrind-kernel returned non-zero value (' + str(retval) + ')'
    fail(retval)

  # Open output and reference files
  os.chdir(current_dir)
  out = open(test_out).read().splitlines()
  ref = open(test_ref).read().splitlines()

  # Check output matches references
  oi = 0
  for line in ref:
    if len(line) == 0:
      continue

    type = line.split()[0]
    text = line[6:]

    # Find next non-blank line in output file
    while not len(out[oi]):
      oi += 1

    if type == 'ERROR':
      # Check first line of error contains reference message
      if not text in out[oi]:
        print 'Expected '  + line
        print 'Found    "' + out[oi] + '"'
        fail()
      # Skip remaining lines of error
      while len(out[oi]):
        oi += 1
    elif type == 'EXACT':
      # Check line of output matches reference exactly
      if not text == out[oi]:
        print 'Expected '  + line
        print 'Found    "' + out[oi] + '"'
        fail()
      oi += 1
    elif type == 'MATCH':
      # Check line of output contains reference text
      if not text in out[oi]:
        print 'Expected '  + line
        print 'Found    "' + out[oi] + '"'
        fail()
      oi += 1
    else:
      print 'Invalid match type in reference file'
      fail()

print 'Running test with optimisations'
run('')
print 'PASSED'

print
print 'Running test without optimisations'
os.environ["OCLGRIND_BUILD_OPTIONS"] = "-cl-opt-disable"
run('_noopt')
print 'PASSED'

# Test passed
sys.exit(0)
