#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#define LOCAL_PORT	43351
#define REMOTE_PORT 43352

static int sock = -1;
static pthread_t msg_tid;
static const char *bye = "BYE";
static void (*msg_handler)(const char *buf, int len) = NULL;

static void *msg_proc(void *arg) {
	char buf[512];
	struct sockaddr_in addrfrom;
	socklen_t fromlen;

	for(;;) {
		int ret = recvfrom(sock, buf, sizeof buf, 0, (struct sockaddr *)&addrfrom, (socklen_t *)&fromlen);
		if (ret <= 0) {
			hilink_log("recvfrom error");
			break;
		}
		else if (ret == strlen(bye) && memcmp(bye, buf, strlen(bye)) == 0) {
			hilink_log("msg terminate");
			break;
		}

		(*msg_handler)(buf, ret);

		hilink_log("receive msg (len = %d) from %s", ret, inet_ntoa(addrfrom.sin_addr));
	}

	return 0;
}

int msg_init(void (*handler)(const char *buf, int len)) {
	struct sockaddr_in addr;
	memset(&addr, 0x00, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(LOCAL_PORT);

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == -1) {
		hilink_log("socket error(%d:%s)", errno, strerror(errno));
		return -1;
	}
	if (bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {
		hilink_log("bind error(%d:%d:%s)", sock, errno, strerror(errno));
		return -1;
	}

	msg_handler = handler;
	pthread_create(&msg_tid, 0, msg_proc, 0);

	return 0;
}

void msg_fini(void) {
	struct sockaddr_in addrto;
	socklen_t lento = sizeof(struct sockaddr_in);
	memset(&addrto, 0x00, sizeof(struct sockaddr_in));
	addrto.sin_family = AF_INET;
	addrto.sin_addr.s_addr = inet_addr("127.0.0.1");
	addrto.sin_port = htons(LOCAL_PORT);

	int s = socket(AF_INET, SOCK_DGRAM, 0);
	sendto(s, bye, strlen(bye), 0, (struct sockaddr *)&addrto, lento);
	pthread_join(msg_tid, 0);

	close(s);
	close(sock);
	sock = -1;
	msg_handler = NULL;
}

int msg_send(const char *buf, int len) {
	struct sockaddr_in addrto;
	socklen_t lento = sizeof(struct sockaddr_in);
	memset(&addrto, 0x00, sizeof(struct sockaddr_in));
	addrto.sin_family = AF_INET;
	addrto.sin_addr.s_addr = inet_addr("127.0.0.1");
	addrto.sin_port = htons(REMOTE_PORT);

	return sendto(sock, buf, len, 0, (struct sockaddr *)&addrto, lento);
}
