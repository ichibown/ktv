# KTV

KTV is an ultra lightweight, schema-based binary protocol serialization/deserialization library. It is implemented with ansi C, so it is easy to import KTV into many platforms, especially Embedded Systems in IoT.

## Usage

#### 1️⃣ Copy `ktv.h` & `ktv.c` into you project.

#### 2️⃣ Define a proto file to describe you business data structure

```
#job
title  *char    
type   byte     // type 0 for full-time, type 1 for intern

#task
id     int2 
status byte
time   *int4

#user
age    byte
gender byte
job    job
tasks  *task
name   *char
mentor *user
```

#### 3️⃣ Parse proto text into binary

Save proto into a text file `example.proto` and then run the following script

```bash
python proto_parser.py example.proto
```

This will generate `example.proto.bin`

#### 4️⃣ Use the `proto.bin` in ktv lib

```c
// load proto.bin
ktv_tree *tree = ktv_tree_new(proto_bin_bytes, proto_bin_length;
                              
// build data object
ktv_obj *user = ktv_obj_new(tree, "user");
ktv_obj_set_byte(user, "age", 30);
ktv_obj_set_byte(user, "gender", 1);

ktv_obj *job = ktv_obj_new(tree, "job");
char *title = "Product Manager";
ktv_array *title_array = ktv_array_new_string(job, "title", title, strlen(title));
ktv_obj_set_array(job, "title", title_array);
ktv_obj_set_byte(job, "type", 2);
                              
ktv_obj_set_obj(user, "job", job);

char *name = "Zhang Ji";
ktv_array *name_array = ktv_array_new_string(user, "name", name, strlen(name));
ktv_obj_set_array(user, "name", name_array);
                              
// serialize to bytes
ktv_buffer *user_buffer = ktv_obj_encode(user);
// send bytes to BLE
send_data_to_ble(user_buffer->buffer, user_buffer->size);
                              
// receive bytes from BLE
ktv_buffer *received_user_buffer = receive_data_from_ble();
// deserialize to ktv_obj
ktv_obj *received_user_obj = ktv_obj_new(tree, "user");
ktv_obj_decode(recieved_user_obj, received_user_buffer);
// get obj values & use in business logic
char* received_name = ktv_obj_get_string(received_user_obj, "name");
```

>  More examples see [ktv_test.c](https://github.com/BownX/ktv/blob/master/ktv_test.c)

## API

### Proto

- starts with `#` to define a model 
- starts with `*` to define an array

- basic types:
  - char: unsigned 1 byte
  - byte: signed 1 byte
  - int2: signed 2 bytes
  - int4: signed 4 bytes

### ktv_obj

```c
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
```

### ktv_array

```c
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
```

### ktv_buffer & encode/decode

```c
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
```

### JSON API

With the help of [cJSON](https://github.com/DaveGamble/cJSON), we can easily convert a JSON string to ktv_obj and vice versa. That will be more convenient for platforms like Android, iOS, wasm to use KTV.

Just copy [ext](https://github.com/BownX/ktv/tree/master/ext) folder into you project and include `ext/ktv_json.h`

```c
char *ktv_obj_to_json(ktv_obj *obj);
void ktv_obj_from_json(ktv_obj *obj, char *json);
```

> More ktv JSON examples  in [ktv_json_test.c](https://github.com/BownX/ktv/blob/master/ktv_json_test.c)
