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
 * Internal Classes
 */

class DialogEventFinder
{
public:
  DialogEventFinder(const std::string & dialogKey, DialogKeyFlag keyFlag)
    : _dialogKey(dialogKey)
    , _dialogKeyFlag(keyFlag) {}

  bool operator() (const DialogEvent& dialogEvent) const 
  { 
     return _dialogKey == dialogEvent.generateKey(_dialogKeyFlag);
  }

private:
  const std::string & _dialogKey;
  DialogKeyFlag _dialogKeyFlag;
}; // class DialogEventFinder

/**
 * Struct DialogKey
 */

bool DialogElement::compareKey(const DialogElement & dialog, DialogKeyFlag keyFlag) const
{
  return compareKey(dialog.generateKey(keyFlag), keyFlag);
}

bool DialogElement::compareKey(const std::string & dialogKey, DialogKeyFlag keyFlag) const
{
  return dialogKey == generateKey(keyFlag);
}

std::string DialogElement::generateKey(DialogKeyFlag keyFlag) const
{
  static const std::string COLON = ":";

  const std::string & keyLocalTag = (direction == DIRECTION_INITIATOR) ? localTag : remoteTag;
  const std::string & keyRemoteTag = (direction == DIRECTION_INITIATOR) ? remoteTag : localTag;
  
  std::ostringstream stream;
  switch(keyFlag)
  {
  case KEY_DIALOG_ID:
    stream << callId << COLON << keyLocalTag << COLON << keyRemoteTag;
    break;
  case KEY_LOCAL_ID:
    stream << callId << COLON << keyLocalTag;
    break;
  case KEY_CALL_ID:
    stream << callId;
    break;
  default:
    break;
  }

  return stream.str();
}

/**
 * class DialogEvent
 */

/*static*/ bool DialogEvent::updateDialogState(const std::string & dialogXml, DialogState state
  , std::string & updatedXml)
{
  TiXmlDocument doc;
  doc.Parse(dialogXml.c_str(), 0, TIXML_ENCODING_UTF8);
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
        dialog_xml_print(doc, updatedXml);
        return true;
      }
    }
  }

  return false;
}

