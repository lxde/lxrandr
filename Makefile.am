SUBDIRS = src po man

#datafiledir=$(datadir)/lxrandr
#datafile_DATA=data/

desktop_in_files=data/lxrandr.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@
desktopdir=$(datadir)/applications

EXTRA_DIST = \
	autogen.sh \
	data/lxrandr.desktop.in
