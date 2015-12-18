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
#include <boost/lexical_cast.hpp>
#include "xmlparser/tinystr.h"
#include "xmlparser/tinyxml.h"

namespace SIPX {
namespace Kamailio {
namespace Plugin {
    
//
// Global Methods
//
int getPriorityByState(const std::string & state);
int getPreferenceByState(const std::string & state);

bool DialogInfo::operator==(const DialogInfo& rhs)
{
  return entity == rhs.entity && state == rhs.state && dialog == rhs.dialog;
}
    
bool DialogInfo::generateKey(std::string & key) const
{
  key = dialog.generateKey();
  return !key.empty();
}

bool DialogInfo::valid() const
{
  return !entity.empty() && !state.empty() && !version.empty() && dialog.valid();
}

void DialogInfo::toJsonObject(json::Object& object) const 
{
  try
  {
      json::Object dialogObject;
      dialog.toJsonObject(dialogObject);

      object["entity"] = json::String(entity);
      object["state"] = json::String(state);
      object["version"] = json::String(version);
      object["dialog"] = dialogObject;

      object["rawPayload"] = json::String(rawPayload);
  } catch (json::Exception& e) 
  {
      OSS_LOG_INFO("[DialogInfo] Unable to convert string to json: " << e.what())
  }
}

void DialogInfo::fromJsonObject(const json::Object& object) 
{
  try 
  {
    entity = json::String(object["entity"]).Value();
    state = json::String(object["state"]).Value();
    version = json::String(object["version"]).Value();
    dialog.fromJsonObject(object["dialog"]);

    rawPayload = json::String(object["rawPayload"]).Value();
  } catch (json::Exception& e) 
  {
    OSS_LOG_INFO("[DialogInfo] Unable to convert json to string: " << e.what())   
  }
}

bool Dialog::operator==(const Dialog& rhs)
{
  return id == rhs.id && callId == rhs.callId
      && localTag == rhs.localTag 
      && remoteTag == rhs.remoteTag;
}

std::string Dialog::generateKey() const
{
    std::ostringstream strm;
    strm << callId << remoteTag << localTag;
    return strm.str();
}

bool Dialog::valid() const
{
  return !id.empty() && !callId.empty() && !localTag.empty() 
                     && !remoteTag.empty() && !state.empty();
}

void Dialog::toJsonObject(json::Object& object) const 
{
  try 
  {
    object["id"] = json::String(id);
    object["callId"] = json::String(callId);
    object["localTag"] = json::String(localTag);
    object["remoteTag"] = json::String(remoteTag);
    object["direction"] = json::String(direction);
    object["state"] = json::String(state);
  } catch (json::Exception& e) 
  {
    OSS_LOG_INFO("[DialogInfo] Unable to convert string to json: " << e.what())
  }
}

void Dialog::fromJsonObject(const json::Object& object) 
{
  try 
  {
    id = json::String(object["id"]).Value();
    callId = json::String(object["callId"]).Value();
    localTag = json::String(object["localTag"]).Value();
    remoteTag = json::String(object["remoteTag"]).Value();
    direction = json::String(object["direction"]).Value();
    state = json::String(object["state"]).Value();
  } catch (json::Exception& e) 
  {
    OSS_LOG_INFO("[DialogInfo] Unable to convert json to string: " << e.what())   
  }
}

//
// Global Methods
//

bool parseDialogInfoXML(const std::string & xml, DialogInfo & dialogInfo) 
{
  TiXmlDocument doc;
  doc.Parse(xml.c_str(), 0, TIXML_ENCODING_UTF8);
  if (!doc.Error()) 
  {
    TiXmlElement * dialogInfoElement = doc.FirstChildElement("dialog-info");
    if (dialogInfoElement != NULL) 
    {
      const char * entity = dialogInfoElement->Attribute("entity");
      const char * state = dialogInfoElement->Attribute("state");
      const char * version = dialogInfoElement->Attribute("version");

      //entity, state and version is mandatory
      if (entity != NULL && state != NULL && version != NULL) 
      {   
        //Store Dialog Info
        dialogInfo.entity = entity;
        dialogInfo.state = state;
        dialogInfo.version = version;
        dialogInfo.rawPayload = xml;

        TiXmlElement* dialogElement = dialogInfoElement->FirstChildElement("dialog");
        const char * id = dialogElement ? dialogElement->Attribute("id") : NULL;
        if (id != NULL) 
        {
          const char * callId = dialogElement->Attribute("call-id");
          const char * localTag = dialogElement->Attribute("local-tag");
          const char * remoteTag = dialogElement->Attribute("remote-tag");
          const char * direction = dialogElement->Attribute("direction");
          
          
          dialogInfo.dialog.id = id;
          dialogInfo.dialog.callId = callId ? callId : std::string();
          dialogInfo.dialog.localTag = localTag ? localTag : std::string();
          dialogInfo.dialog.remoteTag = remoteTag ? remoteTag : std::string();
          dialogInfo.dialog.direction = direction ? direction : std::string();

          /* Get State */
          TiXmlElement * stateElement = dialogElement->FirstChildElement("state");
          if (stateElement) 
          {
            TiXmlNode * stateNode = stateElement->FirstChild();
            const char * state = stateNode ? stateNode->Value() : NULL;
            dialogInfo.dialog.state = state ? state : std::string();                            
          }

          /* Get Local Target */
          TiXmlElement * localElement = dialogElement->FirstChildElement("local");
          if(localElement)
          {
            TiXmlElement * targetElement = localElement->FirstChildElement("target");   
            const char * localUri = targetElement ? targetElement->Attribute("uri") : NULL;
            dialogInfo.dialog.localTarget = localUri ? localUri : std::string();
          }

          /* Get Remote Target */
          TiXmlElement * remoteElement = dialogElement->FirstChildElement("remote");
          if(remoteElement)
          {
            TiXmlElement * targetElement = remoteElement->FirstChildElement("target");   
            const char * remoteUri = targetElement ? targetElement->Attribute("uri") : NULL;
            dialogInfo.dialog.remoteTarget = remoteUri ? remoteUri : std::string();
          }
        }
        
        return true;
      }
    }
  }
  
  return false;
}

bool maskDialogInfoXMLVersion(std::string & xml)
{
  TiXmlDocument doc;
  doc.Parse(xml.c_str(), 0, TIXML_ENCODING_UTF8);
  if (!doc.Error()) 
  {
    TiXmlElement * dialogInfoElement = doc.FirstChildElement("dialog-info");
    if (dialogInfoElement != NULL) 
    {
      dialogInfoElement->SetAttribute("version", "00000000000");
      TiXmlOutStream strm;
      strm << doc;
      xml = strm.c_str();
      return true;
    }
  }
  
  return false;
}

bool updateDialogInfoXMLState(std::string & xml, const std::string & state)
{
  TiXmlDocument doc;
  doc.Parse(xml.c_str(), 0, TIXML_ENCODING_UTF8);
  if (!doc.Error()) 
  {
    TiXmlElement * dialogInfoElement = doc.FirstChildElement("dialog-info");
    if (dialogInfoElement) 
    {
      TiXmlElement* dialogElement = dialogInfoElement->FirstChildElement("dialog");
      TiXmlElement* stateElement = dialogElement ? dialogElement->FirstChildElement("state") : NULL;
      if(stateElement)
      {
        stateElement->Clear();
        stateElement->LinkEndChild(new TiXmlText(state.c_str()));

        TiXmlOutStream strm;
        strm << doc;
        xml = strm.c_str();
        return true;
      }
    }
  }

  return false;
}

bool mergeDialogInfoXMLDialog(std::string & xml, const DialogInfo & rdialogInfo)
{
  TiXmlDocument rdoc;
  rdoc.Parse(rdialogInfo.rawPayload.c_str(), 0, TIXML_ENCODING_UTF8);
  if (!rdoc.Error()) 
  {
    TiXmlElement * rdialogInfoElement = rdoc.FirstChildElement("dialog-info");
    TiXmlElement * rdialogElement = rdialogInfoElement ? rdialogInfoElement->FirstChildElement("dialog") : NULL;
    if(rdialogElement)
    {
      if(xml.empty())
      {
        xml = rdialogInfo.rawPayload;
        return true;
      }

      TiXmlDocument ldoc;
      ldoc.Parse(xml.c_str(), 0, TIXML_ENCODING_UTF8);
      if (!ldoc.Error()) 
      {
        TiXmlElement * ldialogInfoElement = ldoc.FirstChildElement("dialog-info");
        if (ldialogInfoElement) 
        {
          ldialogInfoElement->LinkEndChild(rdialogElement->Clone());
          TiXmlOutStream strm;
          strm << ldoc;
          xml = strm.c_str();
          return true;
        }
      }
    }
  }

  return false;
}

int compareDialogByState(const Dialog & ldialog, const Dialog & rdialog)
{
    return getPriorityByState(ldialog.state) - getPriorityByState(rdialog.state);
}

int compareDialogByPreference(const Dialog & ldialog, const Dialog & rdialog)
{
    return getPreferenceByState(ldialog.state) - getPreferenceByState(rdialog.state);
}

/* Preferred order during collate */
int getPriorityByState(const std::string & state)
{
  if(state == "trying") {
    return 1;
  } else if(state == "early") {
    return 2;
  } else if(state == "confirmed") {
    return 3;
  } else if(state == "terminated") {
    return 4;
  } else {
    return 0;
  }
}

/* Preferred order during aggregate */
int getPreferenceByState(const std::string & state)
{
  if(state == "terminated") {
    return 1;
  } else if(state == "trying") {
    return 2;
  } else if(state == "early") {
    return 3;
  } else if(state == "confirmed") {
    return 4;
  } else {
    return 0;
  }
}


} } } // Namespace SIPX::Kamailio::Plugin
