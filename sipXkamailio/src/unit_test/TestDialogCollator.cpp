#include "gtest/gtest.h"
#include <OSS/OSS.h>
#include <OSS/UTL/Logger.h>
#include <OSS/UTL/Exception.h>

#include <iostream>
#include <sstream>
#include <vector>

#include "DialogEventCollator/DialogCollator.h"

using namespace SIPX::Kamailio::Plugin;

#include <iostream>

const std::string user = "5001@test.collator.inc";

const std::string earlyForkA = "<?xml version=\"1.0\" ?>\n"
    "<dialog-info xmlns=\"urn:ietf:params:xml:ns:dialog-info\" version=\"00000000000\" state=\"full\" entity=\"sip:5001@test.collator.inc;user=phone\">\n"
    "    <dialog id=\"0001@192.168.0.1\" call-id=\"0001@192.168.0.1\" local-tag=\"LOCALTAG-FORK-1\" remote-tag=\"REMOTETAG-FORK-1\" direction=\"recipient\">\n"
    "        <state>early</state>\n"
    "        <remote>\n"
    "            <identity>sip:5002@test.collator.inc</identity>\n"
    "            <target uri=\"sip:5002@192.168.0.1;transport=tcp\" />\n"
    "        </remote>\n"
    "        <local>\n"
    "            <identity>sip:5001@test.collator.inc;user=phone</identity>\n"
    "            <target uri=\"sip:5001@192.168.0.2;transport=tcp\" />\n"
    "        </local>\n"
    "    </dialog>\n"
    "</dialog-info>\n";

const std::string earlyForkB = "<?xml version=\"1.0\" ?>\n"
    "<dialog-info xmlns=\"urn:ietf:params:xml:ns:dialog-info\" version=\"00000000000\" state=\"full\" entity=\"sip:5001@test.collator.inc;user=phone\">\n"
    "    <dialog id=\"0001@192.168.0.1\" call-id=\"0001@192.168.0.1\" local-tag=\"LOCALTAG-FORK-2\" remote-tag=\"REMOTETAG-FORK-1\" direction=\"recipient\">\n"
    "        <state>early</state>\n"
    "        <remote>\n"
    "            <identity>sip:5002@test.collator.inc</identity>\n"
    "            <target uri=\"sip:5002@192.168.0.1;transport=tcp\" />\n"
    "        </remote>\n"
    "        <local>\n"
    "            <identity>sip:5001@test.collator.inc;user=phone</identity>\n"
    "            <target uri=\"sip:5001@192.168.0.3;transport=tcp\" />\n"
    "        </local>\n"
    "    </dialog>\n"
    "</dialog-info>\n";

const std::string earlyCallA = "<?xml version=\"1.0\" ?>\n"
    "<dialog-info xmlns=\"urn:ietf:params:xml:ns:dialog-info\" version=\"00000000000\" state=\"full\" entity=\"sip:5001@test.collator.inc;user=phone\">\n"
    "    <dialog id=\"0002@192.168.0.1\" call-id=\"0002@192.168.0.1\" local-tag=\"LOCALTAG-CALL-1\" remote-tag=\"REMOTETAG-CALL-1\" direction=\"recipient\">\n"
    "        <state>early</state>\n"
    "        <remote>\n"
    "            <identity>sip:5002@test.collator.inc</identity>\n"
    "            <target uri=\"sip:5002@192.168.0.1;transport=tcp\" />\n"
    "        </remote>\n"
    "        <local>\n"
    "            <identity>sip:5001@test.collator.inc;user=phone</identity>\n"
    "            <target uri=\"sip:5001@192.168.0.3;transport=tcp\" />\n"
    "        </local>\n"
    "    </dialog>\n"
    "</dialog-info>\n";    

const std::string confirmedForkA = "<?xml version=\"1.0\" ?>\n"
    "<dialog-info xmlns=\"urn:ietf:params:xml:ns:dialog-info\" version=\"00000000000\" state=\"full\" entity=\"sip:5001@test.collator.inc;user=phone\">\n"
    "    <dialog id=\"0001@192.168.0.1\" call-id=\"0001@192.168.0.1\" local-tag=\"LOCALTAG-FORK-1\" remote-tag=\"REMOTETAG-FORK-1\" direction=\"recipient\">\n"
    "        <state>confirmed</state>\n"
    "        <remote>\n"
    "            <identity>sip:5002@test.collator.inc</identity>\n"
    "            <target uri=\"sip:5002@192.168.0.1;transport=tcp\" />\n"
    "        </remote>\n"
    "        <local>\n"
    "            <identity>sip:5001@test.collator.inc;user=phone</identity>\n"
    "            <target uri=\"sip:5001@test.collator.inc;user=phone\" />\n"
    "        </local>\n"
    "    </dialog>\n"
    "</dialog-info>\n";   

