## Makefile.am for kdebase/kdehelp/search

INCLUDES = $(all_includes)
LDFLAGS = $(all_libraries) $(KDE_RPATH)
LDADD = $(LIB_KDECORE) $(LIBCOMPAT) 
bindir = $(kde_cgidir)

bin_PROGRAMS = helpsearch

helpsearch_SOURCES = search.cpp mansrch.cpp htmlsrch.cpp
noinst_HEADERS = mansrch.h htmlsrch.h


