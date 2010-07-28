NULL=

SUBDIRS=src po data

support_scriptdir=$(datadir)/lxappearance2
support_script_DATA= \
        scripts/install-icon-theme.sh \
        $(NULL)

EXTRA_DIST = \
	$(support_script_DATA) \
	$(NULL)

install-data-hook:
	for script in $(DESTDIR)$(support_scriptdir)/*.sh; do \
	    chmod +x $$script; \
	done
