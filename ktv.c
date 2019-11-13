#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ktv.h"

#define INDEX_INVALID 255

uint8_t ktv_find_field_index(ktv_obj *obj, const char *alias, uint8_t type)
{
    ktv_tree *tree = obj->tree;
    ktv_model *model = tree->models[obj->model_index];
    if (model == NULL)
    {
        return INDEX_INVALID;
    }
    for (size_t i = 0; i < model->field_count; i++)
    {
        ktv_field *field = model->fields[i];
        if (strcmp(field->alias, alias) == 0 && field->type == type)
        {
            return i;
        }
    }
    return INDEX_INVALID;
}

uint8_t ktv_find_model_index(ktv_tree *tree, const char *name)
{
    if (tree == NULL)
    {
        return INDEX_INVALID;
    }
    for (size_t i = 0; i < tree->model_count; i++)
    {
        if (strcmp(tree->models[i]->name, name) == 0)
        {
            return i;
        }
    }
    return INDEX_INVALID;
}

int16_t ktv_bytes_to_int2(uint8_t *buffer)
{
    int16_t value = (int16_t)buffer[0] << 8 |
                    (int16_t)buffer[1] << 0;
    return value;
}

int32_t ktv_bytes_to_int4(uint8_t *buffer)
{
    int32_t value = (int32_t)buffer[0] << 24 |
                    (int32_t)buffer[1] << 16 |
                    (int32_t)buffer[2] << 8 |
                    (int32_t)buffer[3] << 0;
    return value;
}

void *ktv_obj_get_value_ptr(ktv_obj *obj, const char *alias, uint8_t type)
{
    uint8_t field_index = ktv_find_field_index(obj, alias, type);
    if (field_index == INDEX_INVALID || obj->values[field_index] == NULL)
    {
        return NULL;
    }
    return obj->values[field_index];
}

void *ktv_obj_malloc_value_ptr(ktv_obj *obj, const char *alias, uint8_t type, size_t size)
{
    uint8_t field_index = ktv_find_field_index(obj, alias, type);
    if (field_index == INDEX_INVALID)
    {
        return NULL;
    }
    if (obj->values[field_index] == NULL)
    {
        obj->values[field_index] = malloc(size);
    }
    return obj->values[field_index];
}

ktv_array *ktv_array_new_basic(ktv_obj *obj, const char *alias, uint16_t count)
{
    uint8_t field_index = ktv_find_field_index(obj, alias, KTV_TARRAY);
    if (field_index == INDEX_INVALID)
    {
        return NULL;
    }
    ktv_field *field = obj->tree->models[obj->model_index]->fields[field_index];
    ktv_array *array = malloc(sizeof(ktv_array));
    array->type = field->type;
    array->sub_type = field->sub_type;
    array->objects = NULL;
    array->values = NULL;
    array->count = count;
    return array;
}

void ktv_buffer_append(ktv_buffer *buffer, uint8_t *data, size_t size)
{
    if (buffer->size == 0)
    {
        buffer->buffer = malloc(sizeof(uint8_t) * size);
    }
    else
    {
        buffer->buffer = realloc(buffer->buffer, buffer->size + size);
    }
    memcpy(buffer->buffer + buffer->size, data, size);
    buffer->size = buffer->size + size;
}

