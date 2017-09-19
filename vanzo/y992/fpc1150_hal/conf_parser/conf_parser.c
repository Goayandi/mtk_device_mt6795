

#include <stdlib.h>
#include <string.h>

#include "conf_parser.h"


/* filenames for postprocessing data are hysterically long */
#define max_line_len   (FILENAME_MAX + 10)


sensor_config_t theconfig;

char* create_key(const char* setting, const char* child, int array_index)
 {
    int pos = snprintf(key_buffer, sizeof(key_buffer), "%s", setting);
    if (array_index >= 0)
        pos += snprintf(key_buffer + pos, sizeof(key_buffer) - pos,"[%i]", array_index);
    if (child)
        snprintf(key_buffer + pos, sizeof(key_buffer) - pos,".%s",child);

    return key_buffer;
}


/* as fgets, but moves fpos to next line, ignores comments and initial whitespaces */
char *_fgetline(char *buff, int bufflen, FILE *f) {
    int c, i;

    if (bufflen == 0)
        return NULL;

    c = fgetc(f);

    do {
        /* skip initial ws/lb */
        while (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            c = fgetc(f);
        }

        /* skip comment lines */
        if(c == '#') {
            while (c != EOF && c != '\r' && c != '\n')
                c = fgetc(f);

            while (c == ' ' || c == '\t' || c == '\r' || c == '\n')
                c = fgetc(f);
        }

    } while (c != EOF && c == '#');

    if(c == EOF)
        return NULL;

    for(i=0; (i<bufflen-1 && c != EOF && c != '\n'); i++) {
        buff[i] = (c != EOF) ? c : 0;
        c = fgetc(f);
    }
    buff[i] = 0;

    /* skip rest of line */
    while (c != EOF && c != '\r' && c != '\n')
        c = fgetc(f);

    while (c != EOF && c == '\r' && c == '\n')
        c = fgetc(f);

    return buff;
}



/* The first non-comment line should be "configver 1.x". ret 0 == OK */
int _check_config_version(FILE *f)
{
    char buff[20];
    char *str = _fgetline(buff, 20, f);

    return (str) ? strncasecmp(buff, "configver 1.0", 20) : -1;
}

/* Look for "[1020A1,1020A2,...]" */
int _find_sensor_section(const char *sensor, FILE *f)
{
    char buff[40];
    char *str;

    for(;;)
    {
        do {
            str = _fgetline(buff, 40, f);
            if(!str)
                return CONFIG_SECTION_NOT_FOUND;
        } while (*str != '[');

        if(strstr(str, sensor))
        {
            return CONFIG_OK;
        }
    }
}


/* str is e.g. "default_c = {1B, 00, 01}" */
int _parse_data_array(char *str, unsigned char *data, int datalen) {
    char *end;
    int i;
    while(*str && *str != '{') str++;
    if(*str != '{')
        return CONFIG_PARSE_FAILED;
    str++;
    for(i=0; i<datalen; i++)
    {
        data[i] = (unsigned char)strtoul(str, &end, 0);
        if(end == 0 || end == str)
            return CONFIG_PARSE_FAILED;
        str = end;
        while(*str == ',' || *str == ' ') str++;
    }
    return (*str == '}') ? CONFIG_OK : CONFIG_PARSE_FAILED;
}


int _parse_data_string(char *str, char **newstr)
{
    int len;

    while(*str && *str != '"') str++;
    if(*str != '"')
        return CONFIG_PARSE_FAILED;

    str++;

    for(len=0; (str[len] && str[len] != '"'); len++) ;
    if(str[len] != '"')
        return CONFIG_PARSE_FAILED;

    *newstr = (char*)malloc(len+1);
    memcpy(*newstr, str, len);
    (*newstr)[len] = 0;

    return CONFIG_OK;
}


