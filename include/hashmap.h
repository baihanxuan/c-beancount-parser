// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#ifndef HX_CBP_HASHMAP_H
#define HX_CBP_HASHMAP_H

#include "array.h"
#include "object.h"

typedef struct CBP_HashCell {
  CBP_Object *value;
  CBP_Object *key;
  struct CBP_HashCell *next;
} CBP_HashCell;

typedef struct {
  CBP_HashCell *pool;
  CBP_Array *keys;
} CBP_HashMap;

CBP_HashMap *CBP_GetHashMap();

int CBP_HashMap_Upsert(CBP_HashMap *hash_map, const CBP_Object *key,
                       const CBP_Object *value);

int CBP_HashMap_Destroy(CBP_HashMap *hash_map);

CBP_Object *CBP_HashMap_RetrieveByKey(CBP_HashMap *hash_map,
                                      const CBP_Object *key);

#endif