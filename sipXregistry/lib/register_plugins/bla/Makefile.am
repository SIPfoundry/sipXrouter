
SUBDIRS = test

lib_LTLIBRARIES = libRegistrarBlaQueueEvent.la

INCLUDES = -I$(top_srcdir)/include

## Libtool version info - update only immediately before a release
# Current - increment if any interfaces are added, removed, or changed
version_Current=1
# Revision - set to 0 if Current was incremented
#            increment if any implementation has changed
version_Revision=0
# Age - set to 0 of any interfaces have been removed
#       increment if any interfaces have been added
version_Age=0

libRegistrarBlaQueueEvent_la_LDFLAGS = \
	-module \
	-version-info ${version_Current}:${version_Revision}:${version_Age} \
	@PCRE_LDFLAGS@ \
	@SSL_LDFLAGS@

libRegistrarBlaQueueEvent_la_LIBADD = \
	@SIPXCOMMSERVER_LIBS@ \
	@OSSCORE_LIBS@ \
	${prefix}/opt/ossapp/libre/lib/libre.a \
	${prefix}/opt/ossapp/librem/lib/librem.a \
	-ltcmalloc \
        -lpthread  \
        -lstdc++ \
        -lboost_system-mt \
        -lboost_thread-mt \
        -lboost_filesystem-mt \
        -lboost_program_options-mt

libRegistrarBlaQueueEvent_la_CXXFLAGS = \
	@PCRE_CFLAGS@ \
	@SSL_CXXFLAGS@ \
	-I$(top_builddir)/config

libRegistrarBlaQueueEvent_la_SOURCES =  \
   SipBLAQueueEvent.cpp \
   SipBLAQueueEvent.h
