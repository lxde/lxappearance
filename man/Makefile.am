
dist_man_MANS = lxappearance.1

EXTRA_DIST = lxappearance.sgml

dist-hook: lxappearance.1

lxappearance.1: lxappearance.sgml
	(test -f "`which docbook-to-man`" && docbook-to-man $< > $@) || \
	(test -f "`which db2man`" && db2man $< > $@) || \
	(test -f "`which docbook2man`" && docbook2man $< > $@)
