// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#include "date.h"
#include "macros.h"
// #include "object.h"
#include <stdint.h>
#include <stdio.h>
// #include <stdlib.h>

int cbp_IsLeapYear(u64 year) {
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

// })

// int CBP_Date_Destroy(void *date_data) {
//   CBP_GracefulDestroy(free, date_data);
//   return HX_OK;
// }

// int CBP_Date_Copy(void *dst, const void *src) {
//   if (dst == NULL || src == NULL) {
//     goto return_err;
//   }
//   const CBP_Date *date = src;
//   CBP_Date *target = dst;
//   target->day = date->day;
//   target->month = date->month;
//   target->year = date->year;
//   return HX_OK;
// return_err:
//   return HX_ERR;
// }

// int CBP_Date_Compare(const CBP_Object *this_one, const CBP_Object *that_one)
// {
//   const CBP_Date *this_date = this_one->data, *that_date = that_one->data;
//   if (this_date->year < that_date->year) {
//     return -1;
//   }
//   if (this_date->year == that_date->year) {
//     if (this_date->month < that_date->month) {
//       return -1;
//     }
//     if (this_date->month == that_date->month) {
//       if (this_date->day < that_date->day) {
//         return -1;
//       }
//       if (this_date->day == that_date->day) {
//         return 0;
//       }
//       return 1;
//     }
//     return 1;
//   }
//   return 1;
// }

// CBP_Object *CBP_GetDate(const CBP_Object *value) {
//   if (CBP_IsNullObj(value)) {
//     goto return_null;
//   }
//   if (value->type != STRING) {
//     goto return_null;
//   }
//   CBP_Object *obj = NULL;
//   CBP_PtrSafeAssign(CBP_Object, obj,
//                     CBP_GetCustom(NULL, sizeof(CBP_Date), CBP_Date_Destroy,
//                                   CBP_Date_Copy, CBP_Date_Compare),
//                     return_null);

//   CBP_Date *date = obj->data;

//   u64 y, m, d;
//   if (sscanf(value->data, "%llu-%llu-%llu", &y, &m, &d) == 3) {
//     date->year = y;
//     date->month = m;
//     date->day = d;
//   } else {
//     CBP_DestroyObject(obj);
//     goto return_null;
//   }

//   return obj;
// return_null:
//   return NULL;
// }