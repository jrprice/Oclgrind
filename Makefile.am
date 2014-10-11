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

lib_LTLIBRARIES = liboclgrind.la liboclgrind-rt.la liboclgrind-rt-icd.la

liboclgrind_la_SOURCES = src/core/common.h src/core/common.cpp	\
 src/core/Context.h src/core/Context.cpp src/core/Device.h	\
 src/core/Device.cpp src/core/half.h src/core/Kernel.h		\
 src/core/Kernel.cpp src/core/Memory.h src/core/Memory.cpp	\
 src/core/Plugin.h src/core/Plugin.cpp src/core/Program.h	\
 src/core/Program.cpp src/core/Queue.h src/core/Queue.cpp	\
 src/core/WorkItem.h src/core/WorkItem.cpp			\
 src/core/WorkItemBuiltins.cpp src/core/WorkGroup.h		\
 src/core/WorkGroup.cpp src/plugins/InstructionCounter.h	\
 src/plugins/InstructionCounter.cpp src/plugins/RaceDetector.h	\
 src/plugins/RaceDetector.cpp
nodist_liboclgrind_la_SOURCES = src/core/clc_h.cpp config.h
liboclgrind_la_LDFLAGS = -lclangFrontendTool -lclangFrontend		\
-lclangDriver -lclangSerialization -lclangCodeGen -lclangParse		\
-lclangSema -lclangStaticAnalyzerFrontend				\
-lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore			\
-lclangAnalysis -lclangARCMigrate -lclangRewriteFrontend		\
-lclangRewriteCore -lclangEdit -lclangAST -lclangLex -lclangBasic	\
-lLLVMBitReader -lLLVMBitWriter -lLLVMAsmParser -lLLVMInstrumentation	\
-lLLVMipo -lLLVMipa -lLLVMInstCombine -lLLVMLinker -lLLVMMC		\
-lLLVMMCParser -lLLVMScalarOpts -lLLVMTarget -lLLVMTransformUtils	\
-lLLVMVectorize -lLLVMAnalysis -lLLVMCore -lLLVMSupport -ldl -lpthread	\
$(oclgrind_extra_libs) -shared
oclgrind_includedir = $(includedir)/oclgrind
oclgrind_include_HEADERS = src/core/common.h src/core/Context.h		\
 src/core/Device.h src/core/half.h src/core/Kernel.h src/core/Memory.h	\
 src/core/Plugin.h src/core/Program.h src/core/Queue.h			\
 src/core/WorkItem.h src/core/WorkGroup.h src/core/clc.h config.h
src/core/clc_h.cpp: src/core/gen_clc_h.sh	src/core/clc.h
	$(top_srcdir)/src/core/gen_clc_h.sh $(top_srcdir)/src/core/clc.h $@

install-data-hook:
	-$(clang_spir) \
		-cc1 -x cl -O0 -g -emit-pch -triple spir-unknown-unknown \
		$(includedir)/oclgrind/clc.h \
		-o $(includedir)/oclgrind/clc32.noopt.pch
	-$(clang_spir) \
		-cc1 -x cl -g -emit-pch -triple spir-unknown-unknown \
		$(includedir)/oclgrind/clc.h \
		-o $(includedir)/oclgrind/clc32.pch
	-$(clang_spir) \
		-cc1 -x cl -O0 -g -emit-pch -triple spir64-unknown-unknown \
		$(includedir)/oclgrind/clc.h \
		-o $(includedir)/oclgrind/clc64.noopt.pch
	-$(clang_spir) \
		-cc1 -x cl -g -emit-pch -triple spir64-unknown-unknown \
		$(includedir)/oclgrind/clc.h \
		-o $(includedir)/oclgrind/clc64.pch
	@if [ ! -r $(includedir)/oclgrind/clc32.pch          \
	   -o ! -r $(includedir)/oclgrind/clc32.noopt.pch    \
	   -o ! -r $(includedir)/oclgrind/clc64.pch          \
	   -o ! -r $(includedir)/oclgrind/clc64.noopt.pch ]; \
	then \
		echo; \
		echo "WARNING: Failed to generate precompiled headers."; \
		echo; \
	fi

