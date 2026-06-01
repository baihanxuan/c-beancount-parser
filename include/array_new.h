// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#ifndef HX_CBP_ARRAY_H
#define HX_CBP_ARRAY_H

#include "data.h"
#include <stdint.h>

#define CBP_ARRAY_DEFAULT_CAPACITY 16

typedef struct {
  CBP_Data *contents; // CBP_Data[]
  unsigned long long capacity;
  unsigned long long size;
} CBP_Array;

int CBP_Array_Init(CBP_Array *array);

int CBP_Array_InitByCopy(CBP_Array *target_array, const CBP_Array *src_array);

int CBP_Array_Cleanup(CBP_Array *array);

// int CBP_Array_Push(CBP_Array *array, const CBP_Data *value);

int CBP_Array_PushString(CBP_Array *array, const char *value);

int CBP_Array_PushStringFromRange(CBP_Array *array, const char *start,
                                  uint64_t length);

int CBP_Array_PushAccount(CBP_Array *array, const void *ptr);

int CBP_Array_PushInt(CBP_Array *array, const int64_t value);

int CBP_Array_PushPosting(CBP_Array *array, const void *ptr);

int CBP_Array_PushEntry(CBP_Array *array, const void *ptr);

#endif