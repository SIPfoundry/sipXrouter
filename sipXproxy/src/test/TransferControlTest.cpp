//
// Copyright (C) 2007 Pingtel Corp., certain elements licensed under a Contributor Agreement.  
// Contributors retain copyright to elements licensed under a Contributor Agreement.
// Licensed to the User under the LGPL license.
// 
//
// $$
////////////////////////////////////////////////////////////////////////

#include "cppunit/extensions/HelperMacros.h"
#include "cppunit/TestCase.h"
#include "sipxunit/TestUtilities.h"
#include "utl/PluginHooks.h"
#include "net/SipMessage.h"
#include "sipXproxy/TransferControl.h"

class TransferControlTest : public CppUnit::TestCase
{
   CPPUNIT_TEST_SUITE(TransferControlTest);

   CPPUNIT_TEST(nonInvite);
   CPPUNIT_TEST(normalInvite);
   CPPUNIT_TEST(InviteWithReplaces);
   CPPUNIT_TEST(ReferWithReplaces);
   CPPUNIT_TEST(UnAuthenticatedRefer);
   CPPUNIT_TEST(AuthenticatedRefer);

   CPPUNIT_TEST_SUITE_END();

public:

   static TransferControl* xferctl;


   /* ****************************************************************
    * Note: all tests pass NULL for the SipRouter* sipRouter parameter.
    *       Since the TransferControl plugin does not use that
    *       parameter, this should not cause any problem.
    *       Should that change, see CallerAliasTest for how to set
    *       up a SipRouter instance for a test.
    * **************************************************************** */

   // Test that an in-dialog request without a replaces header is not affected
   void nonInvite()
      {
         UtlString identity; // no authenticated identity
         Url requestUri("sip:someone@somewhere");

         const char* message =
            "INFO sip:someone@somewhere SIP/2.0\r\n"
            "Via: SIP/2.0/TCP 10.1.1.3:33855\r\n"
            "To: sip:someone@somewhere;tag=totag\r\n"
            "From: Caller <sip:caller@example.org>; tag=30543f3483e1cb11ecb40866edd3295b\r\n"
            "Call-Id: f88dfabce84b6a2787ef024a7dbe8749\r\n"
            "Cseq: 5 INFO\r\n"
            "Max-Forwards: 20\r\n"
            "Contact: caller@127.0.0.1\r\n"
            "Content-Length: 0\r\n"
            "\r\n";
         SipMessage testMsg(message, strlen(message));

         UtlSList noRemovedRoutes;
         UtlString myRouteName("myhost.example.com");
         RouteState routeState( testMsg, noRemovedRoutes, myRouteName );

         const char unmodifiedRejectReason[] = "unmodified";
         UtlString rejectReason(unmodifiedRejectReason);
         
         UtlString method("INFO");
         AuthPlugin::AuthResult priorResult = AuthPlugin::CONTINUE;
         
         CPPUNIT_ASSERT(AuthPlugin::CONTINUE
                        == xferctl->authorizeAndModify(NULL,
                                                       identity,
                                                       requestUri,
                                                       routeState,
                                                       method,
                                                       priorResult,
                                                       testMsg,
                                                       rejectReason
                                                       ));
         ASSERT_STR_EQUAL(unmodifiedRejectReason, rejectReason.data());

      }

   // Test that a dialog-forming INVITE without a replaces header is not affected
   void normalInvite()
      {
         UtlString identity; // no authenticated identity
         Url requestUri("sip:someone@somewhere");

         const char* message =
            "INVITE sip:someone@somewhere SIP/2.0\r\n"
            "Via: SIP/2.0/TCP 10.1.1.3:33855\r\n"
            "To: sip:someone@somewhere\r\n"
            "From: Caller <sip:caller@example.org>; tag=30543f3483e1cb11ecb40866edd3295b\r\n"
            "Call-Id: f88dfabce84b6a2787ef024a7dbe8749\r\n"
            "Cseq: 1 INVITE\r\n"
            "Max-Forwards: 20\r\n"
            "Contact: caller@127.0.0.1\r\n"
            "Content-Length: 0\r\n"
            "\r\n";
         SipMessage testMsg(message, strlen(message));

         UtlSList noRemovedRoutes;
         UtlString myRouteName("myhost.example.com");
         RouteState routeState( testMsg, noRemovedRoutes, myRouteName );

         const char unmodifiedRejectReason[] = "unmodified";
         UtlString rejectReason(unmodifiedRejectReason);
         
         UtlString method("INVITE");
         AuthPlugin::AuthResult priorResult = AuthPlugin::CONTINUE;
         
         CPPUNIT_ASSERT(AuthPlugin::CONTINUE
                        == xferctl->authorizeAndModify(NULL,
                                                       identity,
                                                       requestUri,
                                                       routeState,
                                                       method,
                                                       priorResult,
                                                       testMsg,
                                                       rejectReason
                                                       ));
         ASSERT_STR_EQUAL(unmodifiedRejectReason, rejectReason.data());
      }



