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
#include <list>
#include <map>

using namespace SIPX::Kamailio::Plugin;

#define PLUGIN_MUTEX_ID "SIPX.Kamailio.Plugin.Mutex"

typedef boost::interprocess::named_mutex mutex;
typedef boost::interprocess::scoped_lock<mutex> mutex_lock;

/**
 * Global Functions
 */

std::string str_to_string(str * value, const std::string & def)
{
  if(value != NULL && value->len > 0)
  {
    return std::string(value->s, value->len);
  }

  return def;
}

std::string str_to_string(str * value)
{
  return str_to_string(value, "");
}

str * string_to_str(const std::string value, str * output)
{
  if(output != NULL)
  {
    char * buffer = new char[value.size()];
    std::copy(value.begin(), value.end(), buffer);
    output->s = buffer;
    output->len = value.size();
  }

  return output;
}

/**
 *  module collate_plugin for kamailio
 */

extern "C" int collate_plugin_init(str* key, str* value, int size);
extern "C" int collate_plugin_destroy();
extern "C" int collate_handle_init(void** handle);
extern "C" int collate_handle_destroy(void** handle);
extern "C" int collate_handle_is_active(void* handle, str* pres_user, str* pres_domain);
extern "C" int collate_handle_queue_dialog(void* handle, str* pres_user, str* pres_domain, str* body);
extern "C" str* collate_queue_xml(void* handle, str* response, str* pres_user, str* pres_domain);
extern "C" str* collate_notify_xml(void* handle, str* response, str* pres_user, str* pres_domain, str* body);
extern "C" str* collate_body_xml(void* handle, str* response, str* pres_user, str* pres_domain, str** body_array, int body_size);
extern "C" void free_collate_body(void* handle, char* body);

extern "C" int bind_collate_plugin(collate_plugin_t* api)
{
  if (!api) {
    return -1;
  }

  api->collate_plugin_init = collate_plugin_init;
  api->collate_plugin_destroy = collate_plugin_destroy;
  api->collate_handle_init = collate_handle_init;
  api->collate_handle_destroy = collate_handle_destroy;
  api->collate_handle_is_active = collate_handle_is_active;
  api->collate_handle_queue_dialog = collate_handle_queue_dialog;
  api->collate_queue_xml = collate_queue_xml;
  api->collate_notify_xml = collate_notify_xml;
  api->collate_body_xml = collate_body_xml;
  api->free_collate_body = free_collate_body;

  OSS_LOG_DEBUG("[Plugin] Bind collate plugin");
  return 0;
}

extern "C" int collate_plugin_init(str* key, str* value, int size)
{
  if(size > 0)
  {
    std::map<std::string, std::string> settings;
    for(int index = 0; index < size; ++index)
    {
      std::string str_key = str_to_string(key+index);
      if(!str_key.empty())
      {
        std::string str_value = str_to_string(value+index);
        settings[str_key] = str_value;  
      }
    }

    DialogCollatorPlugin::start(settings);

    OSS_LOG_DEBUG("[Plugin] Init collate plugin");
  }

  return 0;
}

extern "C" int collate_plugin_destroy()
{
  OSS_LOG_DEBUG("[Plugin] Destroy collate plugin");
  DialogCollatorPlugin::stop();
  return 0;
}

extern "C" int collate_handle_init(void** handle) 
{
  OSS_LOG_DEBUG("[Plugin] Init collate handle");
  DialogCollatorPlugin::create(handle);
  return 0;
}

extern "C" int collate_handle_destroy(void** handle)
{
  OSS_LOG_DEBUG("[Plugin] Destroy collate handle");
  DialogCollatorPlugin::destroy(handle);
  *handle = NULL;
  return 0;
}

extern "C" int collate_handle_is_active(void* handle, str* pres_user, str* pres_domain)
{
  if(handle != NULL) 
  {
    std::string user = str_to_string(pres_user);
    std::string domain = str_to_string(pres_domain);
    
    mutex namedMutex(boost::interprocess::open_or_create, PLUGIN_MUTEX_ID);
    mutex_lock lock(namedMutex);
    return DialogCollatorPlugin::isActive(handle, user, domain) ? 1 : -1;
  }

  return -1;
}

