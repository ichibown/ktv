#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ktv.h"

void print_buffer(uint8_t *buffer, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        if (i % 8 == 0)
            printf("\n");
        printf("0x%02X ", buffer[i]);
    }
}

void codec_test_with_output(ktv_tree *tree)
{
    ktv_obj *user = ktv_obj_new(tree, "user");
    ktv_obj_set_byte(user, "age", 30);
    ktv_obj_set_byte(user, "gender", 1);

    ktv_obj *job = ktv_obj_new(tree, "job");
    char *title = "Product Manager";
    ktv_array *title_array = ktv_array_new_string(job, "title", title, strlen(title));
    ktv_obj_set_array(job, "title", title_array);
    ktv_obj_set_byte(job, "type", 2);

    ktv_obj_set_obj(user, "job", job);

    ktv_obj *task1 = ktv_obj_new(tree, "task");
    ktv_obj_set_int2(task1, "id", 10001);
    ktv_obj_set_byte(task1, "status", 3);
    ktv_obj *task2 = ktv_obj_new(tree, "task");
    ktv_obj_set_int2(task2, "id", -10002);
    ktv_obj_set_byte(task2, "status", 2);
    int32_t task2_times[2] = {1234567, -7654321};
    ktv_array *task2_time_array = ktv_array_new_int4s(task2, "time", task2_times, 2);
    ktv_obj_set_array(task2, "time", task2_time_array);
    ktv_array *task_array = ktv_array_new_objs(user, "tasks", 2);
    ktv_array_set_obj(task_array, 0, task1);
    ktv_array_set_obj(task_array, 1, task2);

    ktv_obj_set_array(user, "tasks", task_array);

    char *name = "Zhang Ji";
    ktv_array *name_array = ktv_array_new_string(user, "name", name, strlen(name));
    ktv_obj_set_array(user, "name", name_array);

    ktv_print_obj(user);

    printf("\n=== User Object Encode ===\n");
    ktv_buffer *user_buffer = ktv_obj_encode(user);
    printf("Encoded Size: %zu\nEncoded Bytes: ", user_buffer->size);

    print_buffer(user_buffer->buffer, user_buffer->size);

    ktv_obj *decoded_user = ktv_obj_new(tree, "user");
    ktv_obj_decode(decoded_user, user_buffer);
    ktv_print_obj(decoded_user);

    ktv_obj_delete(decoded_user);
    ktv_buffer_delete(user_buffer);
    ktv_obj_delete(user);
}

void benchmark_test(ktv_tree *tree, int repeat)
{
    clock_t start, stop;

    ktv_obj *person_alice = ktv_obj_new(tree, "Person");
    ktv_obj_set_array(person_alice, "name", ktv_array_new_string(person_alice, "name", "Alice", 5));
    ktv_obj_set_int4(person_alice, "id", 10000);
    ktv_obj *number1 = ktv_obj_new(tree, "PhoneNumber");
    ktv_obj_set_array(number1, "number", ktv_array_new_string(number1, "number", "123456789", 9));
    ktv_obj_set_byte(number1, "type", 1);
    ktv_obj *number2 = ktv_obj_new(tree, "PhoneNumber");
    ktv_obj_set_array(number2, "number", ktv_array_new_string(number2, "number", "87654321", 8));
    ktv_obj_set_byte(number2, "type", 2);
    ktv_array *alice_phone = ktv_array_new_objs(person_alice, "phone", 2);
    ktv_array_set_obj(alice_phone, 0, number1);
    ktv_array_set_obj(alice_phone, 1, number2);
    ktv_obj_set_array(person_alice, "phone", alice_phone);

    ktv_obj *person_bob = ktv_obj_new(tree, "Person");
    ktv_obj_set_array(person_bob, "name", ktv_array_new_string(person_alice, "name", "Bob", 3));
    ktv_obj_set_int2(person_bob, "id", 20000);
    ktv_obj *number3 = ktv_obj_new(tree, "PhoneNumber");
    ktv_obj_set_array(number3, "number", ktv_array_new_string(number1, "number", "0123456789", 10));
    ktv_obj_set_byte(number3, "type", 3);
    ktv_array *bob_phone = ktv_array_new_objs(person_alice, "phone", 1);
    ktv_array_set_obj(bob_phone, 0, number3);
    ktv_obj_set_array(person_bob, "phone", bob_phone);

    ktv_obj *address_book = ktv_obj_new(tree, "AddressBook");
    ktv_array *person = ktv_array_new_objs(address_book, "person", 2);
    ktv_array_set_obj(person, 0, person_alice);
    ktv_array_set_obj(person, 1, person_bob);
    ktv_obj_set_array(address_book, "person", person);

    start = clock();
    for (size_t i = 0; i < repeat; i++)
    {
        ktv_buffer *buffer = ktv_obj_encode(address_book);
        ktv_buffer_delete(buffer);
    }
    stop = clock();
    double timecost = (double)(stop - start) / CLOCKS_PER_SEC;
    printf("Encode Repeat %d times: %f (s)\n", repeat, timecost);

    ktv_buffer *buffer = ktv_obj_encode(address_book);
    start = clock();
    for (size_t i = 0; i < repeat; i++)
    {
        ktv_obj *decoded = ktv_obj_new(tree, "AddressBook");
        ktv_obj_decode(decoded, buffer);
        ktv_obj_delete(decoded);
    }
    stop = clock();
    ktv_buffer_delete(buffer);
    timecost = (double)(stop - start) / CLOCKS_PER_SEC;
    printf("Decode Repeat %d times: %f (s)\n", repeat, timecost);

    ktv_obj_delete(address_book);
}

int main(int argc, char const *argv[])
{
    FILE *f = fopen("ktv_test.proto.bin", "rb");
    fseek(f, 0, SEEK_END);
    int fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t content[fsize];
    fread(content, fsize, 1, f);
    fclose(f);

    printf("=== Proto Bin File Info ===\n");
    printf("Proto Bin Length = %d\nProto Value = ", fsize);
    print_buffer(content, fsize);
    printf("\n");

    ktv_tree *tree = ktv_tree_new(content, fsize);
    ktv_print_tree(tree);

    codec_test_with_output(tree);
    // benchmark_test(tree, 1000000);

    ktv_tree_delete(tree);
    return 0;
}