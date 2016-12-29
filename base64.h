#ifndef HILINK_BASE64_H__
#define HILINK_BASE64_H__

extern int b64_ntop(unsigned char const *src, size_t srclength, char *target, size_t targsize);
extern int b64_pton(char const *src, unsigned char *target, size_t targsize);

#endif
