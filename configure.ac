AC_INIT([Oclgrind], [0.1], [j.price@bristol.ac.uk], [oclgrind], [https://github.com/jrprice/Oclgrind])
AC_PREREQ([2.63])
AC_CONFIG_SRCDIR([src/])
AM_INIT_AUTOMAKE([foreign])
AC_LANG(C++)
AC_PROG_CXX
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

LT_INIT

# Add LLVM required macro definitions
CPPFLAGS="$CPPFLAGS -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS"

# --with-spir option to specify root of Clang/SPIR generator
AC_ARG_WITH(
        spir,
        AS_HELP_STRING([--with-spir],
                       [directory containing Clang/SPIR generator ]),
        [CPPFLAGS="$CPPFLAGS -I$withval/include";
         LDFLAGS="$LDFLAGS -L$withval/lib"])

# Check for LLVM/Clang headers/libraries
AC_CHECK_HEADERS(
        [llvm/Instruction.h clang/CodeGen/CodeGenAction.h],
        [:],
        AC_MSG_ERROR([LLVM/Clang includes not found (use --with-spir=)]))
AC_CHECK_LIB(
        [clang],
        [clang_getClangVersion],
        [:],
        AC_MSG_ERROR([Clang library not found (use --with-spir)]))

AC_OUTPUT
