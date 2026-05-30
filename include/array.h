// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#ifndef HX_CBP_ARRAY_H
#define HX_CBP_ARRAY_H

#include "object.h"

typedef struct {
  CBP_Object *contents;
  unsigned long long capacity;
  unsigned long long size;
  int is_ref;
} CBP_Array;

typedef void (*CBP_Array_CallbackFn)(const CBP_Object *);

// Get an empty CBP_Array.
CBP_Array *CBP_GetArray();

// Get an array reference.
CBP_Array CBP_GetArrayRef(CBP_Object *contents, unsigned long long size);

// Enlarge a CBP_Array.
int CBP_Array_Enlarge(CBP_Array *array, unsigned long long new_capacity);

// Push an element back.
int CBP_Array_Push(CBP_Array *array, const CBP_Object *element);

// Destroy a CBP_Array.
int CBP_Array_Destroy(CBP_Array *array);

// Safely get an element from a CBP_Array.
CBP_Object *CBP_Array_GetValue(CBP_Array *array, unsigned long long index);

CBP_Array *CBP_CopyFromArray(const CBP_Array *src);

void CBP_Array_ForEach(CBP_Array *array, CBP_Array_CallbackFn callback_function);

#define CBP_Array_TypeCheckedSafeGetValue(                                     \
    target, parser, expected_type, field_name, array, index, error_goto)       \
  {                                                                            \
    CBP_Object *tmp = CBP_Array_GetValue(array, index);                        \
    if (CBP_IsNullObj(tmp)) {                                                  \
      CBP_Parser_RegisterInvalidTypeError(parser, field_name, expected_type,   \
                                          NULLOBJ);                            \
      goto error_goto;                                                         \
    }                                                                          \
    if (tmp->type != expected_type) {                                          \
      CBP_Parser_RegisterInvalidTypeError(parser, field_name, expected_type,   \
                                          tmp->type);                          \
      goto error_goto;                                                         \
    }                                                                          \
    target = tmp;                                                              \
  }

#endif