ktv_tree *ktv_tree_new(uint8_t *parsed_proto, size_t size)
{
    ktv_tree *tree = malloc(sizeof(ktv_tree));
    // parse model count
    uint8_t model_count = parsed_proto[0];
    tree->model_count = model_count;
    tree->models = malloc(sizeof(ktv_model *) * model_count);
    size_t index = 1;
    uint8_t model_index = 0;
    do
    {
        ktv_model *model = malloc(sizeof(ktv_model));
        // parse model name length
        size_t name_length = parsed_proto[index];
        index += 1;
        // parse model name
        char *name = malloc((name_length + 1) * sizeof(char));
        memcpy(name, parsed_proto + index, name_length);
        name[name_length] = '\0';
        model->name = name;
        index += name_length;
        // parse field count
        size_t field_count = parsed_proto[index];
        model->field_count = field_count;
        index += 1;
        // parse fields
        ktv_field **fields = malloc(sizeof(ktv_field *) * field_count);
        model->fields = fields;
        size_t field_index = 0;
        do
        {
            ktv_field *field = malloc(sizeof(ktv_field));
            // parse field alias length
            size_t alias_length = parsed_proto[index];
            index += 1;
            // parse field alias
            char *alias = malloc((alias_length + 1) * sizeof(char));
            memcpy(alias, parsed_proto + index, alias_length);
            alias[alias_length] = '\0';
            index += alias_length;
            // parse field type
            field->alias = alias;
            field->type = parsed_proto[index];
            index += 1;
            field->sub_type = parsed_proto[index];
            index += 1;
            fields[field_index] = field;
            field_index++;
        } while (field_index < field_count);
        tree->models[model_index] = model;
        model_index++;
    } while (index < size);
    return tree;
}

void ktv_tree_delete(ktv_tree *tree)
{
    if (tree == NULL)
    {
        return;
    }
    for (size_t i = 0; i < tree->model_count; i++)
    {
        ktv_model *model = tree->models[i];
        for (size_t j = 0; j < model->field_count; j++)
        {
            ktv_field *field = model->fields[j];
            free(field->alias);
            free(field);
        }
        free(model->name);
        free(model->fields);
        free(model);
    }
    free(tree->models);
    free(tree);
}

ktv_obj *ktv_obj_new(ktv_tree *tree, const char *name)
{
    uint8_t index = ktv_find_model_index(tree, name);
    if (index == INDEX_INVALID)
    {
        return NULL;
    }
    ktv_model *model = tree->models[index];
    ktv_obj *obj = malloc(sizeof(ktv_obj));
    obj->tree = tree;
    obj->model_index = index;
    void **values = malloc(sizeof(void *) * model->field_count);
    for (size_t i = 0; i < model->field_count; i++)
    {
        values[i] = NULL;
    }
    obj->values = values;
    return obj;
}

void ktv_obj_delete(ktv_obj *obj)
{
    if (obj == NULL)
    {
        return;
    }
    ktv_model *model = obj->tree->models[obj->model_index];
    for (size_t i = 0; i < model->field_count; i++)
    {
        void *value = obj->values[i];
        ktv_field *field = model->fields[i];
        if (value != NULL)
        {
            if (field->type == KTV_TMODEL)
            {
                ktv_obj_delete((ktv_obj *)value);
                obj->values[i] = NULL;
            }
            else if (field->type == KTV_TARRAY || field->type == KTV_TMODEL_ARRAY)
            {
                ktv_array_delete((ktv_array *)value);
                obj->values[i] = NULL;
            }
            else
            {
                free(value);
            }
        }
    }
    free(obj->values);
    free(obj);
}

void ktv_obj_set_char(ktv_obj *obj, const char *alias, char new_value)
{
    void *value = ktv_obj_malloc_value_ptr(obj, alias, KTV_TCHAR, sizeof(char));
    *(char *)value = new_value;
}

char ktv_obj_get_char(ktv_obj *obj, const char *alias)
{
    void *value = ktv_obj_get_value_ptr(obj, alias, KTV_TCHAR);
    return value != NULL ? *(char *)value : 0;
}

void ktv_obj_set_byte(ktv_obj *obj, const char *alias, int8_t new_value)
{
    void *value = ktv_obj_malloc_value_ptr(obj, alias, KTV_TBYTE, sizeof(int8_t));
    *(int8_t *)value = new_value;
}

int8_t ktv_obj_get_byte(ktv_obj *obj, const char *alias)
{
    void *value = ktv_obj_get_value_ptr(obj, alias, KTV_TBYTE);
    return value != NULL ? *(int8_t *)value : 0;
}

void ktv_obj_set_int2(ktv_obj *obj, const char *alias, int16_t new_value)
{
    void *value = ktv_obj_malloc_value_ptr(obj, alias, KTV_TINT2, sizeof(int16_t));
    *(int16_t *)value = new_value;
}

