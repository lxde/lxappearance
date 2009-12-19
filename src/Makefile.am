NULL=

INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	$(PACKAGE_CFLAGS) \
	$(NULL)

bin_PROGRAMS = lxappearance

lxappearance_SOURCES = \
	main.c \
	main-dlg.c main-dlg.h \
	demo.c demo.h \
	$(NULL)

lxappearance_CFLAGS = \
	-Wl,--export-dynamic \
	-Werror-implicit-function-declaration \
	$(NULL)

lxappearance_LDADD = $(PACKAGE_LIBS) $(INTLLIBS)
