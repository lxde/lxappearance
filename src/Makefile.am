NULL=

AM_CPPFLAGS = \
	-I$(srcdir) \
	-DPACKAGE_DATA_DIR=\""$(datadir)/lxappearance"\" \
	-DPACKAGE_UI_DIR=\""$(datadir)/lxappearance/ui"\" \
	-DPACKAGE_LIB_DIR=\""$(libdir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	$(NULL)

lxappearance_includedir = $(includedir)/lxappearance
lxappearance_include_HEADERS = lxappearance.h

bin_PROGRAMS = lxappearance
lxappearance_SOURCES = \
	lxappearance.c \
	lxappearance.h \
	widget-theme.c \
	widget-theme.h \
	color-scheme.c \
	color-scheme.h \
	icon-theme.c \
	icon-theme.h \
	cursor-theme.c \
	cursor-theme.h \
	font.c \
	font.h \
	other.c \
	other.h \
	utils.c \
	utils.h \
	plugin.c \
	plugin.h \
	$(NULL)

lxappearance_CFLAGS = \
	$(GTK_CFLAGS) \
	$(XLIB_CFLAGS) \
	$(GMODULE_CFLAGS) \
	$(DBUS_CFLAGS) \
	$(ADDITIONAL_FLAGS) \
	-Werror-implicit-function-declaration \
	$(NULL)

lxappearance_LDADD = \
	$(GTK_LIBS) \
	$(XLIB_LIBS) \
	$(GMODULE_LIBS) \
	$(DBUS_LIBS) \
	$(INTLLIBS) \
	$(NULL)


# Little program to optimize size of xml files
noinst_PROGRAMS=xml-purge
xml_purge_SOURCES=$(top_srcdir)/src/xml-purge.c
xml_purge_CFLAGS=$(GTK_CFLAGS)
xml_purge_LDADD=$(GTK_LIBS)
