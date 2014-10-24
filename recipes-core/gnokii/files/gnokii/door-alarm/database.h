#ifndef __DATABASE_H__
#define __DATABASE_H__

#include <gnokii.h>
#include <gnokii/error.h>
#include <gnokii/common.h>

typedef struct phone_entry_s {
    struct phone_entry_s* next;
    char* phoneNo;
    char* name;
} phone_entry_t;

typedef struct sensor_device_s {
    struct sensor_device_s* next;
    char* sensorId;
    char* sensorName;
} sensor_device_t;

gn_error db_phone_load(const char* dbDir);
gn_error db_register_phone(const char* phoneNo, const char* name);
gn_error db_unregister_phone(const char* phoneNo);

#endif

