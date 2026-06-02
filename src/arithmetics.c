// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#include "arithmetics.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


double CBP_Arith_RawRepresentationToDouble(long long raw_value) {
  return (double)(raw_value) / (double)pow(10, 2);
}

int64_t CBP_Arith_GetFixedPointRepr(const char *string_value) {
  char *decimal = calloc(strlen(string_value) + 1, sizeof(char)),
       *fractional = calloc(strlen(string_value) + 1, sizeof(char));
  if (decimal == NULL || fractional == NULL) {
    goto return_err;
  }

  const char *decimal_point_pos = strrchr(string_value, '.');
  if (decimal_point_pos == NULL) {
    strcpy(decimal, string_value);
  } else {
    strncpy(decimal, string_value, decimal_point_pos - string_value);
    strcpy(fractional, decimal_point_pos + 1);
  }
  int64_t result = 0;
  int is_negative = decimal[0] == '-';
  for (int64_t i = is_negative; i < strlen(decimal); i++) {
    if (decimal[i] >= '0' && decimal[i] <= '9') {
      result *= 10;
      result += (decimal[i] - '0');
    }
  }
  result *= 10;
  for (uint64_t i = 0; i < strlen(fractional); i++) {
    if (fractional[i] >= '0' && fractional[i] <= '9') {
      result += (fractional[i] - '0');
      if (i != strlen(fractional) - 1) {
        result *= 10;
      }
    }
  }
  result *= (is_negative ? -1 : 1);
  free(decimal);
  free(fractional);
  return result;
return_err:
  free(decimal);
  free(fractional);
  return 0;
}

int64_t CBP_Arith_GetQuotedFixedPointRepr(int64_t original_fixed_point_repr,
                                      int64_t fx_rate_fixed_point_repr) {
  int64_t result = original_fixed_point_repr * fx_rate_fixed_point_repr;
  return (result / 100) + ((result % 100) >= 50);
}