int16_t ktv_obj_get_int2(ktv_obj *obj, const char *alias)
{
    void *value = ktv_obj_get_value_ptr(obj, alias, KTV_TINT2);
    return value != NULL ? *(int16_t *)value : 0;
}

void ktv_obj_set_int4(ktv_obj *obj, const char *alias, int32_t new_value)
{
    void *value = ktv_obj_malloc_value_ptr(obj, alias, KTV_TINT4, sizeof(int32_t));
    *(int32_t *)value = new_value;
}

int32_t ktv_obj_get_int4(ktv_obj *obj, const char *alias)
{
    void *value = ktv_obj_get_value_ptr(obj, alias, KTV_TINT4);
    return value != NULL ? *(int32_t *)value : 0;
}

void ktv_obj_set_obj(ktv_obj *obj, const char *alias, ktv_obj *value)
{
    uint8_t field_index = ktv_find_field_index(obj, alias, KTV_TMODEL);
    if (field_index == INDEX_INVALID)
    {
        return;
    }
    // todo: delete replaced obj?
    obj->values[field_index] = value;
}

ktv_obj *ktv_obj_get_obj(ktv_obj *obj, const char *alias)
{
    void *value = ktv_obj_get_value_ptr(obj, alias, KTV_TMODEL);
    return (ktv_obj *)value;
}

void ktv_obj_set_array(ktv_obj *obj, const char *alias, ktv_array *value)
{
    uint8_t field_index = ktv_find_field_index(obj, alias, value->type);
    if (field_index == INDEX_INVALID)
    {
        return;
    }
    ktv_model *model = obj->tree->models[obj->model_index];
    ktv_field *field = model->fields[field_index];
    if (field->type != value->type || field->sub_type != value->sub_type)
    {
        return;
    }
    // todo: delete replaced array?
    obj->values[field_index] = value;
}

ktv_array *ktv_obj_get_array(ktv_obj *obj, const char *alias)
{
    void *value = ktv_obj_get_value_ptr(obj, alias, KTV_TARRAY);
    if (value == NULL)
    {
        value = ktv_obj_get_value_ptr(obj, alias, KTV_TMODEL_ARRAY);
    }
    return (ktv_array *)value;
}

ktv_array *ktv_array_new_string(ktv_obj *obj, const char *alias, char *values, uint16_t count)
{
    ktv_array *array = ktv_array_new_basic(obj, alias, count);
    array->values = malloc(sizeof(char) * count);
    for (size_t i = 0; i < count; i++)
    {
        ((char *)array->values)[i] = values[i];
    }
    return array;
}

ktv_array *ktv_array_new_bytes(ktv_obj *obj, const char *alias, int8_t *values, uint16_t count)
{
    ktv_array *array = ktv_array_new_basic(obj, alias, count);
    array->values = malloc(sizeof(int8_t) * count);
    for (size_t i = 0; i < count; i++)
    {
        ((int8_t *)array->values)[i] = values[i];
    }
    return array;
}

ktv_array *ktv_array_new_int2s(ktv_obj *obj, const char *alias, int16_t *values, uint16_t count)
{
    ktv_array *array = ktv_array_new_basic(obj, alias, count);
    array->values = malloc(sizeof(int16_t) * count);
    for (size_t i = 0; i < count; i++)
    {
        ((int16_t *)array->values)[i] = values[i];
    }
    return array;
}

ktv_array *ktv_array_new_int4s(ktv_obj *obj, const char *alias, int32_t *values, uint16_t count)
{
    ktv_array *array = ktv_array_new_basic(obj, alias, count);
    array->values = malloc(sizeof(int32_t) * count);
    for (size_t i = 0; i < count; i++)
    {
        ((int32_t *)array->values)[i] = values[i];
    }
    return array;
}

