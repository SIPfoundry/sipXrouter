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
#include <OSS/UTL/Logger.h>
#include <vector>
#include <map>

/*
 * TODO:
 * MOH in SIPX Trigger Confirm (Hold) and Terminated (Unhold) Dialog Event
 * during call. The Kamailio DialogCollator currently identify the moh hold and
 * unhold as a new Dialog. Temporary solution is to ignore the state sent by
 * the kamailio if the remote trigger uri is MOH. Need to provide a better
 * solution for handling the moh in kamailio rather than using this hack
 */
#define ENABLE_MOH_FILTER = 1

namespace SIPX {
namespace Kamailio {
namespace Plugin {

DialogCollator::DialogCollator() 
{
    
}

DialogCollator::~DialogCollator() 
{
     
}

bool DialogCollator::connect(const std::string& password, int db)
{
  OSS_LOG_INFO("[DialogCollator] Connecting to Redis Server.");
  return _redisClient.connect(password, db);
}

void DialogCollator::disconnect()
{
  OSS_LOG_INFO("[DialogCollator] Disconnecting to Redis Server.");
  _redisClient.disconnect();
}

/*
 * Kamailio will send a multiple payload on which it may contains multiple state 
 * within the same dialog. Finalize the state of each dialog.
 */
bool DialogCollator::collateLocalPayloads(const std::string & user, const std::string & domain
        , const std::vector<std::string> & payloads
        , CollatedDialogs & collatedDialogs)
{
  boost::unordered_map<std::string, DialogInfo> collatedDialogMap;
  for(std::vector<std::string>::const_iterator iter = payloads.begin()
          ; iter != payloads.end()
          ; ++iter)
  {
    DialogInfo dialogInfo;
    if(parseDialogInfoXML(*iter, dialogInfo))
    {
      if(dialogInfo.valid())
      {

#ifdef ENABLE_MOH_FILTER
          if(!dialogInfo.dialog.remoteTarget.empty() && 
              dialogInfo.dialog.remoteTarget.find("~~mh~") != std::string::npos)
          {
            OSS_LOG_WARNING("[DialogCollator] Collate: Ignoring state for moh event: " 
                    << dialogInfo.dialog.id << ": " << dialogInfo.dialog.state);
            continue;
          }
#endif

          std::string key = generateKey(user, domain, dialogInfo.dialog);
          boost::unordered_map<std::string, DialogInfo>::iterator mapIter = collatedDialogMap.find(key);
          if(mapIter != collatedDialogMap.end()) 
          {
            if(compareDialogByState(dialogInfo.dialog, mapIter->second.dialog) >= 0)
            {
                OSS_LOG_DEBUG("[DialogCollator] Collate: Update local state: " 
                    << key << ": " << mapIter->second.dialog.state << "->" << dialogInfo.dialog.state);
                collatedDialogMap[key] = dialogInfo;
            } else
            {
                OSS_LOG_DEBUG("[DialogCollator] Collate: Ignore local state: " 
                    << key << ": " << mapIter->second.dialog.state << "->" << dialogInfo.dialog.state);
            }
          } else {
            OSS_LOG_DEBUG("[DialogCollator] Collate: New local state: " 
                    << key << ": " << dialogInfo.dialog.state);
            collatedDialogMap[key] = dialogInfo;
          }
      } else {
          OSS_LOG_WARNING("[DialogCollator] Collate: Invalid local state: " 
              << dialogInfo.dialog.id << ": " << dialogInfo.dialog.state);       
      }
    } else
    {
        OSS_LOG_ERROR("[DialogCollator] Collate: Parse error: " << *iter);
    }
  }

  /**
   * Only add collated local state
   */
  for(boost::unordered_map<std::string, DialogInfo>::iterator iter = collatedDialogMap.begin()
      ; iter != collatedDialogMap.end()
      ; ++iter)
  {
      collatedDialogs.push_back(iter->second);
  }

  return !collatedDialogs.empty();
}

/*
 * Compare and finalize the state of each dialog against the states in Redis
 */
bool DialogCollator::collateRemotePayloads(const std::string & user, const std::string & domain
        , CollatedDialogs & collatedDialogs)
{
  bool result = false;
  for(CollatedDialogs::iterator iter = collatedDialogs.begin()
          ; iter != collatedDialogs.end()
          ; ++iter)
  {
    if(iter->valid()) 
    {
      DialogInfo currentDialogInfo;

      std::string key = generateKey(user, domain, iter->dialog);
      if(getDialogInfo(key, currentDialogInfo))
      {
        if(compareDialogByState(iter->dialog, currentDialogInfo.dialog) >= 0)
        {
            OSS_LOG_INFO("[DialogCollator] Collate: Update remote state: " 
                    << key << ": " << currentDialogInfo.dialog.state << "->" << iter->dialog.state);
            setDialogInfo(key, *iter);
        } else {
            OSS_LOG_INFO("[DialogCollator] Collate: Ignore remote state: " 
                    << key << ": " << currentDialogInfo.dialog.state << "<-" << iter->dialog.state);
            *iter = currentDialogInfo; // Replace it with updated dialog state
        }
      } else 
      {
        OSS_LOG_INFO("[DialogCollator] Collate: New remote state: " 
                << key << ": ->" << iter->dialog.state);
        setDialogInfo(key, *iter);
      }

      result = true;
    } else 
    {
      OSS_LOG_ERROR("[DialogCollator] Collate: Invalid remote state: " 
                  << iter->dialog.id << ": payload ->" << iter->rawPayload);
    }
  }

  return result;    
}

/*
 * Aggregate the state of all collated dialog by dialog.id 
 */
void DialogCollator::aggregateLocalPayloads(const std::string & user, const std::string & domain
        , const CollatedDialogs & collatedDialogs
        , AggregatedDialogs & aggregateDialogs)
{
  for(CollatedDialogs::const_iterator iter = collatedDialogs.begin()
          ; iter != collatedDialogs.end()
          ; ++iter)
  {
    AggregatedDialogs::iterator mapIter = aggregateDialogs.find(iter->dialog.id);
    if(mapIter != aggregateDialogs.end()) 
    {
      if(compareDialogByPreference(iter->dialog, mapIter->second.dialog) >= 0)
      {
        OSS_LOG_DEBUG("[DialogCollator] Aggregate: Update local state: " 
            << iter->dialog.id << ": from " << mapIter->second.dialog.state << " to " << iter->dialog.state);
        aggregateDialogs[iter->dialog.id] = *iter;
      } else {
        OSS_LOG_DEBUG("[DialogCollator] Aggregate: Ignore local state: " 
            << iter->dialog.id << ": from " << mapIter->second.dialog.state << " to " << iter->dialog.state);
      }
    } else 
    {
        OSS_LOG_DEBUG("[DialogCollator] Aggregate: New local state for key: " 
            << iter->dialog.id << ": to " << iter->dialog.state);
        aggregateDialogs[iter->dialog.id] = *iter;
    }
  }
}

/*
 * Aggregate the state of all collated dialog including the non terminated state found in Redis.
 */
bool DialogCollator::aggregateRemotePayloads(const std::string & user, const std::string & domain
    , AggregatedDialogs & aggregateDialogs)
{
  CollatedDialogs dialogInfos;
  if(getAllDialogInfo(user, domain, dialogInfos) && !dialogInfos.empty())
  {
    for(CollatedDialogs::iterator iter = dialogInfos.begin()
            ; iter != dialogInfos.end()
            ; ++iter)
    {
      AggregatedDialogs::iterator mapIter = aggregateDialogs.find(iter->dialog.id);
      if(mapIter != aggregateDialogs.end()) 
      {
        if(compareDialogByPreference(iter->dialog, mapIter->second.dialog) >= 0)
        {
          OSS_LOG_INFO("[DialogCollator] Aggregate: Candidate state: " 
              << iter->dialog.id << ": from " << mapIter->second.dialog.state << " to " << iter->dialog.state);
          aggregateDialogs[iter->dialog.id] = *iter;
        } else
        {
          OSS_LOG_INFO("[DialogCollator] Aggregate: Reject state: " 
              << iter->dialog.id << ": from " << mapIter->second.dialog.state << " to " << iter->dialog.state);
        }
      } else 
      {
        if(iter->dialog.state != "terminated")
        {
            OSS_LOG_WARNING("[DialogCollator] Aggregate: Found non-terminated dialog: " 
                << iter->dialog.id << ": " << iter->dialog.state << " --> Force terminate dialog.");

            iter->dialog.state = "terminated";
            if(updateDialogInfoXMLState(iter->rawPayload, iter->dialog.state))
            {
                std::string key = generateKey(user, domain, iter->dialog);
                setDialogInfo(key, *iter);                        
            }
            aggregateDialogs[iter->dialog.id] = *iter;
        }
      }
    }
  } else 
  {
    OSS_LOG_ERROR("[DialogCollator] Aggregate: Unable to retrieve remote state: " << user << "@" << domain);
    return false;
  }

  return true;
}

/**
 * Finalize the aggregated dialog and make sure it contains only one active dialog.
 */
void DialogCollator::finalizeAggregatePayloads(const std::string & user, const std::string & domain
        , AggregatedDialogs & aggregateDialogs)
{
  DialogInfo * activeDialog = NULL;
  for(AggregatedDialogs::iterator iter = aggregateDialogs.begin()
      ; iter != aggregateDialogs.end()
      ; ++iter)
  {
    DialogInfo & currentDialog = iter->second;
    if(activeDialog)
    {
      if(compareDialogByPreference(currentDialog.dialog, activeDialog->dialog) >= 0)
      {
        if(activeDialog->dialog.state != "terminated")
        {
            OSS_LOG_INFO("[DialogCollator] Aggregate: Force terminate dialog: " 
                << activeDialog->dialog.id << ": " 
                << activeDialog->dialog.state);

            activeDialog->dialog.state = "terminated";
            if(updateDialogInfoXMLState(activeDialog->rawPayload, activeDialog->dialog.state))
            {
                std::string key = generateKey(user, domain, activeDialog->dialog);
                setDialogInfo(key, *activeDialog);
            }

            OSS_LOG_INFO("[DialogCollator] Aggregate: New active dialog: " 
                << currentDialog.dialog.id << ": " 
                << currentDialog.dialog.state);
        }

        activeDialog = &currentDialog;
      } else if(currentDialog.dialog.state != "terminated")
      {
        OSS_LOG_INFO("[DialogCollator] Aggregate: Force terminate dialog: " 
                << currentDialog.dialog.id << ": " 
                << currentDialog.dialog.state);
        currentDialog.dialog.state = "terminated";
        if(updateDialogInfoXMLState(currentDialog.rawPayload, currentDialog.dialog.state))
        {
            std::string key = generateKey(user, domain, currentDialog.dialog);
            setDialogInfo(key, currentDialog);
        }
      }
    } else
    {
      activeDialog = &currentDialog;
    }
  }
}

std::string DialogCollator::generateDialogPayload(const AggregatedDialogs & aggregateDialogs)
{
  std::string xml;
  for(AggregatedDialogs::const_iterator iter = aggregateDialogs.begin()
          ; iter != aggregateDialogs.end()
          ; ++iter)
  {
    mergeDialogInfoXMLDialog(xml, iter->second);
  }

  if(!xml.empty())
  {
    /**
     * Required by the Kamailio Presence DialogInfo to mask
     * the version with zero's
     */
    if(maskDialogInfoXMLVersion(xml))
    {
      OSS_LOG_DEBUG("[DialogCollator] Returning collated dialog: " << xml);
      return xml;
    }
  }   

  OSS_LOG_WARNING("[DialogCollator] Returning empty collated dialog");
  return ""; 
}

std::string DialogCollator::collateAndAggregatePayloads(const std::string & user, const std::string & domain, const std::vector<std::string> & payloads)
{
  CollatedDialogs collatedDialogs;
  if(collateLocalPayloads(user, domain, payloads, collatedDialogs))
  {
    if(collateRemotePayloads(user, domain, collatedDialogs))
    {
      AggregatedDialogs aggregateDialogs;
      aggregateLocalPayloads(user, domain, collatedDialogs, aggregateDialogs);
      if(!aggregateDialogs.empty() && aggregateRemotePayloads(user, domain, aggregateDialogs))
      {
        finalizeAggregatePayloads(user, domain, aggregateDialogs);
        return generateDialogPayload(aggregateDialogs);
      }
    }
  }

  return "";
}

void DialogCollator::flushDialog(const std::string & user, const std::string & domain)
{
  std::ostringstream strm;
  strm << user << "@" << domain;

  std::string key = strm.str();
  _redisClient.del(key + ":*:dialog");
}

std::string DialogCollator::generateKey(const std::string & user, const std::string & domain, const Dialog & dialog)
{
  std::ostringstream strm;
  strm << user << "@" << domain << ":" << dialog.generateKey();
  return strm.str();
}

bool DialogCollator::setDialogInfo(const std::string & key, const DialogInfo & dialogInfo)
{
  json::Object object;
  dialogInfo.toJsonObject(object);
  return _redisClient.set(key + ":dialog", object, 3600);
}

bool DialogCollator::getDialogInfo(const std::string & key, DialogInfo & dialogInfo)
{
  json::Object object;
  if(_redisClient.get(key + ":dialog", object)) {
    dialogInfo.fromJsonObject(object);
    return true;
  }
      
  return false;
}

bool DialogCollator::getAllDialogInfo(const std::string & user, const std::string & domain, std::vector<json::Object> & dialogInfos)
{
  std::ostringstream strm;
  strm << user << "@" << domain << ":*:dialog";
  return _redisClient.getAll(dialogInfos, strm.str());
}

bool DialogCollator::getAllDialogInfo(const std::string & user, const std::string & domain, CollatedDialogs & dialogInfos)
{
  std::vector<json::Object> objects;
  if(getAllDialogInfo(user, domain, objects) && !objects.empty())
  {
    for(std::vector<json::Object>::const_iterator iter = objects.begin()
            ; iter != objects.end()
            ; ++iter)
    {
      DialogInfo dialogInfo;
      dialogInfo.fromJsonObject(*iter);
      if(dialogInfo.valid())
      {
        dialogInfos.push_back(dialogInfo);
      }
    }

    return true;
  }

  return false;
}

} } } // Namespace SIPX::Kamailio::Plugin

