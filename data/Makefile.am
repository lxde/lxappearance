NULL=

SUBDIRS= \
	ui \
	$(NULL)

pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = lxappearance.pc

desktopdir=$(datadir)/applications
desktop_in_files = \
	lxappearance.desktop.in \
	$(NULL)
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST= \
	$(desktop_DATA) \
	$(NULL)
