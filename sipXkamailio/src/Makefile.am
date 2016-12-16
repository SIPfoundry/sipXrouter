INCLUDES = -I$(top_srcdir)/include

lib_LTLIBRARIES = libdialogEventCollator.la libRedisMessageQueue.la

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
	@OSSCORE_LIBS@ \
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

libRedisMessageQueue_la_LDFLAGS = \
	-shared \
	-version-info ${version_Current}:${version_Revision}:${version_Age}

libRedisMessageQueue_la_LIBADD = \
	@OSSCORE_LIBS@ \
	-ltcmalloc \
        -lpthread  \
        -lstdc++ \
        -lboost_system-mt \
        -lboost_thread-mt \
        -lboost_filesystem-mt \
        -lboost_program_options-mt \
        -levent \
        -lhiredis

# this is helpful if you're debugging something with lots of inlines (like the UtlLink code)
# 		-fno-gcse -O0 -fno-implement-inlines -fno-default-inline
libRedisMessageQueue_la_CXXFLAGS = \
	@PCRE_CFLAGS@ \
	@SSL_CXXFLAGS@ \
	-DSIPX_TMPDIR=\"@SIPX_TMPDIR@\" \
	-DSIPX_CONFDIR=\"@SIPX_CONFDIR@\" \
	-fno-gcse -O0 -fno-implement-inlines -fno-default-inline

libRedisMessageQueue_la_SOURCES =  \
    MessageQueue/MessageQueuePlugin.cpp \
    MessageQueue/RedisSubscribeQueue.cpp

bin_PROGRAMS = \
    dialog_event_collator_unit_test \
    redis_subscribe_queue_unit_test

dialog_event_collator_unit_test_CXXFLAGS = \
	${common_cxx_flags} \
	-fno-gcse -O0 -fno-implement-inlines -fno-default-inline
	
dialog_event_collator_unit_test_SOURCES = \
	unit_test/TestSuite.cpp \
    unit_test/TestDialogCollator.cpp \
	unit_test/TestDialogInfo.cpp
	
dialog_event_collator_unit_test_LDADD = \
	${libdialogEventCollator_la_LIBADD} \
	-lgtest \
	libdialogEventCollator.la

redis_subscribe_queue_unit_test_CXXFLAGS = \
	${common_cxx_flags} \
	-fno-gcse -O0 -fno-implement-inlines -fno-default-inline
	
redis_subscribe_queue_unit_test_SOURCES = \
	unit_test/TestSuite.cpp \
    unit_test/TestRedisSubscribeQueue.cpp
	
redis_subscribe_queue_unit_test_LDADD = \
	${libRedisMessageQueue_la_LIBADD} \
	-lgtest \
	libRedisMessageQueue.la	

DISTCLEANFILES = Makefile.in
