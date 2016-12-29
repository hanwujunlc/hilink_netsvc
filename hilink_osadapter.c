#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/time.h>

#include "hilink_osadapter.h"

int hilink_sec_get_Ac(unsigned char* pAc, unsigned int ulLen)
{
	unsigned char AC[48] = {0x5F, 0x33, 0x63, 0x32, 0x4D, 0x61, 0x63, 0x2B, 0x7D, 0x4B, 0x32, 0x73, 
		0x2F, 0x34, 0x3D, 0x3A, 0x7B, 0x99, 0x73, 0x75, 0xBC, 0xBF, 0xA3, 0x26, 0x8B, 0x53, 0xB3, 
		0x22, 0x08, 0xDE, 0x6E, 0x76, 0x2F, 0xB8, 0xAE, 0xA9, 0xA2, 0x18, 0x16, 0x7F, 0x32, 0xD7, \
		0xB1, 0x6A, 0xCF, 0xC7, 0xCF, 0x98};

    if (NULL == pAc)
    {
        hilink_printf("\n\r invalid PARAM\n\r");
        return -1;
    }

    hilink_memcpy(pAc, AC, 48);
    return 0;
}
/*******************************************************************************
** 网络状态及参数
*******************************************************************************/

/*
 * 网络状态
 * arguments: 	state[out] 		网络状态，0为断开或已连接但未分配ip，
 								1为已连接且分配ip
 * return:
 */
int hilink_network_state(int* state) {
	*state = 1;
	return 0;
}

/*
 * 获取本机ip地址
 * arguments: 	local_ip[out]		本机ip地址
			  	ip_len[in]			ip地址长度
 * return:
 */
 #define IP_LEN 16
int hilink_get_local_ip(char* local_ip, unsigned char ip_len) {
	struct ifreq ifr;
	char ip_buf[IP_LEN];
        (void)ip_len;

	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		printf("get local ip error\n");
		return -1;
	}

	ifr.ifr_addr.sa_family = AF_INET;

	strncpy(ifr.ifr_name, "eth0", (IFNAMSIZ - 1));
	if(ioctl(fd, SIOCGIFADDR, &ifr) != 0) {
		strncpy(ifr.ifr_name, "wlan0", (IFNAMSIZ - 1));
		if(ioctl(fd, SIOCGIFADDR, &ifr) != 0) {
			close(fd); 
			return -1;
		}
    }

	close(fd);

	memset(ip_buf, 0, IP_LEN);

	sprintf(ip_buf, "%s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

	strncpy(local_ip, ip_buf, strlen(ip_buf));

	return 0;
}

/*
 * dns解析ip地址
 * arguments: 	hostname    [IN] 远端主机名称。
 *				ip_list     [IN] 存放远端主机ip地址列表的数组。
 *				num         [IN] 存放远端主机ip地址列表的数组的大小，范围为[1，4]。
 * return: 0 成功， -1 失败
 */
int hilink_gethostbyname(char* hostname, char ip_list[][40], int num)
{
       struct in_addr sin_addr;
       
       if (!inet_aton(hostname, &sin_addr)) {
                     struct hostent *hp;
                     hp = gethostbyname(hostname);
                     if (!hp) 
                     {
                            return -1;                        
                     }
                     int i = 0;
                     for(i = 0; (hp->h_addr_list[i] != 0) && (i < num); i++)
                     {
                            printf("tmp ip = %s\n", inet_ntoa(*((struct in_addr*)hp->h_addr_list[i])));
                            memcpy(ip_list[i], inet_ntoa(*((struct in_addr*)hp->h_addr_list[i])), 
                                   strlen(inet_ntoa(*((struct in_addr*)hp->h_addr_list[i]))));                             
                     }
       }
       
       return 0;
}


/*******************************************************************************
** 系统时间
*******************************************************************************/

/*
 * 获取当前时间
 * arguments: 	ms[out]			以毫秒为单位
 * return:
 */
int hilink_gettime(unsigned long* ms) {
	int ret = 0;
	struct timeval tv;
	struct timezone tz;

	memset(&tz, 0, sizeof(struct timezone));
	ret = gettimeofday(&tv, &tz);

	*ms = (unsigned long)(tv.tv_usec / 1000 + tv.tv_sec * 1000);

	return ret;
}


/*******************************************************************************
** 随机数
*******************************************************************************/

int hilink_rand(void) {
	return rand();
}

void hilink_srand(unsigned int value) {
	srand(value);
}


/*******************************************************************************
** flash读写
*******************************************************************************/

int hilink_save_flash(char* buf, unsigned int len) {
	FILE *fp = fopen("/data/misc/hilink/m2m_info.txt","wb+");
	if(fp == NULL) {
		printf("open file failed!\r\n");
		return -1;
	}

	if(fwrite(buf, len, 1, fp) != 1) {
		printf("save flash Error!\r\n");
    	fclose(fp);
		return -1;
	}

	fclose(fp);
	return 0;
}