   // Test that an out-of-dialog INVITE with a Replaces header is allowed
   void InviteWithReplaces()
      {
         UtlString identity; // no authenticated identity
         Url requestUri("sip:someone@somewhere");

         const char* message =
            "INVITE sip:someone@somewhere SIP/2.0\r\n"
            "Replaces: valid@callid;to-tag=totagvalue;from-tag=fromtagvalue\r\n"
            "Via: SIP/2.0/TCP 10.1.1.3:33855\r\n"
            "To: sip:someone@somewhere\r\n"
            "From: Caller <sip:caller@example.org>; tag=30543f3483e1cb11ecb40866edd3295b\r\n"
            "Call-Id: f88dfabce84b6a2787ef024a7dbe8749\r\n"
            "Cseq: 1 INVITE\r\n"
            "Max-Forwards: 20\r\n"
            "Contact: caller@127.0.0.1\r\n"
            "Content-Length: 0\r\n"
            "\r\n";
         SipMessage testMsg(message, strlen(message));

         UtlSList noRemovedRoutes;
         UtlString myRouteName("myhost.example.com");
         RouteState routeState( testMsg, noRemovedRoutes, myRouteName );

         const char unmodifiedRejectReason[] = "unmodified";
         UtlString rejectReason(unmodifiedRejectReason);
         
         UtlString method("INVITE");
         AuthPlugin::AuthResult priorResult = AuthPlugin::CONTINUE;
         
         CPPUNIT_ASSERT(AuthPlugin::ALLOW
                        == xferctl->authorizeAndModify(NULL,
                                                       identity,
                                                       requestUri,
                                                       routeState,
                                                       method,
                                                       priorResult,
                                                       testMsg,
                                                       rejectReason
                                                       ));
         ASSERT_STR_EQUAL(unmodifiedRejectReason, rejectReason.data());
      }

   // Test that a REFER with Replaces is allowed
   void ReferWithReplaces()
      {
         UtlString identity; // no authenticated identity
         Url requestUri("sip:someone@somewhere");

         const char* message =
            "REFER sip:someone@somewhere SIP/2.0\r\n"
            "Refer-To: other@elsewhere?replaces=valid@callid;to-tag=totagvalue;from-tag=fromtagvalue\r\n"
            "Via: SIP/2.0/TCP 10.1.1.3:33855\r\n"
            "To: sip:someone@somewhere\r\n"
            "From: Caller <sip:caller@example.org>; tag=30543f3483e1cb11ecb40866edd3295b\r\n"
            "Call-Id: f88dfabce84b6a2787ef024a7dbe8749\r\n"
            "Cseq: 1 INVITE\r\n"
            "Max-Forwards: 20\r\n"
            "Contact: caller@127.0.0.1\r\n"
            "Content-Length: 0\r\n"
            "\r\n";
         SipMessage testMsg(message, strlen(message));

         UtlSList noRemovedRoutes;
         UtlString myRouteName("myhost.example.com");
         RouteState routeState( testMsg, noRemovedRoutes, myRouteName );

         const char unmodifiedRejectReason[] = "unmodified";
         UtlString rejectReason(unmodifiedRejectReason);
         
         UtlString method("REFER");
         AuthPlugin::AuthResult priorResult = AuthPlugin::CONTINUE;
         
         CPPUNIT_ASSERT(AuthPlugin::ALLOW
                        == xferctl->authorizeAndModify(NULL,
                                                       identity,
                                                       requestUri,
                                                       routeState,
                                                       method,
                                                       priorResult,
                                                       testMsg,
                                                       rejectReason
                                                       ));
         ASSERT_STR_EQUAL(unmodifiedRejectReason, rejectReason.data());
      }


