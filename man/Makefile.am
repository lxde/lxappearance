
dist_man_MANS = lxapperance.1

EXTRA_DIST = lxapperance.sgml

dist-hook: lxapperance.1

lxapperance.1: lxapperance.sgml
	docbook-to-man $< > $@
