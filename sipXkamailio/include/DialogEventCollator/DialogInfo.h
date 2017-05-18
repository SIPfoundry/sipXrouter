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

/*
 * Forward Classes
 */
class DialogCollatorAndAggregator;

/*
 * Enum definitions
 */
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

enum DialogKeyFlag
{
  KEY_DIALOG_ID, //call id, local tag and remote tag
  KEY_LOCAL_ID, //call id and local tag
  KEY_CALL_ID //call id
}; // enum DialogKeyType

/*
 * Struct definitions
 */
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

  bool compareKey(const DialogElement & dialog, DialogKeyFlag keyFlag) const;
  bool compareKey(const std::string & dialogKey, DialogKeyFlag keyFlag) const; 
  std::string generateKey(DialogKeyFlag keyFlag) const;
}; // Struct DialogElement
    
struct DialogInfo
{
  std::string entity;
  std::string state;
  std::string version;  
}; // Struct DialogInfoEvent

/*
 * Class definitions
 */
class DialogEvent
{
public:
  static bool updateDialogState(const std::string & dialogXml, DialogState state, std::string & updatedXml);
  static bool mergeDialogEvent(const std::list<DialogEvent> & dialogEvents, std::string & xml); 

public:
  DialogEvent();

  void parse(const std::string& payload, DialogState forceState = STATE_INVALID);
  bool updateDialogState(DialogState state);
  bool valid() const;

  std::string generateKey(DialogKeyFlag keyFlag) const;

public:
  const DialogInfo & dialogInfo() const;
  const DialogElement & dialogElement() const;
  const std::string & payload() const;
  
  const std::string & dialogId() const;
  DialogState dialogState() const;
  DialogDirection dialogDirection() const;

  void toJson(std::string & json) const;
  std::string toJson() const;

  bool newDialog() const;
  void newDialog(bool toggle);

private:
  std::string _payload;

  DialogInfo _dialogInfo;
  DialogElement _dialogElement;


  bool _newDialog;
}; // class DialogEvent

class DialogCollateEvent
{
public:
  typedef std::map<DialogState, std::list<DialogEvent> > DialogEvents;

public:
  DialogCollateEvent(const std::string & dialogKey, DialogKeyFlag dialogKeyFlag = KEY_DIALOG_ID);

  bool addEvent(const DialogEvent & dialogEvent);
  void getEvent(DialogState dialogState, std::list<DialogEvent> & dialogEvents) const;
  bool hasEvent(DialogState dialogState) const;

  bool winningNode(DialogPriority priority
    , DialogEvent & dialogEvent
    , DialogState ignoreState = STATE_INVALID) const;

  bool winningNodes(DialogPriority priority
    , std::list<DialogEvent> & dialogEvents
    , DialogState ignoreState = STATE_INVALID) const;

public:
  const std::string & dialogKey() const;

private:
  bool winningNodes(const std::list<DialogState> & priorityList
    , std::list<DialogEvent> & dialogEvents, DialogState ignoreState = STATE_INVALID) const;

private:
  std::string _dialogKey;
  DialogKeyFlag _dialogKeyFlag;
  DialogEvents _dialogEvents;
}; //class DialogCollateEvent;

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

inline DialogState DialogEvent::dialogState() const
{
  return _dialogElement.state;
}

inline DialogDirection DialogEvent::dialogDirection() const
{
  return _dialogElement.direction;
}

inline bool DialogEvent::newDialog() const
{
  return _newDialog;
}

inline void DialogEvent::newDialog(bool toggle)
{
  _newDialog = toggle;
}

inline const std::string & DialogCollateEvent::dialogKey() const
{
  return _dialogKey;
}

} } } // SIPX::Kamailio::Plugin


#endif  /* _DIALOGINFO_H_ */
