// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#include "array.h"
#include "macros.h"
#include "object.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_ARRAY_CAPACITY 16

CBP_Array *CBP_GetArray() {
  CBP_Array *arr = malloc(sizeof(CBP_Array));
  if (arr == NULL) {
    return NULL;
  }
  arr->capacity = INITIAL_ARRAY_CAPACITY;
  arr->size = 0;
  arr->contents = malloc(sizeof(CBP_Object) * 16);
  if (arr->contents == NULL) {
    free(arr);
    return NULL;
  }
  for (u64 i = 0; i < arr->capacity; i++) {
    CBP_InitNullObj(&arr->contents[i]);
  }
  arr->is_ref = false;
  return arr;
}

CBP_Array *CBP_CopyFromArray(const CBP_Array *src) {
  CBP_Array *arr = malloc(sizeof(CBP_Array));
  if (arr == NULL) {
    goto return_null;
  }
  if (!src->is_ref) {
    arr->capacity = src->capacity;
  } else {
    arr->capacity = src->size + 1;
  }
  arr->size = src->size;
  arr->is_ref = false;
  CBP_PtrSafeAllocate(arr->contents, arr->capacity, sizeof(CBP_Object),
                      free_and_return_null);
  for (u64 i = 0; i < src->capacity; i++) {
    if (i < src->size) {
      CBP_PtrSafeAllocate(arr->contents[i].data, 1,
                          src->contents[i].element_size,
                          free_arr_contents_and_return_null);
      if (src->contents[i].type == CUSTOM &&
          src->contents[i].copy_function != NULL) {
        src->contents[i].copy_function(arr->contents[i].data,
                                       src->contents[i].data);
      } else {
        memcpy(arr->contents[i].data, src->contents[i].data,
               src->contents[i].element_size);
      }
      arr->contents[i].element_size = src->contents[i].element_size;
      arr->contents[i].copy_function = src->contents[i].copy_function;
      arr->contents[i].destroy_function = src->contents[i].destroy_function;
      arr->contents[i].type = src->contents[i].type;
    } else {
      CBP_InitNullObj(&arr->contents[i]);
    }
  }
  return arr;
free_arr_contents_and_return_null:
  for (u64 i = 0; i < src->capacity; i++) {
    CBP_GracefulDestroy(free, arr->contents[i].data);
  }
  CBP_GracefulDestroy(free, arr->contents);
free_and_return_null:
  CBP_GracefulDestroy(free, arr);
return_null:
  return NULL;
}

CBP_Array CBP_GetArrayRef(CBP_Object *contents, u64 size) {
  return (CBP_Array){
      .capacity = 0, .size = size, .contents = contents, .is_ref = true};
}

int CBP_Array_Push(CBP_Array *array, const CBP_Object *element) {
  if (array == NULL) {
    return HX_ERR;
  }

  if (array->is_ref) {
    return HX_ERR;
  }

  if (array->size + 1 >= array->capacity) {
    if (CBP_Array_Enlarge(array, 2 * array->capacity) != HX_OK) {
      return HX_ERR;
    }
    array->capacity *= 2;
  }

  if (array->contents[array->size].data != NULL) {
    free(array->contents[array->size].data);
  }

  array->contents[array->size].element_size = element->element_size;
  array->contents[array->size].type = element->type;
  array->contents[array->size].copy_function = element->copy_function;
  array->contents[array->size].destroy_function = element->destroy_function;

  void *new_data = calloc(element->element_size, 1);

  if (new_data == NULL) {
    return HX_ERR;
  }

  if (element->type == CUSTOM && element->copy_function != NULL &&
      element->destroy_function != NULL) {
    CBP_CopyFn copy_fn = element->copy_function;
    CBP_DestroyFn destroy_fn = element->destroy_function;
    if (copy_fn(new_data, element->data) != HX_OK) {
      // This should free everything that's partially allocated.
      destroy_fn(new_data);
      CBP_InitNullObj(&array->contents[array->size]);
      return HX_ERR;
    }
  } else {
    memcpy(new_data, element->data, element->element_size);
  }
  array->contents[array->size].data = new_data;
  array->size++;

  return HX_OK;
}

int CBP_Array_Destroy(CBP_Array *array) {
  if (array == NULL) {
    return HX_OK;
  }
  if (!array->is_ref) {
    // Only free the contents of an array if it's not a reference
    for (u64 i = 0; i < array->capacity; i++) {
      // CBP_DestroyObject(&(array->contents[i]));
      CBP_Nullify(&array->contents[i]);
    }
    free(array->contents);
  }
  free(array);
  return HX_OK;
}

int CBP_Array_Enlarge(CBP_Array *array, u64 new_capacity) {
  if (array == NULL || array->is_ref) {
    return HX_ERR;
  }
  CBP_Object *temp =
      realloc(array->contents, sizeof(CBP_Object) * new_capacity);
  if (temp == NULL) {
    return HX_ERR;
  }
  array->contents = temp;
  for (u64 i = array->capacity; i < new_capacity; i++) {
    CBP_InitNullObj(&array->contents[i]);
  }
  return HX_OK;
}

CBP_Object *CBP_Array_GetValue(CBP_Array *array, u64 index) {
  if (index >= array->size) {
    return NULL;
  }
  return &(array->contents[index]);
}

// I have really, really immersed in my own art.

void CBP_Array_ForEach(CBP_Array *array,
                       CBP_Array_CallbackFn callback_function) {
  for (u64 i = 0; i < array->size; i++) {
    callback_function(CBP_Array_GetValue(array, i));
  }
}