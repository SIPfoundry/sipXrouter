INCLUDES = -I$(top_srcdir)/include

lib_LTLIBRARIES = libdialogEventCollator.la

## Libtool version info - update only immediately before a release
# Current - increment if any interfaces are added, removed, or changed
version_Current=2
# Revision - set to 0 if Current was incremented
#            increment if any implementation has changed
version_Revision=0
# Age - set to 0 of any interfaces have been removed
#       increment if any interfaces have been added
version_Age=0

libdialogEventCollator_la_LDFLAGS = \
	-shared \
	-version-info ${version_Current}:${version_Revision}:${version_Age}

libdialogEventCollator_la_LIBADD = \
	@LIB_OSS_CORE_LA@ \
	@OSS_CORE_DEP_LIBS@ \
	@LIB_OSS_CARP_LA@ \
	@LIB_OSS_TURN_LA@ \
	${prefix}/opt/ossapp/libre/lib/libre.a \
	${prefix}/opt/ossapp/librem/lib/librem.a \
	-ltcmalloc \
        -lpthread  \
        -lstdc++ \
        -lboost_system-mt \
        -lboost_thread-mt \
        -lboost_filesystem-mt \
        -lboost_program_options-mt

# this is helpful if you're debugging something with lots of inlines (like the UtlLink code)
# 		-fno-gcse -O0 -fno-implement-inlines -fno-default-inline
libdialogEventCollator_la_CXXFLAGS = \
	@PCRE_CFLAGS@ \
	@SSL_CXXFLAGS@ \
	-DSIPX_TMPDIR=\"@SIPX_TMPDIR@\" \
	-DSIPX_CONFDIR=\"@SIPX_CONFDIR@\" \
	-fno-gcse -O0 -fno-implement-inlines -fno-default-inline

libdialogEventCollator_la_SOURCES =  \
    DialogEventCollator/DialogCollator.cpp \
    DialogEventCollator/DialogCollatorPlugin.cpp \
    DialogEventCollator/DialogInfo.cpp \
    xmlparser/tinystr.cpp \
    xmlparser/tinyxml.cpp \
    xmlparser/tinyxmlerror.cpp \
    xmlparser/tinyxmlparser.cpp

bin_PROGRAMS = \
    dialog_event_collator_unit_test 

dialog_event_collator_unit_test_CXXFLAGS = \
	${common_cxx_flags} \
	-fno-gcse -O0 -fno-implement-inlines -fno-default-inline
	
dialog_event_collator_unit_test_SOURCES = \
	unit_test/TestSuite.cpp \
        unit_test/TestDialogCollator.cpp \
	unit_test/TestDialogInfo.cpp
	
dialog_event_collator_unit_test_LDADD = \
	${libdialogEventCollator_la_LIBADD} \
	libdialogEventCollator.la

DISTCLEANFILES = Makefile.in
