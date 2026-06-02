// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#include "date.h"
#include "macros.h"
#include <stdint.h>
#include <stdio.h>

int cbp_IsLeapYear(uint64_t year) {
  if (year < 1582) {
    return year % 4 == 0;
  }
  if (year % 400 == 0) {
    return true;
  }
  if (year % 4 == 0 && year % 100 != 0) {
    return true;
  }
  return false;
}

bool CBP_Date_IsValidDateString(const char *string) {
  uint64_t y, m, d;
  if (sscanf(string, "%llu-%llu-%llu", &y, &m, &d) != 3) {
    return false;
  }

  // Be considerate for users under the reign of Pope Gregory XIII.
  // Calendar system switch can be a pain in the ass.
  if (y == 1582 && m == 10 && d >= 5 && d <= 14) {
    return false;
  }

  if (m > 12 || m == 0) {
    return false;
  }

  if (d == 0) {
    return false;
  }

  const int days_in_month[] = {0,  31, 28, 31, 30, 31, 30,
                               31, 31, 30, 31, 30, 31};

  int max_days = days_in_month[m];

  if (m == 2 && cbp_IsLeapYear(y)) {
    max_days = 29;
  }

  return d <= max_days;
}

int CBP_Date_Init(CBP_Date *date, const char *string) {
  uint64_t y, m, d;
  if (sscanf(string, "%llu-%llu-%llu", &y, &m, &d) != 3) {
    return HX_ERR;
  }
  date->year = y, date->month = m, date->day = d;
  return HX_OK;
}