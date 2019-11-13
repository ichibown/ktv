#ifndef ktv_json_h
#define ktv_json_h

#include <stdint.h>
#include "cJSON.h"
#include "../ktv.h"

char *ktv_obj_to_json(ktv_obj *obj);

void ktv_obj_from_json(ktv_obj *obj, char *json);

#endif