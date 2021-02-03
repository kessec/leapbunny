/* cnotify.c -- Post an event on the cart_events_socket to tell the
 *              application layer about a cartridge hotplug event.
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

#define APP_PAYLOAD_HOTPLUG	64
#define CART_SOCK		"/tmp/cartridge"

struct app_message {
	unsigned int type;
	unsigned int payload;
};

int create_report_socket(const char *path)
{
	struct sockaddr_un sa;
	int s, ret;

	s = socket(AF_UNIX, SOCK_STREAM, 0);
	if(s == -1)
		return s;

	memset(&sa, 0, sizeof(struct sockaddr_un));
	sa.sun_family = AF_UNIX;
	strncpy(sa.sun_path, path, strlen(path));

	ret = connect(s, (struct sockaddr *)&sa, sizeof(struct sockaddr_un));
	if(ret < 0) {
		close(s);
		return ret;
	}

	return s;
}


int main(void)
{
	struct app_message msg;
	int sock;

	sock = create_report_socket(CART_SOCK);
	if (sock < 0) {
		printf("Error: failed to create report socket\n");
		return 1;
	}

	msg.type = 1;
	msg.payload = APP_PAYLOAD_HOTPLUG;
	
	send(sock, &msg, sizeof(struct app_message), 0);
	
	close(sock);

	return 0;
}