   // Test that an unauthenticated REFER without Replaces is challenged
   void UnAuthenticatedRefer()
      {
         UtlString identity; // no authenticated identity
         Url requestUri("sip:someone@somewhere");

         const char* message =
            "REFER sip:someone@somewhere SIP/2.0\r\n"
            "Refer-To: other@elsewhere\r\n"
            "Via: SIP/2.0/TCP 10.1.1.3:33855\r\n"
            "To: sip:someone@somewhere\r\n"
            "From: Caller <sip:caller@example.org>; tag=30543f3483e1cb11ecb40866edd3295b\r\n"
            "Call-Id: f88dfabce84b6a2787ef024a7dbe8749\r\n"
            "Cseq: 1 INVITE\r\n"
            "Max-Forwards: 20\r\n"
            "Contact: caller@127.0.0.1\r\n"
            "Content-Length: 0\r\n"
            "\r\n";
         SipMessage testMsg(message, strlen(message));

         UtlSList noRemovedRoutes;
         UtlString myRouteName("myhost.example.com");
         RouteState routeState( testMsg, noRemovedRoutes, myRouteName );

         const char unmodifiedRejectReason[] = "unmodified";
         UtlString rejectReason(unmodifiedRejectReason);
         
         UtlString method("REFER");
         AuthPlugin::AuthResult priorResult = AuthPlugin::CONTINUE;
         
         CPPUNIT_ASSERT(AuthPlugin::DENY
                        == xferctl->authorizeAndModify(NULL,
                                                       identity,
                                                       requestUri,
                                                       routeState,
                                                       method,
                                                       priorResult,
                                                       testMsg,
                                                       rejectReason
                                                       ));
         ASSERT_STR_EQUAL(unmodifiedRejectReason, rejectReason.data());
      }

   
   // Test that an authenticated REFER without Replaces is allowed and annotated
   void AuthenticatedRefer()
      {
         UtlString identity("controller@domain"); // an authenticated identity
         Url requestUri("sip:someone@somewhere");

         const char* message =
            "REFER sip:someone@somewhere SIP/2.0\r\n"
            "Refer-To: other@elsewhere\r\n"
            "Via: SIP/2.0/TCP 10.1.1.3:33855\r\n"
            "To: sip:someone@somewhere\r\n"
            "From: Caller <sip:caller@example.org>; tag=30543f3483e1cb11ecb40866edd3295b\r\n"
            "Call-Id: f88dfabce84b6a2787ef024a7dbe8749\r\n"
            "Cseq: 1 INVITE\r\n"
            "Max-Forwards: 20\r\n"
            "Contact: caller@127.0.0.1\r\n"
            "Content-Length: 0\r\n"
            "\r\n";
         SipMessage testMsg(message, strlen(message));

         UtlSList noRemovedRoutes;
         UtlString myRouteName("myhost.example.com");
         RouteState routeState( testMsg, noRemovedRoutes, myRouteName );

         const char unmodifiedRejectReason[] = "unmodified";
         UtlString rejectReason(unmodifiedRejectReason);
         
         UtlString method("REFER");
         AuthPlugin::AuthResult priorResult = AuthPlugin::CONTINUE;
         
         CPPUNIT_ASSERT(AuthPlugin::CONTINUE
                        == xferctl->authorizeAndModify(NULL,
                                                       identity,
                                                       requestUri,
                                                       routeState,
                                                       method,
                                                       priorResult,
                                                       testMsg,
                                                       rejectReason
                                                       ));
         ASSERT_STR_EQUAL(unmodifiedRejectReason, rejectReason.data());

         UtlString modifiedReferToStr;
         CPPUNIT_ASSERT(testMsg.getReferToField(modifiedReferToStr));

         Url modifiedReferTo(modifiedReferToStr);
         CPPUNIT_ASSERT(Url::SipUrlScheme == modifiedReferTo.getScheme());

         UtlString transferIdentityValue;
         CPPUNIT_ASSERT(modifiedReferTo.getHeaderParameter("X-Sipx-Authidentity",
                                                           transferIdentityValue));
         Url transferIdentityUrl(transferIdentityValue);
         CPPUNIT_ASSERT(Url::SipUrlScheme == transferIdentityUrl.getScheme());

         UtlString transferIdentity;
         transferIdentityUrl.getIdentity(transferIdentity);
         ASSERT_STR_EQUAL("controller@domain", transferIdentity.data());
      }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TransferControlTest);

TransferControl* TransferControlTest::xferctl = dynamic_cast<TransferControl*>(getAuthPlugin("xfer"));
