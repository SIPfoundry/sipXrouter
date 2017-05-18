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

namespace SIPX {
namespace Kamailio {
namespace Plugin {

/**
 * Global Functions
 */

std::string createRedisKey(const std::string & user, const std::string & key);

void collateDialog(const std::string & user 
  , const std::list<DialogEvent> & dialogEvents
  , std::list<DialogCollator> & dialogCollators);

void collateWinningEvent(DialogCollatorAndAggregator & dialogCAA
  , const std::list<DialogCollator> & dialogCollators
  , std::list<DialogEvent> & winningEvents);

/**
 * Global Class Utils
 */

class DialogEventFinder
{
public:
  DialogEventFinder(const DialogEvent & dialogEvent)
    : _dialogKey(dialogEvent.generateKey(KEY_DIALOG_ID)) 
    , _dialogState(dialogEvent.dialogState()) {}

  DialogEventFinder(const std::string & dialogKey, DialogState dialogState)
    : _dialogKey(dialogKey)
    , _dialogState(dialogState) {}

  bool operator() (const DialogEvent & dialogEvent) const 
  { 
     return _dialogKey == dialogEvent.generateKey(KEY_DIALOG_ID)
            && _dialogState == dialogEvent.dialogState();
  }

private:
  std::string _dialogKey;
  DialogState _dialogState;
}; // class DialogEventFinder

class DialogCollateEventFinder
{
public:
  DialogCollateEventFinder(const std::string & dialogKey) 
    : _dialogKey(dialogKey) {}

  bool operator() (const DialogCollateEvent & collateEvent) const 
  { 
     return _dialogKey == collateEvent.dialogKey();
  }

private:
  const std::string & _dialogKey;
}; // DialogCollateEventFinder

class DialogCollateStateFinder
{
public:
  DialogCollateStateFinder(const std::string & dialogKey, DialogState dialogState) 
    : _dialogKey(dialogKey) 
    , _dialogState(dialogState) {}

  bool operator() (const DialogCollateEvent & collateEvent) const 
  { 
     return _dialogKey == collateEvent.dialogKey() && collateEvent.hasEvent(_dialogState) ;
  }

private:
  const std::string & _dialogKey;
  DialogState _dialogState;
}; // DialogCollateStateFinder

class DialogCollatorFinder
{
public:
  DialogCollatorFinder(const std::string & dialogId) 
    : _dialogId(dialogId) {}

