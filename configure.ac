# configure.ac (Oclgrind)
# Copyright (c) 2013-2015, James Price and Simon McIntosh-Smith,
# University of Bristol. All rights reserved.
#
# This program is provided under a three-clause BSD license. For full
# license terms please see the LICENSE file distributed with this
# source code.

AC_INIT([Oclgrind], [15.2], , [oclgrind], [https://github.com/jrprice/Oclgrind])
AC_PREREQ([2.63])
AC_CONFIG_SRCDIR([src/])
AM_INIT_AUTOMAKE([foreign 1.12])
AC_LANG(C++)
AC_PROG_CXX
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

# Disable alignas attribute for Clang due to bug in LLVM 3.2 Support/AlignOf.h
DISABLE_ALIGNAS="
#ifdef __has_feature
#if __has_feature(cxx_alignas)
#define alignas(x)
#endif
#endif"
AH_BOTTOM([
#ifdef __has_feature
#if __has_feature(cxx_alignas)
#define alignas(x)
#endif
#endif])

LT_INIT

# Check if we're compiling with Clang
AS_CASE([`$CC --version`], [*clang*], [using_clang=yes])
AM_CONDITIONAL([USING_CLANG], [test "$using_clang" == "yes"])

oclgrind_extra_libs=

# Check for C++11
AX_CXX_COMPILE_STDCXX_11(, [mandatory])

# Add LLVM required macro definitions
CPPFLAGS="$CPPFLAGS -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS"

# --with-spir option to specify root of Clang/SPIR generator
AC_ARG_WITH(
        spir,
        [AS_HELP_STRING([--with-spir],
                       [directory containing Clang/SPIR generator ])],
        [CPPFLAGS="$CPPFLAGS -I$withval/include";
         LDFLAGS="$LDFLAGS -L$withval/lib";
         AC_SUBST(clang_spir, $withval/bin/clang)])

# Check for LLVM/Clang headers/libraries
AC_CHECK_HEADERS(
        [llvm/Instruction.h clang/CodeGen/CodeGenAction.h],
        [:],
        [AC_MSG_ERROR([LLVM/Clang includes not found (use --with-spir=)])],
        [$DISABLE_ALIGNAS])

AC_CHECK_LIB(
        [clang],
        [clang_getClangVersion],
        [:],
        [AC_MSG_ERROR([Clang library not found (use --with-spir)])])

# Find Clang/SPIR binary (assume clang on PATH if --with-spir not used)
AC_CHECK_PROG(clang_spir, [clang], `which clang`)


# GNU readline library (for interactive debugger)
AC_ARG_WITH(
        [readline],
        AS_HELP_STRING([--with-readline],
                       [location of GNU readline library]),
        [CPPFLAGS="$CPPFLAGS -I$withval/include";
         LDFLAGS="$LDFLAGS -L$withval/lib"])

have_readline=true
AC_CHECK_HEADER(
        [readline/readline.h],
        [:],
        [have_readline=false])
AC_CHECK_HEADER(
        [readline/history.h],
        [:],
        [have_readline=false])
AC_CHECK_LIB(
        [readline],
        [readline],
        [:],
        [have_readline=false])
AC_CHECK_LIB(
        [readline],
        [add_history],
        [:],
        [have_readline=false])
if test $have_readline = true; then
    AC_DEFINE([HAVE_READLINE], [1], [Define to 1 if GNU readline found])
    oclgrind_extra_libs="$oclgrind_extra_libs -lreadline"
else
    AC_MSG_WARN([GNU readline library not found (use --with-readline)])
fi


AC_SUBST([oclgrind_extra_libs], [$oclgrind_extra_libs])


# Check if Python is available (required to run tests)
AM_PATH_PYTHON(,,[:])
AM_CONDITIONAL([HAVE_PYTHON], [test "$PYTHON" != :])

# Kernel tests
KERNEL_TESTS=""
KERNEL_TEST_INPUTS=""
KERNEL_TEST_OUTPUTS=""
m4_foreach([name], m4_split(m4_include(tests/kernels/TESTS), m4_newline),
[
    KERNEL_TESTS="$KERNEL_TESTS tests/kernels/"name".sim"
    KERNEL_TEST_INPUTS="$KERNEL_TEST_INPUTS tests/kernels/"name".sim"
    KERNEL_TEST_INPUTS="$KERNEL_TEST_INPUTS tests/kernels/"name".cl"
    KERNEL_TEST_INPUTS="$KERNEL_TEST_INPUTS tests/kernels/"name".ref"
    KERNEL_TEST_OUTPUTS="$KERNEL_TEST_OUTPUTS tests/kernels/"name".out"
])
AC_SUBST(KERNEL_TESTS, $KERNEL_TESTS)
AC_SUBST(KERNEL_TEST_INPUTS, $KERNEL_TEST_INPUTS)
AC_SUBST(KERNEL_TEST_OUTPUTS, $KERNEL_TEST_OUTPUTS)

AC_OUTPUT