int hilink_read_flash(char* buf, unsigned int len) {
	FILE *fp = fopen("/data/misc/hilink/m2m_info.txt","rb+");
	if(fp == NULL) {
		printf("open file failed!\r\n");
		return -1;
	}

	if(fread(buf, len, 1, fp) != 1) {
		printf("read flash Error!\r\n");
    	fclose(fp);
		return -1;
	}

	fclose(fp);
	return 0;
}

/*******************************************************************************
** 字符串操作
*******************************************************************************/

unsigned int hilink_strlen(const char* src) {
	return (unsigned int)strlen(src);
}

char* hilink_strncpy(char* dst, const char* src, unsigned int len) {
	return strncpy(dst, src, len);
}

char* hilink_strncat(char *dst, const char *src, unsigned int len) {
	return strncat(dst, src, len);
}

int hilink_strncmp(const char* str1, const char* str2, unsigned int len) {
	return strncmp(str1, str2, len);
}

char* hilink_strchr(char* str,int ch) {
	return strchr(str, ch);
}

char* hilink_strrchr(const char*str, char c) {
	return strrchr(str, c);
}

int hilink_atoi(const char* str) {
	return atoi(str);
}


int hilink_snprintf(char* buf, unsigned int maxlen, const char* format, ...) {
	va_list args;
	int ret;

	va_start(args, format);
	ret = vsnprintf(buf, maxlen, format, args);
	va_end(args);

	return ret;
}

int hilink_sprintf(char* buf, const char* format, ...) {
	va_list args;
	int ret;

	va_start(args, format);
	ret = vsprintf(buf, format, args);
	va_end(args);

	return ret;
}

#include <android/log.h>
#define  LOG_TAG    "hilink_netsvc"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
extern int daemon_flag;
int hilink_printf(const char* format, ...) {
	/*va_list ap;
	int ret;

	va_start(ap, format);
	ret = vprintf(format, ap);
	va_end(ap);

	return ret;*/

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
}


/*******************************************************************************
** 内存操作
*******************************************************************************/

void* hilink_memset(void* dst, int c, unsigned int len) {
	return memset(dst, c, len);
}

void* hilink_memcpy(void* dst, const void* src, unsigned int len) {
	return memcpy(dst, src, len);
}

int hilink_memcmp(const void* str1, const void* str2, unsigned int len) {
	return memcmp(str1, str2, len);
}

void hilink_free(void* pt) {
	free(pt);
}


 /*******************************************************************************
  * 字节顺序交换
 *******************************************************************************/
 unsigned short hilink_htons(unsigned short hs) {
	 return htons(hs);
 }

 unsigned short hilink_ntohs(unsigned short ns) {
	 return ntohs(ns);
 }


/*******************************************************************************
** json接口
*******************************************************************************/
#include "jsonapi.h"
void* hilink_json_parse(const char *value) {
	return my_hilink_json_parse(value);
}

char* hilink_json_get_string_value(void* object, \
	char* name, unsigned int *len) {
	return my_hilink_json_get_string_value(object, name, len);
}

int hilink_json_get_number_value(void* object, \
	char* name, int *value) {
	return my_hilink_json_get_number_value(object, name, value);
}

void hilink_json_delete(void* object) {
	hilink_json_delete(object);
}

int hilink_bi_get_cr(char *buf, unsigned int size) {
	char *rsa_cipher = 
	"1F5EA51C178060124B48BE6B95EF24A42DBBAFEBCC3B44C7B26FC3676ED35B50" \
	"7C93C09A62E8E2274FDD75856D8AC75E8525912E05619EEA17200C8BE43DBFC2" \
	"96B6B0CC8060119D569BF7C42DCE9A305E194C34FE1C4F6D1D6BCE05A379C8F9" \
	"3ABB622E75CE8847353880F0852DD39F636F2EFC867145DB214E99325CD53025" \
	"BBE50FC4D9485FEA5D28CCBB078526942616997A0565D901547D77B683C43892" \
	"2F1AAA8D7C0E3FB8B1FB88DBB1567CC501C144FA36F7D6D4F576937BC95B6DB0" \
	"DD354F8363676520A31A3D8479633347A3C592203C92F466EB6214F8DAD8D24B" \
	"A174D2C493FA4B50B5B7F1397D79A8CE52DD2045F0050A17AD4163F95D6C759A";
    
    
    unsigned int cr_len = strlen(rsa_cipher) + 1;
    if (cr_len <= size) {
         memcpy(buf, rsa_cipher, cr_len);
         return 0;
    }
    else {
         return -1;
    }
}

extern char tv_ssid[256];
int hilink_getssid(unsigned char *pssid, unsigned char *pssidlen) {
	if (*pssidlen < strlen(tv_ssid) + 1) return -1;

	strcpy(pssid, tv_ssid);
	*pssidlen = strlen(tv_ssid) + 1;

	return 0;
}