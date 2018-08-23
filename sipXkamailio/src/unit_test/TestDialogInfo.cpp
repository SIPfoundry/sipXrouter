#include "gtest/gtest.h"
#include <iostream>
#include <sstream>
#include <vector>

#include "DialogEventCollator/DialogInfo.h"

using namespace SIPX::Kamailio::Plugin;

TEST(DialogInfoTest, test_parse_dialog_full_xml)
{
    std::string dialogXml = "<?xml version=\"1.0\"?>"
        "<dialog-info xmlns=\"urn:ietf:params:xml:ns:dialog-info\" version=\"63\" state=\"full\" entity=\"sip:5000@test.sip.inc\">"
            "<dialog id=\"0001@test.sip.inc\" call-id=\"0001@test.sip.inc\" local-tag=\"LOCALTAG-01\" remote-tag=\"REMOTETAG-01\" direction=\"initiator\">"
                "<state>early</state>"
                "<remote>"
                    "<identity>sip:5001@test.sip.inc</identity>"
                    "<target uri=\"sip:5001@192.168.0.1\"/>"
                "</remote>"
                "<local>"
                    "<identity>sip:5000@test.sip.inc</identity>"
                    "<target uri=\"sip:5000@192.168.0.2\"/>"
                "</local>"
            "</dialog>"
        "</dialog-info>";
    
    DialogEvent dialogEvent(dialogXml);
    ASSERT_TRUE(dialogEvent.valid());
    
    DialogInfo dialogInfo = dialogEvent.dialogInfo();
    
    ASSERT_STREQ("sip:5000@test.sip.inc", dialogInfo.entity.c_str());
    ASSERT_STREQ("63", dialogInfo.version.c_str());
    ASSERT_STREQ("full", dialogInfo.state.c_str());

    DialogElement dialogElement = dialogEvent.dialogElement();
    ASSERT_STREQ("0001@test.sip.inc", dialogElement.id.c_str());
    ASSERT_STREQ("0001@test.sip.inc", dialogElement.callId.c_str());
    ASSERT_STREQ("LOCALTAG-01", dialogElement.localTag.c_str());
    ASSERT_STREQ("REMOTETAG-01", dialogElement.remoteTag.c_str());
    ASSERT_EQ(DIRECTION_INITIATOR, dialogElement.direction);
    ASSERT_EQ(STATE_EARLY, dialogElement.state);
}

TEST(DialogInfoTest, test_parse_dialog_minimal_xml)
{
    std::string dialogXml = "<?xml version=\"1.0\"?>"
        "<dialog-info xmlns=\"urn:ietf:params:xml:ns:dialog-info\" version=\"63\" state=\"full\" entity=\"sip:5000@test.sip.inc\">"
        "</dialog-info>";
    
    DialogEvent dialogEvent(dialogXml);
    ASSERT_TRUE(dialogEvent.valid());
    
    DialogInfo dialogInfo = dialogEvent.dialogInfo();
    
    ASSERT_STREQ("sip:5000@test.sip.inc", dialogInfo.entity.c_str());
    ASSERT_STREQ("63", dialogInfo.version.c_str());
    ASSERT_STREQ("full", dialogInfo.state.c_str());

    DialogElement dialogElement = dialogEvent.dialogElement();
    ASSERT_TRUE(dialogElement.id.empty());
    ASSERT_TRUE(dialogElement.callId.empty());
    ASSERT_TRUE(dialogElement.localTag.empty());
    ASSERT_TRUE(dialogElement.remoteTag.empty());
    ASSERT_EQ(DIRECTION_INVALID, dialogElement.direction);
    ASSERT_EQ(STATE_TERMINATED, dialogElement.state);
}