  bool operator() (const DialogCollator & collator) const 
  { 
     return _dialogId == collator.dialogId();
  }

private:
  const std::string & _dialogId;
}; // DialogCollatorFinder

/**
 * class DialogCollator
 */

DialogCollator::DialogCollator(const std::string & user, const std::string & dialogId)
  : _user(user)
  , _dialogId(dialogId)
  , _dialogStateFlag(0)
{

}

void DialogCollator::addEvent(const DialogEvent & dialogEvent)
{
  addEvent(dialogEvent, KEY_DIALOG_ID, _dialogCollatedEvents);
  _dialogStateFlag |= dialogEvent.dialogState();
}

void DialogCollator::addEvent(const std::list<DialogEvent> & dialogEvents)
{
  for(std::list<DialogEvent>::const_iterator iter = dialogEvents.begin()
    ; iter != dialogEvents.end(); ++iter)
  {
    addEvent(*iter);
  }
}

bool DialogCollator::winningNode(DialogCollatorAndAggregator & dialogCAA
  , DialogEvent & dialogEvent) const
{
  std::list<DialogEvent> winningList;
  if(winningNodes(dialogCAA, winningList))
  {
    dialogEvent = winningList.back();
    return true;
  }

  return false;
}

bool DialogCollator::winningNodes(DialogCollatorAndAggregator & dialogCAA
  , std::list<DialogEvent> & winningEvents) const
{
  OSS_LOG_DEBUG("[DialogCollator] Start Selecting Winning Node: " << dialogId());

  if(winningNodes(dialogCAA, _dialogCollatedEvents, PRIORITY_BY_CALL, winningEvents))
  {
    dialogCAA.syncDialogEvents(_user, PRIORITY_BY_CALL, winningEvents);
    dialogCAA.saveDialogEvents(_user, winningEvents);

    DialogCollateEvents dialogCollatedEvents;

    /*
     * If there are a confirmed state per dialog, ignore early state. This is due
     * to fork calls. Kamailio may or may not send terminated state for each leg
     */
    DialogState ignoreState = _dialogStateFlag & STATE_CONFIRMED ? STATE_EARLY : STATE_INVALID;
    addEvent(winningEvents, KEY_LOCAL_ID, dialogCollatedEvents);
    winningNodes(dialogCAA, dialogCollatedEvents, PRIORITY_BY_DIALOG, winningEvents, ignoreState);   

    addEvent(winningEvents, KEY_CALL_ID, dialogCollatedEvents);
    winningNodes(dialogCAA, dialogCollatedEvents, PRIORITY_BY_DIALOG, winningEvents);  
  }
    
  return !winningEvents.empty();
}

void DialogCollator::addEvent(const DialogEvent & dialogEvent, DialogKeyFlag dialogKeyFlag
  , DialogCollateEvents & dialogCollateEvents) const
{
  std::string dialogKey = dialogEvent.generateKey(dialogKeyFlag);
  std::list<DialogCollateEvent>::iterator collateIter = std::find_if(dialogCollateEvents.begin()
    , dialogCollateEvents.end(), DialogCollateEventFinder(dialogKey));
  if(collateIter != dialogCollateEvents.end())
  {
    collateIter->addEvent(dialogEvent);
  } else 
  {
    DialogCollateEvent newCollateEvent(dialogKey, dialogKeyFlag);
    newCollateEvent.addEvent(dialogEvent);
    dialogCollateEvents.push_back(newCollateEvent);
  }
}

void DialogCollator::addEvent(const std::list<DialogEvent> & dialogEvents, DialogKeyFlag dialogKeyFlag
  , DialogCollateEvents & dialogCollateEvents) const
{
  for(std::list<DialogEvent>::const_iterator iter = dialogEvents.begin()
    ; iter != dialogEvents.end(); ++iter)
  {
    addEvent(*iter, dialogKeyFlag, dialogCollateEvents);
  }
}

bool DialogCollator::winningNodes(DialogCollatorAndAggregator & dialogCAA
    , const DialogCollateEvents & dialogCollateEvents
    , DialogPriority priority, std::list<DialogEvent> & winningEvents
    , DialogState ignoreState) const
{
  winningEvents.clear();

  for(std::list<DialogCollateEvent>::const_iterator iter = dialogCollateEvents.begin()
    ; iter != dialogCollateEvents.end(); ++iter)
  {
    std::list<DialogEvent> winningNodes;
    if(iter->winningNodes(priority, winningNodes, ignoreState)) 
    {
      /*
       * for fork calls and redirecting to IVR. This may cause one dialog to be unterminated state.
       * this will fix multiple active calls.
       */
      handleMultipleActiveCalls(dialogCAA, winningNodes);

      OSS_LOG_DEBUG("[DialogCollator] DialogCollator: Winning Candidate Node: " 
                  << winningNodes.back().dialogId() 
                  << " : state -> " << winningNodes.back().dialogState()
                  << " : json -> " << winningNodes.back().toJson());
      winningEvents.push_back(winningNodes.back());
    }
  }

  return !winningEvents.empty();
}

void DialogCollator::handleMultipleActiveCalls(DialogCollatorAndAggregator & dialogCAA, std::list<DialogEvent> & winningEvents) const
{
  if(winningEvents.size() > 1 && winningEvents.back().dialogState() == STATE_CONFIRMED)
  {
    OSS_LOG_WARNING("[DialogCollator] Found multiple active calls: " << _dialogId     
              << " : count -> " << winningEvents.size());

    /**
     * There should be at lease a new dialog if there are multiple active calls.
     */
    int terminateCount = 0;
    std::list<DialogEvent>::iterator activeIter = winningEvents.end();
    for(std::list<DialogEvent>::iterator iter = winningEvents.begin()
        ; iter != winningEvents.end()
        ; ++iter)
    {
      if(!iter->newDialog())
      {
        OSS_LOG_DEBUG("[DialogCollator] DialogCollator: Force terminate existing active dialog: " 
                  << iter->dialogId() 
                  << " : state -> " << iter->dialogState()
                  << " : json -> " << iter->toJson());
        iter->updateDialogState(STATE_TERMINATED);
        dialogCAA.saveDialogEvent(_user, *iter);
        terminateCount++;
      } else 
      {
        /**
         * This shouldn't happened. But in case it does, mark previous
         * active dialog as terminated
         */
        if(activeIter != winningEvents.end())
        {
          OSS_LOG_WARNING("[DialogCollator] DialogCollator: "
                  << "Found more than one new active dialog. "
                  << "Force terminate new active dialog: " 
                  << activeIter->dialogId() 
                  << " : state -> " << activeIter->dialogState()
                  << " : json -> " << activeIter->toJson());
          activeIter->updateDialogState(STATE_TERMINATED);
          dialogCAA.saveDialogEvent(_user, *activeIter);
        }
        
        activeIter = iter;
      }
    }

    if(activeIter != winningEvents.end())
    {
      /**
       * Just in case move new dialog to the end of the list
       */
      winningEvents.splice(winningEvents.end(), winningEvents, activeIter);

      OSS_LOG_WARNING("[DialogCollator] Terminated multiple active calls. Current Active Dialog: "
              << activeIter->dialogId()
              << " : state -> " << activeIter->dialogState()
              << " : json -> " << activeIter->toJson());
    } else
    {
      OSS_LOG_ERROR("[DialogCollator] No new active calls found (Unable to handle multiple active calls): " << _dialogId);      
    }
  } 
}

bool DialogCollator::hasDialogState(const DialogEvent & dialogEvent, DialogState dialogState) const
{
  std::string dialogKey = dialogEvent.generateKey(KEY_DIALOG_ID);
  std::list<DialogCollateEvent>::const_iterator collateIter = std::find_if(_dialogCollatedEvents.begin()
    , _dialogCollatedEvents.end(), DialogCollateStateFinder(dialogKey, dialogState));
  return collateIter != _dialogCollatedEvents.end();
}

/**
 * class DialogAggregator
 */

DialogAggregator::DialogAggregator(const std::string & user)
  : _user(user)
{

}

void DialogAggregator::addEvent(const DialogEvent & dialogEvent)
{
  _dialogEvents.push_back(dialogEvent);
}

void DialogAggregator::addEvent(const std::list<DialogEvent> & dialogEvents)
{
  _dialogEvents.insert(_dialogEvents.end(), dialogEvents.begin(), dialogEvents.end());
}

void DialogAggregator::aggregateEvent(DialogCollatorAndAggregator & dialogCAA)
{
  bool foundActive = false;

  for(std::list<DialogEvent>::reverse_iterator iter = _dialogEvents.rbegin()
          ; iter != _dialogEvents.rend()
          ; ++iter)
  {
    if(iter->dialogState() == STATE_EARLY || iter->dialogState() == STATE_CONFIRMED)
    {
      if(foundActive)
      {
        iter->updateDialogState(STATE_TERMINATED);
        dialogCAA.saveDialogEvent(_user, *iter);  
      } else
      {
        foundActive = true;
      }
    }
  }
}

bool DialogAggregator::mergeEvent(std::string& xml) const
{
  return DialogEvent::mergeDialogEvent(_dialogEvents, xml);
}

std::string DialogAggregator::toJson() const
{
  std::string json;
  toJson(json);
  return json;
}

void DialogAggregator::toJson(std::string & json) const
{
  std::ostringstream strm;
  strm << "{\"aggregatedEvents\":[";
  for(std::list<DialogEvent>::const_iterator iter = _dialogEvents.begin()
          ; iter != _dialogEvents.end()
          ; ++iter)
  {
    const DialogEvent & dialogEvent = *iter;
    if(iter != _dialogEvents.begin()) 
    {
      strm << ",";
    }

    strm << dialogEvent.toJson();
  }
  strm << "]}";
  json = strm.str();
}

/**
 * class DialogCollatorAndAggregator
 */

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

void DialogCollatorAndAggregator::flushDb(const std::string & user)
{
  std::vector<std::string> keys;
  if(_redisClient.getKeys(user + ":*:" + DIALOG_OBJECT_VERSION + ":*", keys))
  {
    for(std::vector<std::string>::iterator iter = keys.begin(); iter != keys.end(); ++iter)
    {
      _redisClient.del(*iter);
    }
  }
}

bool DialogCollatorAndAggregator::collateAndAggregate(const std::string & user
  , const std::list<std::string> & payloads
  , std::string & xml)
{
  if(!payloads.empty())
  {
    DialogEvents dialogEvents;
    if(parse(user, payloads, dialogEvents))
    {
      return collateAndAggregate(user, dialogEvents, xml);  
    }
  }
  
  return false;
}

bool DialogCollatorAndAggregator::collateAndAggregate(const std::string & user
  , DialogEvents & dialogEvents, std::string & xml)
{
  DialogEvents collateDialogEvent;
  collate(user, dialogEvents, collateDialogEvent);
  aggregate(user, collateDialogEvent, xml);
  return !xml.empty();
}

void DialogCollatorAndAggregator::syncDialogEvents(const std::string & user
  , DialogPriority dialogPriority, DialogEvents & dialogEvents)
{
  for(DialogEvents::iterator iter = dialogEvents.begin(); iter != dialogEvents.end(); ++iter)
  {
    DialogEvent & localEvent = *iter;

    DialogEvent remoteEvent;
    if(loadDialogEvent(user, localEvent.generateKey(KEY_DIALOG_ID), remoteEvent) 
      && localEvent.dialogState() != remoteEvent.dialogState())
    {
      DialogCollateEvent dialogCollateEvent(localEvent.dialogId());
      dialogCollateEvent.addEvent(localEvent);
      dialogCollateEvent.addEvent(remoteEvent);
      
      DialogEvent winningNode;
      if(dialogCollateEvent.winningNode(dialogPriority, winningNode))
      {
        if(localEvent.dialogState() != winningNode.dialogState()) 
        {
          localEvent = winningNode;

          if(!saveDialogEvent(user, winningNode))
          {
            OSS_LOG_ERROR("[DialogCollatorAndAggregator] Failed to save dialog event: " 
              << iter->dialogId() << " : " << iter->dialogState());
          }  
        }
      }

      //Mark this dialog as existing dialog
      localEvent.newDialog(false);
    }
  }
}

bool DialogCollatorAndAggregator::saveDialogEvent(const std::string & user, const DialogEvent & dialogEvent)
{
  std::string dialogKey = createRedisKey(user, dialogEvent.generateKey(KEY_DIALOG_ID));
  
  std::ostringstream redisKey;
  redisKey << dialogKey << ":" << DIALOG_OBJECT_VERSION << ":dialog";
  if(_redisClient.set(redisKey.str(), dialogEvent.payload(), 3600))
  {
    std::ostringstream activeKey;
    activeKey << dialogKey << ":" << DIALOG_OBJECT_VERSION << ":active";

    int state = dialogEvent.dialogState();
    if(state >= STATE_EARLY && state <= STATE_CONFIRMED)
    {
      if(!_redisClient.set(activeKey.str(), dialogEvent.payload(), 3600))
      {
        OSS_LOG_WARNING("[DialogCollatorAndAggregator] Unable to store active dialog. " << dialogEvent.dialogId());
      }  
    } else
    {
      _redisClient.del(activeKey.str());
    }

    return true;
  }

  return false;
}

void DialogCollatorAndAggregator::saveDialogEvents(const std::string & user
  , const DialogEvents & dialogEvents)
{
  /**
   * Save full collated state to redis
   */
  for(DialogEvents::const_iterator iter = dialogEvents.begin(); iter != dialogEvents.end(); ++iter)
  {
    if(!saveDialogEvent(user, *iter))
    {
      OSS_LOG_ERROR("[DialogCollatorAndAggregator] Failed to save dialog event: " 
        << iter->dialogId() << " : " << iter->dialogState());
    }
  }
}

bool DialogCollatorAndAggregator::loadDialogEvent(const std::string & user, const std::string & key, DialogEvent & dialogEvent)
{
  std::string dialogKey = createRedisKey(user, key);

  std::ostringstream strm;
  strm << dialogKey << ":" << DIALOG_OBJECT_VERSION << ":dialog";

  std::string payload;
  if(_redisClient.get(strm.str(), payload)) {
    dialogEvent.parse(payload);
    return dialogEvent.valid();
  }
      
  return false;
}


bool DialogCollatorAndAggregator::parse(const std::string & user
  , const std::list<std::string> & payloads
  , DialogEvents & dialogEvents)
{
  dialogEvents.clear();

  for(std::list<std::string>::const_iterator iter = payloads.begin()
    ; iter != payloads.end(); ++iter)
  {
    DialogEvent dialogEvent;
    dialogEvent.parse(*iter);
    if(dialogEvent.valid() 
      && dialogEvent.dialogState() != STATE_INVALID
      && dialogEvent.dialogState() != STATE_TRYING)
    {

#ifdef ENABLE_MOH_FILTER
      if(!dialogEvent.dialogElement().remoteTarget.empty() && 
          dialogEvent.dialogElement().remoteTarget.find("~~mh~") != std::string::npos)
      {
        OSS_LOG_WARNING("[DialogCollator] Collate: Ignoring state for moh event: " 
                  << dialogEvent.dialogId() << " : state -> " << dialogEvent.dialogState());
        continue;
      }
#endif

      /**
       * Add only if dialog id doesn't exist.
       */
      DialogEvents::iterator findIter = std::find_if(dialogEvents.begin(), dialogEvents.end()
        , DialogEventFinder(dialogEvent));
      if(findIter == dialogEvents.end())
      {
        dialogEvents.push_back(dialogEvent);  
      }
    }
  }

  return !dialogEvents.empty();
}

void DialogCollatorAndAggregator::collate(const std::string & user
  , DialogEvents & dialogEvents, DialogEvents & collatedEvents)
{
  std::list<DialogCollator> dialogCollators;

  collateDialog(user, dialogEvents, dialogCollators);
  collateWinningEvent(*this, dialogCollators, collatedEvents);
}

void DialogCollatorAndAggregator::aggregate(const std::string & user
  , DialogEvents & dialogEvents, std::string & xml)
{
  DialogAggregator dialogAggregator(user);

  // Add collated events
  dialogAggregator.addEvent(dialogEvents);

  // Filter current dialog ids sent by kamailio
  std::set<std::string> excludeIds;
  for(DialogEvents::const_iterator iter = dialogEvents.begin()
    ; iter != dialogEvents.end()
    ; ++iter)
  {
    excludeIds.insert(iter->dialogId());
  }

  // Load active dialog filtering current dialog ids and marking it TERMINATED
  DialogEvents activeEvents;
  if(loadActiveDialogs(user, excludeIds, activeEvents, STATE_TERMINATED))
  {
    for(DialogEvents::iterator iter = activeEvents.begin(); iter != activeEvents.end(); ++iter)
    {
      dialogAggregator.addEvent(*iter);
    }
  }

  // Aggegate dialog events
  // Make sure only one active event is present
  dialogAggregator.aggregateEvent(*this);

  if (OSS::log_get_level() >= OSS::PRIO_DEBUG)
  {
    OSS_LOG_DEBUG("[DialogCollatorAndAggregator] Final Aggregate Result " << user << ": " << dialogAggregator.toJson());
  }

  dialogAggregator.mergeEvent(xml);
}

bool DialogCollatorAndAggregator::loadActiveDialogs(const std::string & user
  , const std::set<std::string> & excludeIds, DialogEvents & dialogEvents
  , DialogState dialogState)
{
  std::ostringstream strm;
  strm << user << ":*:" << DIALOG_OBJECT_VERSION << ":active";

  std::vector<std::string> payloads;
  if(_redisClient.getAll(payloads, strm.str())) 
  {
    for(std::vector<std::string>::const_iterator iter = payloads.begin(); iter != payloads.end(); ++iter)
    {
      DialogEvent dialogEvent;
      dialogEvent.parse((*iter), dialogState);
      if(dialogEvent.valid())
      {
        // Check if not excluded
        if(excludeIds.find(dialogEvent.dialogId()) == excludeIds.end())
        {
          OSS_LOG_INFO("[DialogCollatorAndAggregator] Force terminate remote state: " 
            << dialogEvent.dialogId() 
            << " : " << dialogEvent.dialogState());

          dialogEvents.push_back(dialogEvent);
          saveDialogEvent(user, dialogEvent);
        }
      }
    }
  }
      
  return !dialogEvents.empty();
}

/**
 * class DialogCollatorPlugin
 */   

/*static*/ std::string DialogCollatorPlugin::_redisPassword = "";
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
    std::ostringstream stream;
    stream << user << "@" << domain;
    dialogHandle->collateAndAggregate(stream.str(), payloads, xml);
  }

