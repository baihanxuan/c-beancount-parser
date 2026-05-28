// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#ifndef HX_CBP_FILES_H

#define HX_CBP_FILES_H

// Gets the path of the given file from a full path string.
// For example, if full_path = "C:\something\another_thing\voila.txt"
// Then it will return "C:\something\another_thing".
char *CBP_GetFilePath(const char *full_path);

// Separates the path and the file name from a full path string.
int CBP_SeparatePathAndName(char *path, unsigned long long path_cap,
                            char *file_name, unsigned long long file_name_cap,
                            unsigned long long *path_len,
                            unsigned long long *file_name_len,
                            const char *full_path);

// Concatenates paths
char *CBP_ConcatPaths(int count, ...);

#endif