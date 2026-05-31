// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#ifndef HX_CBP_ARITHMETICS_H
#define HX_CBP_ARITHMETICS_H

double CBP_Arith_RawRepresentationToDouble(long long raw_value);

// Note that the two functions are all based on an assumption that HX_CBP_PRECISION == 2.
// This may be changed later.
long long CBP_Arith_GetFixedPointRepr(const char*);

long long CBP_Arith_GetQuotedFixedPointRepr(long long, long long);
#endif