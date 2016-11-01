#include "gtest/gtest.h"
#include <iostream>
#include <sstream>
#include <vector>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>

#include "MessageQueue/RedisSubscribeQueue.h"

using namespace SIPX::Kamailio::Plugin;

std::string fromChannel;
std::string receiveMessage;

void message(char* channel, int channelLen, char* data, int dataLen, void* privdata)
{
    fromChannel = std::string(channel, channelLen);
    receiveMessage = std::string(data, dataLen);

    std::cout << "Receive message: " << receiveMessage << std::endl;
}

TEST(RedisSubscribeQueueTest, test_redis_connect)
{
    RedisSubscribeQueue redisQueue;
    ASSERT_TRUE(redisQueue.connect("127.0.0.1", 6379, "", 0));
    redisQueue.run();

    redisQueue.subscribe("TEST.CHANNEL", 0, message);

    boost::this_thread::sleep(boost::posix_time::milliseconds(20000));

    ASSERT_STREQ("TEST.CHANNEL", fromChannel.c_str());
    ASSERT_STREQ("test1", receiveMessage.c_str());

    redisQueue.stop();
}