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

#include "DialogEventCollator/DialogInfo.h"

#include <OSS/UTL/Logger.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/assign.hpp>
#include "xmlparser/tinystr.h"
#include "xmlparser/tinyxml.h"

namespace SIPX {
namespace Kamailio {
namespace Plugin {

/**
 * Global Functions
 */

template<class T>
T string_to_enum(const std::string& string) { return T::unimplemented_function; }

std::string enum_to_string(DialogState value);

bool dialog_xml_parse(TiXmlElement* element, DialogInfo & dialogInfo);
bool dialog_xml_parse(TiXmlElement* element, DialogElement & dialogElement, DialogState state = STATE_INVALID);
void dialog_xml_print(TiXmlDocument& doc, std::string & xml);

/**
 * class DialogEvent
 */

/*static*/ bool DialogEvent::updateDialogState(const std::string & dialogEvent, DialogState state)
{
  TiXmlDocument doc;
  doc.Parse(dialogEvent.c_str(), 0, TIXML_ENCODING_UTF8);
  if (!doc.Error()) 
  {
    TiXmlElement * dialogInfoElement = doc.FirstChildElement("dialog-info");
    if (dialogInfoElement) 
    {
      TiXmlElement* dialogElement = dialogInfoElement->FirstChildElement("dialog");
      TiXmlElement* stateElement = dialogElement ? dialogElement->FirstChildElement("state") : NULL;
      if(stateElement)
      {
        std::string dialogState = enum_to_string(state);
        stateElement->Clear();
        stateElement->LinkEndChild(new TiXmlText(dialogState.c_str()));
        return true;
      }
    }
  }

  return false;
}

DialogEvent::DialogEvent()
{

}

DialogEvent::DialogEvent(const std::string & payload, DialogState state)
{
  parse(payload, state);
}

void DialogEvent::parse(const std::string& payload, DialogState state)
{
  if(!payload.empty())
  {
    _payload = payload;

    TiXmlDocument doc;
    doc.Parse(payload.c_str(), 0, TIXML_ENCODING_UTF8);
    TiXmlElement * dialogInfo = !doc.Error() ? doc.FirstChildElement("dialog-info") : NULL;
    if (dialog_xml_parse(dialogInfo, _dialogInfo)) 
    {
      if(dialog_xml_parse(dialogInfo->FirstChildElement("dialog"), _dialogElement, state)
        && state != STATE_INVALID)
      {
        dialog_xml_print(doc, _payload);
      }
    }
  }
}

bool DialogEvent::updateDialogState(DialogState state)
{
  if(state != STATE_INVALID && DialogEvent::updateDialogState(_payload, state))
  {
    _dialogElement.state = state;
    return true;
  }

  return false;
}

void DialogEvent::generateMinimalDialog(const std::string & user, const std::string & domain, std::string & xml)
{
  std::stringstream strm;
  strm << "sip:" << user << "@" << domain;
  generateMinimalDialog(strm.str(), xml);
}

void DialogEvent::generateMinimalDialog(const std::string & entity, std::string & xml)
{
  std::stringstream strm;
  strm << "<?xml version=\"1.0\" ?>" << std::endl 
    << "<dialog-info xmlns=\"urn:ietf:params:xml:ns:dialog-info\" version=\"00000000000\" state=\"full\" entity=\"" << entity << "\" />" << std::endl;
  xml = strm.str();
}

bool DialogEvent::valid() const
{
  if(!_dialogInfo.entity.empty() && !_dialogInfo.state.empty() && !_dialogInfo.version.empty()) 
  {
    // Validate dialog if dialog element is define
    if(!_dialogElement.id.empty())
    {
      return !_dialogElement.callId.empty() 
            && _dialogElement.state != STATE_INVALID
            && _dialogElement.direction != DIRECTION_INVALID; 
    }

    return true;
  }

  return false;
}

/**
 * class DialogCollateEvent
 */

DialogCollateEvent::DialogCollateEvent(const std::string & dialogKey)
  : _dialogKey(dialogKey)
{

}

void DialogCollateEvent::addEvent(const DialogEvent* dialogEvent)
{
  if(dialogEvent != NULL)
  {
    std::pair<DialogEvents::iterator, bool> result = _dialogEvents.insert(
        std::make_pair(dialogEvent->dialogState(),dialogEvent)
      );
    if ( !result.second ) {
        OSS_LOG_INFO("[DialogCollateEvent] Collate: Update Dialog Id: " 
                  << dialogEvent->dialogId()
                  << " : state -> " << (result.first)->second->dialogState()
                  << " : payload -> " << (result.first)->second->payload());
        _dialogEvents[dialogEvent->dialogState()] = dialogEvent;
    } else {
        OSS_LOG_DEBUG("[DialogCollateEvent] Collate: New Dialog Id: " 
                  << dialogEvent->dialogId() 
                  << " : state -> " << dialogEvent->dialogState()
                  << " : payload -> " << dialogEvent->payload());
    }
  }
}

const DialogEvent* DialogCollateEvent::winningNode(DialogPriority priority) const
{
  /*Terminated > Confirmed > Proceeding > Early > Trying*/
  static const std::list<DialogState> priorityByCall = boost::assign::list_of
    (STATE_TERMINATED)(STATE_CONFIRMED)(STATE_PROCEEDING)(STATE_EARLY)(STATE_TRYING);

  /*Early > Confirmed > Proceeding > Trying > Terminated*/
  static const std::list<DialogState> priorityByDialog = boost::assign::list_of
    (STATE_EARLY)(STATE_CONFIRMED)(STATE_PROCEEDING)(STATE_TRYING)(STATE_TERMINATED);

  static const std::map<DialogPriority, std::list<DialogState> > priorityMap = 
    boost::assign::map_list_of (PRIORITY_BY_CALL, priorityByCall) 
                               (PRIORITY_BY_DIALOG, priorityByDialog);
  if(priorityMap.find(priority) != priorityMap.end()) 
  {
    return winningNode(priorityMap.at(priority));
  }

  throw std::invalid_argument("DialogPriority not supported.");                               
}

DialogState DialogCollateEvent::winningState(DialogPriority priority) const
{
  const DialogEvent * dialogEvent = winningNode(priority);
  return dialogEvent ? dialogEvent->dialogState() : STATE_INVALID;
}

const DialogEvent* DialogCollateEvent::winningNode(const std::list<DialogState> & priorityList) const
{
  for(std::list<DialogState>::const_iterator iter = priorityList.begin();
    iter != priorityList.end(); ++iter)
  {
    if(_dialogEvents.find((*iter)) != _dialogEvents.end())
    {
      return _dialogEvents.at(*iter);
    }
  }
  return NULL;
}

/**
 * class DialogAggregateEvent
 */

DialogAggregateEvent::DialogAggregateEvent()
{

}

void DialogAggregateEvent::addEvent(const DialogEvent* dialogEvent)
{
  _dialogEvents.push_back(dialogEvent);
}

void DialogAggregateEvent::addEvent(const std::list<DialogEvent> & dialogEvent)
{
  for(std::list<DialogEvent>::const_iterator iter = dialogEvent.begin(); iter != dialogEvent.end(); ++iter)
  {
    _dialogEvents.push_back(&(*iter));
  }
}

void DialogAggregateEvent::addEvent(const std::list<const DialogEvent*> dialogEvent)
{
  _dialogEvents.insert(_dialogEvents.end(), dialogEvent.begin(), dialogEvent.end());
}

void DialogAggregateEvent::mergeEvent(std::string& xml) const
{
  TiXmlDocument mergeDoc;
  TiXmlElement* mergeDialogInfoElement = NULL;
  for(std::list<const DialogEvent*>::const_iterator iter = _dialogEvents.begin()
          ; iter != _dialogEvents.end()
          ; ++iter)
  {
    const DialogEvent * dialogEvent = (*iter);
    std::string payload = dialogEvent ? dialogEvent->payload() : std::string();

    if(mergeDialogInfoElement == NULL)
    {
      mergeDoc.Parse(payload.c_str(), 0, TIXML_ENCODING_UTF8);
      TiXmlElement * dialogInfoElement = !mergeDoc.Error() ? mergeDoc.FirstChildElement("dialog-info") : NULL;
      if(dialogInfoElement)
      {
        mergeDialogInfoElement = dialogInfoElement;
        /*
         * Masking version to zero, this is required by kamailio to let the kamailio handle
         * versioning on Notify 
         */
        mergeDialogInfoElement->SetAttribute("version", "00000000000");
        mergeDialogInfoElement->SetAttribute("state", "full");
      }
    } else
    {
      TiXmlDocument doc;
      doc.Parse(payload.c_str(), 0, TIXML_ENCODING_UTF8);
      TiXmlElement * dialogInfoElement = !doc.Error() ? doc.FirstChildElement("dialog-info") : NULL;
      TiXmlElement * dialogElement = dialogInfoElement ? dialogInfoElement->FirstChildElement("dialog") : NULL;
      if (dialogElement) 
      {
        mergeDialogInfoElement->LinkEndChild(dialogElement->Clone());
      }
    }
  }

  //if mergeDialogInfoElement is set then there is atleast one merging occured
  if(mergeDialogInfoElement)
  {
    dialog_xml_print(mergeDoc, xml);  
  }
}

/**
 * Global Implementation
 */

template<>
DialogDirection string_to_enum<DialogDirection>(const std::string& string)
{
  static const std::map<std::string, DialogDirection> enumMap = 
    boost::assign::map_list_of ("initiator", DIRECTION_INITIATOR) 
                               ("recipient", DIRECTION_RECIPIENT);
  std::map<std::string, DialogDirection>::const_iterator iter = enumMap.find(
      boost::to_lower_copy(string)
    );                               
  if(iter != enumMap.end()) 
  {
    return iter->second;
  }

  return DIRECTION_INVALID;
}

template<>
DialogState string_to_enum<DialogState>(const std::string& string)
{
  static const std::map<std::string, DialogState> enumMap = 
    boost::assign::map_list_of ("trying", STATE_TRYING) 
                               ("proceeding", STATE_PROCEEDING)
                               ("early", STATE_EARLY)
                               ("confirmed", STATE_CONFIRMED)
                               ("terminated", STATE_TERMINATED);
  std::map<std::string, DialogState>::const_iterator iter = enumMap.find(
      boost::to_lower_copy(string)
    );
  if(iter != enumMap.end()) 
  {
    return iter->second;
  }                          
  return STATE_INVALID;
}

std::string enum_to_string(DialogState value) 
{
  static const std::map<DialogState, std::string> enumMap = 
    boost::assign::map_list_of (STATE_TRYING, "trying") 
                               (STATE_PROCEEDING, "proceeding")
                               (STATE_EARLY, "early")
                               (STATE_CONFIRMED, "confirmed")
                               (STATE_TERMINATED, "terminated");
  std::map<DialogState, std::string>::const_iterator iter = enumMap.find(value);
  if(iter != enumMap.end()) 
  {
    return iter->second;
  }                          
  
  throw std::invalid_argument("Unsupported DialogState: " + value);
}

bool dialog_xml_parse(TiXmlElement* element, DialogInfo & dialogInfo)
{
  if (element != NULL) 
  {
    const char * entity = element->Attribute("entity");
    const char * state = element->Attribute("state");
    const char * version = element->Attribute("version");

    //entity, state and version is mandatory
    if (entity != NULL && state != NULL && version != NULL) 
    {   
      //Store Dialog Info
      dialogInfo.entity = entity;
      dialogInfo.state = state;
      dialogInfo.version = version;
      return true;
    }
  }

  return false;
}

bool dialog_xml_parse(TiXmlElement* element, DialogElement & dialogElement, DialogState dialogState)
{
  const char * id = element ? element->Attribute("id") : NULL;
  if(id != NULL)
  {
    /* Get Attributes */
    const char * callId = element->Attribute("call-id");
    const char * localTag = element->Attribute("local-tag");
    const char * remoteTag = element->Attribute("remote-tag");
    const char * direction = element->Attribute("direction");
    
    dialogElement.id = id;
    dialogElement.callId = callId ? callId : std::string();
    dialogElement.localTag = localTag ? localTag : std::string();
    dialogElement.remoteTag = remoteTag ? remoteTag : std::string();
    dialogElement.direction = direction ? string_to_enum<DialogDirection>(direction) : DIRECTION_INVALID;

    if (!dialogElement.callId.empty() && dialogElement.id != dialogElement.callId) {
      dialogElement.id = callId; // Set id as call id attribute basing this on with kamailio format
      element->SetAttribute("id", callId);
    }

    /* Get State */
    TiXmlElement * stateElement = element->FirstChildElement("state");
    if (stateElement) 
    {
      if(dialogState != STATE_INVALID)
      {
        dialogElement.state = dialogState;
        stateElement->Clear();
        stateElement->LinkEndChild(new TiXmlText(enum_to_string(dialogState).c_str()));
      } else
      {
        TiXmlNode * stateNode = stateElement->FirstChild();
        const char * state = stateNode ? stateNode->Value() : NULL;
        dialogElement.state = state ? string_to_enum<DialogState>(state) : STATE_INVALID;
      }                            
    } else {
      // If no state is specified. Considered this as terminated state
      dialogElement.state = STATE_TERMINATED;
    }

    /* Get Local Target */
    TiXmlElement * localElement = element->FirstChildElement("local");
    if(localElement)
    {
      TiXmlElement * targetElement = localElement->FirstChildElement("target");   
      const char * localUri = targetElement ? targetElement->Attribute("uri") : NULL;
      dialogElement.localTarget = localUri ? localUri : std::string();
    }

    /* Get Remote Target */
    TiXmlElement * remoteElement = element->FirstChildElement("remote");
    if(remoteElement)
    {
      TiXmlElement * targetElement = remoteElement->FirstChildElement("target");   
      const char * remoteUri = targetElement ? targetElement->Attribute("uri") : NULL;
      dialogElement.remoteTarget = remoteUri ? remoteUri : std::string();
    }
  } else {
    // If no dialog element is set. Default state to Terminated.
    dialogElement.state = STATE_TERMINATED;
  }

  return true;
}

void dialog_xml_print(TiXmlDocument& doc, std::string & xml)
{
  char* charBuffer=NULL;
  size_t size = 0;
  
  FILE* cstream = open_memstream(&charBuffer, &size);
  if(cstream != 0) 
  {
    doc.Print(cstream, 0);
    fflush(cstream);
    fclose(cstream);
    xml = std::string(charBuffer, size);
    free(charBuffer);
  }
}


} } } // Namespace SIPX::Kamailio::Plugin
