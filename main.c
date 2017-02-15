#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/inotify.h>
#include <sys/limits.h>
#include <sys/poll.h>
#include <pthread.h>

#include "utils.h"
#include "msg.h"

#include "hilink_profile.h"

static struct pollfd *ufds;
static int nfds;
static int pipes[2];

static pthread_mutex_t term_lock;
static pthread_cond_t term_cond;
static int term_flag = 0;

static void msg_handler(const char *buf, int len) {
	hilink_log("receice msg from app: [%d]%s", len, buf);
	if (strstr(buf, "profile")) hilink_msg_handler(buf, len);
	else if (strstr(buf, "network")) handle_network_msg(buf, len);
	else hilink_log("unexcepted msg received!!!");
    bzero((char *)buf, len);
}

static void sig_handler(int sig_num) {
    char c = 0;

    write(pipes[1], &c, 1);
}

static void usage(char *name) {
    fprintf(stderr, "\tsorry, help is not ready for %s", name);
}

extern int daemon_flag;
int main(int argc, char *argv[]) {
  int i;
  int c;

  do {
    c = getopt(argc, argv, "bh");
    if (c == EOF) break;

    switch (c) {
      case 'b':
        daemon_flag = 1;
        break;
      case 'h':
        usage(argv[0]);
        exit(1);
      }
    } while (1);

  if (daemon_flag) {
    char dir[BUFSIZ];
    readlink("/proc/self/exe", dir, BUFSIZ);
    dir[strrchr(dir, '/') - dir] = '\0';

    init_daemon(dir);
  }

  if (i_am_running("/data/hilink_netsvc.pid")) {
    hilink_log("i am running, exit now.");
    return -1;
  }

  system("mkdir -p /data/misc/hilink");

  struct sigaction sigact;
  sigact.sa_handler = sig_handler;
  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags = 0;
  sigaction(SIGINT, &sigact, NULL);
  sigaction(SIGTERM, &sigact, NULL);
  sigaction(SIGQUIT, &sigact, NULL);

  pthread_mutex_init(&term_lock, 0);
  pthread_cond_init(&term_cond, 0);

  hilink_log("hilink_netsvc 1.0 (the revenge) firing up");

  pipe(pipes);

  nfds = 1;
  ufds = calloc(nfds, sizeof(ufds[0]));
  ufds[0].fd = pipes[0];
  ufds[0].events = POLLIN;

  if (config_network() != 0) {
    hilink_log("config_init error");
    return -1;
  }

  if (msg_init(msg_handler) != 0) {
    hilink_log("msg_init error");
    return -1;
  }

  while (1) {
    poll(ufds, nfds, -1);

    if (ufds[0].revents & POLLIN) break;
  }

  close(pipes[0]);
  close(pipes[1]);

  term_flag = 1;
  pthread_mutex_lock(&term_lock);
  pthread_cond_signal(&term_cond);
  pthread_mutex_unlock(&term_lock);

  pthread_mutex_destroy(&term_lock);
  pthread_cond_destroy(&term_cond);

  msg_fini();

  hilink_log("hilink_netsvc 1.0 shutdown");

  return 0;
}