  return !xml.empty();
}

/**
 * Global Function Definition
 */

std::string createRedisKey(const std::string & user
  , const std::string & key)
{
  std::ostringstream stream;
  stream << user << ":" << key;
  return stream.str();
}

void collateDialog(const std::string & user
  , const std::list<DialogEvent> & dialogEvents
  , std::list<DialogCollator> & dialogCollators)
{
  dialogCollators.clear();

  for(std::list<DialogEvent>::const_iterator iter = dialogEvents.begin(); iter != dialogEvents.end(); ++iter)
  {
    std::string dialogId = iter->dialogId();
    std::list<DialogCollator>::iterator collateIter = std::find_if(dialogCollators.begin()
      , dialogCollators.end(), DialogCollatorFinder(dialogId));
    if(collateIter != dialogCollators.end())
    {
      collateIter->addEvent(*iter);
    } else 
    {
      DialogCollator newCollatorEvent(user, dialogId);
      newCollatorEvent.addEvent(*iter);
      dialogCollators.push_back(newCollatorEvent);
    }
  }
}

void collateWinningEvent(DialogCollatorAndAggregator & dialogCAA
  , const std::list<DialogCollator> & dialogCollators
  , std::list<DialogEvent> & winningEvents)
{
  winningEvents.clear();

  for(std::list<DialogCollator>::const_iterator iter = dialogCollators.begin()
    ; iter != dialogCollators.end(); ++iter)
  {
    DialogEvent winningNode;
    if(iter->winningNode(dialogCAA, winningNode)) 
    {
      OSS_LOG_INFO("[DialogCollator] Collate: Final Winning Node: " 
                  << winningNode.dialogId() 
                  << " : state -> " << winningNode.dialogState()
                  << " : json -> " << winningNode.toJson());
      winningEvents.push_back(winningNode);
    }
  }
}

} } } // Namespace SIPX::Kamailio::Plugin

