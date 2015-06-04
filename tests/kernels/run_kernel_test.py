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

def run(output_suffix):

  test_out = 'tests' + os.path.sep + 'kernels' + os.path.sep + \
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
    sys.exit(retval)

  # Open output and reference files
  os.chdir(current_dir)
  out = open(test_out).read().splitlines()
  ref = open(test_ref).read().splitlines()

  # Scan through file to reach argument data
  oi = 0
  ri = 0
  try:
    while re.match('Argument \'.*\': [0-9]+ *bytes', out[oi]) == None:
      oi += 1
    while re.match('Argument \'.*\': [0-9]+ *bytes', ref[ri]) == None:
      ri += 1
  except:
    print 'Error searching for argument data'
    sys.exit(1)

  # Check that an error was produced iff an error was expected
  # An error occured if global memory dump isn't at start of file
  # TODO: Improve this so that more details about the error are checked
  should_error = ri > 1
  if should_error and oi < 2:
    print 'Error expected, but no error reported'
    sys.exit(1)
  if not should_error and oi > 1:
    print 'Error reported, but no error expected'
    sys.exit(1)

  # Check that the global memory dump matches the reference
  match = 1
  while oi < len(out):
    if out[oi] != ref[ri]:
      print '[%d:%d] "%s" vs "%s"' % (oi, ri, out[oi], ref[ri])
      match = 0
    oi += 1
    ri += 1
  if not match:
    print
    print 'Output didn\'t match reference'
    sys.exit(1)

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
