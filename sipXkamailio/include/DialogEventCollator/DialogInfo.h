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

#ifndef _DIALOGINFO_H_
#define _DIALOGINFO_H_

#include <string>
#include <map>
#include <list>
#include <vector>

class TiXmlDocument;

namespace SIPX {
namespace Kamailio {
namespace Plugin {

enum DialogState
{
  STATE_INVALID = 0,
  STATE_TRYING = 1,
  STATE_EARLY = 2,
  STATE_PROCEEDING = 4,
  STATE_CONFIRMED = 8,
  STATE_TERMINATED = 16
}; // enum DialogState

enum DialogDirection
{
  DIRECTION_INVALID = 0,
  DIRECTION_INITIATOR = 1,
  DIRECTION_RECIPIENT = 2
}; // enum DialogDirection

enum DialogPriority
{
  /*Terminated > Confirmed > Proceeding > Early > Trying*/
  PRIORITY_BY_CALL, 
  /*Early > Confirmed > Proceeding > Trying > Terminated*/
  PRIORITY_BY_DIALOG
}; // enum DialogPriority

struct DialogElement
{
  std::string id;
  std::string callId;
  std::string localTag;
  std::string remoteTag;
  std::string remoteTarget;
  std::string localTarget;

  DialogDirection direction;
  DialogState state;

  DialogElement() 
    : direction(DIRECTION_INVALID)
    , state(STATE_INVALID) {}
}; // Struct DialogElement
    
struct DialogInfo
{
  std::string entity;
  std::string state;
  std::string version;  
}; // Struct DialogInfoEvent

class DialogEvent
{
public:
  static bool updateDialogState(const std::string & dialogEvent, DialogState state);
  static void generateMinimalDialog(const std::string & user, const std::string & domain, std::string & xml);
  static void generateMinimalDialog(const std::string & entity, std::string & xml);

public:
  DialogEvent();
  explicit DialogEvent(const std::string & payload, DialogState state = STATE_INVALID);

  void parse(const std::string& payload, DialogState state = STATE_INVALID);
  bool updateDialogState(DialogState state);
  bool valid() const;

public:
  const DialogInfo & dialogInfo() const;
  const DialogElement & dialogElement() const;
  const std::string & payload() const;
  
  const std::string & dialogId() const;
  const std::string & callId() const;
  DialogState dialogState() const;
  DialogDirection dialogDirection() const;

private:
  std::string _payload;

  DialogInfo _dialogInfo;
  DialogElement _dialogElement;
}; // class DialogEvent

class DialogCollateEvent
{
public:
  typedef std::map<DialogState, const DialogEvent*> DialogEvents;

public:
  explicit DialogCollateEvent(const std::string & dialogKey);

  void addEvent(const DialogEvent* dialogEvent);
  const DialogEvent* winningNode(DialogPriority priority = PRIORITY_BY_CALL) const;
  DialogState winningState(DialogPriority priority = PRIORITY_BY_CALL) const;

public:
  const std::string & dialogKey() const;

private:
  const DialogEvent* winningNode(const std::list<DialogState> & priorityList) const;

private:
  std::string _dialogKey;
  DialogEvents _dialogEvents;

}; //class DialogCollateEvent;

class DialogAggregateEvent
{
public:
  DialogAggregateEvent();

  void addEvent(const DialogEvent* dialogEvent);
  void addEvent(const std::list<DialogEvent> & dialogEvent);
  void addEvent(const std::list<const DialogEvent*> dialogEvent);
  
  void mergeEvent(std::string& xml) const;

private:
  std::list<const DialogEvent*> _dialogEvents;
}; //class DialogAggregateEvent

class DialogCollateEventFinder
{
public:
  DialogCollateEventFinder(const std::string & dialogKey) 
    : _dialogKey(dialogKey) {}
  bool operator() (const DialogCollateEvent & dialogEvent) const 
  { 
    return dialogEvent.dialogKey() == _dialogKey; 
  }
private:
    const std::string & _dialogKey;  
}; // DialogCollateEventFinder

inline const DialogInfo & DialogEvent::dialogInfo() const
{
  return _dialogInfo;
}

inline const DialogElement & DialogEvent::dialogElement() const
{
  return _dialogElement;
}

inline const std::string & DialogEvent::payload() const
{
  return _payload;
}

inline const std::string & DialogEvent::dialogId() const
{
  return _dialogElement.id;
}

inline const std::string & DialogEvent::callId() const
{
  return _dialogElement.callId;
}

inline DialogState DialogEvent::dialogState() const
{
  return _dialogElement.state;
}

inline DialogDirection DialogEvent::dialogDirection() const
{
  return _dialogElement.direction;
} 

inline const std::string & DialogCollateEvent::dialogKey() const
{
  return _dialogKey;
}

} } } // SIPX::Kamailio::Plugin


#endif  /* _DIALOGINFO_H_ */