/*static*/ bool DialogEvent::mergeDialogEvent(const std::list<DialogEvent> & dialogEvents, std::string & xml)
{
  TiXmlDocument mergeDoc;
  TiXmlElement* mergeDialogInfoElement = NULL;
  for(std::list<DialogEvent>::const_iterator iter = dialogEvents.begin()
          ; iter != dialogEvents.end()
          ; ++iter)
  {
    const std::string & payload = iter->payload();
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

  return !xml.empty();
}

DialogEvent::DialogEvent()
  : _newDialog(true)
{

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
  if(state != STATE_INVALID && DialogEvent::updateDialogState(_payload, state, _payload))
  {
    _dialogElement.state = state;
    return true;
  }

  return false;
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

std::string DialogEvent::generateKey(DialogKeyFlag keyFlag) const
{
  return _dialogElement.generateKey(keyFlag);
}

void DialogEvent::toJson(std::string & json) const
{
  std::ostringstream strm;
  strm << "{"
      << "\"id\":" << "\"" << dialogId() <<"\","
      << "\"state\":" << "\"" << enum_to_string(dialogState()) <<"\",";
  if(dialogDirection() == DIRECTION_INITIATOR)
  {
    strm << "\"localTag\":" << "\"" << _dialogElement.localTag <<"\","
      << "\"remoteTag\":" << "\"" << _dialogElement.remoteTag <<"\"";
  } else if(dialogDirection() == DIRECTION_RECIPIENT)
  {
    strm << "\"localTag\":" << "\"" << _dialogElement.remoteTag <<"\","
      << "\"remoteTag\":" << "\"" << _dialogElement.localTag <<"\"";
  }
  strm << "}";
  json = strm.str();
}

std::string DialogEvent::toJson() const
{
  std::string json;
  toJson(json);
  return json;
}

/**
 * class DialogCollateEvent
 */

DialogCollateEvent::DialogCollateEvent(const std::string & dialogKey, DialogKeyFlag dialogKeyFlag)
  : _dialogKey(dialogKey)
  , _dialogKeyFlag(dialogKeyFlag)
{

}

bool DialogCollateEvent::addEvent(const DialogEvent & dialogEvent)
{
  DialogEvents::iterator iter = _dialogEvents.find(dialogEvent.dialogState());
  if(iter != _dialogEvents.end())
  {
    // check if dialog already exist
    OSS_LOG_INFO("[DialogCollateEvent] Collate: Add Dialog Id: " 
                << dialogEvent.dialogId() 
                << " : state -> " << dialogEvent.dialogState()
                << " : json -> " << dialogEvent.toJson());
    iter->second.push_back(dialogEvent);
  } else
  {
    OSS_LOG_INFO("[DialogCollateEvent] Collate: New Dialog Id: " 
                << dialogEvent.dialogId() 
                << " : state -> " << dialogEvent.dialogState()
                << " : json -> " << dialogEvent.toJson());
    
    std::list<DialogEvent> newDialogEvents;
    newDialogEvents.push_back(dialogEvent);
    _dialogEvents.insert(std::make_pair(dialogEvent.dialogState(), newDialogEvents));
  }

  return true;
}

void DialogCollateEvent::getEvent(DialogState dialogState, std::list<DialogEvent> & dialogEvents) const
{
  DialogEvents::const_iterator iter = _dialogEvents.find(dialogState);
  if(iter != _dialogEvents.end())
  {
    dialogEvents.insert(dialogEvents.end(), iter->second.begin(), iter->second.end());
  }
}

bool DialogCollateEvent::hasEvent(DialogState dialogState) const
{
  DialogEvents::const_iterator iter = _dialogEvents.find(dialogState);
  return iter != _dialogEvents.end();
}

bool DialogCollateEvent::winningNode(DialogPriority priority, DialogEvent & dialogEvent, DialogState ignoreState) const
{
  std::list<DialogEvent> winningList;
  winningNodes(priority, winningList, ignoreState);
  if(!winningList.empty())
  {
    dialogEvent = winningList.back();
    return true;
  }

  return false;                             
}

bool DialogCollateEvent::winningNodes(DialogPriority priority, std::list<DialogEvent> & dialogEvents, DialogState ignoreState) const
{
  /*Terminated > Confirmed > Early > Proceeding > Trying*/
  static const std::list<DialogState> priorityByCall = boost::assign::list_of
    (STATE_TERMINATED)(STATE_CONFIRMED)(STATE_EARLY)(STATE_PROCEEDING)(STATE_TRYING);

  /*Early > Confirmed > Proceeding > Trying > Terminated*/
  static const std::list<DialogState> priorityByDialog = boost::assign::list_of
    (STATE_EARLY)(STATE_CONFIRMED)(STATE_PROCEEDING)(STATE_TRYING)(STATE_TERMINATED);

  static const std::map<DialogPriority, std::list<DialogState> > priorityMap = 
    boost::assign::map_list_of (PRIORITY_BY_CALL, priorityByCall) 
                               (PRIORITY_BY_DIALOG, priorityByDialog);
  if(priorityMap.find(priority) != priorityMap.end()) 
  {
    return winningNodes(priorityMap.at(priority), dialogEvents, ignoreState);
  } else
  {
    throw std::invalid_argument("DialogPriority not supported.");  
  }
}

bool DialogCollateEvent::winningNodes(const std::list<DialogState> & priorityList
  , std::list<DialogEvent> & dialogEvents, DialogState ignoreState) const
{
  for(std::list<DialogState>::const_iterator iter = priorityList.begin();
    iter != priorityList.end(); ++iter)
  {
    if(ignoreState != *iter && _dialogEvents.find((*iter)) != _dialogEvents.end())
    {
      const std::list<DialogEvent> & tempList = _dialogEvents.at(*iter);
      dialogEvents.insert(dialogEvents.end(), tempList.begin(), tempList.end());
      return true;
    }
  }

  return false;
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

    return true;
  }

  return false;
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
