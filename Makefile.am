## Copyright 2015 Endless Mobile, Inc.

AM_CFLAGS = $(EOS_COMPOSITE_MODE_CFLAGS) -Wall -Werror

BUILT_SOURCES =

ecm-generated.c: ecm-generated.h
ecm-generated.h: $(srcdir)/src/com.endlessm.CompositeMode.xml Makefile
	$(AM_V_GEN) gdbus-codegen \
		--interface-prefix com.endlessm.CompositeMode \
		--generate-c-code ecm-generated \
		--c-namespace Ecm $<
BUILT_SOURCES += ecm-generated.c ecm-generated.h

bin_PROGRAMS = eos-composite-mode

eos_composite_mode_SOURCES = \
	$(BUILT_SOURCES) \
	src/ecm-daemon.c src/ecm-daemon.h \
	src/main.c
eos_composite_mode_LDADD = $(EOS_COMPOSITE_MODE_LIBS)
