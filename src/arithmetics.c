// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#include "arithmetics.h"

double CBP_Arith_RawRepresentationToDouble(long long raw_value) {
  return (double)(raw_value) / (double)100.00;
}