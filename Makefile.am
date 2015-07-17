# Makefile.am (Oclgrind)
# Copyright (c) 2013-2015, James Price and Simon McIntosh-Smith,
# University of Bristol. All rights reserved.
#
# This program is provided under a three-clause BSD license. For full
# license terms please see the LICENSE file distributed with this
# source code.

AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

SUBDIRS = . tests

AM_CPPFLAGS = -I$(top_srcdir)/src/ -Wall -fno-rtti

# Suppress warnings from OpenCL runtime API headers
if USING_CLANG
AM_CPPFLAGS += -Wno-ignored-attributes -Wno-gcc-compat -Wno-availability
endif USING_CLANG

lib_LTLIBRARIES = liboclgrind.la liboclgrind-rt.la liboclgrind-rt-icd.la

LLVM_LIBS = `$(llvm_config) --system-libs --libs bitreader bitwriter	\
 core instrumentation ipo irreader linker mcparser objcarcopts option target`

liboclgrind_la_SOURCES = src/core/common.h src/core/common.cpp		\
 src/core/Context.h src/core/Context.cpp src/core/half.h		\
 src/core/half.cpp src/core/Kernel.h src/core/Kernel.cpp		\
 src/core/KernelInvocation.h src/core/KernelInvocation.cpp		\
 src/core/Memory.h src/core/Memory.cpp src/core/Plugin.h		\
 src/core/Plugin.cpp src/core/Program.h src/core/Program.cpp		\
 src/core/Queue.h src/core/Queue.cpp src/core/WorkItem.h		\
 src/core/WorkItem.cpp src/core/WorkItemBuiltins.cpp			\
 src/core/WorkGroup.h src/core/WorkGroup.cpp				\
 src/plugins/InstructionCounter.h src/plugins/InstructionCounter.cpp	\
 src/plugins/InteractiveDebugger.h src/plugins/InteractiveDebugger.cpp	\
 src/plugins/Logger.h src/plugins/Logger.cpp src/plugins/MemCheck.h	\
 src/plugins/MemCheck.cpp src/plugins/RaceDetector.h			\
 src/plugins/RaceDetector.cpp src/plugins/Uninitialized.h		\
 src/plugins/Uninitialized.cpp
nodist_liboclgrind_la_SOURCES = src/core/clc_h.cpp config.h
liboclgrind_la_LDFLAGS = -lclangFrontend -lclangDriver		\
-lclangSerialization -lclangCodeGen -lclangParse -lclangSema	\
-lclangAnalysis -lclangEdit -lclangAST -lclangLex -lclangBasic	\
${LLVM_LIBS} $(oclgrind_extra_libs) -shared
oclgrind_includedir = $(includedir)/oclgrind
oclgrind_include_HEADERS = src/core/common.h src/core/Context.h	\
 src/core/half.h src/core/Kernel.h src/core/KernelInvocation.h	\
 src/core/Memory.h src/core/Plugin.h src/core/Program.h		\
 src/core/Queue.h src/core/WorkItem.h src/core/WorkGroup.h config.h LICENSE
src/core/clc_h.cpp: src/core/gen_clc_h.sh	src/core/clc.h
	$(top_srcdir)/src/core/gen_clc_h.sh $(top_srcdir)/src/core/clc.h $@

install-data-hook:
	cp -p src/include/oclgrind/clc.h      $(DESTDIR)$(includedir)/oclgrind/
	cp -p src/include/oclgrind/clc32.pch  $(DESTDIR)$(includedir)/oclgrind/
	cp -p src/include/oclgrind/clc64.pch  $(DESTDIR)$(includedir)/oclgrind/

uninstall-hook:
	rm -rf $(DESTDIR)$(includedir)/oclgrind/clc.h
	rm -rf $(DESTDIR)$(includedir)/oclgrind/clc32.pch
	rm -rf $(DESTDIR)$(includedir)/oclgrind/clc64.pch

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
	| $(SED) 's|__VERSION__|'$(VERSION)'|g' \
	>$@
noinst_SCRIPTS = oclgrind.icd \
 src/include/oclgrind/clc.h \
 src/include/oclgrind/clc32.pch \
 src/include/oclgrind/clc64.pch
oclgrind.icd: liboclgrind-rt-icd.la
	printf $(libdir)/ >$@
	$(GREP) dlname $< | $(AWK) -F "'" '{print $$2}' >>$@

src/include/oclgrind/clc.h: $(top_srcdir)/src/core/clc.h
	mkdir -p src/include/oclgrind
	cp $< $@

src/include/oclgrind/clc32.pch: src/include/oclgrind/clc.h
	$(clang) \
		-cc1 -x cl -cl-std=CL1.2 -O0 -g -fno-builtin \
		-emit-pch -triple spir-unknown-unknown \
		-relocatable-pch \
		-isysroot $(abs_builddir)/src/include/oclgrind \
		$< -o $@
src/include/oclgrind/clc64.pch: src/include/oclgrind/clc.h
	$(clang) \
		-cc1 -x cl -cl-std=CL1.2 -O0 -g -fno-builtin \
		-emit-pch -triple spir64-unknown-unknown \
		-relocatable-pch \
		-isysroot $(abs_builddir)/src/include/oclgrind \
		$< -o $@

EXTRA_DIST = NEWS src/core/gen_clc_h.sh src/core/clc.h			\
 src/runtime/oclgrind src/CL/cl.h src/CL/cl_gl.h src/CL/cl_platform.h	\
 src/CL/cl_ext.h src/CL/cl_gl_ext.h src/CL/cl_egl.h src/CL/cl_d3d10.h	\
 src/CL/cl_d3d11.h src/CL/cl_dx9_media_sharing.h src/CL/opencl.h	\
 CMakeLists.txt tests/apps/CMakeLists.txt tests/runtime/CMakeLists.txt	\
 cmake_config.h.in src/core/gen_clc_h.cmake src/runtime/icd.def		\
 src/runtime/runtime.def src/install/INSTALL.darwin			\
 src/install/INSTALL.linux src/install/INSTALL.windows			\
 src/install/install.bat src/install/uninstall.bat			\
 src/install/oclgrind-icd.reg
CLEANFILES = src/core/clc_h.cpp $(bin_SCRIPTS) $(noinst_SCRIPTS)
