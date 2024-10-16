#ifndef _String_H_
#define _String_H_
#include <string.h>

#if defined(IMPLEMENT_STRING) && !defined(IMPLEMENT_VECTOR)
#define IMPLEMENT_VECTOR
#endif
#ifndef _VECTOR_H_
#define _VECTOR_H_

// refrence https://bytesbeneath.com/p/dynamic-arrays-in-c

#include <stddef.h>
#include <stdlib.h>
#include <string.h>


typedef struct {
    size_t capacity;
    size_t element_size;
    size_t length;
} VectorHeader;

#define VECTOR_CAPACITY                 16
#define Vector(T)                       vector_init(sizeof(T), VECTOR_CAPACITY)
#define vector_header(v)                ((VectorHeader *)(v)-1)
#define vector_length(v)                vector_header(v)->length
#define vector_capacity(v)              (vector_header(v)->capacity)
#define vector_pop(v)                   (vector_header(v)->length--,v[vector_length(v)])
#define vector_insert(vector, pos, val) (vector_shift_right(vector, pos, 1,sizeof(*vector)), vector[pos] = val, &vector[pos])

#define free_vector(v)                  free((v)?(vector_header(v)):NULL)
#define vector_append(vector, value)    ((vector) = vector_ensure_capacity(vector, 1),  \
                                        (vector)[vector_header(vector)->length] = (value),                 \
                                        &(vector)[vector_header(vector)->length++])                        

#define vector_extend(v_dest, v_src)                      \
    do {                                                  \
        for (size_t i = 0; i < vector_length(v_src); i++) { \
            vector_append(v_dest, v_src[i]);              \
        }                                                 \
    } while (0)

#define vector_extend_n(v_dest, v_src,n)                      \
    do {                                                  \
        for (size_t i = 0; i < n; i++) { \
            vector_append(v_dest, v_src[i]);              \
        }                                                 \
    } while (0)

#define vector_map(func,des,in)\
do{\
    for(size_t i = 0; i<vector_length(in);i++){\
        vector_append(des,func(in[i]));\
    }\
}while(0)

#define vector_transform(func,in)\
do{\
    for(size_t i = 0; i<vector_length(in);i++){\
        in[i] = func(in[i]);\
    }\
}while(0)

void *vector_init(size_t element_size, size_t capacity);
void *vector_ensure_capacity(void *vector, size_t total_element);

#endif // _VECTOR_H_

#if !defined(_IMPLEMENTED_VECTOR_) && defined(IMPLEMENT_VECTOR)
#define _IMPLEMENTED_VECTOR_

void *vector_init(size_t element_size, size_t capacity) {
    void *ptr = 0;
    VectorHeader *vec_header =
        malloc(sizeof(*vec_header) + capacity * element_size);

    if (vec_header) {
        vec_header->capacity = capacity;
        vec_header->element_size = element_size;
        vec_header->length = 0;
        ptr = vec_header + 1;
    }

    return ptr;
}

void *vector_ensure_capacity(void *vector, size_t total_element) {
    VectorHeader *vec_header = vector_header(vector);
    size_t element_size = vec_header->element_size;
    size_t desired_capacity = vec_header->length + total_element;
    if (vec_header->capacity < desired_capacity) {
        size_t new_capacity = vec_header->capacity * 2;
        while (new_capacity < desired_capacity) {
            new_capacity *= 2;
        }

        size_t new_size = sizeof(*vec_header) + new_capacity * element_size;
        VectorHeader *temp = realloc(vec_header, new_size);
        if (!temp) {
            // todo
            return NULL;
        }

        vec_header = temp;
        vec_header->capacity = new_capacity;
    }

    vec_header += 1;
    return vec_header;
}

void vector_shift_right(void *vector, size_t from_pos, size_t shift_by, size_t element_size) {
    size_t old_length = vector_length(vector);
    void *new_vector = vector_ensure_capacity(vector, shift_by);
    vector_header(vector)->length += shift_by;
    char *base = new_vector;

    if (!new_vector) {
        return;
    }

    size_t offset = element_size * shift_by;
    size_t start = from_pos * element_size;
    size_t end = old_length * element_size;
    memmove(&base[start + offset], &base[start], end - start);
}
#endif // IMPLEMENT_VECTOR

typedef char String;
#define String_init() Vector(char)
#define str_len(String) vector_length(String) - 1;
#define free_string(String) free_vector(String);
char *String_from(const char *cstr);
void str_cat(String **dest, String *src);
String **str_split(const char *str, char *delimeter);
String *str_join(const char **src, const char *delimeter, size_t len_src,
        const char *initial,const char*end);
#endif // _String_H_

#if defined(IMPLEMENT_STRING)
char *String_from(const char *cstr) {
    String *str = String_init();
    const char *temp = cstr;
    while (*temp) {
        vector_append(str, *temp);
        temp++;
    }
    vector_append(str, '\0');
    return str;
}
void str_cat(String **dest, String *src) {
    (void)vector_pop(*dest);
    while (*src) {
        vector_append(*dest, *src++);
    }
    vector_append(*dest, '\0');
}

String **str_split(const char *str, char *delimeter) {
    const char *start = str;
    char *match = NULL;
    size_t delimiter_len = strlen(delimeter);
    String **result = Vector(*result);
    String *temp;
    while ((match = strstr(start, delimeter)) != NULL) {
        temp = String_init();
        vector_extend_n(temp, start, match - start);
        vector_append(temp, '\0');
        vector_append(result, temp);
        start = match + delimiter_len;
    }

    temp = String_init();
    while (*start) {
        vector_append(temp, *start++);
    }
    vector_append(temp, '\0');
    vector_append(result, temp);
    return result;
}

String *str_join(const char **src, const char *delimeter, size_t len_src,
        const char *initial,const char*end) {
    String *result = String_init();
    const char *temp = NULL;
    if(initial){
        temp = initial;
        while (*temp) {
            vector_append(result, *temp++);
        }
    }

    for (size_t i = 0; i < len_src; i++) {
        temp = src[i];
        while (*temp) {
            vector_append(result, *temp++);
        }
        if (i < len_src - 1) {
            temp = delimeter;
            while (*temp) {
                vector_append(result, *temp++);
            }
        }
    }
    if(end){
        temp = end;
        while (*temp) {
            vector_append(result, *temp++);
        }
    }
    vector_append(result, '\0');
    return result;
}
#endif
