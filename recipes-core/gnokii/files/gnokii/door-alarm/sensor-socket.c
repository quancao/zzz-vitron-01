#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <gnokii.h>
#include <gnokii/error.h>
#include <gnokii/common.h>

#include "gsmhelper.h"
#include "sensor-com.h"

extern gn_error shutdown_sensor_com(struct gn_statemachine *state);
extern gn_error init_sensor_com(struct gn_statemachine *state);

static int g_sock_fd;
static char g_buffer[256];
static void process_sensor_info(struct gn_statemachine *state, void *callback_data) 
{
    int len = read(g_sock_fd, g_buffer, sizeof(g_buffer));
    if (len < 0) {
        LOGMSG(LOG_LEVEL_ERROR, "Read communication socket fail.!\n");
        return;
    }
    sensor_msg_t* msg = (sensor_msg_t*)g_buffer;
    if (len < sizeof(*msg) || msg->magic != SENSOR_PROTO_MAGIC) {
        LOGMSG(LOG_LEVEL_ERROR, "Sensor msg is too short or not have magic. Drop \n");
        return;
    }
    switch(msg->command) {
        case SENSOR_COMMAND_STATE_CHANGE:
            //TODO: Process sensor message
            LOGMSG(LOG_LEVEL_ERROR, "Got sensor state change msg. Do processing now !\n");
            break;
        default:
            LOGMSG(LOG_LEVEL_ERROR, "Unknown sensor command: 0x%x\n", msg->command);
            break;
    }
}

gn_error init_sensor_com(struct gn_statemachine *state)
{
    struct sockaddr_un server;

    g_sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (g_sock_fd < 0) {
        return GN_ERR_FAILED; 
    }
    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, SENSOR_SOCKET_NAME);
    if (bind(g_sock_fd, (struct sockaddr *) &server, sizeof(struct sockaddr_un))) {
        LOGMSG(LOG_LEVEL_ERROR, "binding stream socket");
        return GN_ERR_FAILED;
    }
    LOGMSG(LOG_LEVEL_INFO, "Socket has name %s\n", server.sun_path);
    listen(g_sock_fd, 5);
    state->sensorFd = g_sock_fd;
    state->callbacks.on_sensor = process_sensor_info; 
    return GN_ERR_NONE;

}

gn_error shutdown_sensor_com(struct gn_statemachine *state)
{
    close(g_sock_fd);
    unlink(SENSOR_SOCKET_NAME);
    g_sock_fd = -1;
    state->sensorFd = -1;
    state->callbacks.on_sensor = NULL; 
    return GN_ERR_NONE;
}
