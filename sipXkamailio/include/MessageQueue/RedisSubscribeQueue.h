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

#ifndef KAMAILIO_REDIS_SUBSCRIBE_QUEUE_H_
#define KAMAILIO_REDIS_SUBSCRIBE_QUEUE_H_

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <sys/types.h>
#include <hiredis/adapters/libevent.h>	
}

#include <boost/thread.hpp>

#include <string>
#include <vector>

#include "MessageQueue/MessageQueuePlugin.h"

namespace SIPX {
namespace Kamailio {
namespace Plugin {

class RedisSubscribeQueue
{
public:
  RedisSubscribeQueue();
  virtual ~RedisSubscribeQueue();

  bool connect(const std::string &host, int port, const std::string &password, int db);
  bool reconnect();
  void disconnect();

  int subscribe(const std::string& channel, void* privateData, on_message_event_t callback);
  int resubscribe();

  void run();
  void stop();

protected:
  virtual void onMessage(const std::string& type, const std::string &channel, const std::string& data);
  virtual void onConnect(const redisAsyncContext *context, int status);
  virtual void onDisconnect(const redisAsyncContext* context, int status);

  void checkConnections();

private:
  void internalRun();

  static void onRedisMessage(redisAsyncContext* context, void* rawReply, void* privateData);
  static void onRedisConnect(const redisAsyncContext* context, int status);
  static void onRedisDisconnect(const redisAsyncContext* context, int status);

private:
  redisAsyncContext* _redisContext;
  struct event_base* _eventBase;
  boost::thread* _queueEventLoop;

  std::string _redisAddress;
  std::string _subscribeChannel;
  int _redisPort;
  bool _calledConnect;
  bool _terminated;

  on_message_event_t _subscriberEventCallback;
  void* _subscriberPrivateData;
}; // class RedisMessageQueue


} } }  // SIPX::Kamailio::Plugin

#endif // ! KAMAILIO_REDIS_SUBSCRIBE_QUEUE_H_
