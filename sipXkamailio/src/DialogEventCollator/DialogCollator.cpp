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

#include "DialogEventCollator/DialogCollator.h"
#include <boost/unordered_map.hpp>
#include <boost/algorithm/string.hpp>
#include <OSS/UTL/Logger.h>

#include <vector>
#include <map>
#include <set>

/*
 * TODO:
 * MOH in SIPX Trigger Confirm (Hold) and Terminated (Unhold) Dialog Event
 * during call. The Kamailio DialogCollator currently identify the moh hold and
 * unhold as a new Dialog. Temporary solution is to ignore the state sent by
 * the kamailio if the remote trigger uri is MOH. Need to provide a better
 * solution for handling the moh in kamailio rather than using this hack
 */
#define ENABLE_MOH_FILTER 1

/*Update object version if DialogInfoEvent structure has changed*/
#define DIALOG_OBJECT_VERSION "v2.0.0"

#define DEFAULT_REDIS_QUEUE_DIALOG_TTL 60
#define DEFAULT_REDIS_COLLATE_DIALOG_TTL 3600
#define DEFAULT_REDIS_ACTIVE_DIALOG_TTL 3600

namespace SIPX {
namespace Kamailio {
namespace Plugin {

struct FullKeyGenerator
{
  std::string user;
  std::string domain;

  FullKeyGenerator(const std::string & dialogUser, const std::string & dialogDomain)
    : user(dialogUser)
    , domain(dialogDomain) {}

  std::string operator()(const DialogEvent & dialogEvent) const
  {
    std::ostringstream stream;
    stream << user << "@" << domain << ":" << dialogEvent.callId();
    if(dialogEvent.dialogDirection() == DIRECTION_INITIATOR)
    {
      stream << ":" << dialogEvent.dialogElement().localTag << ":"
        << dialogEvent.dialogElement().remoteTag;
    } else if(dialogEvent.dialogDirection() == DIRECTION_RECIPIENT)
    {
      stream << ":" << dialogEvent.dialogElement().remoteTag << ":"
        << dialogEvent.dialogElement().localTag;
    }

    return stream.str();
  }
}; // struct FullKeyGenerator

struct TagKeyGenerator
{
  std::string user;
  std::string domain;

  TagKeyGenerator(const std::string & dialogUser, const std::string & dialogDomain)
    : user(dialogUser)
    , domain(dialogDomain) {}

  std::string operator()(const DialogEvent & dialogEvent) const
  {
    std::ostringstream stream;
    stream << user << "@" << domain << ":" << dialogEvent.callId();
    if(dialogEvent.dialogDirection() == DIRECTION_INITIATOR)
    {
      stream << ":" << dialogEvent.dialogElement().localTag;
    } else if(dialogEvent.dialogDirection() == DIRECTION_RECIPIENT)
    {
      stream << ":" << dialogEvent.dialogElement().remoteTag;
    }

    return stream.str();
  }
}; // struct TagKeyGenerator

struct DialogKeyGenerator
{
  std::string user;
  std::string domain;

  DialogKeyGenerator(const std::string & dialogUser, const std::string & dialogDomain)
    : user(dialogUser)
    , domain(dialogDomain) {}

