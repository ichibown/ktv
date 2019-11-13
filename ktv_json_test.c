#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ktv.h"
#include "ext/ktv_json.h"

int main(int argc, char const *argv[])
{
    FILE *proto_file = fopen("ktv_test.proto.bin", "rb");
    fseek(proto_file, 0, SEEK_END);
    int fsize = ftell(proto_file);
    fseek(proto_file, 0, SEEK_SET);
    uint8_t content[fsize];
    fread(content, fsize, 1, proto_file);
    fclose(proto_file);

    FILE *json_file = fopen("ktv_json_test.json", "rb");
    fseek(json_file, 0, SEEK_END);
    int json_size = ftell(json_file);
    fseek(json_file, 0, SEEK_SET);
    char json[json_size];
    fread(json, json_size, 1, json_file);
    fclose(json_file);

    ktv_tree *tree = ktv_tree_new(content, fsize);
    ktv_print_tree(tree);

    printf("=== JSON Content ===\n%s\n", json);
    ktv_obj *obj = ktv_obj_new(tree, "user");
    ktv_obj_from_json(obj, json);
    ktv_print_obj(obj);

    char *new_json = ktv_obj_to_json(obj);
    printf("=== JSON Content from obj ===\n%s\n", new_json);

    free(new_json);
    ktv_obj_delete(obj);
    ktv_tree_delete(tree);
    return 0;
}
