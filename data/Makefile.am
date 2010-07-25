NULL=

SUBDIRS= \
	ui \
	$(NULL)

desktopdir=$(datadir)/applications
desktop_in_files = \
	lxappearance2.desktop.in \
	$(NULL)
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST= \
	$(desktop_DATA) \
	$(NULL)
