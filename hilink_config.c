#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <pthread.h>
#include <errno.h>

#include "utils.h"
#include "msg.h"

#include "hilink_link.h"
#include "hilink_profile.h"

static hilink_s_context hilink_s_ctx;
static hilink_s_pkt0len st_pkt0len;
static hilink_s_result hilink_s_res;

#define WIFI_RX_LEN0_OPEN    80
#define WIFI_RX_LEN0_WEP     88
#define WIFI_RX_LEN0_TKIP    100
#define WIFI_RX_LEN0_AES     96

static int config_parse_cb(const void *frame, unsigned short len)
{
    int ret;

    ret = hilink_link_parse(frame, len);
	switch(ret)
	{
	case HI_WIFI_STATUS_CHANNEL_LOCKED:
	case HI_WIFI_STATUS_CHANNEL_UNLOCKED:
		break;
	case HI_WIFI_STATUS_FINISH:
		{
		    memset(&hilink_s_res, 0, sizeof(hilink_s_result));
			if (hilink_link_get_result(&hilink_s_res) == 0)
			{
				//wifi_sta_connect(hilink_s_res.ssid, hilink_s_res.ssid_len, hilink_s_res.pwd, hilink_s_res.pwd_len, hilink_s_res.enc_type);
			}
		}
		break;
	}

    return ret;
}

static int fetch_mac(const char *ifname, char *mac) {
	int sockfd, ret = -1;
	struct sockaddr_in sin;
	struct ifreq ifr;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd == -1) return ret;

	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

	if (ioctl(sockfd, SIOCGIFADDR, &ifr) == 0) {
		memcpy(&sin, &ifr.ifr_addr, sizeof(ifr.ifr_addr));
		hilink_log("%s ip address: %s", ifname, inet_ntoa(((struct sockaddr_in *)&sin)->sin_addr));
	}

	if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) == 0) {
		memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
		ret = 0;
	}
	close(sockfd);

	return ret;
}

int handle_network_msg(const char *buf, int len) {
	return 0;
}

char tv_ssid[256];
int config_network(void) {
	char mac[6], mac_str[16], key[22];
	unsigned int ssid_len = 256;
	int ret;

	memset(&hilink_s_ctx, 0, sizeof(hilink_s_context));
	if (hilink_link_init(&hilink_s_ctx) != 0) {
		hilink_log("hilink_link_init error!");
		return -1;
	}

	st_pkt0len.len_open = WIFI_RX_LEN0_OPEN;
	st_pkt0len.len_wep  = WIFI_RX_LEN0_WEP;
	st_pkt0len.len_tkip = WIFI_RX_LEN0_TKIP;
	st_pkt0len.len_aes  = WIFI_RX_LEN0_AES;
	hilink_link_set_pkt0len(&st_pkt0len);

	if (fetch_mac("wlan0", mac) == -1) {
		hilink_log("fetch_mac error!");
		return -1;
	}

	memset(key, 0x30, sizeof key);
	sprintf(mac_str, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	memcpy(key + 17, mac_str + 7, 5);

	memset(tv_ssid, 0x00, sizeof tv_ssid);
	ret = hilink_link_get_devicessid("11", "900q", "C", "B", key, tv_ssid, &ssid_len);
	hilink_log("ssid = %s ssid_len = %d (%d)", tv_ssid, ssid_len, ret);

	return 0;
}

void unconfig_network(void) {
}


static pthread_t profile_tid;
static pthread_mutex_t term_lock;
static pthread_cond_t term_cond;
static int term_flag = 0;

static void *profile_proc(void *arg) {
	int ret;
    while(!term_flag)
    {
        ret = hilink_m2m_process();

        if(ret != 0) hilink_log("hilink_m2m_process error: %d", ret);
		ret = -1;
		struct timeval now;
		gettimeofday(&now, NULL);
		struct timespec wait_time = { now.tv_sec, now.tv_usec * 1000 + 1000 * 1000 * 50 };
		pthread_mutex_lock(&term_lock);
		while (term_flag == 0) {
		  ret = pthread_cond_timedwait(&term_cond, &term_lock, &wait_time) != ETIMEDOUT ? 0 : -1;
		  if (ret == -1) break;
		}
		pthread_mutex_unlock(&term_lock);

		if (ret == 0) break;
    }
}

static dev_info_t dev_tv = {
	"B4430DDE6A17",
	"900q",
	"HBL-25",
	"02E",
	"001",
	"B4:43:0D:DE:6A:17",
	"1.0",
	"20000",
	"20000",
	"1.0",
	1,
};

char tv_code[64];

int config_profile(svc_info_t *svcs, int svc_len) {
	static char mac[6], mac_str[32], sn[32];
	if (fetch_mac("wlan0", mac) == -1) {
		hilink_log("fetch_mac error!");
		return -1; 
	}
	sprintf(sn, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	static dev_info_t dev = {
		tv_code,
		"1010",
		"A2",
		"02E",
		"034",
		mac_str,
		"1.0",
		"20000",
		"20000",
		"1.0",
		1,
	};

	//int ret = hilink_m2m_init(&dev,  (svc_info_t*)&svc_aircon, 6);
	int ret = hilink_m2m_init(&dev, svcs, svc_len);
	
	if (ret != 0) {
		hilink_log("hilink_m2m_init error %d!", ret);
		return -1; 
	}

	term_flag = 0;
	pthread_mutex_init(&term_lock, 0);
	pthread_cond_init(&term_cond, 0);

	pthread_create(&profile_tid, 0, profile_proc, 0);
}

void unconfig_profile(void) {
	term_flag = 1;
	pthread_mutex_lock(&term_lock);
	pthread_cond_signal(&term_cond);
	pthread_mutex_unlock(&term_lock);

	pthread_join(profile_tid, 0);

	pthread_mutex_destroy(&term_lock);
	pthread_cond_destroy(&term_cond);
}
