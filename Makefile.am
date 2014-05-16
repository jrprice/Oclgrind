# Makefile.am (Oclgrind)
# Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
# University of Bristol. All rights reserved.
#
# This program is provided under a three-clause BSD license. For full
# license terms please see the LICENSE file distributed with this
# source code.

AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

AM_CPPFLAGS = -DINSTALL_ROOT=\"$(prefix)\" -I$(top_srcdir)/src/

lib_LTLIBRARIES = libspirsim.la liboclgrind.la liboclgrind-icd.la

libspirsim_la_SOURCES = src/spirsim/common.h src/spirsim/common.cpp	\
 src/spirsim/Device.h src/spirsim/Device.cpp src/spirsim/half.h		\
 src/spirsim/Kernel.h src/spirsim/Kernel.cpp src/spirsim/Memory.h	\
 src/spirsim/Memory.cpp src/spirsim/Program.h src/spirsim/Program.cpp	\
 src/spirsim/Queue.h src/spirsim/Queue.cpp src/spirsim/WorkItem.h	\
 src/spirsim/WorkItem.cpp src/spirsim/WorkItemBuiltins.cpp		\
 src/spirsim/WorkGroup.h src/spirsim/WorkGroup.cpp
nodist_libspirsim_la_SOURCES = src/spirsim/clc_h.cpp
libspirsim_la_LDFLAGS = -lclangFrontendTool -lclangFrontend		\
-lclangDriver -lclangSerialization -lclangCodeGen -lclangParse		\
-lclangSema -lclangStaticAnalyzerFrontend				\
-lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore			\
-lclangAnalysis -lclangARCMigrate -lclangRewriteFrontend		\
-lclangRewriteCore -lclangEdit -lclangAST -lclangLex -lclangBasic	\
-lLLVMBitReader -lLLVMBitWriter -lLLVMAsmParser -lLLVMInstrumentation	\
-lLLVMipo -lLLVMipa -lLLVMInstCombine -lLLVMLinker -lLLVMMC		\
-lLLVMMCParser -lLLVMScalarOpts -lLLVMTarget -lLLVMTransformUtils	\
-lLLVMVectorize -lLLVMAnalysis -lLLVMCore -lLLVMSupport -ldl -lpthread	\
-shared
spirsimincludedir = $(includedir)/spirsim
spirsiminclude_HEADERS = src/spirsim/common.h src/spirsim/Device.h	\
 src/spirsim/half.h src/spirsim/Kernel.h src/spirsim/Memory.h		\
 src/spirsim/Program.h src/spirsim/Queue.h src/spirsim/WorkItem.h	\
 src/spirsim/WorkGroup.h src/spirsim/clc.h
src/spirsim/clc_h.cpp: src/spirsim/gen_clc_h.sh	src/spirsim/clc.h
	$(top_srcdir)/src/spirsim/gen_clc_h.sh $(top_srcdir)/src/spirsim/clc.h $@

install-data-hook:
	-$(clang_spir) \
		-cc1 -x cl -O0 -g -emit-pch -triple spir-unknown-unknown \
		$(includedir)/spirsim/clc.h \
		-o $(includedir)/spirsim/clc32.noopt.pch
	-$(clang_spir) \
		-cc1 -x cl -g -emit-pch -triple spir-unknown-unknown \
		$(includedir)/spirsim/clc.h \
		-o $(includedir)/spirsim/clc32.pch
	-$(clang_spir) \
		-cc1 -x cl -O0 -g -emit-pch -triple spir64-unknown-unknown \
		$(includedir)/spirsim/clc.h \
		-o $(includedir)/spirsim/clc64.noopt.pch
	-$(clang_spir) \
		-cc1 -x cl -g -emit-pch -triple spir64-unknown-unknown \
		$(includedir)/spirsim/clc.h \
		-o $(includedir)/spirsim/clc64.pch
	@if [ ! -r $(includedir)/spirsim/clc32.pch          \
	   -o ! -r $(includedir)/spirsim/clc32.noopt.pch    \
	   -o ! -r $(includedir)/spirsim/clc64.pch          \
	   -o ! -r $(includedir)/spirsim/clc64.noopt.pch ]; \
	then \
		echo; \
		echo "WARNING: Failed to generate precompiled headers."; \
		echo; \
	fi