  std::string operator()(const DialogEvent & dialogEvent) const
  {
    std::ostringstream stream;
    stream << user << "@" << domain << ":" << dialogEvent.callId();
    return stream.str();
  }
}; // struct DialogKeyGenerator

/**
 * Global Functions
 */

template<class KeyGenerator>
void collateBy(const std::list<const DialogEvent*> & dialogEvents
  , DialogPriority priority
  , std::list<const DialogEvent*> & collatedEvents
  , KeyGenerator keyGenerator);

void collateWinningNode(const std::list<DialogCollateEvent> & dialogEvents
  , DialogPriority priority
  , std::list<const DialogEvent*> & collatedEvents);

bool convertToJson(const DialogEvent & dialogEvent, json::Object& object);
bool convertFromJson(const json::Object& object, DialogEvent & dialogEvent);

/**
 * class DialogCollatorAndAggregator
 */

DialogCollatorAndAggregator::DialogCollatorAndAggregator() 
  : _queueDialogTTL(DEFAULT_REDIS_QUEUE_DIALOG_TTL)
  , _collateDialogTTL(DEFAULT_REDIS_COLLATE_DIALOG_TTL)
  , _activeDialogTTL(DEFAULT_REDIS_ACTIVE_DIALOG_TTL)
{

}

bool DialogCollatorAndAggregator::connect(const std::string& password, int db)
{
  OSS_LOG_INFO("[DialogCollator] Connecting to Redis Server.");
  return _redisClient.connect(password, db);
}

void DialogCollatorAndAggregator::disconnect()
{
  OSS_LOG_INFO("[DialogCollator] Disconnecting to Redis Server.");
  _redisClient.disconnect();
}

bool DialogCollatorAndAggregator::isActive(const std::string & user, const std::string & domain)
{
  std::ostringstream strm;
  strm << user << "@" << domain << ":*:" << DIALOG_OBJECT_VERSION << ":active";

  std::vector<std::string> keys;
  if(_redisClient.getKeys(strm.str(), keys)) {
    return !keys.empty();
  }
      
  return false;
}

bool DialogCollatorAndAggregator::queueDialog(const std::string & user, const std::string & domain
  , const std::string & body)
{
  FullKeyGenerator keyGenerator(user, domain);

  std::string queueBody = body;
  if(queueBody.empty()) {
    // If empty we will generate basic minimal dialog (terminated state)
    DialogEvent::generateMinimalDialog(user, domain, queueBody);  
  }

  DialogEvent dialogEvent(queueBody);
  if(dialogEvent.valid() && dialogEvent.dialogState() != STATE_TRYING)
  {
    std::ostringstream queueKey;
    queueKey << keyGenerator(dialogEvent) << ":" << DIALOG_OBJECT_VERSION << ":queue";

    if(!_redisClient.set(queueKey.str(), dialogEvent.payload(), DialogCollatorPlugin::_queueDialogTTL))
    {
      OSS_LOG_WARNING("[DialogCollatorAndAggregator] Unable to store queue dialog. " 
        << dialogEvent.dialogInfo().entity << ": " << dialogEvent.dialogId());
      return false;
    }

    return true;
  }

  return false;
}

bool DialogCollatorAndAggregator::collateAndAggregate(const std::string & user, const std::string & domain
  , const std::list<std::string> & payloads, std::string & xml)
{
  if(!payloads.empty())
  {
    DialogEvents dialogEvents;
    if(parse(user, domain, payloads, dialogEvents))
    {
      return collateAndAggregate(user, domain, dialogEvents, xml);  
    }
  }
  
  return false;
}

bool DialogCollatorAndAggregator::collateAndAggregate(const std::string & user, const std::string & domain
    , const DialogEvents & dialogEvents, std::string & xml)
{
  DialogEventsPtr dialogEventPtr;
  for(DialogEvents::const_iterator iter = dialogEvents.begin(); iter != dialogEvents.end(); ++iter)
  {
    dialogEventPtr.push_back(&(*iter));
  }

  return collateAndAggregate(user, domain, dialogEventPtr, xml);
}

bool DialogCollatorAndAggregator::collateAndAggregate(const std::string & user, const std::string & domain
    , const DialogEventsPtr & dialogEvents, std::string & xml)
{
  DialogEventsPtr collateDialogEvent;
  collate(user, domain, dialogEvents, collateDialogEvent);
  aggregate(user, domain, collateDialogEvent, xml);
  return !xml.empty();
}

bool DialogCollatorAndAggregator::collateAndAggregateQueue(const std::string & user
    , const std::string & domain, std::string &xml)
{
  DialogEvents queueEvents;
  std::vector<std::string> keys;
  if (loadQueueDialogs(user, domain, keys, queueEvents)) 
  {
    // NOTE:
    // Do we sync from presence terminated dialog to phone active dialog?
    // Do we need to collate queue phone active dialogs?

    // This will sync presence active dialogs to phone terminated dialogs;
    if(queueEvents.empty() || !hasActiveDialogs(queueEvents)) 
    {
      // Generated Minimal Terminated dialogs
      DialogEvent::generateMinimalDialog(user, domain, xml);

      // Force terminate dialogs in the redis db
      std::list<std::string> payloads;
      payloads.push_back(xml);

      std::string tempResult;
      collateAndAggregate(user, domain, payloads, tempResult);
    } else
    {
      if(_activeDialogTTL > 0) 
      {
        refreshActiveDialogs(user, domain);
      }
    }
  }

  // Clean Loaded Queue
  for(std::vector<std::string>::const_iterator iter = keys.begin(); iter != keys.end(); ++iter) {
    _redisClient.del(*iter);
  }

  return !xml.empty();
}

bool DialogCollatorAndAggregator::collateAndAggregateNotify(const std::string & user, const std::string & domain
  , const std::string & body, std::string &xml)
{
  std::string queueBody = body;
  if(queueBody.empty()) {
    // If empty we will generate basic minimal dialog (terminated state)
    DialogEvent::generateMinimalDialog(user, domain, queueBody);  
  }

  DialogEvent dialogEvent(queueBody);

  // NOTE:
  // Do we sync from presence terminated dialog to phone active dialog?
  // Do we need to collate queue phone active dialogs?

  // This will sync presence active dialogs to phone terminated dialogs;
  if(dialogEvent.valid())
  {
    if(dialogEvent.dialogState() == STATE_TERMINATED) 
    {
      // Generated Minimal Terminated dialogs
      DialogEvent::generateMinimalDialog(user, domain, xml);

      // Force terminate dialogs in the redis db
      std::list<std::string> payloads;
      payloads.push_back(xml);

      std::string tempResult;
      collateAndAggregate(user, domain, payloads, tempResult);
    } else 
    {
      if(_activeDialogTTL > 0) 
      {
        refreshActiveDialogs(user, domain);
      }
    }
  }

  return !xml.empty();
}

void DialogCollatorAndAggregator::flushAll()
{
  // This is mostly use for unit test;
  std::vector<std::string> args;
  args.push_back("FLUSHALL");

  std::ostringstream strm;
  _redisClient.execute(args, strm);
}

bool DialogCollatorAndAggregator::parse(const std::string & user, const std::string & domain
  , const std::list<std::string> & payloads, DialogEvents & dialogEvents)
{
  dialogEvents.clear();

  std::set<std::string> dialogIds;
  FullKeyGenerator keyGenerator(user, domain);
  for(std::list<std::string>::const_iterator iter = payloads.begin(); iter != payloads.end(); ++iter)
  {
    DialogEvent dialogEvent((*iter));
    if(dialogEvent.valid() && dialogEvent.dialogState() != STATE_TRYING)
    {
      dialogEvents.push_back(dialogEvent);
      dialogIds.insert(keyGenerator(dialogEvent));
    }
  }

  /** Add Dialog from previous events **/
  for(std::set<std::string>::iterator iter = dialogIds.begin(); iter != dialogIds.end(); ++iter)
  {
    DialogEvent dialogEvent;
    if(loadDialogEvent((*iter), dialogEvent))
    {
      if(dialogEvent.valid() && dialogEvent.dialogState() != STATE_TRYING)
      { 
        dialogEvents.push_back(dialogEvent);
      }
    }
  }

  return !dialogEvents.empty();
}

void DialogCollatorAndAggregator::collate(const std::string & user, const std::string & domain
  , const DialogEventsPtr & dialogEvents, DialogEventsPtr & collatedEvents)
{
  FullKeyGenerator keyGenerator(user, domain);
  collateBy(dialogEvents, PRIORITY_BY_CALL, collatedEvents, keyGenerator);

  /**
   * Save full collated state to redis
   */
  for(DialogEventsPtr::iterator iter = collatedEvents.begin(); iter != collatedEvents.end(); ++iter)
  {
    std::string key = keyGenerator(*(*iter));
    if(!saveDialogEvent(key, *(*iter)))
    {
      OSS_LOG_WARNING("[DialogCollatorAndAggregator] Failed to save dialog event: " 
        << (*iter)->dialogId() << " : " << (*iter)->dialogState());
    }
  }

  collateBy(collatedEvents, PRIORITY_BY_DIALOG, collatedEvents, TagKeyGenerator(user, domain));
  collateBy(collatedEvents, PRIORITY_BY_DIALOG, collatedEvents, DialogKeyGenerator(user, domain));
}

void DialogCollatorAndAggregator::aggregate(const std::string & user, const std::string & domain
  , const DialogEventsPtr & dialogEvents, std::string & xml)
{
  DialogEvents activeEvents;
  DialogAggregateEvent aggregateEvent;

  // Add collated events
  aggregateEvent.addEvent(dialogEvents);

  // Filter current dialog ids sent by kamailio
  std::set<std::string> filterIds;
  for(DialogEventsPtr::const_iterator iter = dialogEvents.begin()
    ; iter != dialogEvents.end()
    ; ++iter)
  {
    filterIds.insert((*iter)->dialogId());
  }

  // Load active dialog filtering current dialog ids
  if(loadActiveDialogs(user, domain, filterIds, activeEvents, STATE_TERMINATED))
  {
    aggregateEvent.addEvent(activeEvents);
  }

  // Aggegate dialog events
  aggregateEvent.mergeEvent(xml);
}  

bool DialogCollatorAndAggregator::saveDialogEvent(const std::string & key, const DialogEvent & dialogEvent)
{
  std::ostringstream dialogKey;
  dialogKey << key << ":" << DIALOG_OBJECT_VERSION << ":dialog";
  if(_redisClient.set(dialogKey.str(), dialogEvent.payload(), _collateDialogTTL))
  {
    std::ostringstream activeKey;
    activeKey << key << ":" << DIALOG_OBJECT_VERSION << ":active";

    int state = dialogEvent.dialogState();
    if(state >= STATE_EARLY && state <= STATE_CONFIRMED)
    {
      if(!_redisClient.set(activeKey.str(), dialogEvent.payload(), _activeDialogTTL))
      {
        OSS_LOG_WARNING("[DialogCollatorAndAggregator] Unable to store active dialog. " << dialogEvent.dialogId());
      }  
    } else
    {
      if(!_redisClient.del(activeKey.str()))
      {
        OSS_LOG_WARNING("[DialogCollatorAndAggregator] Unable to remove active dialog. " << dialogEvent.dialogId());
      }
    }

    return true;
  }

  return false;
}

bool DialogCollatorAndAggregator::loadDialogEvent(const std::string & key, DialogEvent & dialogEvent)
{
  std::ostringstream strm;
  strm << key << ":" << DIALOG_OBJECT_VERSION << ":dialog";

  std::string payload;
  if(_redisClient.get(strm.str(), payload)) {
    dialogEvent.parse(payload);
    return dialogEvent.valid();
  }
      
  return false;
}

bool DialogCollatorAndAggregator::loadActiveDialogs(const std::string & user, const std::string & domain
  , const std::set<std::string> & filterIds, DialogEvents & dialogEvents, DialogState dialogState)
{
  FullKeyGenerator keyGenerator(user, domain);

  std::ostringstream strm;
  strm << user << "@" << domain << ":*:" << DIALOG_OBJECT_VERSION << ":active";

  std::vector<std::string> payloads;
  if(_redisClient.getAll(payloads, strm.str())) {
    for(std::vector<std::string>::const_iterator iter = payloads.begin(); iter != payloads.end(); ++iter)
    {
      DialogEvent dialogEvent((*iter), dialogState);
      if(dialogEvent.valid() && dialogEvent.dialogState() != STATE_TRYING)
      {
        // Check if filtered
        if(filterIds.find(dialogEvent.dialogId()) == filterIds.end())
        {
          dialogEvents.push_back(dialogEvent);
          saveDialogEvent(keyGenerator(dialogEvent), dialogEvent);
        }
      }
    }
  }
      
  return !dialogEvents.empty();
}

void DialogCollatorAndAggregator::refreshActiveDialogs(const std::string & user, const std::string & domain)
{
  std::ostringstream strm;
  strm << user << "@" << domain << ":*:" << DIALOG_OBJECT_VERSION << ":active";

  std::vector<std::string> keys;
  if(_redisClient.getKeys(strm.str(), keys)) {
    for(std::vector<std::string>::const_iterator iter = keys.begin(); iter != keys.end(); ++iter) {
      std::string value;
      if (_redisClient.get(*iter, value)) {
        if(!_redisClient.set(*iter, value, _activeDialogTTL))
        {
          OSS_LOG_WARNING("[DialogCollatorAndAggregator] Unable to refresh active dialog. " << user << "@" << domain);
        } 
      }
    }
  }
}

bool DialogCollatorAndAggregator::loadQueueDialogs(const std::string & user, const std::string & domain
  , std::vector<std::string> & keys, DialogEvents & dialogEvents)
{
  std::ostringstream strm;
  strm << user << "@" << domain << "*" << DIALOG_OBJECT_VERSION << ":queue";

  if(_redisClient.getKeys(strm.str(), keys)) {
    for(std::vector<std::string>::const_iterator iter = keys.begin(); iter != keys.end(); ++iter) {
      std::string value;
      if (_redisClient.get(*iter, value)) {
        DialogEvent dialogEvent(value);
        if(dialogEvent.valid() && dialogEvent.dialogState() != STATE_TRYING)
        {
          dialogEvents.push_back(dialogEvent);
        }
      }
    }
  }
      
  return !dialogEvents.empty();
}

bool DialogCollatorAndAggregator::hasActiveDialogs(const DialogEvents & dialogEvents)
{
  for(DialogEvents::const_iterator iter = dialogEvents.begin(); iter != dialogEvents.end(); ++iter)
  {
    if(iter->dialogState() > STATE_TRYING && iter->dialogState() < STATE_TERMINATED) {
      return true;
    }
  }

  return false;
}

/**
 * class DialogCollatorPlugin
 */   

/*static*/ std::string DialogCollatorPlugin::_redisPassword = "";
/*static*/ int DialogCollatorPlugin::_queueDialogTTL = DEFAULT_REDIS_QUEUE_DIALOG_TTL;
/*static*/ int DialogCollatorPlugin::_collateDialogTTL = DEFAULT_REDIS_COLLATE_DIALOG_TTL;
/*static*/ int DialogCollatorPlugin::_activeDialogTTL = DEFAULT_REDIS_ACTIVE_DIALOG_TTL;
/*static*/ int DialogCollatorPlugin::_redisDb = 0;

void DialogCollatorPlugin::start(const PluginSettings & settings)
{
  int logPriority = OSS::PRIO_ERROR;
  if(settings.find("log-level") != settings.end())
  {
    try 
    {
      logPriority = boost::lexical_cast<int>(settings.at("log-level"));
      OSS_LOG_DEBUG("[DialogCollatorPlugin] Plugin Settings - log-level: " << logPriority);
    } catch(std::exception& ex)
    {
      //Revert any settings to error
      logPriority = OSS::PRIO_ERROR;
    }
  }

  std::string logPath;
  if(settings.find("log-path") != settings.end())
  {
    logPath = settings.at("log-path");

    OSS_LOG_DEBUG("[DialogCollatorPlugin] Plugin Settings - log-path: " << logPath);
  }

  if(settings.find("redis-db") != settings.end())
  {
    try 
    {
      _redisDb = boost::lexical_cast<int>(settings.at("redis-db"));
      OSS_LOG_DEBUG("[DialogCollatorPlugin] Plugin Settings - redis-db: " << _redisDb);
    } catch(std::exception& ex)
    {
      //Revert any settings to zero
      _redisDb = 0;
    }
  }

  if(settings.find("redis-password") != settings.end())
  {
    _redisPassword = settings.at("redis-password");

    OSS_LOG_DEBUG("[DialogCollatorPlugin] Plugin Settings - redis-password: " << _redisPassword);
  }

  if(settings.find("queue-dialog-ttl") != settings.end())
  {
    try 
    {
      _queueDialogTTL = boost::lexical_cast<int>(settings.at("queue-dialog-ttl"));
      OSS_LOG_DEBUG("[DialogCollatorPlugin] Plugin Settings - queue-dialog-ttl: " << _queueDialogTTL);
    } catch(std::exception& ex)
    {
      //Revert any settings default to seconds
      _queueDialogTTL = DEFAULT_REDIS_QUEUE_DIALOG_TTL;
    }
  }

  if(settings.find("collate-dialog-ttl") != settings.end())
  {
    try 
    {
      _collateDialogTTL = boost::lexical_cast<int>(settings.at("collate-dialog-ttl"));
      OSS_LOG_DEBUG("[DialogCollatorPlugin] Plugin Settings - collate-dialog-ttl: " << _collateDialogTTL);
    } catch(std::exception& ex)
    {
      //Revert any settings default to seconds
      _collateDialogTTL = DEFAULT_REDIS_COLLATE_DIALOG_TTL;
    }
  }

  if(settings.find("active-dialog-ttl") != settings.end())
  {
    try 
    {
      _activeDialogTTL = boost::lexical_cast<int>(settings.at("active-dialog-ttl"));
      OSS_LOG_DEBUG("[DialogCollatorPlugin] Plugin Settings - active-dialog-ttl: " << _activeDialogTTL);
    } catch(std::exception& ex)
    {
      //Revert any settings default to seconds
      _activeDialogTTL = DEFAULT_REDIS_ACTIVE_DIALOG_TTL;
    }
  }

  // Configure logger
  if(!logPath.empty())
  {
    try
    {
      OSS::logger_init(logPath, static_cast<OSS::LogPriority>(logPriority));
    } catch(std::exception & ex)
    {
      //Don't returned error here other plugin may already init oss logger
    }
  }

  OSS_LOG_INFO("[DialogCollatorPlugin] Start DialogCollatorPlugin");
}

void DialogCollatorPlugin::stop()
{
  OSS::logger_deinit();
  OSS_LOG_INFO("[DialogCollatorPlugin] Stop DialogCollatorPlugin");
}

void DialogCollatorPlugin::create(void** handle)
{
  DialogCollatorAndAggregator * dialogHandle = new DialogCollatorAndAggregator();
  if(dialogHandle != NULL)
  {
    dialogHandle->queueDialogTTL(_queueDialogTTL);
    dialogHandle->collateDialogTTL(_collateDialogTTL);
    dialogHandle->activeDialogTTL(_activeDialogTTL);
    dialogHandle->connect(_redisPassword, _redisDb);
    *handle = reinterpret_cast<void*>(dialogHandle);
    OSS_LOG_DEBUG("[DialogCollatorPlugin] Create DialogCollatorPlugin");  
  }
}

void DialogCollatorPlugin::destroy(void** handle)
{
  if(handle && *handle)
  {
    DialogCollatorAndAggregator * dialogHandle = reinterpret_cast<DialogCollatorAndAggregator*>(*handle);
    dialogHandle->disconnect();

    delete dialogHandle;
    *handle = NULL;

    OSS_LOG_DEBUG("[DialogCollatorPlugin] Destroy DialogCollatorPlugin");
  }
}  

bool DialogCollatorPlugin::isActive(void* handle
    , const std::string & user
    , const std::string & domain)
{
  DialogCollatorAndAggregator * dialogHandle = handle 
    ? reinterpret_cast<DialogCollatorAndAggregator*>(handle)
    : NULL;
  if(dialogHandle)
  {
    return dialogHandle->isActive(user, domain);
  }

  return false;
}

bool DialogCollatorPlugin::queueDialog(void* handle, const std::string & user
    , const std::string & domain, const std::string & body)
{
  DialogCollatorAndAggregator * dialogHandle = handle 
    ? reinterpret_cast<DialogCollatorAndAggregator*>(handle)
    : NULL;
  if(dialogHandle)
  {
    return dialogHandle->queueDialog(user, domain, body);
  }

  return false;
}

bool DialogCollatorPlugin::collateQueue(void* handle, const std::string & user
    , const std::string & domain, std::string & xml)
{
  DialogCollatorAndAggregator * dialogHandle = handle 
    ? reinterpret_cast<DialogCollatorAndAggregator*>(handle)
    : NULL;
  if(dialogHandle)
  {
    return dialogHandle->collateAndAggregateQueue(user, domain, xml);
  }

  return false;
}

bool DialogCollatorPlugin::collateNotify(void* handle, const std::string & user
  , const std::string & domain, const std::string & body, std::string & xml)
{
  DialogCollatorAndAggregator * dialogHandle = handle 
    ? reinterpret_cast<DialogCollatorAndAggregator*>(handle)
    : NULL;
  if(dialogHandle)
  {
    return dialogHandle->collateAndAggregateNotify(user, domain, body, xml);
  }

  return false;
}

bool DialogCollatorPlugin::processEvents(void* handle
    , const std::string & user
    , const std::string & domain
    , const std::list<std::string> & payloads
    , std::string & xml)
{
  DialogCollatorAndAggregator * dialogHandle = handle 
    ? reinterpret_cast<DialogCollatorAndAggregator*>(handle)
    : NULL;
  if(dialogHandle)
  {
    dialogHandle->collateAndAggregate(user, domain, payloads, xml);
  }

  return !xml.empty();
}

/**
 * Global Function Definition
 */

template<class KeyGenerator>
void collateBy(const std::list<const DialogEvent*> & dialogEvents
  , DialogPriority priority
  , std::list<const DialogEvent*> & collatedEvents
  , KeyGenerator keyGenerator)
{
  std::list<DialogCollateEvent> dialogCollateEvents;

  for(std::list<const DialogEvent*>::const_iterator iter = dialogEvents.begin(); iter != dialogEvents.end(); ++iter)
  {
    std::string dialogKey = keyGenerator(*(*iter));
    std::list<DialogCollateEvent>::iterator collateIter = std::find_if(dialogCollateEvents.begin()
      , dialogCollateEvents.end(), DialogCollateEventFinder(dialogKey));
    if(collateIter != dialogCollateEvents.end())
    {
      collateIter->addEvent(*iter);
    } else 
    {
      DialogCollateEvent newCollateEvent(dialogKey);
      newCollateEvent.addEvent(*iter);
      dialogCollateEvents.push_back(newCollateEvent);
    }
  }

  collateWinningNode(dialogCollateEvents, priority, collatedEvents);
}

void collateWinningNode(const std::list<DialogCollateEvent> & dialogEvents
  , DialogPriority priority
  , std::list<const DialogEvent*> & collatedEvents)
{
  collatedEvents.clear();

  for(std::list<DialogCollateEvent>::const_iterator iter = dialogEvents.begin()
    ; iter != dialogEvents.end(); ++iter)
  {
    const DialogEvent * ptr = iter->winningNode(priority);
    if(ptr != NULL) 
    {
      collatedEvents.push_back(ptr);
    }
  }
}

} } } // Namespace SIPX::Kamailio::Plugin