const std::string confirmedForkB = "<?xml version=\"1.0\" ?>\n"
    "<dialog-info xmlns=\"urn:ietf:params:xml:ns:dialog-info\" version=\"00000000000\" state=\"full\" entity=\"sip:5001@test.collator.inc;user=phone\">\n"
    "    <dialog id=\"0001@192.168.0.1\" call-id=\"0001@192.168.0.1\" local-tag=\"LOCALTAG-FORK-2\" remote-tag=\"REMOTETAG-FORK-1\" direction=\"recipient\">\n"
    "        <state>confirmed</state>\n"
    "        <remote>\n"
    "            <identity>sip:5002@test.collator.inc</identity>\n"
    "            <target uri=\"sip:5002@192.168.0.1;transport=tcp\" />\n"
    "        </remote>\n"
    "        <local>\n"
    "            <identity>sip:5001@test.collator.inc;user=phone</identity>\n"
    "            <target uri=\"sip:5001@test.collator.inc;user=phone\" />\n"
    "        </local>\n"
    "    </dialog>\n"
    "</dialog-info>\n";

const std::string confirmedCallA = "<?xml version=\"1.0\" ?>\n"
    "<dialog-info xmlns=\"urn:ietf:params:xml:ns:dialog-info\" version=\"00000000000\" state=\"full\" entity=\"sip:5001@test.collator.inc;user=phone\">\n"
    "    <dialog id=\"0002@192.168.0.1\" call-id=\"0002@192.168.0.1\" local-tag=\"LOCALTAG-CALL-1\" remote-tag=\"REMOTETAG-CALL-1\" direction=\"recipient\">\n"
    "        <state>confirmed</state>\n"
    "        <remote>\n"
    "            <identity>sip:5002@test.collator.inc</identity>\n"
    "            <target uri=\"sip:5002@192.168.0.1;transport=tcp\" />\n"
    "        </remote>\n"
    "        <local>\n"
    "            <identity>sip:5001@test.collator.inc;user=phone</identity>\n"
    "            <target uri=\"sip:5001@192.168.0.3;transport=tcp\" />\n"
    "        </local>\n"
    "    </dialog>\n"
    "</dialog-info>\n";     

const std::string terminatedForkA = "<?xml version=\"1.0\" ?>\n"
    "<dialog-info xmlns=\"urn:ietf:params:xml:ns:dialog-info\" version=\"00000000000\" state=\"full\" entity=\"sip:5001@test.collator.inc;user=phone\">\n"
    "    <dialog id=\"0001@192.168.0.1\" call-id=\"0001@192.168.0.1\" local-tag=\"LOCALTAG-FORK-1\" remote-tag=\"REMOTETAG-FORK-1\" direction=\"recipient\">\n"
    "        <state>terminated</state>\n"
    "        <remote>\n"
    "            <identity>sip:5002@test.collator.inc</identity>\n"
    "            <target uri=\"sip:5002@192.168.0.1;transport=tcp\" />\n"
    "        </remote>\n"
    "        <local>\n"
    "            <identity>sip:5001@test.collator.inc;user=phone</identity>\n"
    "            <target uri=\"sip:5001@test.collator.inc;user=phone\" />\n"
    "        </local>\n"
    "    </dialog>\n"
    "</dialog-info>\n";    

