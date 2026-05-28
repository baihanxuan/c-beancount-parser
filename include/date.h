// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#ifndef HX_CBP_DATE_H
#define HX_CBP_DATE_H

#include "object.h"

typedef struct {
  unsigned long long year, month, day;
} CBP_Date;

CBP_Object *CBP_GetDate(const CBP_Object *value);

int CBP_Date_Destroy(void *);

#endif