int _parse_data(FILE *f, sensor_config_t *data) {
    char buff[max_line_len];
    char *str;

    data->parsed_data = 0;

    while (data->parsed_data != F_ALL) {
        str = _fgetline(buff, max_line_len, f);
        if(!str || *str == '[') {
            return CONFIG_INCOMPLETE_PARSE;
        }

        if(0 == strncmp(str, "default_c", 9)) {
            if(CONFIG_OK == _parse_data_array(str, data->default_c, 3))
                data->parsed_data |= F_DEFAULT_C;
        }
        else if(0 == strncmp(str, "multi_c_1", 9)) {
            if(CONFIG_OK == _parse_data_array(str, data->multi_c[0], 3))
                data->parsed_data |= F_MULTI_C_1;
        }
        else if(0 == strncmp(str, "multi_c_2", 9)) {
            if(CONFIG_OK == _parse_data_array(str, data->multi_c[1], 3))
                data->parsed_data |= F_MULTI_C_2;
        }
        else if(0 == strncmp(str, "multi_c_3", 9)) {
            if(CONFIG_OK == _parse_data_array(str, data->multi_c[2], 3))
                data->parsed_data |= F_MULTI_C_3;
        }
        else if(0 == strncmp(str, "CMN1", 4)) {
            if(CONFIG_OK == _parse_data_string(str, &data->file_cmn1))
                data->parsed_data |= F_FILE_CMN1;
        }
        else if(0 == strncmp(str, "CMN2", 4)) {
            if(CONFIG_OK == _parse_data_string(str, &data->file_cmn2))
                data->parsed_data |= F_FILE_CMN2;
        }
        else if(0 == strncmp(str, "DBLR", 4)) {
            if(CONFIG_OK == _parse_data_string(str, &data->file_dblr))
                data->parsed_data |= F_FILE_DBLR;
        }
        else if(0 == strncmp(str, "crop_area", 9)) {
            if(CONFIG_OK == _parse_data_array(str, data->crop_area, 4))
                data->parsed_data |= F_FILE_CROP;
        }
    }

    return CONFIG_OK;
}

int _parse_sensor_config(char *sensor, char *configfile, sensor_config_t *data)
{
    FILE *f = NULL;

    data->file_cmn1 = NULL;
    data->file_cmn2 = NULL;
    data->file_dblr = NULL;

    f = fopen(configfile, "r");

    if(f == NULL)
    {
        return CONFIG_COULD_NOT_OPEN_FILE;
    }

    if(_check_config_version(f) != 0)
    {
        fclose(f);
        return CONFIG_WRONG_VERSION;
    }

    if(_find_sensor_section(sensor, f) != CONFIG_OK)
    {
        fclose(f);
        return CONFIG_SECTION_NOT_FOUND;
    }

    if(_parse_data(f, data) != CONFIG_OK)
    {
        fclose(f);
        return CONFIG_PARSE_FAILED;
    }
    fclose(f);
    return CONFIG_OK;
}

char* _fgetvalue(const char* section, const char* key, char *buff, int bufflen, FILE *f)
{
    char* str;
    int status = _find_sensor_section(section, f);

    if (status)
        return NULL;

    while (1) {
        str = _fgetline(buff, bufflen, f);
        if (str == NULL)
            return NULL;
        if (strncmp(str, key, strlen(key)) == 0)
            break;
    }

    str = strchr(str, '=');
    if (str == NULL || str + 1 >= str + bufflen - 1)
        return NULL;

    return str + 1;
}

int getIntValue(FILE* f, const char* section, const char* key,  int* value)
{
    rewind(f);
    char buffer[256];
    char* str = _fgetvalue(section, key, buffer, sizeof(buffer), f);
    if (str == NULL)
        return CONFIG_PARSE_FAILED;

    char* end;
    *value = strtoul(str, &end, 0);
    if(end == str)
        return CONFIG_PARSE_FAILED;

    return 0;
}

int getFloatValue(FILE* f, const char* section, const char* key, float* value)
{
    rewind(f);
    char buffer[256];
    char* str = _fgetvalue(section, key, buffer, sizeof(buffer), f);
    if (str == NULL)
        return CONFIG_PARSE_FAILED;

    char* end;
    *value = strtod(str, &end);
    if(end == str)
        return CONFIG_PARSE_FAILED;

    return 0;
}

int getStringValue(FILE* f, const char* section, const char* key, char* string, size_t buflen)
{
    rewind(f);
    char buffer[256];
    char* str = _fgetvalue(section, key, buffer, sizeof(buffer), f);
    if (str == NULL)
        return CONFIG_PARSE_FAILED;

    char* first = strchr(str, '"');
    if (first == NULL && first + 1 != buffer + sizeof(buffer) - 1) {
        return CONFIG_PARSE_FAILED;
    }

    ++first;

    char* last = strchr(first, '"');
    if (last == NULL)
        return CONFIG_PARSE_FAILED;

    if (buflen < 1 + last - first)
        return CONFIG_INCOMPLETE_PARSE;

    *last = '\0';
    memcpy(string, first, 1 + last - first);

    return 0;
}

int _delete_config(sensor_config_t *config)
{
    if(config->file_cmn1) {
        free(config->file_cmn1);
        config->file_cmn1 = NULL;
    }
    if(config->file_cmn2) {
        free(config->file_cmn2);
        config->file_cmn2 = NULL;
    }
    if(config->file_dblr) {
        free(config->file_dblr);
        config->file_dblr = NULL;
    }
    return CONFIG_OK;
}


int parse_sensor_config(char *sensor)
{
    return _parse_sensor_config(sensor, "sensor.config", &theconfig);
}

int delete_config()
{
    return _delete_config(&theconfig);
}
