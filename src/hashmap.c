// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#include "hashmap.h"
#include "array.h"
#include "data.h"
#include "macros.h"
#include "models.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HASHMAP_PRIME 31
#define POOL_SIZE 256

// begin internal functions

CBP_HashCell *cbp_HashMap_GetHashCell() {
  CBP_HashCell *cell = calloc(1, sizeof(CBP_HashCell));
  if (cell == NULL) {
    return NULL;
  }
  cell->key = NULL;
  cell->next = NULL;
  return cell;
}

int cbp_HashMap_Hash(const char *key, uint64_t *hash) {
  if (key == NULL || hash == NULL) {
    goto return_err;
  }

  uint64_t result = 0;
  for (uint64_t i = 0; i < strlen(key); i++) {
    result = (result * HASHMAP_PRIME) + key[i];
  }
  *hash = result;
  return HX_OK;
return_err:
  return HX_ERR;
}

int cbp_HashMap_CleanupValue(CBP_HashCell *cell) {
  switch (cell->value.type) {
  case INT:
    break;
  case STRING:
    if (cell->value.data.s != NULL) {
      free(cell->value.data.s);
    }
    break;
  case ACCOUNT:
    CBP_Models_CleanupAccount(cell->value.data.ptr);
    break;
  case POSTING:
    CBP_Models_CleanupPosting(cell->value.data.ptr);
    break;
  case ENTRY:
    CBP_Models_CleanupEntry(cell->value.data.ptr);
    break;
  default:
    break;
  }
  return HX_OK;
}

int cbp_HashMap_CopyValue(CBP_HashCell *cell, const CBP_Data value) {
  switch (value.type) {
  case INT:
    cell->value.data.i = value.data.i;
    break;
  case STRING:
    if ((cell->value.data.s = strdup(value.data.s)) == NULL) {
      return HX_ERR;
    }
    break;
  case ACCOUNT:
    cell->value.data.ptr = calloc(1, sizeof(CBP_BeancountAccount));
    if (cell->value.data.ptr == NULL) {
      return HX_ERR;
    }
    return CBP_Models_CopyAccount(cell->value.data.ptr, value.data.ptr);
  case POSTING:
    cell->value.data.ptr = calloc(1, sizeof(CBP_BeancountPosting));
    if (cell->value.data.ptr == NULL) {
      return HX_ERR;
    }
    return CBP_Models_CopyPosting(cell->value.data.ptr, value.data.ptr);
  case ENTRY:
    cell->value.data.ptr = calloc(1, sizeof(CBP_BeancountJournalEntry));
    if (cell->value.data.ptr == NULL) {
      return HX_ERR;
    }
    return CBP_Models_CopyEntry(cell->value.data.ptr, value.data.ptr);
  default:
    break;
  }
  return HX_OK;
}

int cbp_HashMap_Update(CBP_HashCell *cell, const CBP_Data value,
                       bool is_new_cell) {
  if (!is_new_cell) {
    cbp_HashMap_CleanupValue(cell);
  }

  cell->value.type = value.type;

  return cbp_HashMap_CopyValue(cell, value);
}

// end internal functions

int CBP_HashMap_Init(CBP_HashMap *hashmap) {
  hashmap->pool = calloc(POOL_SIZE, sizeof(CBP_HashCell));
  if (hashmap->pool == NULL) {
    return HX_ERR;
  }
  for (int i = 0; i < POOL_SIZE; i++) {
    hashmap->pool[i].key = NULL;
    hashmap->pool[i].next = NULL;
  }
  CBP_Array_Init(&hashmap->keys);
  return HX_OK;
}

int CBP_HashMap_Upsert(CBP_HashMap *hashmap, const char *key,
                       const CBP_Data value) {
  if (hashmap == NULL || key == NULL) {
    return HX_ERR;
  }
  uint64_t hash;
  if (cbp_HashMap_Hash(key, &hash) != HX_OK) {
    return HX_ERR;
  }
  uint64_t pool_index = hash % POOL_SIZE;
  CBP_HashCell *cell = &hashmap->pool[pool_index];
  while (cell->next != NULL) {
    if (cell->key != NULL && strcmp(cell->key, key) == 0) {
      return cbp_HashMap_Update(cell, value, false);
    }
    cell = cell->next;
  }

  if (cell->key != NULL && strcmp(cell->key, key) == 0) {
    return cbp_HashMap_Update(cell, value, false);
  }

  CBP_HashCell *target_cell = NULL;
  if (cell->key == NULL) {
    target_cell = cell;
  } else {
    cell->next = cbp_HashMap_GetHashCell();
    if (cell->next == NULL) {
      return HX_ERR;
    }
    target_cell = cell->next;
  }
  target_cell->key = strdup(key);
  CBP_Array_PushString(&hashmap->keys, key);
  return cbp_HashMap_Update(target_cell, value, true);
}

