//
// Copyright (C) 2007 Pingtel Corp., certain elements licensed under a Contributor Agreement.
// Contributors retain copyright to elements licensed under a Contributor Agreement.
// Licensed to the User under the LGPL license.
//
//
// $$
////////////////////////////////////////////////////////////////////////

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>
#include <sipxunit/TestUtilities.h>

#include <os/OsDefs.h>
#include <os/OsConfigDb.h>
#include <os/OsLogger.h>

#include <utl/PluginHooks.h>

#include <net/SipMessage.h>

#include <registry/RegisterPlugin.h>
#include "../SipBLAQueueEvent.h"

#define PLUGIN_LIB_DIR TEST_DIR "/../.libs/"

/**
 * Unittest for SipMessage
 */
class SipBLAQueueEventTest : public CppUnit::TestCase
{
   CPPUNIT_TEST_SUITE(SipBLAQueueEventTest);
   CPPUNIT_TEST(testBLAQueueEvent);
   CPPUNIT_TEST_SUITE_END();

public:

   void testBLAQueueEvent()
      {
         /*Os::Logger::instance().enableConsoleOutput();

         PluginHooks regPlugin( "getRegisterPlugin", "SIP_REGISTER" );

         OsConfigDb configuration;
         configuration.set("SIP_REGISTER_HOOK_LIBRARY.BLA",
                           PLUGIN_LIB_DIR "libRegistrarBlaQueueEvent.so" );
         configuration.set("SIP_REGISTER.BLA.REDIS.ADDRESS", "127.0.0.1");
         configuration.set("SIP_REGISTER.BLA.REDIS.CHANNEL", "SIPX.BLA.REGISTER");

         regPlugin.readConfig(configuration);

         PluginIterator getPlugin(regPlugin);
         SipBLAQueueEvent* plugin
            = (SipBLAQueueEvent*)getPlugin.next();

         CPPUNIT_ASSERT(regPlugin.entries() > 0);
         CPPUNIT_ASSERT(plugin);*/

         SipBLAQueueEvent* plugin = (SipBLAQueueEvent*)getRegisterPlugin("BLA");
         OsConfigDb configuration;
         configuration.set("SIP_REGISTER_HOOK_LIBRARY.BLA",
                           PLUGIN_LIB_DIR "libRegistrarBlaQueueEvent.so" );
         configuration.set("SIP_REGISTER.BLA.REDIS.ADDRESS", "127.0.0.1");
         configuration.set("SIP_REGISTER.BLA.REDIS.CHANNEL", "SIPX.BLA.REGISTER");

         regPlugin.readConfig(configuration);

         CPPUNIT_ASSERT(plugin);

         const char DumbMessage[] =
            "REGISTER sip:sipx.local SIP/2.0\r\n"
            "Via: SIP/2.0/TCP sipx.local:33855;branch=z9hG4bK-10cb6f9378a12d4218e10ef4dc78ea3d\r\n"
            "To: Sip User <sip:sipuser@pingtel.org>;tag=totag\r\n"
            "From: Sip Send <sip:sipsend@pingtel.org>; tag=30543f3483e1cb11ecb40866edd3295b\r\n"
            "Call-ID: f88dfabce84b6a2787ef024a7dbe8749\r\n"
            "Cseq: 1 REGISTER\r\n"
            "Max-Forwards: 20\r\n"
            "User-Agent: DUMB/0.01\r\n"
            "Contact: sip:me@127.0.0.1\r\n"
            "Expires: 1000\r\n"
            "Date: Fri, 16 Jul 2004 02:16:15 GMT\r\n"
            "Content-Length: 0\r\n"
            "\r\n";
         SipMessage testDumbReg( DumbMessage, strlen( DumbMessage ) );
         plugin->takeAction(testDumbReg, 3600, NULL);

         const char NotDumbMessage[] =
            "REGISTER sip:sipx.local SIP/2.0\r\n"
            "Via: SIP/2.0/TCP sipx.local:33855;branch=z9hG4bK-10cb6f9378a12d4218e10ef4dc78ea3d\r\n"
            "To: sip:sipx.local\r\n"
            "From: Sip Send <sip:sipsend@pingtel.org>; tag=30543f3483e1cb11ecb40866edd3295b\r\n"
            "Call-ID: f88dfabce84b6a2787ef024a7dbe8749\r\n"
            "Cseq: 1 REGISTER\r\n"
            "Max-Forwards: 20\r\n"
            "User-Agent: NOTDUMB/0.01\r\n"
            "Contact: sip:me@127.0.0.1\r\n"
            "Expires: 1000\r\n"
            "Date: Fri, 16 Jul 2004 02:16:15 GMT\r\n"
            "Content-Length: 0\r\n"
            "\r\n";
         SipMessage testNotDumbReg( NotDumbMessage, strlen( NotDumbMessage ) );

         plugin->takeAction(NotDumbMessage, 3600, NULL);

         const char DV1Message[] =
            "REGISTER sip:sipx.local SIP/2.0\r\n"
            "Via: SIP/2.0/TCP sipx.local:33855;branch=z9hG4bK-10cb6f9378a12d4218e10ef4dc78ea3d\r\n"
            "To: sip:sipx.local\r\n"
            "From: Sip Send <sip:sipsend@pingtel.org>; tag=30543f3483e1cb11ecb40866edd3295b\r\n"
            "Call-ID: f88dfabce84b6a2787ef024a7dbe8749\r\n"
            "Cseq: 1 REGISTER\r\n"
            "Max-Forwards: 20\r\n"
            "User-Agent: SOMEDV/1.01\r\n"
            "Contact: sip:me@127.0.0.1\r\n"
            "Expires: 1000\r\n"
            "Date: Fri, 16 Jul 2004 02:16:15 GMT\r\n"
            "Content-Length: 0\r\n"
            "\r\n";
         SipMessage testDV1Reg( DV1Message, strlen( DV1Message ) );

         plugin->takeAction(DV1Message, 3600, NULL);

      };

};

CPPUNIT_TEST_SUITE_REGISTRATION(SipBLAQueueEventTest);
