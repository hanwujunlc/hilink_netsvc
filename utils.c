#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <android/log.h>

#ifndef  LOG_TAG
#define  LOG_TAG    "hilink_netsvc"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#endif

int daemon_flag = 0;

int hilink_log(const const char *format, ...) {
    int ret = 0;
    char *msg;

    va_list ap;
    int argno = 0;
    va_start(ap, format);
    vasprintf(&msg, format, ap);
    va_end(ap);

    if (!daemon_flag) {
        fprintf(stderr, "%s", msg);
        fprintf(stderr, "%s", "\n");
        fflush(stderr);
    }
    else LOGI("%s", msg);

    free(msg);

    return ret;
}

void init_daemon(const char *dir)
{
    int i;

    int pid = fork();
    if (pid)
        exit(0);
    else if (pid < 0)
        exit(1);

    setsid();
    pid = fork();
    if (pid)
        exit(0);
    else if (pid < 0)
        exit(1);

    FILE *fp = fopen("/proc/sys/fs/file-max", "r");
    if (fp) {
        char max_file[32];
        fgets(max_file, sizeof max_file, fp);
        fclose(fp);
        int max_file_i = atoi(max_file);
        for (i = 0; i < max_file_i; ++i)
            close(i);
    }

    chdir(dir);

    umask(0);

    daemon_flag = 1;
}

static int lockfile(int fd)
{
    struct flock fl;

    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;

    return(fcntl(fd, F_SETLK, &fl));
}

int i_am_running(const char *name)
{
    char buf[16];

    int fd = open(name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0) {
        hilink_log("can't open %s", name);
        exit(1);
    }
    if (lockfile(fd) == -1) {
        if (errno == EACCES || errno == EAGAIN) {
            hilink_log("file: %s already locked", name);
            close(fd);
            return 1;
        }

        hilink_log("can't lock %s", name);
        exit(1);
    }
    ftruncate(fd, 0);
    sprintf(buf, "%ld", (long)getpid());
    write(fd, buf, strlen(buf) + 1);

    return 0;
}

int get_pid_by_name(const char *name)
{
  DIR *dir;
  struct dirent *ptr;
  FILE *fp;
  char filepath[64];
  char filetext[64];

  int ret = 0;

  dir = opendir("/proc");
  if (NULL == dir)
    return ret;

  while ((ptr = readdir(dir)) != NULL) {
    if ((strcmp(ptr->d_name, ".") == 0) || (strcmp(ptr->d_name, "..") == 0)) continue;
    if (DT_DIR != ptr->d_type) continue;

    sprintf(filepath, "/proc/%s/cmdline", ptr->d_name);
    fp = fopen(filepath, "r");
    if (NULL == fp) continue;

    fread(filetext, 1, 64, fp);
    filetext[63] = '\0';

    if (strstr(filetext, name)) {
      fclose(fp);

      ret = atoi(ptr->d_name);
      break;
    }

    fclose(fp);
  }

  closedir(dir);

  return ret;
}

int get_pid_by_names(const char *names[], int name_len)
{
  int i;
  DIR *dir;
  struct dirent *ptr;
  FILE *fp;
  char filepath[64];
  char filetext[64];

  int ret = 0;

  dir = opendir("/proc");
  if (NULL == dir)
    return ret;

  while ((ptr = readdir(dir)) != NULL) {
    if ((strcmp(ptr->d_name, ".") == 0) || (strcmp(ptr->d_name, "..") == 0)) continue;
    if (DT_DIR != ptr->d_type) continue;

    sprintf(filepath, "/proc/%s/cmdline", ptr->d_name);
    fp = fopen(filepath, "r");
    if (NULL == fp) continue;

    fread(filetext, 1, 64, fp);
    filetext[63] = '\0';

    for (i = 0; i < name_len; i++) {
      if (!strstr(filetext, names[i])) continue;

      fclose(fp);

      ret = atoi(ptr->d_name);
      break;
    }

    fclose(fp);
  }

  closedir(dir);

  return ret;
}


void trim_str(char *p)
{
  int i, j = 0, s = 0, l = 0;
  char *tmp;
  for (i = 0; p[i] != '\0'; i++) {
    if (p[i] != ' ' && p[i] != '\r'  && p[i] != '\n') {
      p[j++] = p[i];
      s = 1;
    }
    if (p[i] == ' ' && s) p[j++] = p[i];
  }
  p[j] = '\0';

  l = strlen(p);
  for (i = l - 1; i >= 0; i--){
    if (p[i] != ' ') break;
    else p[i] = '\0';
  }
}
