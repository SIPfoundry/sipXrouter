#include "gtest/gtest.h"
#include <iostream>
#include <sstream>
#include <vector>

#include "DialogEventCollator/DialogInfo.h"

using namespace SIPX::Kamailio::Plugin;

TEST(DialogInfoTest, test_parse_dialog_xml)
{
    std::string dialogXml = "<?xml version=\"1.0\" ?>"
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
    
    DialogEvent dialogEvent;
    dialogEvent.parse(dialogXml);
    ASSERT_TRUE(dialogEvent.valid());
    
    DialogInfo dialogInfo = dialogEvent.dialogInfo();
    
    ASSERT_STREQ("sip:5000@test.sip.inc", dialogInfo.entity.c_str());
    ASSERT_STREQ("full", dialogInfo.state.c_str());

    DialogElement dialogElement = dialogEvent.dialogElement();
    ASSERT_STREQ("0001@test.sip.inc", dialogElement.id.c_str());
    ASSERT_STREQ("0001@test.sip.inc", dialogElement.callId.c_str());
    ASSERT_STREQ("LOCALTAG-01", dialogElement.localTag.c_str());
    ASSERT_STREQ("REMOTETAG-01", dialogElement.remoteTag.c_str());
    ASSERT_EQ(DIRECTION_INITIATOR, dialogElement.direction);
    ASSERT_EQ(STATE_EARLY, dialogElement.state);

    ASSERT_TRUE(dialogEvent.updateDialogState(STATE_TERMINATED));
    ASSERT_EQ(STATE_TERMINATED, dialogEvent.dialogElement().state);

    DialogEvent updatedEvent;
    updatedEvent.parse(dialogEvent.payload());
    ASSERT_TRUE(updatedEvent.valid());

    dialogInfo = updatedEvent.dialogInfo();
    
    ASSERT_STREQ("sip:5000@test.sip.inc", dialogInfo.entity.c_str());
    ASSERT_STREQ("full", dialogInfo.state.c_str());

    dialogElement = updatedEvent.dialogElement();
    ASSERT_STREQ("0001@test.sip.inc", dialogElement.id.c_str());
    ASSERT_STREQ("0001@test.sip.inc", dialogElement.callId.c_str());
    ASSERT_STREQ("LOCALTAG-01", dialogElement.localTag.c_str());
    ASSERT_STREQ("REMOTETAG-01", dialogElement.remoteTag.c_str());
    ASSERT_EQ(DIRECTION_INITIATOR, dialogElement.direction);
    ASSERT_EQ(STATE_TERMINATED, dialogElement.state);
}