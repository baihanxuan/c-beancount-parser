// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#include "object.h"
#include "macros.h"
// #include <malloc.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// #include <stdio.h>

#define CBP_ObjectEqCValues_Meta(CTypeFnName, CBPObjEnum, CType)               \
  int CBP_EqC##CTypeFnName(const CBP_Object *lhs, CType value) {               \
    if (lhs == NULL || lhs->type != CBPObjEnum || lhs->data == NULL) {         \
      return false;                                                            \
    }                                                                          \
    return *(CType *)lhs->data == value;                                       \
  }

#define CBP_ObjectDef_Meta(CBPObjFn, CBPObjEnum, CType)                        \
  CBP_Object *CBP_Get##CBPObjFn(CType value) {                                 \
    CBP_Object *obj = calloc(1, sizeof(CBP_Object));                           \
    if (obj == NULL) {                                                         \
      return NULL;                                                             \
    }                                                                          \
    obj->type = CBPObjEnum;                                                    \
    obj->data = calloc(1, sizeof(CType));                                      \
    obj->destroy_function = NULL;                                              \
    obj->copy_function = NULL;                                                 \
    if (obj->data == NULL) {                                                   \
      free(obj);                                                               \
      return NULL;                                                             \
    }                                                                          \
    *(CType *)(obj->data) = value;                                             \
    obj->element_size = sizeof(CType);                                         \
    return obj;                                                                \
  }

CBP_ObjectDef_Meta(Int, INT, i64);
CBP_ObjectDef_Meta(Uint, UINT, u64);
CBP_ObjectDef_Meta(Char, CHAR, char);

CBP_ObjectEqCValues_Meta(int, INT, i64);
CBP_ObjectEqCValues_Meta(uint, UINT, u64);
CBP_ObjectEqCValues_Meta(char, CHAR, char);

CBP_Object *CBP_GetString(const char *value) {
  CBP_Object *obj = calloc(1, sizeof(CBP_Object));
  if (obj == NULL) {
    return NULL;
  }
  obj->type = STRING;
  obj->element_size = sizeof(char) * (strlen(value) + 1);
  obj->data = calloc(obj->element_size, sizeof(char));
  obj->destroy_function = NULL;
  obj->copy_function = NULL;
  if (obj->data == NULL) {
    free(obj);
    return NULL;
  }
  // memset(obj->data, 0, obj->element_size);
  strcpy(obj->data, value);
  return obj;
}

CBP_Object *CBP_GetStringFromRange(const char *start_ptr, u64 length) {
  CBP_Object *obj = malloc(sizeof(CBP_Object));
  if (obj == NULL) {
    return NULL;
  }
  obj->type = STRING;
  obj->element_size = length + 1;
  obj->data = calloc(1, obj->element_size);
  obj->destroy_function = NULL;
  obj->copy_function = NULL;
  if (obj->data == NULL) {
    free(obj);
    return NULL;
  }
  // memset(obj->data, 0, obj->element_size);
  memcpy(obj->data, start_ptr, length);
  // ((char *)obj->data)[length] = '\0';
  return obj;
}

CBP_Object *CBP_GetCustom(const void *value, u64 size,
                          const CBP_DestroyFn destroy_function,
                          const CBP_CopyFn copy_function) {
  CBP_Object *obj = calloc(1, sizeof(CBP_Object));
  if (obj == NULL) {
    return NULL;
  }
  obj->type = CUSTOM;
  obj->element_size = size;
  obj->data = calloc(1, size);
  if (obj->data == NULL) {
    free(obj);
    return NULL;
  }
  if (destroy_function == NULL || copy_function == NULL) {
    free(obj);
    return NULL;
  }
  obj->destroy_function = destroy_function;
  obj->copy_function = copy_function;
  // memset(obj->data, 0, size);
  if (value != NULL) {
    memcpy(obj->data, value, size);
  }
  return obj;
}

