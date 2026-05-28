// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#include "errors.h"
#include "macros.h"
#include "parser.h"
#include <stdio.h>

int CBP_Parser_RegisterError(CBP_Parser *parser, const char *what) {
  if (parser->states.nearest_error != NULL) {
    CBP_DestroyObject(parser->states.nearest_error);
  }
  char buffer[BUFFER_SIZE + 1];
  snprintf(buffer, BUFFER_SIZE, "In %s, line %llu: %s",
           parser->states.current_working_file_name, parser->states.line, what);
  parser->states.nearest_error = CBP_GetString(buffer);
  return HX_OK;
}

int CBP_Parser_RegisterInvalidTypeError(CBP_Parser *parser,
                                        const char *object_name,
                                        enum CBP_ObjectType expected_type,
                                        enum CBP_ObjectType got_type) {
  static const char *types_to_string[] = {"null", "int",    "uint",
                                          "char", "string", "custom"};
  char buffer[BUFFER_SIZE + 1];
  snprintf(buffer, BUFFER_SIZE,
           "Invalid type: For object \"%s\", expected %s, got %s", object_name,
           types_to_string[expected_type], types_to_string[got_type]);
  return CBP_Parser_RegisterError(parser, buffer);
}

int CBP_Parser_RegisterUnmatchedArgumentSizeError(
    CBP_Parser *parser, const char *line_type,
    const char *expected_argument_size, unsigned long long got_argument_size) {
  char buffer[BUFFER_SIZE + 1];
  snprintf(
      buffer, BUFFER_SIZE,
      "Unmatched argument size: For \"%s\", expected %s arguments, got %llu",
      line_type, expected_argument_size, got_argument_size);
  return CBP_Parser_RegisterError(parser, buffer);
}

int CBP_Parser_RegisterUnknownArgumentError(CBP_Parser *parser,
                                            const char *slot_name,
                                            const char *accepted_arguments,
                                            const char *got_argument) {
  char buffer[BUFFER_SIZE + 1];
  snprintf(buffer, BUFFER_SIZE,
           "Unknown argument: For \"%s\", expected argument in the range of "
           "[%s], got %s",
           slot_name, accepted_arguments, got_argument);
  return CBP_Parser_RegisterError(parser, buffer);
}