// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#include "object.h"
#include "macros.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define CBP_ObjectEqCValues_Meta(CTypeFnName, CBPObjEnum, CType, EqPredicate)  \
  int CBP_EqC##CTypeFnName(const CBP_Object *lhs, CType value) {               \
    if (lhs == NULL || lhs->type != CBPObjEnum || lhs->data == NULL) {         \
      return false;                                                            \
    }                                                                          \
    return EqPredicate || *(CType *)lhs->data == value;                        \
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

#define CBP_ObjectDef_Cliche(target, CBPObjEnum, CBPElemSize, CBPDestroyFn,    \
                             CBPCopyFn, ErrorGoto)                             \
  {                                                                            \
    target->type = CBPObjEnum;                                                 \
    target->element_size = CBPElemSize;                                        \
    CBP_PtrSafeAssign(void, target->data, calloc(1, CBPElemSize), ErrorGoto);  \
    target->destroy_function = CBPDestroyFn;                                   \
    target->copy_function = CBPCopyFn;                                         \
  }

/* CBP Object Creation */
CBP_ObjectDef_Meta(Int, INT, i64);
CBP_ObjectDef_Meta(Uint, UINT, u64);
CBP_ObjectDef_Meta(Char, CHAR, char);

CBP_Object *CBP_GetString(const char *value) {
  CBP_Object *obj = NULL;
  CBP_PtrSafeAssign(CBP_Object, obj, calloc(1, sizeof(CBP_Object)),
                    return_null);
  CBP_ObjectDef_Cliche(obj, STRING, sizeof(char) * (strlen(value) + 1), NULL,
                       NULL, return_null);
  strcpy(obj->data, value);
  return obj;
return_null:
  CBP_GracefulDestroy(CBP_DestroyObject, obj);
  return NULL;
}

CBP_Object *CBP_GetStringFromRange(const char *start_ptr, u64 length) {
  CBP_Object *obj = NULL;
  CBP_PtrSafeAssign(CBP_Object, obj, calloc(1, sizeof(CBP_Object)),
                    return_null);
  CBP_ObjectDef_Cliche(obj, STRING, sizeof(char) * (length + 1), NULL, NULL,
                       return_null);
  memcpy(obj->data, start_ptr, length);
  return obj;
return_null:
  CBP_GracefulDestroy(CBP_DestroyObject, obj);
  return NULL;
}

CBP_Object *CBP_GetCustom(const void *value, u64 size,
                          const CBP_DestroyFn destroy_function,
                          const CBP_CopyFn copy_function) {
  CBP_Object *obj = NULL;
  CBP_PtrSafeAssign(CBP_Object, obj, calloc(1, sizeof(CBP_Object)),
                    return_null);
  CBP_ObjectDef_Cliche(obj, CUSTOM, size, destroy_function, copy_function,
                       return_null);
  if (value != NULL) {
    memcpy(obj->data, value, size);
  }
  return obj;
return_null:
  if (obj != NULL) {
    CBP_GracefulDestroy(free, obj->data);
    CBP_GracefulDestroy(free, obj);
  }
  return NULL;
}

CBP_Object *CBP_GetConstView(const CBP_Object *src) {
  CBP_Object *obj = NULL;
  CBP_PtrSafeAssign(CBP_Object, obj, calloc(1, sizeof(CBP_Object)),
                    return_null);
  CBP_ObjectDef_Cliche(obj, CONSTVIEW, sizeof(CBP_Object *), NULL, NULL,
                       return_null);
  obj->data = (void *)src;
  return obj;
return_null:
  CBP_GracefulDestroy(free, obj);
  return NULL;
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
      memcpy(obj->data, src->data, src->element_size);
      obj->copy_function = NULL;
      obj->destroy_function = NULL;
    }
  }
  return obj;
}

/* End CBP Object Creation */

/* CBP Object Equation */

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

CBP_ObjectEqCValues_Meta(int, INT, i64, false);
CBP_ObjectEqCValues_Meta(uint, UINT, u64, false);
CBP_ObjectEqCValues_Meta(char, CHAR, char, false);
CBP_ObjectEqCValues_Meta(string, STRING, const char *,
                         strcmp((char *)lhs->data, value) == 0);

/* End CBP Object Equation */

/* CBP Object Nullification-related methods */

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

/* End CBP Object Nullification-related methods */