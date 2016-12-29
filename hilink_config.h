#ifndef HILINK_CONFIG_H__
#define HILINK_CONFIG_H__

extern int handle_network_msg(const char *buf, int len);
extern int config_network(void);
extern int unconfig_network(void);

extern int config_profile(svc_info_t *svcs, int svc_len);
extern void unconfig_profile(void);

#endif
