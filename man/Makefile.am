
dist_man_MANS = lxrandr.1

EXTRA_DIST = lxrandr.sgml

dist-hook: lxpanel.1

lxrandr.1: lxrandr.sgml
	docbook-to-man $< > $@