ktv_array *ktv_array_new_objs(ktv_obj *obj, const char *alias, uint16_t capacity)
{
    uint8_t field_index = ktv_find_field_index(obj, alias, KTV_TMODEL_ARRAY);
    if (field_index == INDEX_INVALID || capacity == 0)
    {
        return NULL;
    }
    ktv_field *field = obj->tree->models[obj->model_index]->fields[field_index];
    ktv_array *array = malloc(sizeof(ktv_array));
    array->type = field->type;
    array->sub_type = field->sub_type;
    array->values = NULL;
    array->count = capacity;
    array->objects = malloc(sizeof(ktv_obj *) * capacity);
    for (size_t i = 0; i < capacity; i++)
    {
        array->objects[i] = NULL;
    }

    return array;
}

void ktv_array_delete(ktv_array *array)
{
    if (array == NULL)
    {
        return;
    }
    if (array->type == KTV_TARRAY)
    {
        if (array->values != NULL)
        {
            free(array->values);
        }
        free(array);
        return;
    }
    for (size_t i = 0; i < array->count; i++)
    {
        if (array->objects[i] != NULL)
        {
            ktv_obj_delete(array->objects[i]);
        }
    }
    free(array->objects);
    free(array);
}

char *ktv_array_get_string(ktv_array *array)
{
    return (char *)array->values;
}

int8_t *ktv_array_get_bytes(ktv_array *array)
{
    return (int8_t *)array->values;
}

int16_t *ktv_array_get_int2s(ktv_array *array)
{
    return (int16_t *)array->values;
}

int32_t *ktv_array_get_int4s(ktv_array *array)
{
    return (int32_t *)array->values;
}

ktv_obj *ktv_array_get_obj(ktv_array *array, uint16_t index)
{
    if (index > array->count)
    {
        return NULL;
    }
    return array->objects[index];
}

void ktv_array_set_obj(ktv_array *array, uint16_t index, ktv_obj *obj)
{
    if (index > array->count)
    {
        return;
    }
    array->objects[index] = obj;
}

