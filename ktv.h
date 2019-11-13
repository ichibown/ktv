#ifndef ktv_h
#define ktv_h

#include <stdint.h>

#define KTV_TCHAR 0x01
#define KTV_TBYTE 0x02
#define KTV_TINT2 0x03
#define KTV_TINT4 0x04
#define KTV_TARRAY 0x10
#define KTV_TMODEL 0x11
#define KTV_TMODEL_ARRAY 0x12

struct ktv_field;
struct ktv_model;

typedef struct ktv_field
{
    char *alias;      // field alias / JSON key
    uint8_t type;     // KTV_T*
    uint8_t sub_type; // type = TARRAY | type = TMODEL or TMODEL_ARRAY (value = model index)
} ktv_field;

typedef struct ktv_model
{
    char *name;
    uint8_t field_count;
    struct ktv_field **fields;
} ktv_model;

typedef struct ktv_tree
{
    uint8_t model_count;
    struct ktv_model **models;
} ktv_tree;

typedef struct ktv_obj
{
    ktv_tree *tree;
    uint8_t model_index;
    void **values;
} ktv_obj;

typedef struct ktv_array
{
    uint8_t type;
    uint8_t sub_type;
    void *values;
    ktv_obj **objects;
    uint16_t count;
} ktv_array;

typedef struct ktv_buffer
{
    size_t size;
    uint8_t *buffer;
} ktv_buffer;

/**
 * generate model tree from parsed proto
 */
ktv_tree *ktv_tree_new(uint8_t *parsed_proto, size_t size);

/**
 * release model tree
 */
void ktv_tree_delete(ktv_tree *tree);

/**
 * create an object
 */
ktv_obj *ktv_obj_new(ktv_tree *tree, const char *name);

/**
 * release an object
 * recursively release all child object/array
 */
void ktv_obj_delete(ktv_obj *obj);

/**
 * get/set value for ktv_obj by type
 */
void ktv_obj_set_char(ktv_obj *obj, const char *alias, char value);
char ktv_obj_get_char(ktv_obj *obj, const char *alias);

void ktv_obj_set_byte(ktv_obj *obj, const char *alias, int8_t value);
int8_t ktv_obj_get_byte(ktv_obj *obj, const char *alias);

void ktv_obj_set_int2(ktv_obj *obj, const char *alias, int16_t value);
int16_t ktv_obj_get_int2(ktv_obj *obj, const char *alias);

void ktv_obj_set_int4(ktv_obj *obj, const char *alias, int32_t value);
int32_t ktv_obj_get_int4(ktv_obj *obj, const char *alias);

void ktv_obj_set_obj(ktv_obj *obj, const char *alias, ktv_obj *value);
ktv_obj *ktv_obj_get_obj(ktv_obj *obj, const char *alias);

void ktv_obj_set_array(ktv_obj *obj, const char *alias, ktv_array *value);
ktv_array *ktv_obj_get_array(ktv_obj *obj, const char *alias);

/**
 * create an array by type
 */
ktv_array *ktv_array_new_string(ktv_obj *obj, const char *alias, char *values, uint16_t count);
ktv_array *ktv_array_new_bytes(ktv_obj *obj, const char *alias, int8_t *values, uint16_t count);
ktv_array *ktv_array_new_int2s(ktv_obj *obj, const char *alias, int16_t *values, uint16_t count);
ktv_array *ktv_array_new_int4s(ktv_obj *obj, const char *alias, int32_t *valeus, uint16_t count);
ktv_array *ktv_array_new_objs(ktv_obj *obj, const char *alias, uint16_t capacity);

/**
 *  release an array
 */
void ktv_array_delete(ktv_array *array);

/**
 * get value from array by type
 */
char *ktv_array_get_string(ktv_array *array);
int8_t *ktv_array_get_bytes(ktv_array *array);
int16_t *ktv_array_get_int2s(ktv_array *array);
int32_t *ktv_array_get_int4s(ktv_array *array);
ktv_obj *ktv_array_get_obj(ktv_array *array, uint16_t index);
void ktv_array_set_obj(ktv_array *array, uint16_t index, ktv_obj *obj);

/**
 * object -> bytes
 */
ktv_buffer *ktv_obj_encode(ktv_obj *obj);

/**
 * bytes -> object
 */
void ktv_obj_decode(ktv_obj *obj, ktv_buffer *buffer);

/**
 * create buffer
 */
ktv_buffer *ktv_buffer_new(uint8_t *buffer, size_t size);

/**
 * delete buffer
 */
void ktv_buffer_delete(ktv_buffer *buffer);

/**
 * [Debug] print ktv tree
 */
void ktv_print_tree(ktv_tree *tree);

/**
 * [Debug] print ktv object
 */
void ktv_print_obj(ktv_obj *obj);

#endif