#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.64])
AC_INIT([lxappearance],[0.6.3],[http://lxde.org/])
AM_INIT_AUTOMAKE([1.11 -Wall -Werror foreign subdir-objects no-dist-gzip dist-xz])

# intltool
IT_PROG_INTLTOOL([0.40.0])

# Support silent build rules. Disable by either passing --disable-silent-rules
# to configure or passing V=1 to make
AM_SILENT_RULES([yes])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([.])

# C compiler
AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_CC_C_O
AC_PROG_CC_STDC

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.


AC_ARG_ENABLE(man,
AS_HELP_STRING([--enable-man],[regenerate roff man pages from Docbook @<:@default=no@:>@]),
[case "${enableval}" in
  yes)  enable_man=yes ;;
  no)   enable_man=no ;;
  *) AC_MSG_ERROR([bad value "${enableval}" for --enable-man, use "no" (default) or "yes".]) ;;
esac],[])

AC_ARG_ENABLE(more_warnings,
       [AS_HELP_STRING([--enable-more-warnings],
               [Add more warnings @<:@default=no@:>@])],
       [enable_more_warnings="${enableval}"],
       [enable_more_warnings=no]
)

AC_ARG_ENABLE(gtk3,
AS_HELP_STRING([--enable-gtk3],[enable to use gtk-3.0 instead of gtk-2.0]),
[case "${enableval}" in
  yes)  enable_gtk3=yes ;;
  no)   enable_gtk3=no ;;
  *) AC_MSG_ERROR([bad value "${enableval}" for --enable-gtk3, use "yes" (default) or "no".]) ;;
esac],[])

PKG_CHECK_MODULES(XLIB, "x11")
AC_SUBST(XLIB_CFLAGS)
AC_SUBST(XLIB_LIBS)

if test "x$enable_gtk3" = "xyes" ; then
  CFLAGS="$CFLAGS -DENABLE_GTK3 -Wno-deprecated-declarations"
  gtk_modules="gtk+-3.0 >= 3.0.0 gthread-2.0"
else
  gtk_modules="gtk+-2.0 >= 2.12.0 gthread-2.0"
fi

PKG_CHECK_MODULES(GTK, [$gtk_modules])
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

AC_ARG_ENABLE(dbus,
       [AS_HELP_STRING([--enable-dbus],
               [Use Dbus-glib support (deprecated now) @<:@default=no@:>@])],
       [enable_dbus="${enableval}"],
       [enable_dbus=no]
)


if test x"$enable_dbus" = x"yes"; then
       PKG_CHECK_MODULES(DBUS, [dbus-1 >= 0.95])
       CFLAGS="$CFLAGS -DENABLE_DBUS"
fi
AC_SUBST(DBUS_CFLAGS)
AC_SUBST(DBUS_LIBS)



gmodule_modules="gmodule-export-2.0"
PKG_CHECK_MODULES(GMODULE, [$gmodule_modules])
AC_SUBST(GMODULE_CFLAGS)
AC_SUBST(GMODULE_LIBS)
AC_SUBST(gtk_modules)

# gio_modules="gthread-2.0 gio-unix-2.0 glib-2.0 >= 2.18.0"
# PKG_CHECK_MODULES(GIO, [$gio_modules])
# AC_SUBST(GIO_CFLAGS)
# AC_SUBST(GIO_LIBS)

# menu_modules="libmenu-cache >= 0.3.2"
# PKG_CHECK_MODULES(MENU_CACHE, [$menu_modules])
# AC_SUBST(MENU_CACHE_CFLAGS)
# AC_SUBST(MENU_CACHE_LIBS)

