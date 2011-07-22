man_MANS = \
	lxappearance.1

man_XMANS = \
	lxappearance.xml

EXTRA_DIST = \
	$(man_MANS) \
	$(man_XMANS)


if ENABLE_MAN

lxappearance.1: lxappearance.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

CLEANFILES = $(man_MANS)
else

$(man_MANS):
	@echo ""
	@echo "Warning: you need to run configure with '--enable-man'"
	@echo "         in order to regenerate man pages."
	@echo ""
endif
