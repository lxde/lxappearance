NULL=

SUBDIRS = src po man

ui_in_files= \
	data/lxappearance.glade \
	data/demo.glade \
	$(NULL)
uidir=$(datadir)/lxappearance
ui_DATA = $(ui_in_files:.glade=.ui)

%.ui: %.glade
	cp $< $@

support_scriptdir=$(datadir)/lxappearance
support_script_DATA= \
        scripts/install-icon-theme.sh \
        $(NULL)

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
	$(ui_DATA) \
	$(support_script_DATA) \
	$(desktop_DATA) \
	$(NULL)

