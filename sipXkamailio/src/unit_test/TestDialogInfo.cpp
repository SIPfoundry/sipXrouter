#include "gtest/gtest.h"
#include <iostream>
#include <sstream>
#include <vector>

#include "DialogEventCollator/DialogInfo.h"

using namespace SIPX::Kamailio::Plugin;

TEST(DialogInfoTest, test_parse_dialog_xml)
{
    std::string dialogXml = "<?xml version=\"1.0\"?>"
        "<dialog-info xmlns=\"urn:ietf:params:xml:ns:dialog-info\" version=\"63\" state=\"full\" entity=\"sip:5000@test.domain.com\">"
            "<dialog id=\"26bfed15-b490e778-5d4d2d53@192.168.0.111\" call-id=\"26bfed15-b490e778-5d4d2d53@192.168.0.111\" local-tag=\"CFA9E939-97CA593C\" remote-tag=\"2FC6CE3C-E3F38EF3\" direction=\"initiator\">"
                "<state>early</state>"
                "<remote>"
                    "<identity>sip:5001@test.domain.com;user=phone</identity>"
                    "<target uri=\"sip:5001@192.168.0.109\"/>"
                "</remote>"
                "<local>"
                    "<identity>sip:5000@test.domain.com</identity>"
                    "<target uri=\"sip:5000@192.168.0.111\"/>"
                "</local>"
            "</dialog>"
        "</dialog-info>";
    
    DialogInfo dialogInfo;
    ASSERT_TRUE(parseDialogInfoXML(dialogXml, dialogInfo));
    
    ASSERT_STREQ("sip:5000@test.domain.com", dialogInfo.entity.c_str());
    ASSERT_STREQ("full", dialogInfo.state.c_str());

    ASSERT_TRUE(dialogInfo.valid());
    
    ASSERT_STREQ("26bfed15-b490e778-5d4d2d53@192.168.0.111", dialogInfo.dialog.id.c_str());
    ASSERT_STREQ("26bfed15-b490e778-5d4d2d53@192.168.0.111", dialogInfo.dialog.callId.c_str());
    ASSERT_STREQ("CFA9E939-97CA593C", dialogInfo.dialog.localTag.c_str());
    ASSERT_STREQ("2FC6CE3C-E3F38EF3", dialogInfo.dialog.remoteTag.c_str());
    ASSERT_STREQ("initiator", dialogInfo.dialog.direction.c_str());
    ASSERT_STREQ("early", dialogInfo.dialog.state.c_str());
}