// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#ifndef HX_CBP_FILES_H

#define HX_CBP_FILES_H

#include <stdint.h>
#define HX_CBP_FILES_NAME_CAP 256

// Gets the path of the given file from a full path string.
// For example, if full_path = "C:\something\another_thing\voila.txt"
// Then it will return "C:\something\another_thing".
char *CBP_GetFilePath(const char *full_path);

// Separates the path and the file name from a full path string.
int CBP_SeparatePathAndName(char *path, uint64_t path_cap,
                            char *file_name, uint64_t file_name_cap,
                            uint64_t *path_len,
                            uint64_t *file_name_len,
                            const char *full_path);

// Concatenates paths
char *CBP_ConcatPaths(int count, ...);

#endif