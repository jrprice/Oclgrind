# configure.ac (Oclgrind)
# Copyright (c) 2013-2015, James Price and Simon McIntosh-Smith,
# University of Bristol. All rights reserved.
#
# This program is provided under a three-clause BSD license. For full
# license terms please see the LICENSE file distributed with this
# source code.

AC_INIT([Oclgrind], [15.5], , [oclgrind], [https://github.com/jrprice/Oclgrind])
AC_PREREQ([2.63])
AC_CONFIG_SRCDIR([src/])
AM_INIT_AUTOMAKE([foreign 1.12])
AC_LANG(C++)
AC_PROG_CXX
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile tests/Makefile])

LT_INIT

# Check if we're compiling with Clang
AS_CASE([`$CC --version`], [*clang*], [using_clang=yes])
AM_CONDITIONAL([USING_CLANG], [test "$using_clang" == "yes"])

oclgrind_extra_libs=

# Check for C++11
AX_CHECK_COMPILE_FLAG([-std=c++11], [],
                      [AC_MSG_ERROR([C++11 support is required])])
CXXFLAGS="$CXXFLAGS -std=c++11"


# --with-llvm option to specify root of LLVM installation
AC_ARG_WITH(
        llvm,
        [AS_HELP_STRING([--with-llvm],
                       [directory containing LLVM installation])],
        [export PATH="$withval/bin:$PATH"])

# Get path to llvm-config
AC_CHECK_PROG(llvm_config, [llvm-config], `which llvm-config`)
if test -z $llvm_config; then
  AC_MSG_ERROR([llvm-config not found (use --with-llvm=)])
fi

# Check version of LLVM
AC_MSG_CHECKING([llvm version])
llvm_full_version=`$llvm_config --version`
llvm_version=`echo $llvm_full_version | cut -b 1,3`
AC_MSG_RESULT($llvm_full_version)
if test $llvm_version -lt 36; then
  AC_MSG_ERROR([LLVM version must be >= 3.6])
fi
AC_DEFINE_UNQUOTED([LLVM_VERSION],
                   [$llvm_version],
                   [Version of LLVM we are building against])

# Add flags for LLVM
CPPFLAGS="$CPPFLAGS `$llvm_config --cppflags`"
LDFLAGS="$LDFLAGS `$llvm_config --ldflags`"


# --with-clang option to specify root of Clang installation
AC_ARG_WITH(
        clang,
        [AS_HELP_STRING([--with-clang],
                       [directory containing Clang installation])],
        [export PATH="$withval/bin:$PATH"
         CPPFLAGS="$CPPFLAGS -I$withval/include/"
         LDFLAGS="$LDFLAGS -L$withval/lib/"])

CPPFLAGS_old="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS -std=c++11"

# Check for Clang binaries, headers and libraries
AC_CHECK_PROG(clang, [clang], `which clang`)
AC_CHECK_HEADERS(
        [clang/CodeGen/CodeGenAction.h],
        [:],
        [AC_MSG_ERROR([Clang headers not found (use --with-clang=)])])
AC_CHECK_LIB(
        [clangFrontend],
        [main],
        [:],
        [AC_MSG_ERROR([Clang libraries not found (use --with-clang)])])

CPPFLAGS="$CPPFLAGS_old"


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
m4_foreach([name], m4_split(m4_include(tests/kernels/TESTS), m4_newline),
[
    KERNEL_TESTS="$KERNEL_TESTS kernels/"name".sim"
    KERNEL_TEST_INPUTS="$KERNEL_TEST_INPUTS kernels/"name".sim"
    KERNEL_TEST_INPUTS="$KERNEL_TEST_INPUTS kernels/"name".cl"
    KERNEL_TEST_INPUTS="$KERNEL_TEST_INPUTS kernels/"name".ref"
])
AC_SUBST(KERNEL_TESTS, $KERNEL_TESTS)
AC_SUBST(KERNEL_TEST_INPUTS, $KERNEL_TEST_INPUTS)

AC_OUTPUT
