
## All tests under this GNU variable should run relatively quickly
## and of course require no setup
# for performance numbers, add to TESTS: UtlListPerformance UtlHashMapPerformance
TESTS = testsuite

check_PROGRAMS = testsuite

testsuite_LDADD = \
    $(top_builddir)/src/libsipXregistry.la \
	@SIPXUNIT_LIBS@ \
	@SIPXCOMMSERVER_LIBS@ \
	@OSSCORE_LIBS@ \
	@OSS_CORE_DEP_LIBS@ \
	-lmongoclient \
	-ltcmalloc \
        -lpthread  \
        -lstdc++ \
        -lboost_system-mt \
        -lboost_thread-mt \
        -lboost_filesystem-mt \
        -lboost_program_options-mt

testsuite_LDFLAGS = \
	-rdynamic

testsuite_CXXFLAGS = \
	-I$(top_srcdir)/include \
	@PCRE_CFLAGS@ \
	@CPPUNIT_CFLAGS@ \
	@SSL_CXXFLAGS@ \
	-DTEST_DIR=\"@abs_builddir@\" \
	-fno-gcse -O0

testsuite_SOURCES = \
	SipBLAQueueEventTest.cpp \
	../SipBLAQueueEvent.cpp
