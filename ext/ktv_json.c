#include <string.h>
#include <stdio.h>
#include "cJSON.h"
#include "ktv_json.h"
#include "../ktv.h"

void ktv_obj_from_json_object(ktv_obj *obj, cJSON *json)
{
    if (json == NULL || obj == NULL)
    {
        return;
    }
    ktv_model *model = obj->tree->models[obj->model_index];
    for (size_t i = 0; i < model->field_count; i++)
    {
        ktv_field *field = model->fields[i];
        cJSON *item = cJSON_GetObjectItemCaseSensitive(json, field->alias);
        if (item == NULL)
        {
            continue;
        }
        if (cJSON_IsNumber(item) && field->type == KTV_TCHAR)
        {
            ktv_obj_set_char(obj, field->alias, (char)item->valueint);
            continue;
        }
        if (cJSON_IsNumber(item) && field->type == KTV_TBYTE)
        {
            ktv_obj_set_byte(obj, field->alias, (int8_t)item->valueint);
            continue;
        }
        if (cJSON_IsNumber(item) && field->type == KTV_TINT2)
        {
            ktv_obj_set_int2(obj, field->alias, (int16_t)item->valueint);
            continue;
        }
        if (cJSON_IsNumber(item) && field->type == KTV_TINT4)
        {
            ktv_obj_set_int4(obj, field->alias, (int32_t)item->valueint);
            continue;
        }
        if (cJSON_IsObject(item) && field->type == KTV_TMODEL)
        {
            ktv_obj *inner_obj = ktv_obj_new(obj->tree, obj->tree->models[field->sub_type]->name);
            ktv_obj_from_json_object(inner_obj, item);
            ktv_obj_set_obj(obj, field->alias, inner_obj);
            continue;
        }
        if (cJSON_IsString(item) && item->valuestring != NULL && field->type == KTV_TARRAY && field->sub_type == KTV_TCHAR)
        {
            ktv_array *array = ktv_array_new_string(obj, field->alias, item->valuestring, strlen(item->valuestring));
            ktv_obj_set_array(obj, field->alias, array);
            continue;
        }
        if (cJSON_IsArray(item) && item->child != NULL && field->type == KTV_TARRAY)
        {
            size_t count = cJSON_GetArraySize(item);
            if (field->sub_type == KTV_TBYTE)
            {
                int8_t data[count];
                for (size_t j = 0; j < count; j++)
                {
                    cJSON *array_item = cJSON_GetArrayItem(item, j);
                    data[j] = array_item != NULL ? array_item->valueint : 0;
                }
                ktv_obj_set_array(obj, field->alias, ktv_array_new_bytes(obj, field->alias, data, count));
            }
            else if (field->sub_type == KTV_TINT2)
            {
                int16_t data[count];
                for (size_t j = 0; j < count; j++)
                {
                    cJSON *array_item = cJSON_GetArrayItem(item, j);
                    data[j] = array_item != NULL ? array_item->valueint : 0;
                }
                ktv_obj_set_array(obj, field->alias, ktv_array_new_int2s(obj, field->alias, data, count));
            }
            else if (field->sub_type == KTV_TINT4)
            {
                int32_t data[count];
                for (size_t j = 0; j < count; j++)
                {
                    cJSON *array_item = cJSON_GetArrayItem(item, j);
                    data[j] = array_item != NULL ? array_item->valueint : 0;
                }
                ktv_obj_set_array(obj, field->alias, ktv_array_new_int4s(obj, field->alias, data, count));
            }
            continue;
        }
        if (cJSON_IsArray(item) && item->child != NULL && field->type == KTV_TMODEL_ARRAY)
        {
            size_t count = cJSON_GetArraySize(item);
            ktv_array *obj_array = ktv_array_new_objs(obj, field->alias, count);
            for (size_t j = 0; j < count; j++)
            {
                cJSON *array_item = cJSON_GetArrayItem(item, j);
                if (array_item != NULL)
                {
                    ktv_obj *obj_item = ktv_obj_new(obj->tree, obj->tree->models[field->sub_type]->name);
                    ktv_obj_from_json_object(obj_item, array_item);
                    ktv_array_set_obj(obj_array, j, obj_item);
                }
            }
            ktv_obj_set_array(obj, field->alias, obj_array);
        }
    }
}

cJSON *ktv_obj_to_json_object(ktv_obj *obj)
{
    cJSON *json = cJSON_CreateObject();
    ktv_model *model = obj->tree->models[obj->model_index];
    uint8_t field_count = model->field_count;
    for (size_t i = 0; i < field_count; i++)
    {
        ktv_field *field = model->fields[i];
        void *value = obj->values[i];
        if (value == NULL)
        {
            continue;
        }
        cJSON *item = NULL;
        if (field->type == KTV_TCHAR)
        {
            item = cJSON_CreateNumber((*(uint8_t *)value));
        }
        else if (field->type == KTV_TBYTE)
        {
            item = cJSON_CreateNumber(*(int8_t *)value);
        }
        else if (field->type == KTV_TINT2)
        {
            item = cJSON_CreateNumber(*(int16_t *)value);
        }
        else if (field->type == KTV_TINT4)
        {
            item = cJSON_CreateNumber(*(int32_t *)value);
        }
        else if (field->type == KTV_TMODEL)
        {
            item = ktv_obj_to_json_object((ktv_obj *)value);
        }
        else if (field->type == KTV_TARRAY)
        {
            ktv_array *array = (ktv_array *)value;
            if (field->sub_type == KTV_TCHAR)
            {
                item = cJSON_CreateString((char *)array->values);
            }
            else
            {
                int data[array->count];
                for (size_t j = 0; j < array->count; j++)
                {
                    switch (field->sub_type)
                    {
                    case KTV_TBYTE:
                        data[j] = ((int8_t *)array->values)[j];
                        break;
                    case KTV_TINT2:
                        data[j] = ((int16_t *)array->values)[j];
                        break;
                    case KTV_TINT4:
                        data[j] = ((int32_t *)array->values)[j];
                        break;
                    default:
                        break;
                    }
                }
                item = cJSON_CreateIntArray(data, array->count);
            }
        }
        else if (field->type == KTV_TMODEL_ARRAY)
        {
            item = cJSON_CreateArray();
            ktv_array *array = (ktv_array *)value;
            int data[array->count];
            for (size_t j = 0; j < array->count; j++)
            {
                if (array->objects[j] != NULL)
                {
                    cJSON *array_item = ktv_obj_to_json_object((ktv_obj *)array->objects[j]);
                    cJSON_AddItemToArray(item, array_item);
                }
            }
        }
        if (item == NULL)
        {
            continue;
        }
        cJSON_AddItemToObject(json, field->alias, item);
    }
    return json;
}

char *ktv_obj_to_json(ktv_obj *obj)
{
    if (obj == NULL)
    {
        return NULL;
    }
    cJSON *json = ktv_obj_to_json_object(obj);
    char *json_str = cJSON_Print(json);
    cJSON_Delete(json);
    return json_str;
}

void ktv_obj_from_json(ktv_obj *obj, char *json_str)
{
    cJSON *json = cJSON_Parse(json_str);
    ktv_obj_from_json_object(obj, json);
    cJSON_Delete(json);
}