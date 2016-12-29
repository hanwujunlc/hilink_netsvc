#include <stdio.h>
#include <unistd.h>
#include <strings.h>

#include "hilink_profile.h"
#include "hilink_config.h"
#include "jsonapi.h"
#include "msg.h"
#include "base64.h"

extern int b64_ntop(unsigned char const *src, size_t srclength, char *target, size_t targsize);
extern int b64_pton(char const *src, unsigned char *target, size_t targsize);

static int _get_respone_flags = -1;
static int _put_respone_flags = -1;
static char _get_data[1024];
static char _put_data[1024];

int hilink_msg_handler(const char *buf, int len) {
	//parse buf call json api

	msg_send("received", strlen("received") + 1);
	hilink_log("hilink_msg_handler receive msg length %d, buf = %s  !", len, buf);
	void *obj = my_hilink_json_parse(buf);
	//call my_hilink_json_get_string_value(void* obj, char* name, unsigned int *len);
	char *cmd = my_hilink_json_get_string_value(obj, "cmd", 0);
	char *type = my_hilink_json_get_string_value(obj, "type", 0); 
	hilink_log("cmd =  %s, type = %s  !", cmd, type);

	if (0 == strcmp(type, "profile")) {
		if (0 == strcmp(cmd, "start")) {
			hilink_log("hilink_msg_handler start !");
			char *svc_list = my_hilink_json_get_string_value(obj, "svc_list", 0);
			hilink_log("hilink_msg_handler svc_list ! = %s", svc_list);

			int len = strlen(svc_list);
			char base_buf[len];
			b64_pton(svc_list, base_buf, len);

			//TODO base_buf -> svc_info_t[]
			svc_info_t svcs[5];
 			config_profile(svcs, 5);

		} else if (0 == strcmp(cmd, "disable")) {
			hilink_log("hilink_msg_handler disable !");
			unconfig_profile();
		} else if (0 == strcmp(cmd, "upload")) {
			char *svc_id = my_hilink_json_get_string_value(obj, "svc_id", 0);
			char *payload = my_hilink_json_get_string_value(obj, "payload", 0);
			int len = strlen(payload);
			char base_buf[len];
			b64_pton(payload, base_buf, len);
			hilink_upload_char_state(svc_id, base_buf, strlen(base_buf));

		} else if (0 == strcmp(cmd, "get_char_state_response")) {
			char *svc_id = my_hilink_json_get_string_value(obj, "svc_id", 0);
			char *payload = my_hilink_json_get_string_value(obj, "payload", 0);
			_get_respone_flags = 1;

			bzero(_get_data, 1024);
			memcpy(_get_data, payload, strlen(payload) + 1);

		} else if (0 == strcmp(cmd, "put_char_state_response")) {
			char *svc_id = my_hilink_json_get_string_value(obj, "svc_id", 0);
			char *state = my_hilink_json_get_string_value(obj, "state", 0);
			_put_respone_flags = 1;

			bzero(_put_data, 1024);
			memcpy(_put_data, state, strlen(state) + 1);
		} else {

		}
	}
	my_hilink_json_delete(obj);

	
	//case {\"type\":\"profile\", \"cmd\":\"enable\",  \"dev_info\": \"base64(xxxx)\",\"svc_list\":\"base64(xxxxxxx)\" }; 
	//call config_profile(svc_info_t *svcs, int svc_len)
	//case {\"type\":\"profile\", \"cmd\":\"disable\"}; call unconfig_profile
	//case {\"type\":\"profile\", \"cmd\":\"upload\", \"svc_id\":\"1234\", \"payload\":\"base64(xxxxxx)\"}; 
	//call hilink_upload_char_state(char* svc_id, char* payload, unsigned int len)

	//case "{\"type\":\"profile\", \"cmd\":\"get_char_state_response\", \"svc_id\":\"%s\", \"payload\":\"base64(xxxxx)\"}: 
	//wakeup hilink_get_char_state
	//case {\"type\":\"profile\", \"cmd\":\"put_char_state_response\", \"svc_id\":\"%s\", \"state\":\"ok/error\"}: 
	//wakeup hilink_put_char_state
}

int hilink_get_char_state(const char* svc_id, const char* in, unsigned int in_len, char** out,
	unsigned int* out_len) {

	hilink_log("hilink_get_char_state receive msg svc_id %s\n, in  length %d , in buf = %s  !", svc_id, in_len, in);
	int ret;
	//char *buf;
	//asprintf(&buf, "{\"type\":\"profile\", \"cmd\":\"get_char_state\", \"svc_id\":\"%s\", \"payload\":\"%s\"}", svc_id, base64(in));
	//sprintf(&buf, "{\"type\":\"profile\", \"cmd\":\"get_char_state\", \"svc_id\":\"%s\", \"payload\":\"%s\"}", svc_id, base64(in));
	int len = (in_len / 3 + 1) * 4 + 1;
	char base_buf[len];
	b64_ntop(in, strlen(in) + 1, base_buf, len);

	char buf[1024];
	sprintf(buf, "{\"type\":\"profile\", \"cmd\":\"get_char_state\", \"svc_id\":\"%s\", \"payload\":\"%s\"}", svc_id, base_buf);
	ret = msg_send(buf, strlen(buf) + 1) > 0 ? 0 : -1;
	//free(buf);
	_get_respone_flags = 0;

	//wait for response
	while(0 == _get_respone_flags) {
		sleep(1);
	}

	memcpy(*out, _get_data, strlen(_get_data) + 1);
	*out_len = strlen(_get_data);

	return 0;
}

int hilink_put_char_state(const char* svc_id,
		const char* payload, unsigned int len) {
	hilink_log("hilink_put_char_state receive msg svc_id %s\n, in length %d , in buf = %s  !", svc_id, len, payload);

	int ret;
	//char *buf;
	//asprintf(&buf, "{\"type\":\"profile\", \"cmd\":\"put_char_state\", \"svc_id\":\"%s\", \"payload\":\"%s\"}", svc_id, base64(payload));
	int base_len = (len / 3 + 1) * 4 + 1;
	char base_buf[base_len];
	b64_ntop(payload, strlen(payload) + 1, base_buf, base_len);

	char buf[1024];

	sprintf(buf, "{\"type\":\"profile\", \"cmd\":\"put_char_state\", \"svc_id\":\"%s\", \"payload\":\"%s\"}", svc_id, base_buf);
	
	ret = msg_send(buf, strlen(buf) + 1) > 0 ? 0 : -1;
	//free(buf);
	_put_respone_flags = 0;
	////wait for response

	while(0 == _put_respone_flags) {
		sleep(1);
	}

	//memcpy(*out, _put_data, strlen(_put_data) + 1);
	//out_len = strlen(_put_data);

	return 0;
}

//******************************related to configuration or upgrade ***************************************** 
int hilink_notify_wifi_param(char* ssid, unsigned int ssid_len,
		char* pwd, unsigned int pwd_len, int mode ) {
	return 0;
}

int hilink_ota_trig(int mode) {
	if (mode != 0 && mode != 1) return -12;

	return -900;
}

int hilink_ota_get_ver(char** version, int* ver_len) {
	*version = (char *)malloc(4);
	strcpy(*version, "1.0");
	*ver_len = strlen(*version) + 1;

	return 0;
}

int hilink_ota_get_intro(char** introduction, int* intro_len) {
	*introduction = (char *)malloc(32);
	strcpy(*introduction, "skyworth tv");
	*intro_len = strlen(*introduction) + 1;

	return 0;
}