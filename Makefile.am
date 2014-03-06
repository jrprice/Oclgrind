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
libspirsim_la_LDFLAGS = -lclangCodeGen -lclangAST -lclang		\
-lLLVMBitReader -lLLVMBitWriter -lLLVMAsmParser -lLLVMInstrumentation	\
-lLLVMipo -lLLVMipa -lLLVMInstCombine -lLLVMLinker -lLLVMMC		\
-lLLVMScalarOpts -lLLVMTarget -lLLVMTransformUtils -lLLVMVectorize	\
-lLLVMAnalysis -lLLVMCore -lLLVMSupport -ldl -lpthread -shared
spirsimincludedir = $(includedir)/spirsim
spirsiminclude_HEADERS = src/spirsim/common.h src/spirsim/Device.h	\
 src/spirsim/half.h src/spirsim/Kernel.h src/spirsim/Memory.h		\
 src/spirsim/Program.h src/spirsim/Queue.h src/spirsim/WorkItem.h	\
 src/spirsim/WorkGroup.h src/spirsim/clc.h
src/spirsim/clc_h.cpp: Makefile src/spirsim/gen_clc_h.sh	\
 src/spirsim/clc.h
	$(top_srcdir)/src/spirsim/gen_clc_h.sh $(top_srcdir)/src/spirsim/clc.h $@

install-data-hook:
	-@clang_spir@ \
	-cc1 -x cl -O0 -g -emit-pch -triple spir64-unknown-unknown \
	$(includedir)/spirsim/clc.h \
	-o $(includedir)/spirsim/clc.noopt.pch
	-@clang_spir@ \
	-cc1 -x cl -g -emit-pch -triple spir64-unknown-unknown \
	$(includedir)/spirsim/clc.h \
	-o $(includedir)/spirsim/clc.pch
	@if [ ! -r $(includedir)/spirsim/clc.pch  \
	   -o ! -r $(includedir)/spirsim/clc.noopt.pch ]; \
	then \
		echo; \
		echo "WARNING: Failed to generate precompiled headers."; \
		echo; \
	fi

uninstall-hook:
	rm -rf $(includedir)/spirsim/clc.pch
	rm -rf $(includedir)/spirsim/clc.noopt.pch

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
oclgrind_kernel_SOURCES = src/oclgrind-kernel/main.cpp
oclgrind_kernel_LDADD = libspirsim.la

bin_SCRIPTS = oclgrind
oclgrind: $(top_srcdir)/src/oclgrind/oclgrind
	cat $(top_srcdir)/src/oclgrind/oclgrind \
	| $(SED) 's|__ROOT__|'$(prefix)'|g' \
	| $(SED) 's|__VERSION__|'$(VERSION)'|g' \
	>oclgrind

EXTRA_DIST = src/spirsim/gen_clc_h.sh src/spirsim/clc.h			\
 src/oclgrind/oclgrind src/CL/cl.h src/CL/cl_gl.h src/CL/cl_platform.h	\
 src/CL/cl_ext.h src/CL/cl_gl_ext.h src/CL/cl_d3d10.h			\
 src/CL/cl_d3d11.h src/CL/cl_dx9_media_sharing.h src/CL/opencl.h	\
 CMakeLists.txt cmake_config.h.in src/spirsim/gen_clc_h.cmake		\
 src/oclgrind/icd.def
CLEANFILES = src/spirsim/clc_h.cpp $(bin_SCRIPTS)