int CBP_HashMap_Delete(CBP_HashMap *hashmap, const char *key) {
  if (hashmap == NULL || key == NULL) {
    return HX_ERR;
  }
  uint64_t hash;
  if (cbp_HashMap_Hash(key, &hash) != HX_OK) {
    return HX_ERR;
  }
  uint64_t pool_index = hash % POOL_SIZE;
  CBP_HashCell *cell = &hashmap->pool[pool_index];
  CBP_HashCell *prev = NULL;
  bool is_first_child = true;
  while (cell->next != NULL) {
    if (cell->key != NULL && strcmp(cell->key, key) == 0) {
      free(cell->key);
      cell->key = NULL;
      cbp_HashMap_CleanupValue(cell);
      if (prev != NULL) {
        prev->next = cell->next;
      }
      if (!is_first_child) {
        free(cell);
      }
      break;
    }
    prev = cell;
    cell = cell->next;
    is_first_child = false;
  }

  if (cell->key != NULL && strcmp(cell->key, key) == 0) {
    free(cell->key);
    cell->key = NULL;
    cbp_HashMap_CleanupValue(cell);
    if (prev != NULL) {
      prev->next = cell->next;
    }
    if (!is_first_child) {
      free(cell);
    }
  }
  return HX_OK;
}

CBP_Data *CBP_HashMap_RetrieveByKey(const CBP_HashMap *hash_map,
                                    const char *key) {
  uint64_t hash;
  CBP_Data *value = NULL;
  if (cbp_HashMap_Hash(key, &hash) != HX_OK) {
    return NULL;
  }
  uint64_t pool_index = hash % POOL_SIZE;

  const CBP_HashCell *cell = &hash_map->pool[pool_index];
  do {
    if (cell->key != NULL && strcmp(cell->key, key) == 0) {
      return (CBP_Data *)&cell->value;
    }
    if (cell->next != NULL) {
      cell = cell->next;
    }
  } while (cell->next != NULL);

  if (cell->key != NULL && strcmp(cell->key, key) == 0) {
    return (CBP_Data *)&cell->value;
  }

  return NULL;
}

int CBP_HashMap_Cleanup(CBP_HashMap *hash_map) {
  if (hash_map == NULL || hash_map->pool == NULL) {
    return HX_OK;
  }
  for (int i = 0; i < POOL_SIZE; i++) {
    CBP_HashCell *cell = hash_map->pool + i;
    int is_first_child = true;

    while (cell != NULL && cell->next != NULL) {
      if (cell->key != NULL) {
        free(cell->key);
        cell->key = NULL;
      }
      cbp_HashMap_CleanupValue(cell);
      CBP_HashCell *this_cell = cell;
      cell = cell->next;
      if (!is_first_child) {
        free(this_cell);
      } else {
        is_first_child = false;
      }
    }
    if (cell->key != NULL) {
      free(cell->key);
      cell->key = NULL;
    }
    cbp_HashMap_CleanupValue(cell);
    if (!is_first_child) {
      free(cell);
    }
  }
  free(hash_map->pool);
  hash_map->pool = NULL;
  CBP_Array_Cleanup(&hash_map->keys);
  return HX_OK;
}

int CBP_HashMap_Copy(CBP_HashMap *dst_hashmap, const CBP_HashMap *src_hashmap) {
  dst_hashmap->pool = calloc(POOL_SIZE, sizeof(CBP_HashCell));
  if (dst_hashmap->pool == NULL) {
    return HX_ERR;
  }
  for (int i = 0; i < POOL_SIZE; i++) {
    dst_hashmap->pool[i].key = NULL;
  }
  CBP_Array_InitByCopy(&dst_hashmap->keys, &src_hashmap->keys);

  for (uint64_t i = 0; i < POOL_SIZE; i++) {
    CBP_HashCell *dst_cell = dst_hashmap->pool + i;
    CBP_HashCell *cell = src_hashmap->pool + i;
    while (cell != NULL && dst_cell != NULL) {
      if (cell->key == NULL) {
        cell = cell->next;
        continue;
      }
      dst_cell->key = strdup(cell->key);
      cbp_HashMap_CopyValue(dst_cell, cell->value);
      if (cell->next != NULL) {
        dst_cell->next = cbp_HashMap_GetHashCell();
      } else {
        dst_cell->next = NULL;
      }
      dst_cell = dst_cell->next;
      cell = cell->next;
    }
  }
  return HX_OK;
}
