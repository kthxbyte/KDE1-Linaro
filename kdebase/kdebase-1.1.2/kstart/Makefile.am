####### Fiddle here

INCLUDES = $(all_includes)
LDFLAGS = $(all_libraries) $(KDE_RPATH)
LDADD   =       $(LIB_KDEUI)

#######	Files

bin_PROGRAMS	= kstart
kstart_SOURCES	= kstart.C
METASOURCES =	kstart.moc 

noinst_HEADERS = kstart.h version.h

messages:
	$(XGETTEXT) -C -ktranslate -ki18n -x $(includedir)/kde.pot $(kstart_SOURCES) && mv messages.po ../po/kstart.pot

EXTRA_DIST = CHANGES LICENSE.readme

