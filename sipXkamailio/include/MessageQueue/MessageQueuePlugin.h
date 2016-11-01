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

#ifndef KAMAILIO_MESSAGE_QUEUE_H_
#define KAMAILIO_MESSAGE_QUEUE_H_

extern "C" {
#ifdef SER
#include "str.h"
#else
  struct _str {
    char* s; /**< Pointer to the first character of the string */
    int len; /**< Length of the string */
  };
  typedef _str str;
#endif

  typedef void (*on_message_event_t)(char* channel, int channelLen, char* data, int dataLen, void* privdata);
  typedef int (*message_queue_init_t)(str* key, str* value, int size);
  typedef int (*message_queue_destroy_t)();
  typedef int (*message_queue_subscribe_event_t)(str* channel, void* priv, on_message_event_t message_event);

  typedef struct message_queue_plugin_exports_ {
    message_queue_init_t message_queue_plugin_init;
    message_queue_destroy_t message_queue_plugin_destroy;
    message_queue_subscribe_event_t message_queue_subscribe_event;
  } message_queue_plugin_t;
  
  int bind_message_queue_plugin(message_queue_plugin_t* api);
  typedef int (*bind_message_queue_plugin_t)(message_queue_plugin_t* api);
}

#endif // ! KAMAILIO_MESSAGE_QUEUE_H_