extern "C" int collate_handle_queue_dialog(void* handle, str* pres_user, str* pres_domain, str* body)
{
  if(handle != NULL) 
  {
    std::string user = str_to_string(pres_user);
    std::string domain = str_to_string(pres_domain);
    std::string content = str_to_string(body);

    mutex namedMutex(boost::interprocess::open_or_create, PLUGIN_MUTEX_ID);
    mutex_lock lock(namedMutex);
    return DialogCollatorPlugin::queueDialog(handle, user, domain, content) ? 0 : -1;
  }

  return -1;
}


extern "C" str* collate_queue_xml(void* handle, str* response, str* pres_user, str* pres_domain)
{
  if(handle != NULL) 
  {
    std::string user = str_to_string(pres_user);
    std::string domain = str_to_string(pres_domain);
    std::string xml;

    mutex namedMutex(boost::interprocess::open_or_create, PLUGIN_MUTEX_ID);
    mutex_lock lock(namedMutex);
    if(DialogCollatorPlugin::collateQueue(handle, user, domain, xml))
    {
      return string_to_str(xml, response);
    }

    if(response->len <= 0)
    {
      OSS_LOG_DEBUG("[Plugin] Returning an empty queue dialog body: " << user << "@" << domain);
    }
  }

  return NULL;
}

extern "C" str* collate_notify_xml(void* handle, str* response, str* pres_user, str* pres_domain, str* body)
{
  if(handle != NULL) 
  {
    std::string user = str_to_string(pres_user);
    std::string domain = str_to_string(pres_domain);
    std::string notifyBody = str_to_string(body);
    std::string xml;

    mutex namedMutex(boost::interprocess::open_or_create, PLUGIN_MUTEX_ID);
    mutex_lock lock(namedMutex);
    if(DialogCollatorPlugin::collateNotify(handle, user, domain, notifyBody, xml))
    {
      return string_to_str(xml, response);
    }

    if(response->len <= 0)
    {
      OSS_LOG_DEBUG("[Plugin] Returning an empty notify dialog body: " << user << "@" << domain);
    }
  }

  return NULL;
}


extern "C" str* collate_body_xml(void* handle, str* response, str* pres_user, str* pres_domain, str** body_array, int body_size) 
{
  if(handle != NULL && body_array != NULL && body_size > 0) 
  {
    std::string user = str_to_string(pres_user);
    std::string domain = str_to_string(pres_domain);
    
    OSS_LOG_INFO("[DialogCollatorPlugin] Started collate dialog body: " << user << "@" << domain);

    std::list<std::string> payloads;
    for(int index = 0; index < body_size; ++index) 
    {
      std::string body = str_to_string(body_array[index]);
      if (OSS::log_get_level() >= OSS::PRIO_DEBUG)
      {
        OSS_LOG_DEBUG("[DialogCollatorPlugin] Dialog Event Payloads " << body);
      }

      if(!body.empty()) 
      {
        payloads.push_back(body);
      }
    }

    std::string xml;
    if(!payloads.empty())
    {
      mutex namedMutex(boost::interprocess::open_or_create, PLUGIN_MUTEX_ID);
      mutex_lock lock(namedMutex);
      if(DialogCollatorPlugin::processEvents(handle, user, domain, payloads, xml)) 
      {
        return string_to_str(xml, response);
      }

      if(response->len <= 0)
      {
        OSS_LOG_DEBUG("[Plugin] Returning an empty dialog body: " << user << "@" << domain);
      }
    }
  }

  return NULL;
}

extern "C" void free_collate_body(void* handle, char* body) 
{
  OSS_LOG_INFO("[Plugin] Cleanup xml resources");
  if(body != NULL) 
  {
    delete[] body;
  }
}