/*
 * Copyright (c) 2015 SIPfoundry, Inc. All rights reserved.
 *
 * Contributed to SIPfoundry under a Contributor Agreement
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 */

#include "MessageQueue/MessageQueuePlugin.h"
#include "MessageQueue/RedisSubscribeQueue.h"

#include <OSS/OSS.h>
#include <OSS/UTL/Logger.h>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/lexical_cast.hpp>
#include <map>
#include <string>

using namespace SIPX::Kamailio::Plugin;

#define REDIS_DEFAULT_PORT 6379

RedisSubscribeQueue* redisSubsribeQueue = 0;

extern "C" int message_queue_init(str* key, str* value, int size);
extern "C" int message_queue_destroy();
extern "C" int message_queue_subscribe_event(str* channel, void* priv, on_message_event_t message_event);

extern "C" int bind_message_queue_plugin(message_queue_plugin_t* api)
{
  if (!api) {
    return -1;
  }

  api->message_queue_plugin_init = message_queue_init;
  api->message_queue_plugin_destroy = message_queue_destroy;
  api->message_queue_subscribe_event = message_queue_subscribe_event;
  return 0;
}

extern "C" int message_queue_init(str* key, str* value, int size)
{
  if(size > 0)
  {
    std::map<std::string, std::string> properties;
    for(int index = 0; index < size; ++index)
    {
      std::string str_key = key[index].len > 0 ? std::string(key[index].s, key[index].len) : std::string("");
      std::string str_value = value[index].len > 0 ? std::string(value[index].s, value[index].len) : std::string("");
      properties[str_key] = str_value;
    }

    //Check if required logger configuration is provided
    std::map<std::string, std::string>::iterator iter = properties.find("log-path");
    if(iter != properties.end())
    {
      std::string log_path = properties["log-path"];
      if(!log_path.empty())
      {
        std::string log_level = properties.find("log-level") != properties.end() ? properties["log-level"] : 0;
        try
        {
          int log_priority = boost::lexical_cast<int>(log_level);
          OSS::logger_init(log_path, static_cast<OSS::LogPriority>(log_priority));
        } catch(std::exception & ex)
        {
          //Do nothing, other pluging may have already initialize the logger
        }
      }

      OSS_LOG_INFO("[Plugin] Initialize message queue plugin resources");
    }

    std::string redisAddress;
    iter = properties.find("redis-address");
    if(iter != properties.end())
    {
      redisAddress = properties["redis-address"];
    }

    int redisPort = REDIS_DEFAULT_PORT;
    iter = properties.find("redis-port");
    if(iter != properties.end())
    {
      redisPort = boost::lexical_cast<int>(properties["redis-port"]);
    }

    std::string redisChannel;
    iter = properties.find("redis-channel");
    if(iter != properties.end())
    {
      redisChannel = properties["redis-channel"];
    }

    std::string redisPassword;
    iter = properties.find("redis-password");
    if(iter != properties.end())
    {
      redisPassword = properties["redis-password"];
    }

    int redisDb = 0;
    iter = properties.find("redis-db");
    if(iter != properties.end())
    {
      redisDb = boost::lexical_cast<int>(properties["redis-db"]);
    }

    redisSubsribeQueue = new RedisSubscribeQueue();
    if(!redisSubsribeQueue->connect(redisAddress, redisPort, redisPassword, redisDb)) {
    	OSS_LOG_WARNING("[Plugin] Failed to connect to redis server");
    }

    redisSubsribeQueue->run();
  }

  return 0;
}

extern "C" int message_queue_destroy()
{
  if(redisSubsribeQueue) 
  {
    redisSubsribeQueue->stop();  
  }
  
  OSS::logger_deinit();
  return 0;
}

extern "C" int message_queue_subscribe_event(str* channel, void* priv, on_message_event_t message_event)
{
  std::string subscribeChannel = channel ? std::string(channel->s, channel->len) : std::string("");
  if(!subscribeChannel.empty())
  {
    redisSubsribeQueue->subscribe(subscribeChannel, priv, message_event);
  }
  return 0;
}