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

#include "OSS/JSON/reader.h"
#include "OSS/JSON/writer.h"
#include "OSS/JSON/elements.h"

#include <string>
#include <list>

namespace SIPX {
namespace Kamailio {
namespace Plugin {

struct Dialog
{
  std::string id;
  std::string callId;
  std::string localTag;
  std::string remoteTag;
  std::string direction;
  std::string state;
  std::string remoteTarget;
  std::string localTarget;

  bool operator==(const Dialog& rhs);
  bool operator!=(const Dialog& rhs) { return !(*this == rhs); };

  std::string generateKey() const;
  bool valid() const;
  void toJsonObject(json::Object& object) const;
  void fromJsonObject(const json::Object& object);
};
    
struct DialogInfo
{
  std::string rawPayload;
  std::string entity;
  std::string state;
  std::string version;
  Dialog dialog;

  bool operator==(const DialogInfo& rhs);
  bool operator!=(const DialogInfo& rhs) { return !(*this == rhs); }

  bool generateKey(std::string & key) const;
  bool valid() const;
  void toJsonObject(json::Object& object) const;
  void fromJsonObject(const json::Object& object);
};

//
// Global Methods
//

bool parseDialogInfoXML(const std::string & xml, DialogInfo & dialogInfo);
int compareDialogByState(const Dialog & ldialog, const Dialog & rdialog);
int compareDialogByPreference(const Dialog & ldialog, const Dialog & rdialog);
int compareDialogInfo(const DialogInfo & ldialogInfo, const DialogInfo & rdialogInfo);

bool maskDialogInfoXMLVersion(std::string & xml);
bool mergeDialogInfoXMLDialog(std::string & xml, const DialogInfo & rdialogInfo);
bool updateDialogInfoXMLState(std::string & xml, const std::string & state);

} } } // SIPX::Kamailio::Plugin


#endif  /* _DIALOGINFO_H_ */
