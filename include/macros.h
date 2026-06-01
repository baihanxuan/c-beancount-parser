// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#ifndef HX_CBP_MACROS_H
#define HX_CBP_MACROS_H

#define u64 unsigned long long
#define i64 long long
#define u32 unsigned int
#define i32 int

#define HX_OK 0
#define HX_ERR 1

#define BUFFER_SIZE 2048
#define HX_EPSILON 1e-6

#define CBP_PtrSafeAssign(type, lhs, rhs, error_goto)                          \
  {                                                                            \
    type *tmp = rhs;                                                           \
    if (tmp == NULL) {                                                         \
      goto error_goto;                                                         \
    }                                                                          \
    lhs = tmp;                                                                 \
  }

#define CBP_GracefulDestroy(destroy_func, object)                              \
  {                                                                            \
    if (object != NULL) {                                                      \
      destroy_func(object);                                                    \
    }                                                                          \
    object = NULL;                                                             \
  }

#define CBP_PtrSafeAllocate(target, num_of_elements, size_of_element,          \
                            error_goto)                                        \
  {                                                                            \
    target = calloc(num_of_elements, size_of_element);                         \
    if (target == NULL) {                                                      \
      goto error_goto;                                                         \
    }                                                                          \
  }
#endif