const std::string terminatedForkB = "<?xml version=\"1.0\" ?>\n"
    "<dialog-info xmlns=\"urn:ietf:params:xml:ns:dialog-info\" version=\"00000000000\" state=\"full\" entity=\"sip:5001@test.collator.inc;user=phone\">\n"
    "    <dialog id=\"0001@192.168.0.1\" call-id=\"0001@192.168.0.1\" local-tag=\"LOCALTAG-FORK-2\" remote-tag=\"REMOTETAG-FORK-1\" direction=\"recipient\">\n"
    "        <state>terminated</state>\n"
    "        <remote>\n"
    "            <identity>sip:5002@test.collator.inc</identity>\n"
    "            <target uri=\"sip:5002@192.168.0.1;transport=tcp\" />\n"
    "        </remote>\n"
    "        <local>\n"
    "            <identity>sip:5001@test.collator.inc;user=phone</identity>\n"
    "            <target uri=\"sip:5001@test.collator.inc;user=phone\" />\n"
    "        </local>\n"
    "    </dialog>\n"
    "</dialog-info>\n";

const std::string terminatedCallA = "<?xml version=\"1.0\" ?>\n"
    "<dialog-info xmlns=\"urn:ietf:params:xml:ns:dialog-info\" version=\"00000000000\" state=\"full\" entity=\"sip:5001@test.collator.inc;user=phone\">\n"
    "    <dialog id=\"0002@192.168.0.1\" call-id=\"0002@192.168.0.1\" local-tag=\"LOCALTAG-CALL-1\" remote-tag=\"REMOTETAG-CALL-1\" direction=\"recipient\">\n"
    "        <state>terminated</state>\n"
    "        <remote>\n"
    "            <identity>sip:5002@test.collator.inc</identity>\n"
    "            <target uri=\"sip:5002@192.168.0.1;transport=tcp\" />\n"
    "        </remote>\n"
    "        <local>\n"
    "            <identity>sip:5001@test.collator.inc;user=phone</identity>\n"
    "            <target uri=\"sip:5001@192.168.0.3;transport=tcp\" />\n"
    "        </local>\n"
    "    </dialog>\n"
    "</dialog-info>\n";        


const std::string terminatedForkTrying = "<?xml version=\"1.0\" ?>\n"
    "<dialog-info xmlns=\"urn:ietf:params:xml:ns:dialog-info\" version=\"00000000000\" state=\"full\" entity=\"sip:5001@test.collator.inc;user=phone\">\n"
    "    <dialog id=\"0001@192.168.0.1\" call-id=\"0001@192.168.0.1\" local-tag=\"TRYINGTAG-FORK-1\" remote-tag=\"REMOTETAG-FORK-1\" direction=\"recipient\">\n"
    "        <state>terminated</state>\n"
    "        <remote>\n"
    "            <identity>sip:5002@test.collator.inc</identity>"
    "            <target uri=\"sip:5002@192.168.0.1;transport=tcp\" />"
    "        </remote>\n"
    "        <local>\n"
    "            <identity>sip:5001@test.collator.inc;user=phone</identity>\n"
    "            <target uri=\"sip:5001@test.collator.inc;user=phone\" />\n"
    "        </local>\n"
    "    </dialog>\n"
    "</dialog-info>\n";

const std::string terminatedCallTrying = "<?xml version=\"1.0\" ?>\n"
    "<dialog-info xmlns=\"urn:ietf:params:xml:ns:dialog-info\" version=\"00000000000\" state=\"full\" entity=\"sip:5001@test.collator.inc;user=phone\">\n"
    "    <dialog id=\"0002@192.168.0.1\" call-id=\"0002@192.168.0.1\" local-tag=\"TRYINGTAG-CALL-1\" remote-tag=\"REMOTETAG-CALL-1\" direction=\"recipient\">\n"
    "        <state>terminated</state>\n"
    "        <remote>\n"
    "            <identity>sip:5002@test.collator.inc</identity>\n"
    "            <target uri=\"sip:5002@192.168.0.1;transport=tcp\" />\n"
    "        </remote>\n"
    "        <local>\n"
    "            <identity>sip:5001@test.collator.inc;user=phone</identity>\n"
    "            <target uri=\"sip:5001@192.168.0.3;transport=tcp\" />\n"
    "        </local>\n"
    "    </dialog>\n"
    "</dialog-info>\n";        

