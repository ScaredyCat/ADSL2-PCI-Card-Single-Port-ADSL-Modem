noinst_PROGRAMS = sw_debug

LDFLAGS = @LDFLAGS@ $(IFX_LDFLAGS)
CFLAGS = @CFLAGS@ $(IFX_CFLAGS)

INCLUDES = -I$(top_builddir)/src/qgen

sw_debug_SOURCES = debug.c
sw_debug_XTRAS = $(top_builddir)/src/switch/libsw.a \
			$(top_builddir)/src/lib/libatm.la
sw_debug_LDADD = $(sw_debug_XTRAS) -lfl
			
sw_debug_DEPENDENCIES = $(sw_debug_XTRAS)

EXTRA_DIST = demo README

