// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#ifndef HX_CBP_PARSER_H
#define HX_CBP_PARSER_H

#include "array.h"
#include "date.h"
#include "hashmap.h"
#include <stdbool.h>

typedef struct {
  CBP_Array files_to_include;
  char *working_dir;
  unsigned long long line;
  bool is_in_transaction;
  char *current_file_name;
  CBP_HashMap pad_balance_map;
} CBP_ParserStates;

typedef struct {
  char *payee;
  char *remarks;
  CBP_Array postings;
  CBP_HashMap balance_map;
  CBP_Date posted_at;
  char *catch_all_account;
} CBP_TxnStates;

typedef struct CBP_Parser {
  char *title;
  CBP_HashMap *accounts;
  CBP_Array *journal_entries;
  CBP_Array *error_list;
  CBP_ParserStates states;
  CBP_TxnStates current_txn_states;
} CBP_Parser;

CBP_Parser *CBP_GetParser(const char *working_dir,
                          const char *current_file_name);

CBP_Parser *CBP_GetSubParser(const char *working_dir,
                             const char *current_file_name,
                             CBP_Parser *main_parser);

int CBP_CleanupParser(CBP_Parser *parser);

int CBP_CleanupSubParser(CBP_Parser *parser);

int CBP_Parser_CleanupCurrentTxnStates(CBP_TxnStates *txn_states);

int CBP_Parser_CleanupStates(CBP_ParserStates *parser_states);

int CBP_Parser_InitCurrentTxnStates(CBP_TxnStates *txn_states);

int CBP_Parser_InitStates(CBP_ParserStates *parser_states,
                          const char *working_dir,
                          const char *current_file_name);

extern CBP_Parser *CBP_Parse(const char *file_name);

#endif