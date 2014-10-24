#ifndef __SENSOR_COM_H__
#define __SENSOR_COM_H__

#include <stdint.h>

#define SENSOR_SOCKET_NAME "sms.sensor.socket"
#define SENSOR_PROTO_MAGIC 0xCAFECAC0

#define SENSOR_COMMAND_STATE_CHANGE 0x1

#define SENSOR_STATE_OFF 0x0
#define SENSOR_STATE_ON 0x1
typedef struct sensor_msg_s {
	uint32_t magic;
	uint32_t command;
	uint32_t state;
	char	 data[0];
} sensor_msg_t;

#define default_sensor_msg(msg, cmd, state_) do {\
	(msg)->magic = SENSOR_PROTO_MAGIC;\
	(msg)->command = SENSOR_COMMAND_STATE_CHANGE;\
	(msg)->state = state_;\
} while(0)

#endif /*  __SENSOR_COM_H__ */