uninstall-hook:
	rm -rf $(includedir)/oclgrind/clc32.pch
	rm -rf $(includedir)/oclgrind/clc32.noopt.pch
	rm -rf $(includedir)/oclgrind/clc64.pch
	rm -rf $(includedir)/oclgrind/clc64.noopt.pch

RUNTIME_SOURCES = src/runtime/async_queue.h				\
 src/runtime/async_queue.cpp src/runtime/icd.h src/runtime/runtime.cpp

liboclgrind_rt_la_SOURCES = $(RUNTIME_SOURCES)
liboclgrind_rt_la_LIBADD = liboclgrind.la
liboclgrind_rt_la_LDFLAGS = -shared

liboclgrind_rt_icd_la_CPPFLAGS = -DOCLGRIND_ICD $(AM_CPPFLAGS)
liboclgrind_rt_icd_la_SOURCES = $(RUNTIME_SOURCES)
liboclgrind_rt_icd_la_LIBADD = liboclgrind.la
liboclgrind_rt_icd_la_LDFLAGS = -shared

bin_PROGRAMS = oclgrind-kernel
oclgrind_kernel_SOURCES = src/kernel/oclgrind-kernel.cpp	\
 src/kernel/Simulation.h src/kernel/Simulation.cpp
oclgrind_kernel_LDADD = liboclgrind.la

bin_SCRIPTS = oclgrind
oclgrind: $(top_srcdir)/src/runtime/oclgrind
	cat $(top_srcdir)/src/runtime/oclgrind \
	| $(SED) 's|__LIBDIR__|'$(libdir)'|g' \
	| $(SED) 's|__VERSION__|'$(VERSION)'|g' \
	>$@
noinst_SCRIPTS = oclgrind.icd
oclgrind.icd: liboclgrind-rt-icd.la
	printf $(libdir)/ >$@
	$(GREP) dlname $< | $(AWK) -F "'" '{print $$2}' >>$@

check_PROGRAMS = tests/apps/vecadd/vecadd
tests_apps_vecadd_vecadd_LDADD = liboclgrind-rt.la
TESTS = $(check_PROGRAMS)

TEST_EXTENSIONS = .sim
SIM_LOG_COMPILER = $(PYTHON)				\
  $(top_srcdir)/tests/kernels/run_kernel_test.py	\
  ${abs_top_builddir}/oclgrind-kernel
AM_TESTS_ENVIRONMENT = export AM_TESTS=1;

if HAVE_PYTHON
TESTS += $(KERNEL_TESTS)
XFAIL_TESTS =							\
	tests/kernels/atomics/atomic_intergroup_race.sim	\
	tests/kernels/atomics/atomic_cmpxchg_race.sim \
	tests/kernels/alignment/packed.sim
else
check-local:
	@echo
	@echo "WARNING: Kernel tests skipped (Python required)."
	@echo
endif

EXTRA_DIST = NEWS src/core/gen_clc_h.sh src/core/clc.h			\
 src/runtime/oclgrind src/CL/cl.h src/CL/cl_gl.h src/CL/cl_platform.h	\
 src/CL/cl_ext.h src/CL/cl_gl_ext.h src/CL/cl_d3d10.h			\
 src/CL/cl_d3d11.h src/CL/cl_dx9_media_sharing.h src/CL/opencl.h	\
 CMakeLists.txt tests/apps/CMakeLists.txt cmake_config.h.in		\
 src/core/gen_clc_h.cmake src/runtime/icd.def src/runtime/runtime.def	\
 tests/kernels/run_kernel_test.py tests/kernels/TESTS			\
 $(KERNEL_TEST_INPUTS)
CLEANFILES = src/core/clc_h.cpp $(bin_SCRIPTS) $(noinst_SCRIPTS)	\
 $(KERNEL_TEST_OUTPUTS)
