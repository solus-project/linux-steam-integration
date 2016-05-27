AC_INIT([linux-steam-integration], 0.1, [ikey@solus-project.com], [linux-steam-integration], [https://github.com/solus-project/linux-steam-integration/issues])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([-Wno-portability no-dist-gzip dist-xz foreign subdir-objects])
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CC_STDC
LT_PREREQ(2.2)
AC_CONFIG_HEADERS([config.h])
AC_PREFIX_DEFAULT(/usr/local)
AM_SILENT_RULES([yes])
LT_INIT([disable-static])
AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_FILES([Makefile])

# Check if we're using the new C++ ABI, which will require LD_PRELOAD magic
AC_LANG_PUSH([C++])
AC_MSG_CHECKING(['_GLIBCXX_USE_CXX11_ABI' value])
AC_COMPUTE_INT([LSI_NEW_CXX_ABI], [_GLIBCXX_USE_CXX11_ABI], [[#include <string>]], [LSI_NEW_CXX_ABI=0])
AC_MSG_RESULT([$LSI_NEW_CXX_ABI])
AC_LANG_POP([C++])

if test x$LSI_NEW_CXX_ABI = "x1"; then
        use_new_abi="yes"
        AC_DEFINE([LSI_USE_NEW_ABI], [1], [Using new C++ ABI])
else
        use_new_abi="no"
fi

# Source: https://mail.gnome.org/archives/commits-list/2012-September/msg01730.html
# adl_RECURSIVE_EVAL(VALUE, RESULT)
# =================================
# Interpolate the VALUE in loop until it doesn't change,
# and set the result to $RESULT.
# WARNING: It's easy to get an infinite loop with some unsane input.
# For example ${datadir} becomes ${datarootdir}, and then ${prefix}/share, and
# finally ${prefix} is replaced by the prefix.
AC_DEFUN([adl_RECURSIVE_EVAL],
[_lcl_receval="$1"
$2=`(test "x$prefix" = xNONE && prefix="$ac_default_prefix"
     test "x$exec_prefix" = xNONE && exec_prefix="${prefix}"
     _lcl_receval_old=''
     while test "[$]_lcl_receval_old" != "[$]_lcl_receval"; do
       _lcl_receval_old="[$]_lcl_receval"
       eval _lcl_receval="\"[$]_lcl_receval\""
     done
     echo "[$]_lcl_receval")`])

# Shadow location of real Steam
AC_ARG_WITH([real-steam-binary], AS_HELP_STRING([--with-real-steam-binary],
        [the real Steam binary to use]), [steambin=${withval}],
        [steambin="$libdir/steam/steam"])

adl_RECURSIVE_EVAL(["$steambin"], [STEAMBIN])
AC_SUBST(STEAMBIN)

AC_DEFINE_UNQUOTED(STEAM_BINARY, "$STEAMBIN",
                   [The location of the real Steam binary])
AC_OUTPUT

AC_MSG_RESULT([
        linux-steam-integration $VERSION
        ========

        prefix:                 ${prefix}
        libdir:                 ${libdir}
        sysconfdir:             ${sysconfdir}
        exec_prefix:            ${exec_prefix}
        bindir:                 ${bindir}
        datarootdir:            ${datarootdir}

        New C++ ABI:            ${use_new_abi}
        Real Steam binary:      ${STEAMBIN}

        compiler:               ${CC}
        cflags:                 ${CFLAGS}
        ldflags:                ${LDFLAGS}
])
