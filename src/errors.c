// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#include "errors.h"
#include "array_new.h"
#include "macros.h"
// #include "object.h"
#include "data.h"
#include "parser_new.h"
#include <stdarg.h>
#include <stdio.h>

int CBP_Parser_RegisterError(CBP_Parser *parser, const char *format, ...) {
  if (parser == NULL || format == NULL) {
    return HX_ERR;
  }
  char buffer[BUFFER_SIZE + 1];
  int written_bytes =
      snprintf(buffer, BUFFER_SIZE,
               "In %s, line %llu: ", parser->states.current_file_name,
               parser->states.line);
  va_list args;
  va_start(args, format);
  vsnprintf(buffer + written_bytes, BUFFER_SIZE, format, args);
  va_end(args);
  CBP_Array_PushString(parser->error_list, buffer);
  return HX_OK;
}

int CBP_Parser_RegisterInvalidTypeError(CBP_Parser *parser,
                                        const char *object_name,
                                        enum CBP_ObjectType expected_type,
                                        enum CBP_ObjectType got_type) {
  static const char *types_to_string[] = {"null", "constview", "int",   "uint",
                                          "char", "string",    "custom"};
  return CBP_Parser_RegisterError(
      parser, "Invalid type: For object \"%s\", expected %s, got %s",
      object_name, types_to_string[expected_type], types_to_string[got_type]);
}

int CBP_Parser_RegisterUnmatchedArgumentSizeError(
    CBP_Parser *parser, const char *line_type,
    const char *expected_argument_size, unsigned long long got_argument_size) {
  return CBP_Parser_RegisterError(
      parser,
      "Unmatched argument size: For \"%s\", expected %s arguments, got %llu",
      line_type, expected_argument_size, got_argument_size);
}

int CBP_Parser_RegisterUnknownArgumentError(CBP_Parser *parser,
                                            const char *slot_name,
                                            const char *accepted_arguments,
                                            const char *got_argument) {
  return CBP_Parser_RegisterError(parser,
                                  "Unknown argument: For \"%s\", expected "
                                  "argument in the range of [%s], got %s",
                                  slot_name, accepted_arguments, got_argument);
}