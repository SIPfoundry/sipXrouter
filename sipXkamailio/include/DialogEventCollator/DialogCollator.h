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

class DialogCollatorAndAggregator;

class DialogCollator
{
public:
  typedef std::list<DialogCollateEvent> DialogCollateEvents;

public:
  DialogCollator(const std::string & user, const std::string & dialogId);

  void addEvent(const DialogEvent & dialogEvent);
  void addEvent(const std::list<DialogEvent> & dialogEvents);

  bool winningNode(DialogCollatorAndAggregator & dialogCAA
    , DialogEvent & winningEvent) const;

  bool winningNodes(DialogCollatorAndAggregator & dialogCAA
    , std::list<DialogEvent> & winningEvents) const;

public:
  const std::string & dialogId() const;

private:
  void addEvent(const DialogEvent & dialogEvent, DialogKeyFlag dialogKeyFlag, DialogCollateEvents & dialogCollateEvents) const;  
  void addEvent(const std::list<DialogEvent> & dialogEvents, DialogKeyFlag dialogKeyFlag, DialogCollateEvents & dialogCollateEvents) const;
  bool winningNodes(DialogCollatorAndAggregator & dialogCAA
    , const DialogCollateEvents & dialogCollateEvents
    , DialogPriority priority, std::list<DialogEvent> & winningEvents
    , DialogState ignoreState = STATE_INVALID) const;

  void handleMultipleActiveCalls(DialogCollatorAndAggregator & dialogCAA, std::list<DialogEvent> & winningEvents) const;
  bool hasDialogState(const DialogEvent & dialogEvent, DialogState dialogState) const;

private:
  std::string _user;
  std::string _dialogId;
  int _dialogStateFlag;

  DialogCollateEvents _dialogCollatedEvents;
}; // class DialogCollator  

class DialogAggregator
{
public:
  DialogAggregator(const std::string & user);

  void addEvent(const DialogEvent & dialogEvent);
  void addEvent(const std::list<DialogEvent> & dialogEvents);
  
  void aggregateEvent(DialogCollatorAndAggregator & dialogCAA);
  bool mergeEvent(std::string& xml) const;
  std::string toJson() const;
  void toJson(std::string & json) const;

private:
  std::string _user;

  std::list<DialogEvent> _dialogEvents;
}; //class DialogAggregator

class DialogCollatorAndAggregator
{
public:
  typedef std::list<DialogEvent> DialogEvents;
  typedef std::list<DialogCollateEvent> DialogCollatedEvents;

public:
  bool connect(const std::string& password = "", int db = 0);
  void disconnect();
  void flushDb(const std::string & user);

public:
  bool collateAndAggregate(const std::string & user
    , const std::list<std::string> & payloads
    , std::string & xml);

  bool collateAndAggregate(const std::string & user
    , DialogEvents & dialogEvents
    , std::string & xml);

  void syncDialogEvents(const std::string & user
    , DialogPriority dialogPriority
    , DialogEvents & dialogEvents);

  bool saveDialogEvent(const std::string & user
    , const DialogEvent & dialogEvents);

  void saveDialogEvents(const std::string & user
    , const DialogEvents & dialogEvents);

  bool loadDialogEvent(const std::string & user
    , const std::string & key
    , DialogEvent & dialogEvent);

private:

  bool parse(const std::string & user
    , const std::list<std::string> & payloads, DialogEvents & dialogEvents);
  void collate(const std::string & user
    , DialogEvents & dialogEvents, DialogEvents & collatedEvents);
  void aggregate(const std::string & user
    , DialogEvents & dialogEvents, std::string & xml);

  bool loadActiveDialogs(const std::string & user
    , const std::set<std::string> & excludeIds
    , DialogEvents & dialogEvents
    , DialogState state = STATE_INVALID);
  

private:
  OSS::Persistent::RedisClient _redisClient;

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

  static bool processEvents(void* handle, const std::string & user
    , const std::string & domain
    , const std::list<std::string> & payloads
    , std::string & xml);

public:
  static std::string _redisPassword;
  static int _redisDb;
  static bool _useDb;

}; // class DialogCollatorPlugin

/**
 * inline function definition
 */

inline const std::string & DialogCollator::dialogId() const
{
  return _dialogId;
}

} } } // SIPX::Kamailio::Plugin

#endif  /* DIALOGCOLLATOR_H */

