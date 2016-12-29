#ifndef JSONAPI_H__
#define JSONAPI_H__

#ifdef __cplusplus
extern "C" {
#endif

void* my_hilink_json_parse(const char *value);
char* my_hilink_json_get_string_value(void* object, char* name, unsigned int *len);
int my_hilink_json_get_number_value(void* object, char* name, int *value);
void my_hilink_json_delete(void* object);

#ifdef __cplusplus
}
#endif

#endif