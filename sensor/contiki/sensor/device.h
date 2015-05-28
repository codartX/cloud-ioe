/*
 *  Created by Jun Fang on 14-11-24.
 *  Copyright (c) 2014年 Jun Fang. All rights reserved.
 */

#ifndef _DEVICE_H
#define _DEVICE_H

#include "main.h"
#include "policy.h"
#include "object.h"
#include "resource.h"

typedef struct _device_t {
    uint8_t device_id[DEV_ID_SIZE];
    object_instance_t *obj_list;
    dev_policy_t *policy_list; 
    uint32_t timestamp;
} device_t;

extern device_t g_device;

int32_t device_init(uint8_t *device_id);

void device_deinit();

int32_t device_insert_object(object_instance_t *object);

int32_t device_remove_object(uint8_t *object_name);

object_instance_t *device_find_object(uint8_t *object_name);

int32_t device_insert_policy(dev_policy_t *policy);

int32_t device_remove_policy(uint32_t policy_id);

dev_policy_t *device_find_policy(uint32_t policy_id);

#endif