const std::string earlyAndTerminatedForkA = "<?xml version=\"1.0\" ?>\n"
    "<dialog-info xmlns=\"urn:ietf:params:xml:ns:dialog-info\" version=\"00000000000\" state=\"full\" entity=\"sip:5001@test.collator.inc;user=phone\">\n"
    "    <dialog id=\"0001@192.168.0.1\" call-id=\"0001@192.168.0.1\" local-tag=\"LOCALTAG-FORK-1\" remote-tag=\"REMOTETAG-FORK-1\" direction=\"recipient\">\n"
    "        <state>early</state>\n"
    "        <remote>\n"
    "            <identity>sip:5002@test.collator.inc</identity>\n"
    "            <target uri=\"sip:5002@192.168.0.1;transport=tcp\" />\n"
    "        </remote>\n"
    "        <local>\n"
    "            <identity>sip:5001@test.collator.inc;user=phone</identity>\n"
    "            <target uri=\"sip:5001@192.168.0.2;transport=tcp\" />\n"
    "        </local>\n"
    "    </dialog>\n"    
    "    <dialog id=\"0002@192.168.0.1\" call-id=\"0002@192.168.0.1\" local-tag=\"LOCALTAG-CALL-1\" remote-tag=\"REMOTETAG-CALL-1\" direction=\"recipient\">\n"
    "        <state>terminated</state>\n"
    "        <remote>\n"
    "            <identity>sip:5002@test.collator.inc</identity>\n"
    "            <target uri=\"sip:5002@192.168.0.1;transport=tcp\" />\n"
    "        </remote>\n"
    "        <local>\n"
    "            <identity>sip:5001@test.collator.inc;user=phone</identity>\n"
    "            <target uri=\"sip:5001@192.168.0.3;transport=tcp\" />\n"
    "        </local>\n"
    "    </dialog>\n"
    "</dialog-info>\n";         

TEST(DialogCollatorTest, test_aggregate_finalize_kamailio_payload_simple_call)
{
    DialogCollatorAndAggregator dialogCA;
    dialogCA.flushDb(user);

    std::list<std::string> payloads;

    payloads.push_back(terminatedCallTrying);
    payloads.push_back(terminatedCallTrying);
    payloads.push_back(earlyCallA);
    payloads.push_back(terminatedCallTrying);
    payloads.push_back(earlyCallA);
    payloads.push_back(earlyCallA);
    payloads.push_back(terminatedCallTrying);
    payloads.push_back(earlyCallA);
    payloads.push_back(earlyCallA);
    payloads.push_back(terminatedCallTrying);
    payloads.push_back(earlyCallA);
    payloads.push_back(earlyCallA);

    {
        std::string xml;
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, payloads, xml));
        EXPECT_STREQ(earlyCallA.c_str(), xml.c_str());
    }
    
    payloads.clear();
    payloads.push_back(terminatedCallTrying);
    payloads.push_back(earlyCallA);
    payloads.push_back(earlyCallA);
    payloads.push_back(confirmedCallA);
    payloads.push_back(terminatedCallTrying);
    payloads.push_back(earlyCallA);
    payloads.push_back(earlyCallA);
    payloads.push_back(confirmedCallA);
    payloads.push_back(confirmedCallA);
    payloads.push_back(terminatedCallTrying);
    payloads.push_back(earlyCallA);
    payloads.push_back(earlyCallA);
    payloads.push_back(confirmedCallA);
    payloads.push_back(confirmedCallA);

    {
        std::string xml;
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, payloads, xml));
        EXPECT_STREQ(confirmedCallA.c_str(), xml.c_str());
    }

    payloads.clear();
    payloads.push_back(terminatedCallTrying);
    payloads.push_back(earlyCallA);
    payloads.push_back(earlyCallA);
    payloads.push_back(confirmedCallA);
    payloads.push_back(confirmedCallA);
    payloads.push_back(terminatedCallA);
    payloads.push_back(terminatedCallTrying);
    payloads.push_back(earlyCallA);
    payloads.push_back(earlyCallA);
    payloads.push_back(confirmedCallA);
    payloads.push_back(confirmedCallA);
    payloads.push_back(terminatedCallA);
    payloads.push_back(terminatedCallA);


    {
        std::string xml;
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, payloads, xml));
        EXPECT_STREQ(terminatedCallA.c_str(), xml.c_str());
    }

    dialogCA.flushDb(user);
}

