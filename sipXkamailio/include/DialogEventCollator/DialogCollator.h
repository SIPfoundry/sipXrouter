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

#ifndef DIALOGCOLLATOR_H_
#define DIALOGCOLLATOR_H_

#include "DialogEventCollator/DialogInfo.h"

#include <boost/noncopyable.hpp>
#include <OSS/Persistent/RedisClient.h>
#include <OSS/JSON/reader.h>
#include <OSS/JSON/writer.h>
#include <OSS/JSON/elements.h>
#include <list>

namespace SIPX {
namespace Kamailio {
namespace Plugin {

class DialogCollatorAndAggregator
{
public:
  typedef std::list<DialogEvent> DialogEvents;
  typedef std::list<const DialogEvent*> DialogEventsPtr;
  typedef std::list<DialogCollateEvent> DialogCollatedEvents;

public:
  DialogCollatorAndAggregator();

public:
  bool connect(const std::string& password = "", int db = 0);
  void disconnect();

public:
  bool isActive(const std::string & user, const std::string & domain);

  bool queueDialog(const std::string & user, const std::string & domain
    , const std::string & body);

  bool collateAndAggregate(const std::string & user, const std::string & domain
    , const std::list<std::string> & payloads, std::string & xml);

  bool collateAndAggregate(const std::string & user, const std::string & domain
    , const DialogEvents & dialogEvents, std::string & xml);

  bool collateAndAggregate(const std::string & user, const std::string & domain
    , const DialogEventsPtr & dialogEvents, std::string & xml);

  bool collateAndAggregateQueue(const std::string & user, const std::string & domain
    , std::string &xml);

  bool collateAndAggregateNotify(const std::string & user, const std::string & domain
    , const std::string & body, std::string &xml);

  void flushAll();


private:

  bool parse(const std::string & user, const std::string & domain
    , const std::list<std::string> & payloads, DialogEvents & dialogEvents);
  void collate(const std::string & user, const std::string & domain
    , const DialogEventsPtr & dialogEvents, DialogEventsPtr & collatedEvents);
  void aggregate(const std::string & user, const std::string & domain
    , const DialogEventsPtr & dialogEvents, std::string & xml);

  bool saveDialogEvent(const std::string & key, const DialogEvent & dialogEvent);
  bool loadDialogEvent(const std::string & key, DialogEvent & dialogEvent);
  bool loadActiveDialogs(const std::string & user, const std::string & domain
    , const std::set<std::string> & filterIds, DialogEvents & dialogEvents
    , DialogState state = STATE_INVALID);

  void refreshActiveDialogs(const std::string & user, const std::string & domain);

  bool loadQueueDialogs(const std::string & user, const std::string & domain
    , std::vector<std::string> & keys
    , DialogEvents & dialogEvents);

  bool hasActiveDialogs(const DialogEvents & dialogEvents);

public:
  int queueDialogTTL() const;
  int collateDialogTTL() const;
  int activeDialogTTL() const;

  void queueDialogTTL(int ttl);
  void collateDialogTTL(int ttl);
  void activeDialogTTL(int ttl);
  

private:
  OSS::Persistent::RedisClient _redisClient;
  int _queueDialogTTL;
  int _collateDialogTTL;
  int _activeDialogTTL;

}; // class DialogCollatorAndAggregator

class DialogCollatorPlugin
{
public:
  typedef std::map<std::string, std::string> PluginSettings;

public:
  static void start(const PluginSettings & settings);
  static void stop();

  static void create(void** handle);
  static void destroy(void** handle);

  static bool isActive(void* handle, const std::string & user
    , const std::string & domain);

  static bool queueDialog(void* handle, const std::string & user
    , const std::string & domain, const std::string & body);

  static bool collateQueue(void* handle, const std::string & user
    , const std::string & domain, std::string & xml);

  static bool collateNotify(void* handle, const std::string & user
    , const std::string & domain, const std::string & body, std::string & xml);

  static bool processEvents(void* handle, const std::string & user
    , const std::string & domain
    , const std::list<std::string> & payloads
    , std::string & xml);

public:
  static std::string _redisPassword;
  static int _redisDb;
  static int _queueDialogTTL;
  static int _collateDialogTTL;
  static int _activeDialogTTL;

}; // class DialogCollatorPlugin

/**
 * Template/Inline function definition
 */

inline int DialogCollatorAndAggregator::queueDialogTTL() const
{
  return _queueDialogTTL;
}

inline int DialogCollatorAndAggregator::collateDialogTTL() const
{
  return _collateDialogTTL;
}

inline int DialogCollatorAndAggregator::activeDialogTTL() const
{
  return _activeDialogTTL;
} 

inline void DialogCollatorAndAggregator::queueDialogTTL(int ttl)
{
  _queueDialogTTL = ttl;
}

inline void DialogCollatorAndAggregator::collateDialogTTL(int ttl)
{
  _collateDialogTTL = ttl;
}

inline void DialogCollatorAndAggregator::activeDialogTTL(int ttl)
{
  _activeDialogTTL = ttl;
}

} } } // SIPX::Kamailio::Plugin

#endif  /* DIALOGCOLLATOR_H */

