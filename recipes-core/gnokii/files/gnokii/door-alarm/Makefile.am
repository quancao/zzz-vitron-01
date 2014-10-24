DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

bin_PROGRAMS = alarmsms
alarmsms_SOURCES = door-alarm.c database.c gsmhelper.c cJSON.c cJSON.h encode.c sensor-socket.c sensor-com.h

alarmsms_LDADD = $(top_builddir)/common/libgnokii.la $(INTLLIBS) $(TERMLDFLAGS) $(TERMLIBS)
alarmsms_CFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/door-alarm
alarmsms_LDFLAGS = -lm

bin_PROGRAMS += testjson
testjson_SOURCES = cJSON.c cJSON.h test.c
testjson_LDADD = 
testjson_CFLAGS = -I$(top_srcdir)/door-alarm -DTESTFILE=\"$(abs_top_srcdir)/door-alarm/phone.json\"
testjson_LDFLAGS = -lm

bin_PROGRAMS += decodetool
decodetool_SOURCES = decode_tool.c encode.c

bin_PROGRAMS += sensortest
sensortest_SOURCES = sensor-test.c
