// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#ifndef HX_CBP_ERRORS_H
#define HX_CBP_ERRORS_H

#include "parser.h"

int CBP_Parser_RegisterError(CBP_Parser *parser, const char *what);

int CBP_Parser_RegisterInvalidTypeError(CBP_Parser *parser,
                                        const char *object_name,
                                        enum CBP_ObjectType expected_type,
                                        enum CBP_ObjectType got_type);

int CBP_Parser_RegisterUnmatchedArgumentSizeError(
    CBP_Parser *parser, const char *line_type,
    const char *expected_argument_size, unsigned long long got_argument_size);

int CBP_Parser_RegisterUnknownArgumentError(CBP_Parser *parser,
                                            const char *slot_name,
                                            const char *accepted_arguments,
                                            const char *got_argument);
#endif