ktv_buffer *ktv_obj_encode(ktv_obj *obj)
{
    if (obj == NULL)
    {
        return NULL;
    }
    ktv_buffer *buffer = ktv_buffer_new(NULL, 0);
    ktv_model *model = obj->tree->models[obj->model_index];
    uint8_t field_count = model->field_count;
    for (size_t i = 0; i < field_count; i++)
    {
        ktv_field *field = model->fields[i];
        void *value = obj->values[i];
        if (field->type == KTV_TCHAR)
        {
            uint8_t data[1] = {*(char *)value >> 0};
            ktv_buffer_append(buffer, data, 1);
        }
        else if (field->type == KTV_TBYTE)
        {
            uint8_t data[1] = {*(int8_t *)value >> 0};
            ktv_buffer_append(buffer, data, 1);
        }
        else if (field->type == KTV_TINT2)
        {
            uint8_t data[2] = {*(int16_t *)value >> 8,
                               *(int16_t *)value >> 0};
            ktv_buffer_append(buffer, data, 2);
        }
        else if (field->type == KTV_TINT4)
        {
            uint8_t data[4] = {*(int32_t *)value >> 24,
                               *(int32_t *)value >> 16,
                               *(int32_t *)value >> 8,
                               *(int32_t *)value >> 0};
            ktv_buffer_append(buffer, data, 4);
        }
        else if (field->type == KTV_TMODEL)
        {
            ktv_buffer *model_buffer = ktv_obj_encode((ktv_obj *)value);
            uint8_t model_length[2] = {0, 0};
            if (model_buffer != NULL)
            {
                model_length[0] = (uint16_t)model_buffer->size >> 8;
                model_length[1] = (uint16_t)model_buffer->size >> 0;
                ktv_buffer_append(buffer, model_length, 2);
                ktv_buffer_append(buffer, model_buffer->buffer, model_buffer->size);
                ktv_buffer_delete(model_buffer);
            }
            else
            {
                ktv_buffer_append(buffer, model_length, 2);
            }
        }
        else if (field->type == KTV_TARRAY)
        {
            uint8_t array_count[2] = {0, 0};
            ktv_array *array_value = (ktv_array *)value;
            if (array_value != NULL)
            {
                array_count[0] = array_value->count >> 8;
                array_count[1] = array_value->count >> 0;
                ktv_buffer_append(buffer, array_count, 2);
                for (size_t j = 0; j < array_value->count; j++)
                {
                    if (field->sub_type == KTV_TBYTE || field->sub_type == KTV_TCHAR)
                    {
                        ktv_buffer_append(buffer, ((uint8_t *)array_value->values) + j, 1);
                    }
                    else if (field->sub_type == KTV_TINT2)
                    {
                        int16_t int2_value = ((int64_t *)array_value->values)[j];
                        uint8_t data[2] = {int2_value >> 8, int2_value >> 0};
                        ktv_buffer_append(buffer, data, 2);
                    }
                    else if (field->sub_type == KTV_TINT4)
                    {
                        int32_t int4_value = ((int32_t *)array_value->values)[j];
                        uint8_t data[4] = {int4_value >> 24,
                                           int4_value >> 16,
                                           int4_value >> 8,
                                           int4_value >> 0};
                        ktv_buffer_append(buffer, data, 4);
                    }
                }
            }
            else
            {
                ktv_buffer_append(buffer, array_count, 2);
            }
        }
        else if (field->type == KTV_TMODEL_ARRAY)
        {
            ktv_array *array_value = (ktv_array *)value;
            ktv_buffer *array_buffer = ktv_buffer_new(NULL, 0);
            for (size_t i = 0; array_value != NULL && i < array_value->count; i++)
            {
                uint8_t model_length[2] = {0, 0};
                ktv_buffer *model_buffer = ktv_obj_encode(array_value->objects[i]);
                if (model_buffer != NULL)
                {
                    model_length[0] = (uint16_t)model_buffer->size >> 8;
                    model_length[1] = (uint16_t)model_buffer->size >> 0;
                    ktv_buffer_append(array_buffer, model_length, 2);
                    ktv_buffer_append(array_buffer, model_buffer->buffer, model_buffer->size);
                    ktv_buffer_delete(model_buffer);
                }
                else
                {
                    ktv_buffer_append(array_buffer, model_length, 2);
                }
            }
            uint8_t array_count[2] = {0, 0};
            if (array_buffer->size > 0)
            {
                array_count[0] = (uint16_t)array_value->count >> 8;
                array_count[1] = (uint16_t)array_value->count >> 0;
                ktv_buffer_append(buffer, array_count, 2);
                ktv_buffer_append(buffer, array_buffer->buffer, array_buffer->size);
            }
            else
            {
                ktv_buffer_append(buffer, array_count, 2);
            }
            ktv_buffer_delete(array_buffer);
        }
    }
    return buffer;
}

