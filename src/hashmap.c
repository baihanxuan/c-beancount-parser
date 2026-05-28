// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#include "hashmap.h"
#include "array.h"
#include "macros.h"
#include "object.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define HASHMAP_PRIME 31
#define POOL_SIZE 256

CBP_HashCell *cbp_HashMap_GetHashCell() {
  CBP_HashCell *cell = calloc(1, sizeof(CBP_HashCell));
  if (cell == NULL) {
    return NULL;
  }
  cell->key = NULL;
  cell->value = NULL;
  cell->next = NULL;
  return cell;
}

int cbp_HashMap_Hash(const CBP_Object *object, u64 *hash) {
  if (object == NULL || object->data == NULL || hash == NULL) {
    goto return_err;
  }
  const unsigned char *bytes = (const unsigned char *)object->data;
  u64 result = 0;
  for (u64 i = 0; i < object->element_size; i++) {
    result = (result * HASHMAP_PRIME) + bytes[i];
  }
  *hash = result;
  return HX_OK;
return_err:
  return HX_ERR;
}

CBP_HashMap *CBP_GetHashMap() {
  CBP_HashMap *hashMap = calloc(1, sizeof(CBP_HashMap));
  if (hashMap == NULL) {
    return NULL;
  }
  hashMap->pool = calloc(POOL_SIZE, sizeof(CBP_HashCell));
  if (hashMap->pool == NULL) {
    free(hashMap);
    return NULL;
  }
  hashMap->keys = CBP_GetArray();
  if (hashMap->keys == NULL) {
    if (hashMap->pool != NULL) {
      free(hashMap->pool);
    }
    return NULL;
  }
  return hashMap;
}

int CBP_HashMap_Upsert(CBP_HashMap *hash_map, const CBP_Object *key,
                       const CBP_Object *value) {
  if (hash_map == NULL || CBP_IsNullObj(key) || CBP_IsNullObj(value)) {
    goto return_err;
  }
  u64 hash;
  if (cbp_HashMap_Hash(key, &hash) != HX_OK) {
    goto return_err;
  }
  u64 pool_index = hash % POOL_SIZE;
  CBP_HashCell *cell = &hash_map->pool[pool_index];
  // while (cell->next != NULL) {
  do {
    if (CBP_Eq(cell->key, key)) {
      CBP_DestroyObject(cell->value);
      cell->value = CBP_CopyFromObject(value);
      return HX_OK;
    }
    if (cell->next != NULL) {
      cell = cell->next;
    }
  } while (cell->next != NULL);

  if (CBP_Eq(cell->key, key)) {
    CBP_DestroyObject(cell->value);
    cell->value = CBP_CopyFromObject(value);
    return HX_OK;
  }

  CBP_HashCell *target_cell = NULL;
  if (cell->key == NULL && cell->value == NULL) {
    target_cell = cell;
  } else {
    cell->next = cbp_HashMap_GetHashCell();
    if (cell->next == NULL) {
      goto return_err;
    }
    target_cell = cell->next;
  }
  if (target_cell != NULL) {
    target_cell->key = CBP_CopyFromObject(key);
    target_cell->value = CBP_CopyFromObject(value);
  } else {
    goto return_err;
  }

  if (CBP_Array_Push(hash_map->keys, key) != HX_OK) {
    goto return_err;
  }
  return HX_OK;
return_err:
  return HX_ERR;
}

CBP_Object *CBP_HashMap_RetrieveByKey(CBP_HashMap *hash_map,
                                      const CBP_Object *key) {
  u64 hash;
  CBP_Object *value = NULL;
  if (cbp_HashMap_Hash(key, &hash) != HX_OK) {
    return NULL;
  }
  u64 pool_index = hash % POOL_SIZE;

  const CBP_HashCell *cell = &hash_map->pool[pool_index];
  do {
    if (CBP_Eq(cell->key, key)) {
      return cell->value;
    }
    if (cell->next != NULL) {
      cell = cell->next;
    }
  } while (cell->next != NULL);

  if (CBP_Eq(cell->key, key)) {
    return cell->value;
  }

  return NULL;
}

int CBP_HashMap_Destroy(CBP_HashMap *hash_map) {
  if (hash_map == NULL) {
    return HX_OK;
  }
  for (int i = 0; i < POOL_SIZE; i++) {
    CBP_HashCell *cell = hash_map->pool + i;
    int is_first_child = true;

    while (cell != NULL && cell->next != NULL) {
      CBP_DestroyObject(cell->key);
      CBP_DestroyObject(cell->value);
      CBP_HashCell *this_cell = cell;
      cell = cell->next;
      if (!is_first_child) {
        free(this_cell);
      } else {
        is_first_child = false;
      }
    }
    CBP_DestroyObject(cell->key);
    CBP_DestroyObject(cell->value);
    if (!is_first_child) {
      free(cell);
    }
  }
  free(hash_map->pool);
  CBP_GracefulDestroy(CBP_Array_Destroy, hash_map->keys);
  free(hash_map);
  return HX_OK;
}