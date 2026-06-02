// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#include "files.h"
#include "macros.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef _WIN32
#define SLASH '\\'
#elifdef __linux__
#define SLASH '/'
#endif

char *CBP_GetFilePath(const char *full_path) {
  char *path = NULL;
  if (full_path == NULL) {
    goto return_current_path;
  }
  const char *last_slash_pos = strrchr(full_path, SLASH);
  if (last_slash_pos == NULL) {
    if (strrchr(full_path, '/') != NULL) {
      last_slash_pos = strrchr(full_path, '/');
    } else {
      goto return_current_path;
    }
  }
  uint64_t length = last_slash_pos - full_path;
  if (length == 0) {
    length = 1;
  }
  path = malloc(length + 1);
  if (path == NULL) {
    return NULL;
  }
  memcpy(path, full_path, length);
  path[length] = '\0';
  return path;

return_current_path:
  path = strdup(".");
  return path;
}

int CBP_SeparatePathAndName(char *path, uint64_t path_cap, char *file_name,
                            uint64_t file_name_cap, uint64_t *path_len,
                            uint64_t *file_name_len, const char *full_path) {
  if (full_path == NULL) {
    return HX_ERR;
  }
  const char *last_slash_pos = strrchr(full_path, SLASH);
  if (last_slash_pos == NULL) {
    if (strrchr(full_path, '/') != NULL) {
      last_slash_pos = strrchr(full_path, '/');
    } else {
      goto return_current_path_and_name;
    }
  }

  uint64_t length = last_slash_pos - full_path;
  if (length == 0) {
    length = 1;
  }

  if (path != NULL && path_cap > 0) {
    snprintf(path, path_cap, "%.*s", (int)length, full_path);
  }
  if (file_name != NULL && file_name_cap > 0) {
    snprintf(file_name, file_name_cap, "%s", last_slash_pos + 1);
  }

  if (path_len != NULL) {
    *path_len = length + 1;
  }
  if (file_name_len != NULL) {
    *file_name_len = strlen(last_slash_pos + 1) + 1;
  }

  return HX_OK;

return_current_path_and_name:
  if (path != NULL) {
    snprintf(path, path_cap, ".");
  }
  if (file_name != NULL) {
    snprintf(file_name, file_name_cap, "%s", full_path);
  }
  if (path_len != NULL) {
    *path_len = 2; // "."
  }
  if (file_name_len != NULL) {
    *file_name_len = strlen(full_path) + 1;
  }
  return HX_OK;
}

char *CBP_ConcatPaths(int count, ...) {
  va_list args;
  char *res = NULL;
  uint64_t path_length = 0;

  va_start(args, count);

  for (int i = 0; i < count; i++) {
    char *segment = va_arg(args, char *);
    uint64_t segment_len = strlen(segment);
    if (res == NULL) {
      res = calloc(segment_len + 1, sizeof(char));
      if (res == NULL) {
        goto return_null;
      }
    } else {
      res = realloc(res, path_length + segment_len + 1);
      if (res == NULL) {
        goto return_null;
      }
    }
    memcpy(res + path_length, segment, segment_len);
    path_length += segment_len;
    if (i != count - 1) {
      res[path_length] = SLASH;
      path_length++;
    }
  }

  va_end(args);

  res = realloc(res, path_length + 1);
  if (res == NULL) {
    goto return_null;
  }

  res[path_length] = '\0';

  return res;

return_null:
  return NULL;
}