#ifndef MSG_H__
#define MSG_H__

extern int msg_init(void (*handler)(const char *buf, int len));
extern void msg_fini(void);
extern int msg_send(const char *buf, int len);

#endif