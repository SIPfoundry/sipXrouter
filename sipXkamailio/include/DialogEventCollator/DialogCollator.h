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
#include <boost/unordered_map.hpp>
#include <OSS/Persistent/RedisClient.h>
#include <vector>
#include <map>

namespace SIPX {
namespace Kamailio {
namespace Plugin {
    
class DialogCollator : boost::noncopyable
{
public:
  typedef std::vector<DialogInfo> CollatedDialogs;
  typedef boost::unordered_map<std::string, DialogInfo> AggregatedDialogs;

public:
  DialogCollator();
  ~DialogCollator();

  bool connect(const std::string& password = "", int db = 0);
  void disconnect();

  bool collateLocalPayloads(const std::string & user, const std::string & domain
      , const std::vector<std::string> & payloads
      , CollatedDialogs & collatedDialogs);

  bool collateRemotePayloads(const std::string & user, const std::string & domain
      , CollatedDialogs & collatedDialogs);

  void aggregateLocalPayloads(const std::string & user, const std::string & domain
      , const CollatedDialogs & collatedDialogs
      , AggregatedDialogs & aggregateDialogs);

  bool aggregateRemotePayloads(const std::string & user, const std::string & domain
      , AggregatedDialogs & aggregateDialogs);

  void finalizeAggregatePayloads(const std::string & user, const std::string & domain
      , AggregatedDialogs & aggregateDialogs);

  std::string generateDialogPayload(const AggregatedDialogs & aggregateDialogs);

  std::string collateAndAggregatePayloads(const std::string & user, const std::string & domain
      , const std::vector<std::string> & payloads);
  
  void flushDialog(const std::string & user, const std::string & domain);

private:
  std::string generateKey(const std::string & user, const std::string & domain, const Dialog & dialog);
  
  bool setDialogInfo(const std::string & key, const DialogInfo & dialogInfo);
  bool getDialogInfo(const std::string & key, DialogInfo & dialogInfo);
  bool getAllDialogInfo(const std::string & user, const std::string & domain, std::vector<json::Object> & dialogInfos);
  bool getAllDialogInfo(const std::string & user, const std::string & domain, CollatedDialogs & dialogInfos);
    
private:
    OSS::RedisClient _redisClient;
};

} } } // SIPX::Kamailio::Plugin

#endif  /* DIALOGCOLLATOR_H */

