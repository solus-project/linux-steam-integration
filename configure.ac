AC_INIT([linux-steam-integration], 0.1, [ikey@solus-project.com], [linux-steam-integration], [https://github.com/solus-project/linux-steam-integration/issues])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([-Wno-portability no-dist-gzip dist-xz foreign subdir-objects])
AC_PROG_CC
AC_PROG_CC_STDC
LT_PREREQ(2.2)
AC_CONFIG_HEADERS([config.h])
AC_PREFIX_DEFAULT(/usr/local)
AM_SILENT_RULES([yes])
LT_INIT([disable-static])
AC_CONFIG_MACRO_DIR([m4])

# emul32 directory. On Solus this is /usr/lib32. multiarch systems are likely
# using /usr/lib/i386-linux-gnu/
AC_ARG_WITH([emul32-libdir], AS_HELP_STRING([--with-emul32-libdir],
        [the path in which 32-bit libraries are stored]), [emul32dir=${withval}],
        [emul32dir="/usr/lib32"])

AC_SUBST(emul32dir)
AC_DEFINE_UNQUOTED(EMUL32LIBDIR, "$emul32dir",
                   [The location of 32-bit libraries in the OS configuration])


AC_CONFIG_FILES([Makefile])

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

        emul32-libdir:          ${emul32dir}

        compiler:               ${CC}
        cflags:                 ${CFLAGS}
        ldflags:                ${LDFLAGS}
])
