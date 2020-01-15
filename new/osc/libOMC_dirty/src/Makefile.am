DEBUG_CFLAGS = -DDEBUG
LIBTOOL = $(SHELL) $(top_builddir)/libtool

lib_LTLIBRARIES = libOMC.la
include_HEADERS = libomc.h
pkginclude_HEADERS = NetworkReturnAddress.h \
			OSC-address-space.h \
			OSC-callbacklist.h \
			OSC-common.h \
			OSC-dispatch.h \
			OSC-drop.h \
			OSC-internal-messages.h \
			OSC-pattern-match.h \
			OSC-priority-queue.h \
			OSC-receive.h \
			OSC-string-help.h \
			OSC-timetag.h \
			NetworkUDP.h 

libOMC_la_CFLAGS = $(DEBUG_CFLAGS)
libOMC_la_LDFLAGS = -rdynamic 
libOMC_la_SOURCES = OSC-address-space.c \
		 OSC-callbacklist.c \
		 OSC-drop.c \
		 OSC-pattern-match.c \
		 OSC-priority-queue.c \
		 OSC-receive.c \
		 OSC-string-help.c \
		 OSC-system-dependent.c \
		 OSC-timetag.c \
		 NetworkReturnAddress.c 

