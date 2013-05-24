AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

AM_CPPFLAGS = -I./llvm/build/include/
AM_LDFLAGS = -L./llvm/build/lib/ -lLLVMSupport -lLLVMCore		\
-lLLVMBitReader -lLLVMAsmParser -lLLVMAnalysis -lLLVMTransformUtils	\
-lLLVMScalarOpts -lLLVMTarget -lLLVMIRReader

bin_PROGRAMS = oclgrind
oclgrind_SOURCES = src/main.cpp src/Simulator.h src/Simulator.cpp	\
src/WorkItem.h src/WorkItem.cpp