CBP_Object *CBP_GetConstView(const CBP_Object *src) {
  CBP_Object *obj = calloc(1, sizeof(CBP_Object));
  if (obj == NULL) {
    return NULL;
  }
  obj->type = CONSTVIEW;
  obj->element_size = sizeof(CBP_Object *);
  obj->destroy_function = NULL;
  obj->copy_function = NULL;
  obj->data = (void *)src;
  return obj;
}

CBP_Object *CBP_CopyFromObject(const CBP_Object *src) {
  CBP_Object *obj = calloc(1, sizeof(CBP_Object));
  if (obj == NULL) {
    return NULL;
  }
  obj->type = src->type;
  obj->element_size = src->element_size;
  if (src->type == CONSTVIEW) {
    obj->data = src->data;
  } else {
    obj->data = calloc(1, src->element_size);
    if (obj->data == NULL) {
      free(obj);
      return NULL;
    }
    if (src->type == CUSTOM && src->copy_function != NULL &&
        src->destroy_function != NULL) {
      src->copy_function(obj->data, src->data);
      obj->destroy_function = src->destroy_function;
      obj->copy_function = src->copy_function;
    } else {
      // memset(obj->data, 0, src->element_size);
      memcpy(obj->data, src->data, src->element_size);
      obj->copy_function = NULL;
      obj->destroy_function = NULL;
    }
  }
  // printf("[DEBUG INFO] Done memcpy'ing. Return.\n");
  return obj;
}

int CBP_EqCstring(const CBP_Object *lhs, const char *rhs) {
  if (lhs == NULL || lhs->data == NULL || lhs->type != STRING) {
    return 0;
  }
  return strcmp((char *)lhs->data, rhs) == 0;
}

int CBP_Nullify(CBP_Object *obj) {
  if (obj == NULL) {
    return HX_ERR;
  }
  if (obj->data != NULL) {
    if (obj->type == CUSTOM) {
      // Custom objects must implement their own nullification function
      // for memory safety.
      // return HX_ERR;
      if (obj->destroy_function == NULL) {
        return HX_ERR;
      }
      CBP_DestroyFn destroy_fn = obj->destroy_function;
      destroy_fn(obj->data);
    } else if (obj->type == CONSTVIEW) {
      // Do nothing.
    } else {
      free(obj->data);
    }
  }
  obj->element_size = 0;
  obj->type = NULLOBJ;
  return HX_OK;
}

int CBP_DestroyObject(CBP_Object *obj) {
  if (obj == NULL) {
    return HX_OK; // The object has been destroyed.
  }

  if (obj->data != NULL) {
    if (obj->type == CUSTOM) {
      // Custom objects must implement their own destroy function
      // for memory safety.
      if (obj->destroy_function == NULL) {
        return HX_ERR;
      }

      CBP_DestroyFn destroy_fn = obj->destroy_function;
      destroy_fn(obj->data);
    } else if (obj->type == CONSTVIEW) {
      // do nothing;
    } else {
      free(obj->data);
    }
  }
  free(obj);
  return HX_OK;
}

int CBP_InitNullObj(CBP_Object *obj) {
  obj->type = NULLOBJ;
  obj->data = NULL;
  obj->element_size = 0;
  return HX_OK;
}

int CBP_Eq(const CBP_Object *lhs, const CBP_Object *rhs) {
  if (lhs == NULL || rhs == NULL) {
    return (lhs == rhs);
  }

  if (lhs == rhs || lhs->data == rhs->data) {
    return true;
  }

  if (lhs->element_size != rhs->element_size) {
    return false;
  }

  return memcmp(lhs->data, rhs->data, lhs->element_size) == 0;
}

int CBP_IsNullObj(const CBP_Object *obj) {
  if (obj == NULL) {
    return 1;
  }
  if (obj->type == NULLOBJ) {
    return 2;
  }
  if (obj->data == NULL) {
    return 3;
  }
  return 0;
}