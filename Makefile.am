# Makefile.am (Oclgrind)
# Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
# University of Bristol. All rights reserved.
#
# This program is provided under a three-clause BSD license. For full
# license terms please see the LICENSE file distributed with this
# source code.

AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

AM_CPPFLAGS = -I$(top_srcdir)/src/

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
 src/spirsim/WorkGroup.h
CLEANFILES = src/spirsim/clc_h.cpp
src/spirsim/clc_h.cpp: Makefile src/spirsim/gen_clc_h.sh	\
 src/spirsim/clc.h
	$(top_srcdir)/src/spirsim/gen_clc_h.sh $(top_srcdir)/src/spirsim/clc.h $@

liboclgrind_la_SOURCES = src/oclgrind/async_queue.h	\
 src/oclgrind/async_queue.cpp src/oclgrind/icd.h	\
 src/oclgrind/runtime.cpp
liboclgrind_la_LIBADD = libspirsim.la
liboclgrind_la_LDFLAGS = -shared

liboclgrind_icd_la_SOURCES = src/oclgrind/async_queue.h	\
 src/oclgrind/async_queue.cpp src/oclgrind/icd.h	\
 src/oclgrind/runtime-icd.cpp
liboclgrind_icd_la_LIBADD = libspirsim.la
liboclgrind_icd_la_LDFLAGS = -shared

bin_PROGRAMS = oclgrind-kernel
oclgrind_kernel_SOURCES = src/oclgrind-kernel/main.cpp
oclgrind_kernel_LDADD = libspirsim.la

EXTRA_DIST = src/spirsim/gen_clc_h.sh src/spirsim/clc.h
