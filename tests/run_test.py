# run_test.py (Oclgrind)
# Copyright (c) 2013-2015, James Price and Simon McIntosh-Smith,
# University of Bristol. All rights reserved.
#
# This program is provided under a three-clause BSD license. For full
# license terms please see the LICENSE file distributed with this
# source code.

import os
import re
import subprocess
import sys

# Check arguments
if len(sys.argv) != 2:
  print 'Usage: python run_test.py EXE'
  sys.exit(1)
if not os.path.isfile(sys.argv[1]):
  print 'Test executable not found'
  sys.exit(1)

# Construct paths to test inputs/outputs
test_exe    = sys.argv[1]
test_dir    = os.path.dirname(os.path.realpath(test_exe))
test_name   = os.path.splitext(os.path.basename(test_exe))[0]

# Enable race detection and uninitialized memory plugins
os.environ["OCLGRIND_CHECK_API"] = "1"
os.environ["OCLGRIND_DATA_RACES"] = "1"
os.environ["OCLGRIND_UNINITIALIZED"] = "1"

def run(output_suffix):

  test_out = test_dir + os.path.sep + test_name + output_suffix + '.out'

  # Run test
  out = open(test_out, 'w')
  retval = subprocess.call([test_exe], stdout=out, stderr=out)
  out.close()
  if retval != 0:
    print test_name + ' returned non-zero value (' + str(retval) + ')'
    sys.exit(retval)

  # Open output file
  out = open(test_out).read().splitlines()

  # Check that an error was produced iff an error was expected
  # Assume any text in output is an error
  # TODO: Allow a test to require an error
  # TODO: Improve this so that more details about the error are checked
  if len(out) > 0:
    print 'Error reported, but no error expected'
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
