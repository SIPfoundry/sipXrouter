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

#include "MessageQueue/RedisSubscribeQueue.h"
#include <boost/algorithm/string.hpp>
#include <sstream>

#include <OSS/OSS.h>
#include <OSS/UTL/Logger.h>

#define SUBSCRIBE_REPLY_SIZE 3
#define DEFAULT_EVENT_LOOP_TIMER 10;

namespace SIPX {
namespace Kamailio {
namespace Plugin {

RedisSubscribeQueue::RedisSubscribeQueue()
  : _redisContext(0)
  , _eventBase(0)
  , _queueEventLoop(0)
  , _redisAddress("127.0.0.1")
  , _redisPort(6379)
  , _calledConnect(false)
  , _terminated(false)
  , _subscriberEventCallback(0)  
  , _subscriberPrivateData(0)
{
  _eventBase = event_base_new();
}

RedisSubscribeQueue::~RedisSubscribeQueue()
{
  stop();

  if (_eventBase)
  {
  	event_base_free(_eventBase);
  	_eventBase = 0;
  }

  if (_redisContext)
  {
    redisAsyncFree(_redisContext);
    _redisContext = 0;
  }
}

bool RedisSubscribeQueue::connect(const std::string &host, int port, const std::string & password, int db)
{
  _redisAddress = host;
  _redisPort = port;
  return reconnect();
}

bool RedisSubscribeQueue::reconnect()
{
  OSS_LOG_INFO("[RedisSubscribeQueue] Connecting to " << _redisAddress << ":" << _redisPort);
  _redisContext = redisAsyncConnect(_redisAddress.c_str(), _redisPort);
  if(_redisContext->err) {
    OSS_LOG_ERROR("[RedisSubscribeQueue] Failed to connect to " << _redisAddress << ":" << _redisPort 
      << " reason: " << _redisContext->errstr);
    return false;
  }

  /**
   * TODO
   * Add support for password and selecting db
   **/

  /** Store RedisSubscribeQueue to context **/
  _redisContext->data = this;
  redisLibeventAttach(_redisContext, _eventBase);
  redisAsyncSetConnectCallback(_redisContext, &RedisSubscribeQueue::onRedisConnect);
  redisAsyncSetDisconnectCallback(_redisContext, &RedisSubscribeQueue::onRedisDisconnect);
  _calledConnect = true;

  OSS_LOG_NOTICE("[RedisSubscribeQueue] Connected to " << _redisAddress << ":" << _redisPort);
  return true;
}

void RedisSubscribeQueue::disconnect()
{
  OSS_LOG_NOTICE("[RedisSubscribeQueue] Disconnect to " << _redisAddress << ":" << _redisPort);
  if (_redisContext)
  {
  	redisAsyncDisconnect(_redisContext);
  }
}

int RedisSubscribeQueue::subscribe(const std::string& channel, void* privateData, on_message_event_t callback)
{
  _subscribeChannel = channel;
  _subscriberEventCallback = callback;
  _subscriberPrivateData = privateData;

  return resubscribe();
}

int RedisSubscribeQueue::resubscribe()
{
  OSS_LOG_NOTICE("[RedisSubscribeQueue] SUBSCRIBE to channel: " << _subscribeChannel);
  int result = redisAsyncCommand(_redisContext, &RedisSubscribeQueue::onRedisMessage, this, "SUBSCRIBE %s", _subscribeChannel.c_str());
  return result;
}

void RedisSubscribeQueue::run()
{
  assert(_calledConnect && !_queueEventLoop);
  OSS_LOG_DEBUG("[RedisSubscribeQueue] Start Running Thead Loop");
  _queueEventLoop = new boost::thread(boost::bind(&RedisSubscribeQueue::internalRun, this));
}

void RedisSubscribeQueue::stop()
{
  _terminated = true;

  OSS_LOG_DEBUG("[RedisSubscribeQueue] Stopping thread loop");
  if (_queueEventLoop)
  {
    OSS_LOG_DEBUG("[RedisSubscribeQueue] Waiting thread to exit");
    _queueEventLoop->join();
    delete _queueEventLoop;
    _queueEventLoop = 0;
    OSS_LOG_DEBUG("[RedisSubscribeQueue] Thread exitted");
  }

  if (_eventBase)
  {
    event_base_free(_eventBase);
    _eventBase = 0;
  }

  _calledConnect = false;
  OSS_LOG_DEBUG("[RedisSubscribeQueue] Stop thread loop");
}

void RedisSubscribeQueue::internalRun()
{
  OSS_LOG_DEBUG("[RedisSubscribeQueue] Thread Event Loop started");
  while (!_terminated && _eventBase)
  {
    checkConnections();

    /** Set timer for event base loop*/
    struct timeval tv;
    tv.tv_sec=DEFAULT_EVENT_LOOP_TIMER;
    tv.tv_usec=0;
    event_base_loopexit(_eventBase, &tv);

    event_base_loop(_eventBase, 0x04);
  }

  OSS_LOG_DEBUG("[RedisSubscribeQueue] Thread Event Loop exitted");
}

void RedisSubscribeQueue::onMessage(const std::string& type, const std::string &channel, const std::string& data)
{
  OSS_LOG_DEBUG("[RedisSubscribeQueue] Receive Redis Subscribe Message Event");
  if(_subscriberEventCallback && boost::iequals(type, "message"))
  {
    OSS_LOG_DEBUG("[RedisSubscribeQueue] Notify Subscriber callback --> " << channel << ":" << data);
    _subscriberEventCallback((char*)channel.c_str(), (int)channel.size()
    	, (char*)data.c_str(), (int)data.size(), _subscriberPrivateData);
  }
}

void RedisSubscribeQueue::onConnect(const redisAsyncContext* context, int status)
{
  OSS_LOG_DEBUG("[RedisSubscribeQueue] Receive Redis Connect Event");
  if(status != REDIS_OK)
  {
    OSS_LOG_WARNING("[RedisSubscribeQueue] Failed to reconnect resetting context");
  	_redisContext = 0;
  }
}

void RedisSubscribeQueue::onDisconnect(const redisAsyncContext* context, int status)
{
  OSS_LOG_DEBUG("[RedisSubscribeQueue] Receive Redis Disconnect Event");
  _redisContext = 0;
}

void RedisSubscribeQueue::checkConnections()
{
  OSS_LOG_DEBUG("[RedisSubscribeQueue] Checking Redis Async Connections");
  if(!_redisContext)
  {
    OSS_LOG_WARNING("[RedisSubscribeQueue] Attempting to reconnect to redis server");
    if(reconnect())
    {
      //This will only fail if we are disconnecting to redis
      //which means that incase of failure the callback will
      //reset ContextAsync and will resubscribe
      OSS_LOG_WARNING("[RedisSubscribeQueue] Attempting to resubscribe to current channel");
      resubscribe();
    }
  }
}

void RedisSubscribeQueue::onRedisMessage(redisAsyncContext* context, void* rawReply, void* privateData)
{
  redisReply *reply = (redisReply*)rawReply;
  if (!reply) 
  {
    return;
  }

  RedisSubscribeQueue* mq = (RedisSubscribeQueue*)privateData;
  if(!mq)
  {
    return;
  }

  if (reply->type == REDIS_REPLY_ARRAY) 
  {
  	if(reply->elements >= SUBSCRIBE_REPLY_SIZE)
  	{
	    std::string type(reply->element[0]->str, reply->element[0]->len); //1st element is message
	    std::string channel(reply->element[1]->str, reply->element[1]->len); //2nd element is channel
	    std::string data(reply->element[2]->str, reply->element[2]->len); //3rd element is data
      mq->onMessage(type,channel,data);
  	}
  }
}

void RedisSubscribeQueue::onRedisConnect(const redisAsyncContext *context, int status)
{
  if(context && context->data)
  {
  	RedisSubscribeQueue* queue = (RedisSubscribeQueue*)context->data;
  	queue->onConnect(context, status);
  }
}

void RedisSubscribeQueue::onRedisDisconnect(const redisAsyncContext *context, int status)
{
  if(context && context->data)
  {
  	RedisSubscribeQueue* queue = (RedisSubscribeQueue*)context->data;
  	queue->onDisconnect(context, status);
  }
}

} } } // Namespace SIPX::Kamailio::Plugin
