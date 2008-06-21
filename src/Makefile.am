INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@GTK_CFLAGS@

bin_PROGRAMS = lxrandr

lxrandr_SOURCES = \
	lxrandr.c 

lxrandr_LDADD = @GTK_LIBS@ $(INTLLIBS)