TEST(DialogCollatorTest, test_aggregate_finalize_kamailio_payload_simple_call_recover)
{
    DialogCollatorAndAggregator dialogCA;
    dialogCA.flushDb(user);

    std::list<std::string> payloads;

    // Call with unterminated call

    payloads.clear();
    payloads.push_back(terminatedCallTrying);
    payloads.push_back(earlyCallA);
    payloads.push_back(earlyCallA);
    payloads.push_back(confirmedCallA);
    payloads.push_back(terminatedCallTrying);
    payloads.push_back(earlyCallA);
    payloads.push_back(earlyCallA);
    payloads.push_back(confirmedCallA);
    payloads.push_back(confirmedCallA);
    payloads.push_back(terminatedCallTrying);
    payloads.push_back(earlyCallA);
    payloads.push_back(earlyCallA);
    payloads.push_back(confirmedCallA);
    payloads.push_back(confirmedCallA);

    {
        std::string xml;
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, payloads, xml));
        EXPECT_STREQ(confirmedCallA.c_str(), xml.c_str());
    }

    //New call with new dialog
    payloads.clear();
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);

    {
        std::string xml;
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, payloads, xml));
        EXPECT_STREQ(earlyAndTerminatedForkA.c_str(), xml.c_str());
    }
    
    payloads.clear();
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(confirmedForkA);

    {
        std::string xml;
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, payloads, xml));
        EXPECT_STREQ(confirmedForkA.c_str(), xml.c_str());
    }

    payloads.clear();
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(terminatedForkA);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(terminatedForkA);
    payloads.push_back(terminatedForkA);


    {
        std::string xml;
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, payloads, xml));
        EXPECT_STREQ(terminatedForkA.c_str(), xml.c_str());
    }

    //dialogCA.flushDb(user);
}

TEST(DialogCollatorTest, test_aggregate_finalize_kamailio_payload_fork_call)
{
    DialogCollatorAndAggregator dialogCA;
    dialogCA.flushDb(user);

    std::list<std::string> payloads;

    payloads.push_back(terminatedForkTrying);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkB);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkB);
    payloads.push_back(earlyForkB);

    {
        std::string xml;
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, payloads, xml));
        EXPECT_STREQ(earlyForkB.c_str(), xml.c_str());
    }
    
    payloads.clear();
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkB);
    payloads.push_back(earlyForkB);
    payloads.push_back(confirmedForkA);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkB);
    payloads.push_back(earlyForkB);
    payloads.push_back(confirmedForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkB);
    payloads.push_back(earlyForkB);
    payloads.push_back(confirmedForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(terminatedForkB);

    {
        std::string xml;
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, payloads, xml));
        EXPECT_STREQ(confirmedForkA.c_str(), xml.c_str());
    }

    payloads.clear();
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkB);
    payloads.push_back(earlyForkB);
    payloads.push_back(confirmedForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(terminatedForkB);
    payloads.push_back(terminatedForkA);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkB);
    payloads.push_back(earlyForkB);
    payloads.push_back(confirmedForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(terminatedForkB);
    payloads.push_back(terminatedForkA);
    payloads.push_back(terminatedForkA);


    {
        std::string xml;
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, payloads, xml));
        EXPECT_STREQ(terminatedForkB.c_str(), xml.c_str());
    }

    dialogCA.flushDb(user);
}

TEST(DialogCollatorTest, test_aggregate_finalize_kamailio_payload_fork_call_missing_terminate_a)
{
    DialogCollatorAndAggregator dialogCA;
    dialogCA.flushDb(user);

    std::list<std::string> payloads;

    payloads.push_back(terminatedForkTrying);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkB);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkB);
    payloads.push_back(earlyForkB);

    {
        std::string xml;
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, payloads, xml));
        EXPECT_STREQ(earlyForkB.c_str(), xml.c_str());
    }
    
    payloads.clear();
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkB);
    payloads.push_back(earlyForkB);
    payloads.push_back(confirmedForkA);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkB);
    payloads.push_back(earlyForkB);
    payloads.push_back(confirmedForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkB);
    payloads.push_back(earlyForkB);
    payloads.push_back(confirmedForkA);
    payloads.push_back(confirmedForkA);

    {
        std::string xml;
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, payloads, xml));
        EXPECT_STREQ(confirmedForkA.c_str(), xml.c_str());
    }

    payloads.clear();
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkB);
    payloads.push_back(earlyForkB);
    payloads.push_back(confirmedForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(terminatedForkA);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkB);
    payloads.push_back(earlyForkB);
    payloads.push_back(confirmedForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(terminatedForkA);
    payloads.push_back(terminatedForkA);


    {
        std::string xml;
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, payloads, xml));
        EXPECT_STREQ(terminatedForkA.c_str(), xml.c_str());
    }

    dialogCA.flushDb(user);
}

