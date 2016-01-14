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

#include "DialogEventCollator/DialogCollatorPlugin.h"
#include "DialogEventCollator/DialogCollator.h"

#include <OSS/OSS.h>
#include <OSS/UTL/Logger.h>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>

#include <string>
#include <vector>
#include <map>

using namespace SIPX::Kamailio::Plugin;

/**
 *  module collate_plugin for kamailio
 */

extern "C" int collate_plugin_init(str* key, str* value, int size);
extern "C" int collate_plugin_destroy();
extern "C" int collate_handle_init(void** handle);
extern "C" int collate_handle_destroy(void** handle);
extern "C" str* collate_body_xml(void* handle, str* response, str* pres_user, str* pres_domain, str** body_array, int body_size);
extern "C" void free_collate_body(void* handle, char* body);

typedef boost::interprocess::named_mutex mutex;
typedef boost::interprocess::scoped_lock<mutex> mutex_lock;

extern "C" int bind_collate_plugin(collate_plugin_t* api)
{
  if (!api) {
    return -1;
  }

  api->collate_plugin_init = collate_plugin_init;
  api->collate_plugin_destroy = collate_plugin_destroy;
  api->collate_handle_init = collate_handle_init;
  api->collate_handle_destroy = collate_handle_destroy;
  api->collate_body_xml = collate_body_xml;
  api->free_collate_body = free_collate_body;
  return 0;
}

extern "C" int collate_plugin_init(str* key, str* value, int size)
{
  if(size > 0)
  {
    std::map<std::string, std::string> properties;
    for(int index = 0; index < size; ++index)
    {
      std::string str_key = key[index].len > 0 ? std::string(key[index].s, key[index].len) : std::string("");
      std::string str_value = value[index].len > 0 ? std::string(value[index].s, value[index].len) : std::string("");
      properties[str_key] = str_value;
    }

    //Check if required logger configuration is provided
    std::map<std::string, std::string>::iterator iter = properties.find("log-path");
    if(iter != properties.end())
    {
      std::string log_path = properties["log-path"];
      if(!log_path.empty())
      {
        std::string log_level = properties.find("log-level") != properties.end() ? properties["log-level"] : 0;
        try
        {
          int log_priority = boost::lexical_cast<int>(log_level);
          OSS::logger_init(log_path, static_cast<OSS::LogPriority>(log_priority));
        } catch(std::exception & ex)
        {
          return -1;
        }
      }

      OSS_LOG_INFO("[Plugin] Initialize collate plugin resources");
    }
  }

  return 0;
}

extern "C" int collate_plugin_destroy()
{
  //Clean up plugin allocated resources
  OSS_LOG_INFO("[Plugin] Destroy collate plugin resources");
  OSS::logger_deinit();

  return 0;
}

extern "C" int collate_handle_init(void** handle) 
{
  OSS_LOG_INFO("[Plugin] Initialize collate handle resources");
  
  DialogCollator * dialogCollator = new DialogCollator();
  if(!dialogCollator->connect())
  {
    OSS_LOG_ERROR("[Plugin] Unable to allocate dialog collator handle.");
    return -1;
  }

  *handle = reinterpret_cast<void*>(dialogCollator);
  return 0;
}

extern "C" int collate_handle_destroy(void** handle)
{
  OSS_LOG_INFO("[Plugin] Destroy handle resources");
  if(handle && *handle)
  {
    DialogCollator * collator = reinterpret_cast<DialogCollator*>(*handle);
    collator->disconnect();
    delete collator;
  }

  *handle = NULL;
  return 0;
}

extern "C" str* collate_body_xml(void* handle, str* response, str* pres_user, str* pres_domain, str** body_array, int body_size) 
{
  if(handle != NULL && body_array != NULL && body_size > 0) 
  {
    std::string user = pres_user ? std::string(pres_user->s, pres_user->len) : std::string("");
    std::string domain = pres_domain ? std::string(pres_domain->s, pres_domain->len) : std::string("");
    
    OSS_LOG_INFO("[Plugin] Started collate dialog body: " << user << "@" << domain);

    std::vector<std::string> payloads;
    for(int index = 0; index < body_size; ++index) 
    {
      str* body_index = body_array[index];
      std::string body = body_index ? std::string(body_index->s, body_index->len) : std::string("");;
      if(!body.empty()) 
      {
        payloads.push_back(body);
      }
    }

    if(!payloads.empty())
    {
      mutex named_mtx(boost::interprocess::open_or_create, "SIPX.Kamailio.Plugin.Mutex");
      mutex_lock lock(named_mtx);

      DialogCollator * dialogCollator = reinterpret_cast<DialogCollator*>(handle);
      std::string dialog_xml = dialogCollator->collateAndAggregatePayloads(user, domain, payloads);
      if(!dialog_xml.empty()) {
        char * response_char = new char[dialog_xml.size()];
        std::copy(dialog_xml.begin(), dialog_xml.end(), response_char);
        response->s = response_char;
        response->len = dialog_xml.size();
        return response; // Allocated response will be free when calling free_collate_body
      } else
      {
        OSS_LOG_WARNING("[Plugin] Returning an empty dialog body: " << user << "@" << domain);
      }
    }

    OSS_LOG_WARNING("[Plugin] Collating an empty dialog body: " << user << "@" << domain);
  }

  return NULL;
}

extern "C" void free_collate_body(void* handle, char* body) 
{
  OSS_LOG_INFO("[Plugin] Cleanup xml resources");
  if(body != NULL) {
    delete[] body;
  }
}