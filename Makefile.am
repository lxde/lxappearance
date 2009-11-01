## Process this file with automake to produce Makefile.in

SUBDIRS = src po man

support_scriptdir=$(datadir)/lxappearance
support_script_DATA= \
        scripts/install-icon-theme.sh

desktopdir=$(datadir)/applications
desktop_in_files=lxappearance.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

install-data-hook:
	for script in $(DESTDIR)$(support_scriptdir)/*.sh; do \
	    chmod +x $$script; \
	done

EXTRA_DIST = \
	autogen.sh \
	$(support_script_DATA) \
	$(desktop_DATA)