if test x"$enable_more_warnings" = x"yes"; then
  ADDITIONAL_FLAGS="-Wall -Werror=all -Werror=format -Werror=implicit-function-declaration -Werror=implicit-int -Werror=missing-braces -Werror=parentheses -Werror=return-type -Werror=strict-aliasing -Werror=switch -Wuninitialized -Werror=unused-label -Werror=unused-value -Wextra -Wno-missing-field-initializers -Wno-unused-parameter -Werror=missing-declarations -Wredundant-decls -Wmissing-noreturn -Wpointer-arith -Wcast-align -Wwrite-strings -Werror=inline -Werror=format-nonliteral -Wformat-nonliteral -Werror=format-security -Wformat-security -Winit-self -Werror=missing-include-dirs -Werror=undef -Werror=aggregate-return -Wmissing-format-attribute -Werror=nested-externs -fno-strict-aliasing -fmessage-length=0 -Wp,-D_FORTIFY_SOURCE=2 -DG_DISABLE_DEPRECATED -DG_DISABLE_SINGLE_INCLUDES -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES -DGTK_DISABLE_DEPRECATED -DGTK_DISABLE_SINGLE_INCLUDES"
fi
AC_SUBST(ADDITIONAL_FLAGS)

AC_ARG_ENABLE(debug,
    [AC_HELP_STRING([--enable-debug],
        [build lxappearance with debug support @<:@default=no@:>@])],
    [enable_debug="${enableval}"],
    [enable_debug=no]
)

if test "$enable_debug" = "yes"; then
    # turn on debug and disable optimization
    CPPFLAGS="$CPPFLAGS -DG_ENABLE_DEBUG -O0 -g"
    case "$CC" in
    gcc*)
        CPPFLAGS="$CPPFLAGS -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers"
        ;;
    *)
        ;;
    esac
    dnl Be more strict on portability
    CPPFLAGS="$CPPFLAGS -D_POSIX_C_SOURCE=200112L -D_XOPEN_SOURCE=700"
else
    # turn off glib debug checks
    CPPFLAGS="$CPPFLAGS -DG_DISABLE_ASSERT -DG_DISABLE_CHECKS -DG_DISABLE_CAST_CHECKS"
fi

# Generate po/LINGUAS on the fly rather than relying on translators
# to maintain it manually. This also overcome the problem that Transifex
# cannot add a language to po/LINGUAS if a new po file is submitted.
rm -f $srcdir/po/LINGUAS
for po_file in `ls $srcdir/po/*.po | sort`;
do
    lang=`echo "$po_file" | sed "s|.*/po/\(.*\)\.po|\1|g"`
    echo $lang >> $srcdir/po/LINGUAS
done

GETTEXT_PACKAGE=lxappearance
AC_SUBST(GETTEXT_PACKAGE)
AM_GLIB_GNU_GETTEXT
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])

if test x"$enable_man" = x"yes"; then
	AC_PATH_PROG([XSLTPROC], [xsltproc])
	if test -z "$XSLTPROC"; then
		AC_MSG_ERROR([xsltproc is required to regenerate the pre-built man page; consider --disable-man])
	fi

	dnl check for DocBook DTD and stylesheets in the local catalog.
	JH_CHECK_XML_CATALOG([-//OASIS//DTD DocBook XML V4.1.2//EN],
		[DocBook XML DTD V4.1.2], [], AC_MSG_ERROR([DocBook XML DTD is required to regenerate the pre-built man page; consider --disable-man]))
	JH_CHECK_XML_CATALOG([http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl],
		[DocBook XSL Stylesheets >= 1.70.1], [], AC_MSG_ERROR([DocBook XSL Stylesheets are required to regenerate the pre-built man page; consider --disable-man]))

	rm -f $srcdir/man/lxappearance.1
fi

AM_CONDITIONAL(ENABLE_MAN, test "x$enable_man" != "xno")

AC_CONFIG_FILES([
    Makefile
    data/Makefile
    data/ui/Makefile
    data/ui/about.glade
    data/lxappearance.pc
    man/Makefile
    po/Makefile.in
    src/Makefile
])

AC_OUTPUT

echo
echo lxappearance....................... : Version $VERSION
echo
echo Enable debug....................: "$enable_debug"
echo Enable Dbus-glib support........: "$enable_dbus"
echo Prefix..........................: $prefix
echo
echo The binary will be installed in $prefix/bin
echo
echo http://lxde.org/
echo
echo "Ready to make lxappearance. Type 'make' to continue."
echo

