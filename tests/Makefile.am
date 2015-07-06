# Makefile.am (Oclgrind)
# Copyright (c) 2013-2015, James Price and Simon McIntosh-Smith,
# University of Bristol. All rights reserved.
#
# This program is provided under a three-clause BSD license. For full
# license terms please see the LICENSE file distributed with this
# source code.

AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

AM_CFLAGS = -std=c99 -I$(top_srcdir)/src/ -I${srcdir}/common -Wall
LDADD     = ../liboclgrind-rt.la libcommon.la

noinst_LTLIBRARIES = libcommon.la
libcommon_la_SOURCES = common/common.c common/common.h

check_PROGRAMS = \
  apps/vecadd/vecadd \
  runtime/map_buffer
TESTS = $(check_PROGRAMS)

if HAVE_PYTHON

TEST_EXTENSIONS = .sim
LOG_COMPILER = $(PYTHON) $(srcdir)/run_test.py
SIM_LOG_COMPILER = $(PYTHON)        \
  $(srcdir)/kernels/run_kernel_test.py  \
  ${abs_top_builddir}/oclgrind-kernel
AM_TESTS_ENVIRONMENT = \
  export OCLGRIND_PCH_DIR=$(abs_top_builddir)/src/include/oclgrind;

TESTS += $(KERNEL_TESTS)
XFAIL_TESTS =             \
  kernels/uninitialized/padded_struct_memcpy_fp.sim

clean-local:
	find . -name '*.out' -exec rm -f {} \;

else
check-local:
	@echo
	@echo "WARNING: Tests will not be run (Python required)."
	@echo
endif

EXTRA_DIST = run_test.py kernels/run_kernel_test.py \
  kernels/TESTS $(KERNEL_TEST_INPUTS)
