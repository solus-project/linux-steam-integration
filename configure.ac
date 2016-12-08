AC_INIT([linux-steam-integration], 0.2, [ikey@solus-project.com], [linux-steam-integration], [https://github.com/solus-project/linux-steam-integration/issues])
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

# Turn on the nica submodule support
AM_CONDITIONAL([NICA_SUBMODULE], true)
AM_CONDITIONAL([COVERAGE], false)

AC_CONFIG_FILES([Makefile
                 src/libnica/Makefile])

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

# The LD_PRELOAD list required to integrate gracefully with the official Steam Runtime
AC_ARG_WITH([preload-libs], AS_HELP_STRING([--with-preload-libs],
        [the preload libs to use with Steams own runtime]), [preloadlibs="${withval}"],
        [preloadlibs="/usr/\$LIB/libxcb.so.1:/usr/\$LIB/libX11.so.6:/usr/\$LIB/libstdc++.so.6"])

AC_DEFINE_UNQUOTED([LSI_PRELOAD_LIBS], "$preloadlibs",
                   [Libraries to preload with Steams runtime])

m4_define([gtk_required_version], [3.4.0])

# Check if frontend is requested
have_frontend="no"
AC_ARG_ENABLE(frontend, AS_HELP_STRING([--enable-frontend], [enable GTK frontend]))
if test "x$enable_frontend" = "xyes" ; then
        PKG_CHECK_MODULES([GTK], [gtk+-3.0 >= gtk_required_version])
        AC_DEFINE([HAVE_FRONTEND], [1], [Enabled GTK+ frontend])
        have_frontend="yes"

fi
AM_CONDITIONAL([HAVE_FRONTEND], [test x$have_frontend = "xyes"])

# Whether LSI should stand in for steam binary
replace_steam="yes"
AC_ARG_ENABLE(replace-steam, AS_HELP_STRING([--disable-replace-steam], [disable Steam binary replacement]))
if test "x$enable_replace_steam" = "xno"; then
    replace_steam="no"
fi
AM_CONDITIONAL([REPLACE_STEAM], [test x$replace_steam = "xyes"])

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
        Replace Steam:          ${replace_steam}
        Real Steam binary:      ${STEAMBIN}
        GTK+ Frontend:          ${have_frontend}
        preload-libs:           ${preloadlibs}

        compiler:               ${CC}
        cflags:                 ${CFLAGS}
        ldflags:                ${LDFLAGS}
])
