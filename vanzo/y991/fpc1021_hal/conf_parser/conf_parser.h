
#ifndef _CONF_PARSER_H_
#define _CONF_PARSER_H_
#include <stdio.h>
enum {
    CONFIG_OK = 0,
    CONFIG_COULD_NOT_OPEN_FILE = 1,
    CONFIG_WRONG_VERSION = 2,
    CONFIG_SECTION_NOT_FOUND = 3,
    CONFIG_PARSE_FAILED = 4,
    CONFIG_INCOMPLETE_PARSE = 5
};

#define FPC_CONFIG_NUM_ADC          "numAdc"
#define FPC_CONFIG_SETTING_GAIN     "gain"
#define FPC_CONFIG_SETTING_OFFSET   "offset"
#define FPC_CONFIG_SETTING_PXL      "pxl"

static char key_buffer[64];



#define F_DEFAULT_C     0x0001
#define F_MULTI_C_1     0x0002
#define F_MULTI_C_2     0x0004
#define F_MULTI_C_3     0x0008
#define F_FILE_CMN1     0x0010
#define F_FILE_CMN2     0x0020
#define F_FILE_DBLR     0x0040
#define F_FILE_CROP     0x0080

#define F_ALL           0x00FF

typedef struct {
    unsigned int parsed_data;
    unsigned char default_c[3];
    unsigned char multi_c[3][3];
    unsigned char crop_area[4];
    char *file_cmn1;
    char *file_cmn2;
    char *file_dblr;
} sensor_config_t;


#ifdef __cplusplus
extern "C" {
#endif

extern sensor_config_t theconfig;
char* create_key(const char* setting, const char* child, int array_index);
int parse_sensor_config(char *sensor);
int delete_config();

//leet functions for unix ppl, pls understand
int getIntValue(FILE* f, const char* section, const char* key,  int* value);
int getFloatValue(FILE* f, const char* section, const char* key, float* value);
int getStringValue(FILE* f, const char* section, const char* key, char* string, size_t buflen);

#ifdef __cplusplus
}
#endif


#endif
