#ifndef UTILS_H__
#define UTILS_H__

extern int hilink_log(const const char *format, ...);
extern void init_daemon(const char *dir);
extern int i_am_running(const char *name);
extern int get_pid_by_name(const char *name);
extern void trim_str(char *p);

#endif