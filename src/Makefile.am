NULL=

AM_CPPFLAGS = \
	-I$(srcdir) \
	-DPACKAGE_DATA_DIR=\""$(datadir)/lxappearance"\" \
	-DPACKAGE_UI_DIR=\""$(datadir)/lxappearance/ui"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	$(NULL)

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
	other.c \
	other.h \
	utils.c \
	utils.h \
	$(NULL)

lxappearance_CFLAGS = \
	$(GTK_CFLAGS) \
	-Werror-implicit-function-declaration \
	$(NULL)

lxappearance_LDADD = \
	$(GTK_LIBS) \
	$(INTLLIBS) \
	$(NULL)


# Little program to optimize size of xml files
noinst_PROGRAMS=xml-purge
xml_purge_SOURCES=$(top_srcdir)/src/xml-purge.c
xml_purge_CFLAGS=$(GTK_CFLAGS)
xml_purge_LDADD=$(GTK_LIBS)
