// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#ifndef HX_CBP_HASHMAP_H
#define HX_CBP_HASHMAP_H

#include "array_new.h"
#include "data.h"

typedef struct CBP_HashCell {
  CBP_Data value;
  char *key; // = Cstring
  struct CBP_HashCell *next;
} CBP_HashCell;

typedef struct {
  CBP_HashCell *pool;
  CBP_Array keys;
} CBP_HashMap;

int CBP_HashMap_Init(CBP_HashMap *hashmap);

int CBP_HashMap_Cleanup(CBP_HashMap *hashmap);

int CBP_HashMap_Copy(CBP_HashMap *dst_hashmap, const CBP_HashMap *src_hashmap);

CBP_Data *CBP_HashMap_RetrieveByKey(const CBP_HashMap *hashmap,
                                    const char *key);

int CBP_HashMap_Upsert(CBP_HashMap *hashmap, const char *key,
                       const CBP_Data value);
#endif