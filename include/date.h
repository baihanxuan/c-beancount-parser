// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#ifndef HX_CBP_DATE_H
#define HX_CBP_DATE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef _WIN32
#define CBP_DATE_FORMATSTRING "%llu-%llu-%llu"
#elifdef __linux__
#define CBP_DATE_FORMATSTRING "%lu-%lu-%lu"
#endif

typedef struct {
  uint64_t year, month, day;
} CBP_Date;


extern bool CBP_Date_IsValidDateString(const char* string);

extern int CBP_Date_Init(CBP_Date* date, const char* string);

#endif