uninstall-hook:
	rm -rf $(includedir)/spirsim/clc32.pch
	rm -rf $(includedir)/spirsim/clc32.noopt.pch
	rm -rf $(includedir)/spirsim/clc64.pch
	rm -rf $(includedir)/spirsim/clc64.noopt.pch

OCLGRIND_SOURCES = src/oclgrind/async_queue.h		\
 src/oclgrind/async_queue.cpp src/oclgrind/icd.h	\
 src/oclgrind/runtime.cpp

liboclgrind_la_SOURCES = $(OCLGRIND_SOURCES)
liboclgrind_la_LIBADD = libspirsim.la
liboclgrind_la_LDFLAGS = -shared

liboclgrind_icd_la_CPPFLAGS = -DOCLGRIND_ICD $(AM_CPPFLAGS)
liboclgrind_icd_la_SOURCES = $(OCLGRIND_SOURCES)
liboclgrind_icd_la_LIBADD = libspirsim.la
liboclgrind_icd_la_LDFLAGS = -shared

bin_PROGRAMS = oclgrind-kernel
oclgrind_kernel_SOURCES = src/oclgrind-kernel/main.cpp			\
 src/oclgrind-kernel/Simulation.h src/oclgrind-kernel/Simulation.cpp
oclgrind_kernel_LDADD = libspirsim.la

bin_SCRIPTS = oclgrind
oclgrind: $(top_srcdir)/src/oclgrind/oclgrind
	cat $(top_srcdir)/src/oclgrind/oclgrind \
	| $(SED) 's|__ROOT__|'$(prefix)'|g' \
	| $(SED) 's|__VERSION__|'$(VERSION)'|g' \
	>oclgrind

if !HAVE_PYTHON
check-TESTS:
	@echo "ERROR: Python required for running tests."
endif

check_PROGRAMS = tests/apps/vecadd/vecadd
tests_apps_vecadd_vecadd_LDADD = liboclgrind.la

TESTS = $(KERNEL_TESTS) $(check_PROGRAMS)
XFAIL_TESTS =								\
	tests/kernels/atomics/atomic_intergroup_race.sim	\
	tests/kernels/atomics/atomic_cmpxchg_race.sim
TEST_EXTENSIONS = .sim
SIM_LOG_COMPILER = $(PYTHON)				\
  $(top_srcdir)/tests/kernels/run_kernel_test.py	\
  ${abs_top_builddir}/oclgrind-kernel
AM_TESTS_ENVIRONMENT = export AM_TESTS=1;

EXTRA_DIST = src/spirsim/gen_clc_h.sh src/spirsim/clc.h			\
 src/oclgrind/oclgrind src/CL/cl.h src/CL/cl_gl.h src/CL/cl_platform.h	\
 src/CL/cl_ext.h src/CL/cl_gl_ext.h src/CL/cl_d3d10.h			\
 src/CL/cl_d3d11.h src/CL/cl_dx9_media_sharing.h src/CL/opencl.h	\
 CMakeLists.txt tests/apps/CMakeLists.txt cmake_config.h.in		\
 src/spirsim/gen_clc_h.cmake src/oclgrind/icd.def			\
 src/oclgrind/runtime.def tests/kernels/run_kernel_test.py		\
 tests/kernels/TESTS $(KERNEL_TEST_INPUTS)
CLEANFILES = src/spirsim/clc_h.cpp $(bin_SCRIPTS)	\
 $(KERNEL_TEST_OUTPUTS)