void ktv_obj_decode(ktv_obj *obj, ktv_buffer *buffer)
{
    if (obj == NULL || buffer == NULL || buffer->size == 0)
    {
        return;
    }
    ktv_model *model = obj->tree->models[obj->model_index];
    uint8_t field_count = model->field_count;
    size_t index = 0;
    for (size_t i = 0; i < field_count && index < buffer->size; i++)
    {
        ktv_field *field = model->fields[i];
        if (field->type == KTV_TCHAR)
        {
            char value = buffer->buffer[index];
            ktv_obj_set_char(obj, field->alias, value);
            index++;
        }
        else if (field->type == KTV_TBYTE)
        {
            int8_t value = buffer->buffer[index];
            ktv_obj_set_byte(obj, field->alias, value);
            index++;
        }
        else if (field->type == KTV_TINT2)
        {
            int16_t value = ktv_bytes_to_int2(&buffer->buffer[index]);
            ktv_obj_set_int2(obj, field->alias, value);
            index += 2;
        }
        else if (field->type == KTV_TINT4)
        {
            int32_t value = ktv_bytes_to_int4(&buffer->buffer[index]);
            ktv_obj_set_int4(obj, field->alias, value);
            index += 4;
        }
        else if (field->type == KTV_TARRAY)
        {
            uint16_t count = ktv_bytes_to_int2(&buffer->buffer[index]);
            index += 2;
            if (count == 0)
            {
                continue;
            }
            if (field->sub_type == KTV_TCHAR)
            {
                char data[count];
                for (size_t j = 0; j < count; j++)
                    data[j] = buffer->buffer[index + j];
                ktv_obj_set_array(obj, field->alias, ktv_array_new_string(obj, field->alias, data, count));
                index += count;
            }
            else if (field->sub_type == KTV_TBYTE)
            {
                int8_t data[count];
                for (size_t j = 0; j < count; j++)
                    data[j] = buffer->buffer[index + j];
                ktv_obj_set_array(obj, field->alias, ktv_array_new_bytes(obj, field->alias, data, count));
                index += count;
            }
            else if (field->sub_type == KTV_TINT2)
            {
                int16_t data[count];
                for (size_t j = 0; j < count; j++)
                {
                    data[j] = ktv_bytes_to_int2(buffer->buffer + index + j * 2);
                }
                ktv_obj_set_array(obj, field->alias, ktv_array_new_int2s(obj, field->alias, data, count));
                index += count * 2;
            }
            else if (field->sub_type == KTV_TINT4)
            {
                int32_t data[count];
                for (size_t j = 0; j < count; j++)
                    data[j] = ktv_bytes_to_int4(buffer->buffer + index + j * 4);
                ktv_obj_set_array(obj, field->alias, ktv_array_new_int4s(obj, field->alias, data, count));
                index += count * 4;
            }
        }
        else if (field->type == KTV_TMODEL)
        {
            uint16_t size = ktv_bytes_to_int2(&buffer->buffer[index]);
            index += 2;
            ktv_buffer *model_buffer = ktv_buffer_new(buffer->buffer + index, size);
            ktv_obj *model_obj = ktv_obj_new(obj->tree, obj->tree->models[field->sub_type]->name);
            ktv_obj_decode(model_obj, model_buffer);
            ktv_obj_set_obj(obj, field->alias, model_obj);
            ktv_buffer_delete(model_buffer);
            index += size;
        }
        else if (field->type == KTV_TMODEL_ARRAY)
        {
            uint16_t count = ktv_bytes_to_int2(&buffer->buffer[index]);
            index += 2;
            if (count == 0)
            {
                continue;
            }
            ktv_array *models = ktv_array_new_objs(obj, field->alias, count);
            for (size_t j = 0; j < count; j++)
            {
                uint16_t model_size = ktv_bytes_to_int2(&buffer->buffer[index]);
                index += 2;
                ktv_buffer *model_buffer = ktv_buffer_new(buffer->buffer + index, model_size);
                ktv_obj *model_obj = ktv_obj_new(obj->tree, obj->tree->models[field->sub_type]->name);
                ktv_obj_decode(model_obj, model_buffer);
                ktv_array_set_obj(models, j, model_obj);
                ktv_buffer_delete(model_buffer);
                index += model_size;
            }
            ktv_obj_set_array(obj, field->alias, models);
        }
    }
    return;
}

ktv_buffer *ktv_buffer_new(uint8_t *data, size_t size)
{
    ktv_buffer *buffer = malloc(sizeof(ktv_buffer));
    buffer->size = size;
    if (data != NULL && size > 0)
    {
        buffer->buffer = malloc(size);
        memcpy(buffer->buffer, data, size);
    }
    else
    {
        buffer->buffer = NULL;
        buffer->size = 0;
    }
    return buffer;
}

void ktv_buffer_delete(ktv_buffer *buffer)
{
    free(buffer->buffer);
    free(buffer);
}

