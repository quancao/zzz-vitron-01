#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <unistd.h>

#include "sensor-com.h"

/*
 * Send a datagram to a receiver whose name is specified in the command
 * line arguments.  The form of the command line is <programname> <pathname>
 */


int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_un sock_name;

    /* Create socket on which to send. */
    sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("opening datagram socket");
        _exit(1);
    }
    /* Construct name of socket to send to. */
    sock_name.sun_family = AF_UNIX;
    strcpy(sock_name.sun_path, SENSOR_SOCKET_NAME);

    sensor_msg_t msg;
    default_sensor_msg(&msg, SENSOR_COMMAND_STATE_CHANGE, SENSOR_STATE_ON);
    /* Send message. */
    if (sendto(sock, &msg, sizeof(msg), 0,
        &sock_name, sizeof(struct sockaddr_un)) < 0) {
        perror("sending datagram message");
    }
    close(sock);
    return 0;
}

