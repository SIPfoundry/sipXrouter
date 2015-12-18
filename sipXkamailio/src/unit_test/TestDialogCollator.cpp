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

TEST(DialogCollatorTest, test_collate_payload)
{
    const std::string user = "5001";
    const std::string domain = "test.collator.inc";

    std::vector<std::string> payloads;

    payloads.push_back("<?xml version=\"1.0\"?>"
        "<dialog-info xmlns=\"urn:ietf:params:xml:ns:dialog-info\" version=\"0\" state=\"full\" entity=\"sip:5001@test.collator.inc;user=phone\">"
        "  <dialog id=\"2a0d8e89-bc792e90-d222401f@192.168.0.154\" call-id=\"2a0d8e89-bc792e90-d222401f@192.168.0.154\" local-tag=\"43690258-FA62CA0B\" remote-tag=\"56D8D0BD-72762C24\" direction=\"recipient\">"
        "    <state>terminated</state>"
        "    <remote>"
        "      <identity>sip:5002@test.collator.inc</identity>"
        "      <target uri=\"sip:5002@192.168.0.154\"/>"
        "    </remote>"
        "    <local>"
        "      <identity>sip:5001@test.collator.inc;user=phone</identity>"
        "      <target uri=\"sip:5001@test.collator.inc;user=phone\"/>"
        "    </local>"
        "  </dialog>"
        "</dialog-info>");

    payloads.push_back("<?xml version=\"1.0\"?>"
        "<dialog-info xmlns=\"urn:ietf:params:xml:ns:dialog-info\" version=\"0\" state=\"full\" entity=\"sip:5001@test.collator.inc;transport=TCP\">"
        "  <dialog id=\"ZGRlNjY5ZWI3YjliMjMyMzQxZDhlZTdkZmY2NmExMmU.\" call-id=\"ZGRlNjY5ZWI3YjliMjMyMzQxZDhlZTdkZmY2NmExMmU.\" local-tag=\"D06ED01A-239E03CD\" remote-tag=\"f0532531\" direction=\"recipient\">"
        "    <state>confirmed</state>"
        "    <remote>"
        "      <identity>sip:5003@test.collator.inc;transport=TCP</identity>"
        "      <target uri=\"sip:5003@192.168.0.113:56808;transport=TCP\"/>"
        "    </remote>"
        "    <local>"
        "      <identity>sip:5001@test.collator.inc;transport=TCP</identity>"
        "      <target uri=\"sip:5001@test.collator.inc;transport=TCP\"/>"
        "    </local>"
        "  </dialog>"
        "</dialog-info>");

    payloads.push_back("<?xml version=\"1.0\"?>"
        "<dialog-info xmlns=\"urn:ietf:params:xml:ns:dialog-info\" version=\"0\" state=\"full\" entity=\"sip:5001@test.collator.inc;user=phone\">"
        "  <dialog id=\"2a0d8e89-bc792e90-d222401f@192.168.0.154\" call-id=\"2a0d8e89-bc792e90-d222401f@192.168.0.154\" local-tag=\"bQAQC5\" remote-tag=\"56D8D0BD-72762C24\" direction=\"recipient\">"
        "    <state>terminated</state>"
        "    <remote>"
        "      <identity>sip:5002@test.collator.inc</identity>"
        "      <target uri=\"sip:5002@192.168.0.154\"/>"
        "    </remote>"
        "    <local>"
        "      <identity>sip:5001@test.collator.inc;user=phone</identity>"
        "      <target uri=\"sip:5001@test.collator.inc;user=phone\"/>"
        "    </local>"
        "  </dialog>"
        "</dialog-info>");

    std::string expectedPayload = "<?xml version=\"1.0\" ?><dialog-info xmlns=\"urn:ietf:params:xml:ns:dialog-info\" version=\"00000000000\" state=\"full\" entity=\"sip:5001@test.collator.inc;transport=TCP\"><dialog id=\"ZGRlNjY5ZWI3YjliMjMyMzQxZDhlZTdkZmY2NmExMmU.\" call-id=\"ZGRlNjY5ZWI3YjliMjMyMzQxZDhlZTdkZmY2NmExMmU.\" local-tag=\"D06ED01A-239E03CD\" remote-tag=\"f0532531\" direction=\"recipient\"><state>confirmed</state><remote><identity>sip:5003@test.collator.inc;transport=TCP</identity><target uri=\"sip:5003@192.168.0.113:56808;transport=TCP\" /></remote><local><identity>sip:5001@test.collator.inc;transport=TCP</identity><target uri=\"sip:5001@test.collator.inc;transport=TCP\" /></local></dialog><dialog id=\"2a0d8e89-bc792e90-d222401f@192.168.0.154\" call-id=\"2a0d8e89-bc792e90-d222401f@192.168.0.154\" local-tag=\"43690258-FA62CA0B\" remote-tag=\"56D8D0BD-72762C24\" direction=\"recipient\"><state>terminated</state><remote><identity>sip:5002@test.collator.inc</identity><target uri=\"sip:5002@192.168.0.154\" /></remote><local><identity>sip:5001@test.collator.inc;user=phone</identity><target uri=\"sip:5001@test.collator.inc;user=phone\" /></local></dialog></dialog-info>";

    DialogCollator collator;
    collator.flushDialog(user, domain); // Clean test user and domain

    std::string payload = collator.collateAndAggregatePayloads(user, domain, payloads);
    ASSERT_STREQ(payload.c_str(), expectedPayload.c_str());

    payloads.clear();

    payloads.push_back("<?xml version=\"1.0\"?>"
        "<dialog-info xmlns=\"urn:ietf:params:xml:ns:dialog-info\" version=\"0\" state=\"full\" entity=\"sip:5001@test.collator.inc;transport=TCP\">"
        "  <dialog id=\"ZGRlNjY5ZWI3YjliMjMyMzQxZDhlZTdkZmY2NmExMmU.\" call-id=\"ZGRlNjY5ZWI3YjliMjMyMzQxZDhlZTdkZmY2NmExMmU.\" local-tag=\"D06ED01A-239E03CD\" remote-tag=\"f0532531\" direction=\"recipient\">"
        "    <state>confirmed</state>"
        "    <remote>"
        "      <identity>sip:5003@test.collator.inc;transport=TCP</identity>"
        "      <target uri=\"sip:5003@192.168.0.113:56808;transport=TCP\"/>"
        "    </remote>"
        "    <local>"
        "      <identity>sip:5001@test.collator.inc;transport=TCP</identity>"
        "      <target uri=\"sip:5001@test.collator.inc;transport=TCP\"/>"
        "    </local>"
        "  </dialog>"
        "</dialog-info>");

    payloads.push_back("<?xml version=\"1.0\"?>"
        "<dialog-info xmlns=\"urn:ietf:params:xml:ns:dialog-info\" version=\"0\" state=\"full\" entity=\"sip:5001@test.collator.inc;user=phone\">"
        "  <dialog id=\"2a0d8e89-bc792e90-d222401g@192.168.0.154\" call-id=\"2a0d8e89-bc792e90-d222401f@192.168.0.154\" local-tag=\"bQAQC5\" remote-tag=\"56D8D0BD-72762C24\" direction=\"recipient\">"
        "    <state>confirmed</state>"
        "    <remote>"
        "      <identity>sip:5002@test.collator.inc</identity>"
        "      <target uri=\"sip:5002@192.168.0.154\"/>"
        "    </remote>"
        "    <local>"
        "      <identity>sip:5001@test.collator.inc;user=phone</identity>"
        "      <target uri=\"sip:5001@test.collator.inc;user=phone\"/>"
        "    </local>"
        "  </dialog>"
        "</dialog-info>");

    expectedPayload = "<?xml version=\"1.0\" ?><dialog-info xmlns=\"urn:ietf:params:xml:ns:dialog-info\" version=\"00000000000\" state=\"full\" entity=\"sip:5001@test.collator.inc;transport=TCP\"><dialog id=\"ZGRlNjY5ZWI3YjliMjMyMzQxZDhlZTdkZmY2NmExMmU.\" call-id=\"ZGRlNjY5ZWI3YjliMjMyMzQxZDhlZTdkZmY2NmExMmU.\" local-tag=\"D06ED01A-239E03CD\" remote-tag=\"f0532531\" direction=\"recipient\"><state>confirmed</state><remote><identity>sip:5003@test.collator.inc;transport=TCP</identity><target uri=\"sip:5003@192.168.0.113:56808;transport=TCP\" /></remote><local><identity>sip:5001@test.collator.inc;transport=TCP</identity><target uri=\"sip:5001@test.collator.inc;transport=TCP\" /></local></dialog><dialog id=\"2a0d8e89-bc792e90-d222401f@192.168.0.154\" call-id=\"2a0d8e89-bc792e90-d222401f@192.168.0.154\" local-tag=\"43690258-FA62CA0B\" remote-tag=\"56D8D0BD-72762C24\" direction=\"recipient\"><state>terminated</state><remote><identity>sip:5002@test.collator.inc</identity><target uri=\"sip:5002@192.168.0.154\" /></remote><local><identity>sip:5001@test.collator.inc;user=phone</identity><target uri=\"sip:5001@test.collator.inc;user=phone\" /></local></dialog></dialog-info>";

    payload = collator.collateAndAggregatePayloads("5001", "test.collator.inc", payloads);
    ASSERT_STREQ(expectedPayload.c_str(), payload.c_str());

    collator.flushDialog(user, domain); // Clean test user and domain

    OSS::logger_deinit();
}
