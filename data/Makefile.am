man_in_files = lxrandr.xml

man_MANS = $(man_in_files:.xml=.1)

if ENABLE_REGENERATE_MAN
.xml.1:
	$(XSLTPROC) -o $@ -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<
endif

desktop_in_files = lxrandr.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@
desktopdir=$(datadir)/applications

DISTCLEANFILES = $(desktop_DATA)

EXTRA_DIST = \
	$(man_MANS) \
	$(man_in_files) \
	$(desktop_DATA) \
	$(desktop_in_files)
