#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define JIM_IMPLEMENTATION
#include "./jim.h"

#include "./test_expected.h"

#define ARRAY_LEN(xs) (sizeof(xs) / sizeof((xs)[0]))

typedef struct {
    size_t capacity;
    size_t size;
    char *data;
} Buffer;

void buffer_clean(Buffer *buffer)
{
    buffer->size = 0;
}

size_t buffer_write(const void *ptr, size_t size, size_t nmemb,
                    Buffer *sink)
{
    size_t esize = size * nmemb; // effective size

    if (sink->size + esize <= sink->capacity) {
        memcpy(sink->data + sink->size,
               ptr,
               esize);
        sink->size += esize;
        return esize;
    } else {
        return 0;
    }
}

static char static_memory_for_buffer[1024];

static Buffer buffer = {
    .capacity = sizeof(static_memory_for_buffer),
    .data = static_memory_for_buffer
};

void case_01(Jim *jim)
{
    jim_object_begin(jim);
    {
        jim_member_key(jim, "keys", NULL);
        jim_array_begin(jim);
        {
            for (int key = 0; key < 10; ++key) {
                jim_integer(jim, key);
            }
        }
        jim_array_end(jim);

        jim_member_key(jim, "names", NULL);
        jim_array_begin(jim);
        {
            jim_string(jim, "foo", NULL);
            jim_string(jim, "bar", NULL);
            jim_string(jim, "baz", NULL);
        }
        jim_array_end(jim);
    }
    jim_object_end(jim);
}

void case_02(Jim *jim)
{
    jim_object_begin(jim);
    {
        jim_member_key(jim, "null", NULL);
        jim_null(jim);

        jim_member_key(jim, "bool", NULL);
        jim_array_begin(jim);
        {
            jim_bool(jim, 0);
            jim_bool(jim, 1);
        }
        jim_array_end(jim);

        jim_member_key(jim, "integers", NULL);
        jim_array_begin(jim);
        {
            for (int i = -3; i <= 3; ++i) {
                jim_integer(jim, i);
            }
        }
        jim_array_end(jim);

        jim_member_key(jim, "floats", NULL);
        jim_array_begin(jim);
        {
            jim_float(jim, 0.0, 4);
            jim_float(jim, -0.0, 4);
            jim_float(jim, 3.1415, 4);
            jim_float(jim, 2.71828, 5);
            jim_float(jim, 1.6180, 4);
            jim_float(jim, 0.0 / 0.0, 4);
            jim_float(jim, 1.0 / 0.0, 4);
            jim_float(jim, -1.0 / 0.0, 4);
        }
        jim_array_end(jim);

        jim_member_key(jim, "string", NULL);
        jim_array_begin(jim);
        {
            jim_string(jim, "Hello\tWorld\n", NULL);
            unsigned int size = 4;
            jim_string(jim, "\0\0\0\0", &size);
        }
        jim_array_end(jim);
    }
    jim_object_end(jim);
}

typedef struct {
    const char *name;
    void (*run)(Jim *jim);
} Test_Case;

#define TEST_CASE(case_name) \
    { \
        .name = #case_name, \
        .run = case_name \
    }

const Test_Case test_cases[] = {
    TEST_CASE(case_01),
    TEST_CASE(case_02)
};

void record(const char *header_path)
{
    FILE *stream = fopen(header_path, "w");

    Jim jim_stream = {
        .sink = stream,
        .write = (Jim_Write) fwrite,
    };

    Jim jim_buffer = {
        .sink = &buffer,
        .write = (Jim_Write) buffer_write,
    };

    fprintf(stream, "const char *test_cases_expected[] = {\n");
    for (size_t i = 0; i < ARRAY_LEN(test_cases); ++i) {
        buffer_clean(&buffer);
        test_cases[i].run(&jim_buffer);
        fprintf(stream, "    ");
        const unsigned int size = buffer.size;
        jim_string(&jim_stream, buffer.data, &size);
        fprintf(stream, ",\n");
    }
    fprintf(stream, "};\n");

    fclose(stream);

    printf("Updated %s\n", header_path);
}

void test(void)
{
    Jim jim_buffer = {
        .sink = &buffer,
        .write = (Jim_Write) buffer_write,
    };

    for (size_t i = 0; i < ARRAY_LEN(test_cases); ++i) {
        printf("%s ... ", test_cases[i].name);

        buffer_clean(&buffer);
        test_cases[i].run(&jim_buffer);

        if (buffer.size != strlen(test_cases_expected[i])
                || memcmp(buffer.data, test_cases_expected[i], buffer.size) != 0) {
            printf("FAILED!\n");
            printf("Expected: %s\n", test_cases_expected[i]);
            printf("Actual:   ");
            fwrite(buffer.data, 1, buffer.size, stdout);
            printf("\n");
            exit(1);
        }

        printf("OK\n");
    }
}

int main(int argc, char **argv)
{
    if (argc >= 2) {
        if (strcmp(argv[1], "record") == 0) {
            record("test_expected.h");
        } else {
            fprintf(stderr, "Usage: ./test [record]\n");
            fprintf(stderr, "ERROR: unknown subcommand %s.\n", argv[1]);
        }
    } else  {
        test();
    }
}
