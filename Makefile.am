AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

AM_CFLAGS = -wno-deprecated-declarations

lib_LTLIBRARIES = libspirsim.la liboclgrind.la

libspirsim_la_SOURCES = src/sprisim/common.h src/spirsim/common.cpp	\
 src/spirsim/Device.h src/spirsim/Device.cpp src/spirsim/Queue.h	\
 src/spirsim/Queue.cpp src/spirsim/half.h src/spirsim/WorkItem.h	\
 src/spirsim/WorkItem.cpp src/spirsim/WorkItemBuiltins.cpp		\
 src/spirsim/WorkGroup.h src/spirsim/WorkGroup.cpp			\
 src/spirsim/Program.h src/spirsim/Program.cpp src/spirsim/Kernel.h	\
 src/spirsim/Kernel.cpp src/spirsim/Memory.h src/spirsim/Memory.cpp
nodist_libspirsim_la_SOURCES = src/spirsim/clc_h.cpp
libspirsim_la_LDFLAGS = -lclangCodeGen -lclangAST -lclang		\
-lLLVMBitReader -lLLVMBitWriter -lLLVMAsmParser -lLLVMInstrumentation	\
-lLLVMipo -lLLVMipa -lLLVMInstCombine -lLLVMLinker -lLLVMMC		\
-lLLVMScalarOpts -lLLVMTarget -lLLVMTransformUtils -lLLVMVectorize	\
-lLLVMAnalysis -lLLVMCore -lLLVMSupport -ldl -lpthread -shared
spirsimincludedir = $(includedir)/spirsim
spirsiminclude_HEADERS = src/spirsim/Device.h src/spirsim/Kernel.h	\
src/spirsim/Memory.h src/spirsim/Program.h
CLEANFILES = src/spirsim/clc_h.cpp
src/spirsim/clc_h.cpp: Makefile src/spirsim/gen_clc_h.sh src/spirsim/clc.h
	src/spirsim/gen_clc_h.sh src/spirsim/clc.h $@

liboclgrind_la_SOURCES = src/oclgrind/init.cpp src/oclgrind/icd.h	\
src/oclgrind/rename_api.h src/oclgrind/runtime.cpp			\
src/oclgrind/async_queue.h src/oclgrind/async_queue.cpp
liboclgrind_la_CPPFLAGS = -I$(top_srcdir)/src/
liboclgrind_la_LIBADD = libspirsim.la
liboclgrind_la_LDFLAGS = -shared

bin_PROGRAMS = oclgrind-kernel
oclgrind_kernel_SOURCES = src/oclgrind-kernel/main.cpp
oclgrind_kernel_CPPFLAGS = -I$(top_srcdir)/src/
oclgrind_kernel_LDADD = libspirsim.la