TEST(DialogCollatorTest, test_aggregate_finalize_kamailio_payload_fork_call_missing_terminate_b)
{
    DialogCollatorAndAggregator dialogCA;
    dialogCA.flushDb(user);

    std::list<std::string> payloads;

    payloads.push_back(terminatedForkTrying);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkB);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkB);
    payloads.push_back(earlyForkB);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkB);
    payloads.push_back(earlyForkB);
    payloads.push_back(earlyForkA);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkB);
    payloads.push_back(earlyForkB);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);

    {
        std::string xml;
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, payloads, xml));
        EXPECT_STREQ(earlyForkA.c_str(), xml.c_str());
    }
    
    payloads.clear();
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkB);
    payloads.push_back(earlyForkB);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkB);
    payloads.push_back(earlyForkB);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkB);
    payloads.push_back(earlyForkB);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(confirmedForkA);

    {
        std::string xml;
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, payloads, xml));
        EXPECT_STREQ(confirmedForkA.c_str(), xml.c_str());
    }

    payloads.clear();
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkB);
    payloads.push_back(earlyForkB);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(terminatedForkA);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkB);
    payloads.push_back(earlyForkB);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(terminatedForkA);
    payloads.push_back(terminatedForkA);


    {
        std::string xml;
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, payloads, xml));
        EXPECT_STREQ(terminatedForkA.c_str(), xml.c_str());
    }

    dialogCA.flushDb(user);
}

TEST(DialogCollatorTest, test_aggregate_finalize_kamailio_payload_multiple_active_call)
{
    DialogCollatorAndAggregator dialogCA;
    dialogCA.flushDb(user);

    std::list<std::string> payloads;

    payloads.push_back(terminatedForkTrying);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);

    {
        std::string xml;
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, payloads, xml));
        EXPECT_STREQ(earlyForkA.c_str(), xml.c_str());
    }
    
    payloads.clear();
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(confirmedForkA);

    {
        std::string xml;
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, payloads, xml));
        EXPECT_STREQ(confirmedForkA.c_str(), xml.c_str());
    }

    payloads.clear();
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(confirmedForkB);
    payloads.push_back(confirmedForkB);

    {
        std::string xml;
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, payloads, xml));
        EXPECT_STREQ(confirmedForkB.c_str(), xml.c_str());
    }

    payloads.clear();
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(confirmedForkB);
    payloads.push_back(confirmedForkB);
    payloads.push_back(terminatedForkB);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(confirmedForkB);
    payloads.push_back(confirmedForkB);
    payloads.push_back(terminatedForkB);

    {
        std::string xml;
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, payloads, xml));
        EXPECT_STREQ(terminatedForkB.c_str(), xml.c_str());
    }

    dialogCA.flushDb(user);
}

TEST(DialogCollatorTest, test_aggregate_finalize_kamailio_payload_multiple_active_call_race_condition)
{
    DialogCollatorAndAggregator dialogCA;
    dialogCA.flushDb(user);

    std::list<std::string> payloads;

    payloads.push_back(terminatedForkTrying);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);

    {
        std::string xml;
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, payloads, xml));
        EXPECT_STREQ(earlyForkA.c_str(), xml.c_str());
    }

    payloads.clear();
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(confirmedForkB);
    payloads.push_back(confirmedForkB);
    payloads.push_back(confirmedForkA);
    payloads.push_back(confirmedForkA);

    {
        std::string xml;
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, payloads, xml));
        EXPECT_STREQ(confirmedForkB.c_str(), xml.c_str());
    }

    payloads.clear();
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(confirmedForkB);
    payloads.push_back(confirmedForkB);    
    payloads.push_back(confirmedForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(terminatedForkB);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(confirmedForkB);
    payloads.push_back(confirmedForkB);    
    payloads.push_back(confirmedForkA);
    payloads.push_back(confirmedForkA);
    payloads.push_back(terminatedForkB);

    {
        std::string xml;
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, payloads, xml));
        EXPECT_STREQ(terminatedForkB.c_str(), xml.c_str());
    }

    dialogCA.flushDb(user);
}
