## Copyright 2015 Endless Mobile, Inc.

AM_CFLAGS = $(EOS_COMPOSITE_MODE_CFLAGS) -Wall -Werror

CLEANFILES =
EXTRA_DIST =
BUILT_SOURCES =

ecm-generated.c: ecm-generated.h
ecm-generated.h: $(srcdir)/src/com.endlessm.CompositeMode.xml Makefile
	$(AM_V_GEN) gdbus-codegen \
		--interface-prefix com.endlessm.CompositeMode \
		--generate-c-code ecm-generated \
		--c-namespace Ecm $<
EXTRA_DIST += src/com.endlessm.CompositeMode.xml
BUILT_SOURCES += ecm-generated.c ecm-generated.h
CLEANFILES += $(BUILT_SOURCES)

bin_PROGRAMS = eos-composite-mode

eos_composite_mode_SOURCES = \
	$(BUILT_SOURCES) \
	src/ecm-daemon.c src/ecm-daemon.h \
	src/main.c
eos_composite_mode_LDADD = $(EOS_COMPOSITE_MODE_LIBS)

EXTRA_DIST += src/ecm-settings.c

servicedir = $(datadir)/dbus-1/services
service_DATA = com.endlessm.CompositeMode.service

com.endlessm.CompositeMode.service: Makefile
	$(AM_V_GEN) \
	( echo "[D-BUS Service]" ; \
	  echo "Name=com.endlessm.CompositeMode" ; \
	  echo "Exec=${bindir}/eos-composite-mode" ) > $@.tmp && \
	mv $@.tmp $@
CLEANFILES += com.endlessm.CompositeMode.service

gsettings_SCHEMAS = src/com.endlessm.CompositeMode.gschema.xml
EXTRA_DIST += $(gsettings_SCHEMAS)
@GSETTINGS_RULES@

autostartdir = $(sysconfdir)/xdg/autostart
dist_autostart_DATA = com.endlessm.CompositeMode.desktop

gdmautostartdir = $(datadir)/gdm/greeter/autostart
dist_gdmautostart_DATA = com.endlessm.CompositeMode.desktop

EXTRA_DIST += debian
