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

const std::string user = "5001";
const std::string domain = "test.collator.inc";   

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
    "    <dialog id=\"0002@192.168.0.1\" call-id=\"0002@192.168.0.1\" local-tag=\"LOCALTAG-FORK-1\" remote-tag=\"REMOTETAG-FORK-2\" direction=\"recipient\">\n"
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

const std::string terminatedForkTrying = "<?xml version=\"1.0\" ?>"
    "<dialog-info xmlns=\"urn:ietf:params:xml:ns:dialog-info\" version=\"00000000000\" state=\"full\" entity=\"sip:5001@test.collator.inc;user=phone\">"
    "    <dialog id=\"0001@192.168.0.1\" call-id=\"0001@192.168.0.1\" local-tag=\"TRYINGTAG-1\" remote-tag=\"REMOTETAG-FORK-1\" direction=\"recipient\">"
    "        <state>terminated</state>"
    "        <remote>"
    "            <identity>sip:5002@test.collator.inc</identity>"
    "            <target uri=\"sip:5002@192.168.0.1;transport=tcp\" />"
    "        </remote>\n"
    "        <local>\n"
    "            <identity>sip:5001@test.collator.inc;user=phone</identity>\n"
    "            <target uri=\"sip:5001@test.collator.inc;user=phone\" />\n"
    "        </local>\n"
    "    </dialog>\n"
    "</dialog-info>\n";

const std::string terminatedForkAB = "<?xml version=\"1.0\" ?>\n"
    "<dialog-info xmlns=\"urn:ietf:params:xml:ns:dialog-info\" version=\"00000000000\" state=\"full\" entity=\"sip:5001@test.collator.inc\">\n"
    "    <dialog id=\"0001@192.168.0.1\" call-id=\"0001@192.168.0.1\" local-tag=\"LOCALTAG-FORK-1\" remote-tag=\"REMOTETAG-FORK-1\" direction=\"recipient\">\n"
    "        <state>terminated</state>\n"
    "        <remote>\n"
    "            <identity>sip:5002@test.collator.inc</identity>\n"
    "            <target uri=\"sip:5002@192.168.0.1;transport=tcp\" />\n"
    "        </remote>\n"
    "        <local>\n"
    "            <identity>sip:5001@test.collator.inc;user=phone</identity>\n"
    "            <target uri=\"sip:5001@192.168.0.2;transport=tcp\" />\n"
    "        </local>\n"
    "    </dialog>\n"    
    "    <dialog id=\"0001@192.168.0.1\" call-id=\"0001@192.168.0.1\" local-tag=\"LOCALTAG-FORK-2\" remote-tag=\"REMOTETAG-FORK-1\" direction=\"recipient\">\n"
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

TEST(DialogCollatorTest, test_aggregate_finalize_kamailio_payload_fork_call)
{
    DialogCollatorAndAggregator dialogCA;
    dialogCA.flushAll();

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
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, domain, payloads, xml));
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
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, domain, payloads, xml));
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
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, domain, payloads, xml));
        EXPECT_STREQ(terminatedForkB.c_str(), xml.c_str());
    }
}

TEST(DialogCollatorTest, test_aggregate_only_minimal_termination)
{
    DialogCollatorAndAggregator dialogCA;
    dialogCA.flushAll();

    std::list<std::string> payloads;

    // Insert a global terminated dialog
    std::string terminatedDialog;
    DialogEvent::generateMinimalDialog(user, domain, terminatedDialog);
    payloads.push_back(terminatedDialog);

    {
        std::string xml;
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, domain, payloads, xml));
        EXPECT_STREQ(terminatedDialog.c_str(), xml.c_str());
    }

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
    payloads.push_back(earlyForkB);
    payloads.push_back(terminatedForkTrying);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkA);
    payloads.push_back(earlyForkB);
    payloads.push_back(earlyForkB);

    {
        std::string xml;
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, domain, payloads, xml));
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
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, domain, payloads, xml));
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
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, domain, payloads, xml));
        EXPECT_STREQ(terminatedForkB.c_str(), xml.c_str());
    }
}

TEST(DialogCollatorTest, test_aggregate_with_minimal_termination)
{
    DialogCollatorAndAggregator dialogCA;
    dialogCA.flushAll();

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
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, domain, payloads, xml));
        EXPECT_STREQ(earlyForkB.c_str(), xml.c_str());
    }

    payloads.clear();
    // Insert a global terminated dialog
    std::string terminatedDialog;
    DialogEvent::generateMinimalDialog(user, domain, terminatedDialog);
    payloads.push_back(terminatedDialog);

    {
        std::string xml;
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, domain, payloads, xml));
        EXPECT_STREQ(terminatedForkAB.c_str(), xml.c_str());
    }
}

TEST(DialogCollatorTest, test_collate_queue)
{
    DialogCollatorAndAggregator dialogCA;
    dialogCA.flushAll();

    std::list<std::string> payloads;

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
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, domain, payloads, xml));
        EXPECT_STREQ(confirmedForkA.c_str(), xml.c_str());
    }

    {
        std::string xml;
        std::string terminatedDialog;
        DialogEvent::generateMinimalDialog(user, domain, terminatedDialog);
        EXPECT_TRUE(dialogCA.queueDialog(user, domain, confirmedForkA));
        EXPECT_TRUE(dialogCA.queueDialog(user, domain, terminatedForkB));
        EXPECT_TRUE(dialogCA.queueDialog(user, domain, terminatedDialog));
        EXPECT_TRUE(dialogCA.collateAndAggregateQueue(user, domain, xml));
        EXPECT_STREQ(confirmedForkA.c_str(), xml.c_str());
    }

    {
        std::string xml;
        std::string terminatedDialog;
        DialogEvent::generateMinimalDialog(user, domain, terminatedDialog);
        EXPECT_TRUE(dialogCA.queueDialog(user, domain, terminatedForkB));
        EXPECT_TRUE(dialogCA.queueDialog(user, domain, terminatedDialog));
        EXPECT_TRUE(dialogCA.collateAndAggregateQueue(user, domain, xml));
        EXPECT_STREQ(terminatedDialog.c_str(), xml.c_str());
    }
}

TEST(DialogCollatorTest, test_collate_queue_with_empty_body)
{
    DialogCollatorAndAggregator dialogCA;
    dialogCA.flushAll();

    std::list<std::string> payloads;

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
        EXPECT_TRUE(dialogCA.collateAndAggregate(user, domain, payloads, xml));
        EXPECT_STREQ(confirmedForkA.c_str(), xml.c_str());
    }

    {
        std::string xml;
        std::string terminatedDialog;
        DialogEvent::generateMinimalDialog(user, domain, terminatedDialog);
        EXPECT_TRUE(dialogCA.queueDialog(user, domain, confirmedForkA));
        EXPECT_TRUE(dialogCA.queueDialog(user, domain, terminatedForkB));
        EXPECT_TRUE(dialogCA.queueDialog(user, domain, ""));
        EXPECT_TRUE(dialogCA.collateAndAggregateQueue(user, domain, xml));
        EXPECT_STREQ(confirmedForkA.c_str(), xml.c_str());
    }

    {
        std::string xml;
        std::string terminatedDialog;
        DialogEvent::generateMinimalDialog(user, domain, terminatedDialog);
        EXPECT_TRUE(dialogCA.queueDialog(user, domain, terminatedForkB));
        EXPECT_TRUE(dialogCA.queueDialog(user, domain, ""));
        EXPECT_TRUE(dialogCA.collateAndAggregateQueue(user, domain, xml));
        EXPECT_STREQ(terminatedDialog.c_str(), xml.c_str());
    }
}