void ktv_print_tree(ktv_tree *root)
{
    printf("\n=== KTV Tree Dump (%d Models) ===\n", root->model_count);
    int index = 0;
    char *basic_types[5] = {"", "char", "byte", "int2", "int4"};
    do
    {
        ktv_model *model = root->models[index];
        printf("Model [%d]: name = <%s>, fields count = [%d]\n", index, model->name, model->field_count);
        int field_index = 0;
        do
        {
            ktv_field *field = model->fields[field_index];
            uint8_t type = field->type;
            uint8_t sub_type = field->sub_type;
            char *type_str = "";
            char *sub_type_str = "";
            switch (type)
            {
            case KTV_TCHAR:
            case KTV_TBYTE:
            case KTV_TINT2:
            case KTV_TINT4:
                type_str = basic_types[type];
                break;
            case KTV_TARRAY:
                type_str = "Array of ";
                sub_type_str = basic_types[sub_type];
                break;
            case KTV_TMODEL:
                type_str = root->models[sub_type]->name;
                break;
            case KTV_TMODEL_ARRAY:
                type_str = "Array of ";
                sub_type_str = root->models[sub_type]->name;
                break;
            default:
                type_str = "ERROR";
                break;
            }
            printf("\tField [%d]: alias = <%s>, \ttype = <%s%s>\n", field_index, field->alias, type_str, sub_type_str);
            field_index++;
        } while (field_index < model->field_count);
        index++;
        printf("\n");
    } while (index < root->model_count);
    printf("\n");
}

void ktv_print_obj_internal(ktv_obj *obj, int tabs)
{
    ktv_model *model = obj->tree->models[obj->model_index];
    uint8_t field_count = model->field_count;
    for (size_t i = 0; i < field_count; i++)
    {
        for (int tab = 0; tab < tabs; tab++)
        {
            printf("\t");
        }
        ktv_field *field = model->fields[i];
        void *value = obj->values[i];
        printf("Field = <%s>, ", field->alias);
        if (value == NULL)
        {
            printf("Value = NULL\n");
            continue;
        }
        if (field->type == KTV_TCHAR)
        {
            printf("Value = [%d]\n", *(uint8_t *)value);
        }
        if (field->type == KTV_TBYTE)
        {
            printf("Value = [%d]\n", *(int8_t *)value);
        }
        else if (field->type == KTV_TINT2)
        {
            printf("Value = [%d]\n", *(int16_t *)value);
        }
        else if (field->type == KTV_TINT4)
        {
            printf("Value = [%d]\n", *(int32_t *)value);
        }
        else if (field->type == KTV_TARRAY)
        {
            ktv_array *array = (ktv_array *)value;
            printf("Values = [");
            char str[array->count + 1];
            str[array->count] = '\0';
            for (size_t i = 0; i < array->count; i++)
            {
                switch (field->sub_type)
                {
                case KTV_TCHAR:
                    str[i] = ((char *)array->values)[i];
                    break;
                case KTV_TBYTE:
                    printf(" %d ", ((int8_t *)array->values)[i]);
                    break;
                case KTV_TINT2:
                    printf(" %d ", ((int16_t *)array->values)[i]);
                    break;
                case KTV_TINT4:
                    printf(" %d ", ((int32_t *)array->values)[i]);
                    break;
                default:
                    break;
                }
            }
            if (field->sub_type == KTV_TCHAR)
            {
                printf("%s", str);
            }
            printf("]");
            printf("\n");
        }
        else if (field->type == KTV_TMODEL)
        {
            printf("Value = \n");
            ktv_print_obj_internal((ktv_obj *)value, tabs + 1);
        }
        else if (field->type == KTV_TMODEL_ARRAY)
        {
            printf("Values = \n");
            ktv_array *array = (ktv_array *)value;
            for (size_t j = 0; j < array->count; j++)
            {
                if (array->objects[j] == NULL)
                {
                    printf("NULL");
                }
                else
                {
                    ktv_print_obj_internal((ktv_obj *)array->objects[j], tabs + 1);
                }
            }
        }
    }
}

void ktv_print_obj(ktv_obj *obj)
{
    if (obj == NULL)
    {
        return;
    }
    ktv_model *model = obj->tree->models[obj->model_index];
    printf("\n=== KTV Object Dump (%s) ===\n", model->name);
    ktv_print_obj_internal(obj, 0);
    printf("\n");
}