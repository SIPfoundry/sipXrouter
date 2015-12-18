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

#ifndef DIALOG_COLLATOR_H_
#define DIALOG_COLLATOR_H_

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

  typedef int (*collate_plugin_init_t)(str* key, str* value, int size);
  typedef int (*collate_plugin_destroy_t)();
  typedef int (*collate_handle_init_t)(void** handle);
  typedef int (*collate_handle_destroy_t)(void** handle);
  typedef str* (*collate_body_xml_t)(void* handle, str* response, str* pres_user, str* pres_domain, str** body_array, int n);
  typedef void (*free_collate_body_t)(void* handle, char* body);

  typedef struct collate_plugin_exports_ {
    collate_plugin_init_t collate_plugin_init;
    collate_plugin_destroy_t collate_plugin_destroy;
    collate_handle_init_t collate_handle_init;
    collate_handle_destroy_t collate_handle_destroy;
    collate_body_xml_t collate_body_xml;
    free_collate_body_t free_collate_body;
  } collate_plugin_t;
  
  int bind_collate_plugin(collate_plugin_t* api);
  typedef int (*bind_collate_plugin_t)(collate_plugin_t* api);
}

#endif // ! DIALOG_COLLATOR_H_