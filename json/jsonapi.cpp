#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <map>

#include "json.h"
#include "jsonapi.h"

using namespace std;

static map<string, string> hilink_members;
void* my_hilink_json_parse(const char *value) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(value, root);

	Json::Value::Members members = root.getMemberNames();
	for (Json::Value::Members::iterator iter = members.begin(); iter != members.end(); iter++) {
		string key = *iter;
		string value;
		if (root[key].isString()) {
			value = root[key].asString();
		} else if (root[key].isInt()) {
			char buf[10];
			sprintf(buf, "%d", root[key].asInt());
			value = buf; 
		} else {
			value = "";
			printf("my_hilink_json_parse other data !\n");
		}
		
		hilink_members.insert(std::pair<string, string>(key, value));
	}

	return (void *)&hilink_members;
}

char* my_hilink_json_get_string_value(void* object, char* name, unsigned int *len) {
	if(hilink_members.end() == hilink_members.find(name)) return NULL;
	return (char *)hilink_members[name].c_str();
}

int my_hilink_json_get_number_value(void* object, char* name, int *value) {
	if(hilink_members.end() == hilink_members.find(name)) return -1;
	*value = atoi(hilink_members[name].c_str());
	return 0;
}

void my_hilink_json_delete(void* object) {
	map<string, string> *members = (map<string, string> *)object;
	if (members && !members->empty())
		members->clear();
}