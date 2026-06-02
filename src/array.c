// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#include "array.h"
#include "data.h"
#include "macros.h"
#include "models.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// begin internal functions

int cbp_Array_AutoEnlarge(CBP_Array *array) {
  if (array->size + 1 >= array->capacity) {
    CBP_Data *tmp =
        realloc(array->contents, 2 * array->capacity * sizeof(CBP_Data));
    if (tmp == NULL) {
      free(array->contents);
      array->contents = NULL;
      return HX_ERR;
    }
    array->contents = tmp;
    array->capacity *= 2;
  }
  return HX_OK;
}

int cbp_Array_PushPtr(CBP_Array *array, int enum_type, const void *ptr) {
  const uint64_t sizes[] = {0,
                            0,
                            0,
                            sizeof(CBP_BeancountAccount),
                            sizeof(CBP_BeancountPosting),
                            sizeof(CBP_BeancountJournalEntry)};
  if (cbp_Array_AutoEnlarge(array) != HX_OK) {
    return HX_ERR;
  }
  array->contents[array->size].type = enum_type;
  array->contents[array->size].data.ptr = calloc(1, sizes[enum_type]);
  void *target_ptr = array->contents[array->size].data.ptr;
  switch (enum_type) {
  case ACCOUNT:
    CBP_Models_CopyAccount(target_ptr, ptr);
    break;
  case POSTING:
    CBP_Models_CopyPosting(target_ptr, ptr);
    break;
  case ENTRY:
    CBP_Models_CopyEntry(target_ptr, ptr);
    break;
  default:
    break;
  }
  array->size++;
  return HX_OK;
}

// end internal functions

int CBP_Array_Init(CBP_Array *array) {
  CBP_Data *tmp_data = calloc(CBP_ARRAY_DEFAULT_CAPACITY, sizeof(CBP_Data));
  if (tmp_data == NULL) {
    return HX_ERR;
  }
  array->capacity = CBP_ARRAY_DEFAULT_CAPACITY;
  array->size = 0;
  array->contents = tmp_data;
  return HX_OK;
}

int CBP_Array_InitByCopy(CBP_Array *target_array, const CBP_Array *src_array) {
  if (target_array == NULL || src_array == NULL) {
    return HX_ERR;
  }
  target_array->capacity = src_array->capacity;
  target_array->size = src_array->size;
  target_array->contents = calloc(target_array->capacity, sizeof(CBP_Data));
  if (target_array->contents == NULL) {
    return HX_ERR;
  }
  for (uint64_t i = 0; i < src_array->size; i++) {
    target_array->contents[i].type = src_array->contents[i].type;
    switch (src_array->contents[i].type) {
    case INT:
      target_array->contents[i].data.i = src_array->contents[i].data.i;
      break;
    case STRING:
      target_array->contents[i].data.s = strdup(src_array->contents[i].data.s);
      if (target_array->contents[i].data.s == NULL) {
        return HX_ERR;
      }
      break;
    case ACCOUNT:
    case ENTRY:
      continue;
      break;
    case POSTING: {
      CBP_BeancountPosting *posting = calloc(1, sizeof(CBP_BeancountPosting));
      if (posting == NULL) {
        return HX_ERR;
      }
      const CBP_BeancountPosting *src_posting = src_array->contents[i].data.ptr;
      if (CBP_Models_CopyPosting(posting, src_posting) != HX_OK) {
        return HX_ERR;
      }
      target_array->contents[i].data.ptr = (void *)posting;
    } break;
    default:
      return HX_ERR;
    }
  }
  return HX_OK;
}

int CBP_Array_Cleanup(CBP_Array *array) {
  if (array == NULL || array->contents == NULL) {
    return HX_OK;
  }
  for (uint64_t i = 0; i < array->size; i++) {
    CBP_Data *current_data = array->contents + i;
    switch (current_data->type) {
    case INT:
      continue;
      break;
    case STRING:
      if (current_data->data.s != NULL) {
        free(current_data->data.s);
        current_data->data.s = NULL;
      }
      break;
    case POSTING:
      CBP_Models_CleanupPosting(current_data->data.ptr);
      break;

    case ACCOUNT:
      CBP_Models_CleanupAccount(current_data->data.ptr);
      break;
    case ENTRY:
      CBP_Models_CleanupEntry(current_data->data.ptr);
      break;
    default:
      break;
    }
  }
  free(array->contents);
  array->size = 0;
  array->capacity = 0;
  array->contents = NULL;
  return HX_OK;
}

int CBP_Array_PushString(CBP_Array *array, const char *value) {
  if (cbp_Array_AutoEnlarge(array) != HX_OK) {
    return HX_ERR;
  }
  array->contents[array->size].type = STRING;
  array->contents[array->size].data.s = strdup(value);
  array->size++;
  return HX_OK;
}

int CBP_Array_PushStringFromRange(CBP_Array *array, const char *start,
                                  uint64_t length) {
  char *value = calloc(length + 1, sizeof(char));
  if (value == NULL) {
    return HX_ERR;
  }
  strncpy(value, start, length);
  int result = CBP_Array_PushString(array, value);
  free(value);
  return result;
}

int CBP_Array_PushPosting(CBP_Array *array, const void *ptr) {
  return cbp_Array_PushPtr(array, POSTING, ptr);
}

int CBP_Array_PushEntry(CBP_Array *array, const void *ptr) {
  return cbp_Array_PushPtr(array, ENTRY, ptr);
}

int CBP_Array_PushAccount(CBP_Array *array, const void *ptr) {
  return cbp_Array_PushPtr(array, ACCOUNT, ptr);
}

int CBP_Array_PushInt(CBP_Array *array, const int64_t value) {
  if (cbp_Array_AutoEnlarge(array) != HX_OK) {
    return HX_ERR;
  }
  array->contents[array->size].type = INT;
  array->contents[array->size].data.i = value;
  array->size++;
  return HX_OK;
}