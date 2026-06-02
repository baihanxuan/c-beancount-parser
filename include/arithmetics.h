// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#ifndef HX_CBP_ARITHMETICS_H
#define HX_CBP_ARITHMETICS_H

#include <stdint.h>

double CBP_Arith_RawRepresentationToDouble(int64_t raw_value);

// Note that the two functions are all based on an assumption that HX_CBP_PRECISION == 2.
// This may be changed later.
int64_t CBP_Arith_GetFixedPointRepr(const char*);

int64_t CBP_Arith_GetQuotedFixedPointRepr(int64_t, int64_t);
#endif