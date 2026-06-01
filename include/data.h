// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#ifndef HX_CBP_DATA_H
#define HX_CBP_DATA_H

#include <stdint.h>
enum CBP_ObjectType { NIL, INT, STRING, ACCOUNT, POSTING, ENTRY };

typedef struct CBP_Data {
  enum CBP_ObjectType type;
  union {
    int64_t i;
    char *s;
    void *ptr;
  } data;
} CBP_Data;

CBP_Data *CBP_GetString(const char *string);
CBP_Data *CBP_GetStringFromRange(const char *start, uint64_t length);

#endif