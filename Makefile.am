## Process this file with automake to produce Makefile.in

SUBDIRS = src po

scriptdir=$(datadir)/lxappearance
script_DATA= \
        scripts/install-icon-theme.sh

desktopdir=$(datadir)/applications
desktop_DATA=lxappearance.desktop

install-data-hook:
	for script in $(scriptdir)/*.sh; do \
	    chmod +x $$script; \
	done

EXTRA_DIST = \
	autogen.sh \
	$(script_DATA) \
	$(desktop_DATA)
