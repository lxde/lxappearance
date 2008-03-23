## Process this file with automake to produce Makefile.in

INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@

bin_PROGRAMS = lxappearance

lxappearance_SOURCES = \
	main.c \
	glade-support.c glade-support.h \
	main-dlg-ui.c main-dlg-ui.h \
	main-dlg.c main-dlg.h

lxappearance_LDADD = @PACKAGE_LIBS@ $(INTLLIBS)

