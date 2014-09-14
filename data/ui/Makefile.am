NULL=

# GtkBuilder UI definition files
uidir=$(datadir)/lxappearance/ui
ui_in_files= \
	about.glade \
	lxappearance.glade \
	$(NULL)
ui_DATA = $(ui_in_files:.glade=.ui)

# Purge GtkBuilder UI files
.glade.ui:
	sed 's/<!--.*-->//' < $< | sed ':a;N;$$!ba;s/ *\n *</</g' > $@

EXTRA_DIST= \
	$(ui_DATA) \
	$(ui_in_files) \
	$(NULL)
