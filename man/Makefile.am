
dist_man_MANS = lxappearance.1

EXTRA_DIST = lxappearance.sgml

dist-hook: lxappearance.1

lxappearance.1: lxappearance.sgml
	docbook-to-man $< > $@
