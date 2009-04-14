
dist_man_MANS = lxrandr.1

EXTRA_DIST = lxrandr.sgml

dist-hook: lxpanel.1

lxrandr.1: lxrandr.sgml
	(test -f "`which docbook-to-man`" && docbook-to-man $< > $@) || \
	(test -f "`which db2man`" && db2man $< > $@) || \
	(test -f "`which docbook2man`" && docbook2man $< > $@)
