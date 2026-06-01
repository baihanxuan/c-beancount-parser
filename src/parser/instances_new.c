// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#include "array_new.h"
#include "hashmap_new.h"
#include "macros.h"
#include "parser_new.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

int CBP_Parser_InitStates(CBP_ParserStates *parser_states,
                          const char *working_dir,
                          const char *current_file_name) {
  if (parser_states == NULL || working_dir == NULL ||
      current_file_name == NULL) {
    return HX_ERR;
  }
  parser_states->working_dir = strdup(working_dir);
  parser_states->current_file_name = strdup(current_file_name);

  if (parser_states->working_dir == NULL ||
      parser_states->current_file_name == NULL) {
    goto cleanup_and_return_err;
  }

  if (CBP_Array_Init(&parser_states->files_to_include) != HX_OK) {
    goto cleanup_and_return_err;
  }

  parser_states->line = 0;
  parser_states->is_in_transaction = false;

  if (CBP_HashMap_Init(&parser_states->pad_balance_map) != HX_OK) {
    goto cleanup_and_return_err;
  }

  return HX_OK;
cleanup_and_return_err:
  if (parser_states->working_dir != NULL) {
    free(parser_states->working_dir);
  }
  if (parser_states->current_file_name != NULL) {
    free(parser_states->current_file_name);
  }
  CBP_Array_Cleanup(&parser_states->files_to_include);
  CBP_HashMap_Cleanup(&parser_states->pad_balance_map);
  return HX_ERR;
}

int CBP_Parser_InitCurrentTxnStates(CBP_TxnStates *txn_states) {
  if (txn_states == NULL) {
    return HX_ERR;
  }
  txn_states->payee = NULL;
  txn_states->remarks = NULL;
  if (CBP_Array_Init(&txn_states->postings) != HX_OK) {
    goto cleanup_and_return_err;
  }
  if (CBP_HashMap_Init(&txn_states->balance_map) != HX_OK) {
    goto cleanup_and_return_err;
  }
  txn_states->catch_all_account = NULL;
  return HX_OK;

cleanup_and_return_err:
  CBP_Array_Cleanup(&txn_states->postings);
  CBP_HashMap_Cleanup(&txn_states->balance_map);
  return HX_ERR;
}

int CBP_Parser_CleanupStates(CBP_ParserStates *parser_states) {
  if (parser_states == NULL) {
    return HX_OK;
  }
  CBP_Array_Cleanup(&parser_states->files_to_include);
  if (parser_states->working_dir != NULL) {
    free(parser_states->working_dir);
    parser_states->working_dir = NULL;
  }
  if (parser_states->current_file_name != NULL) {
    free(parser_states->current_file_name);
    parser_states->current_file_name = NULL;
  }
  parser_states->line = 0;
  parser_states->is_in_transaction = false;
  CBP_HashMap_Cleanup(&parser_states->pad_balance_map);
  return HX_OK;
}

int CBP_Parser_CleanupCurrentTxnStates(CBP_TxnStates *txn_states) {
  if (txn_states == NULL) {
    return HX_OK;
  }
  if (txn_states->payee != NULL) {
    free(txn_states->payee);
    txn_states->payee = NULL;
  }
  if (txn_states->remarks != NULL) {
    free(txn_states->remarks);
    txn_states->remarks = NULL;
  }
  CBP_Array_Cleanup(&txn_states->postings);
  CBP_HashMap_Cleanup(&txn_states->balance_map);
  if (txn_states->catch_all_account != NULL) {
    free(txn_states->catch_all_account);
    txn_states->catch_all_account = NULL;
  }
  return HX_OK;
}

CBP_Parser *CBP_GetParser(const char *working_dir,
                          const char *current_file_name) {
  CBP_Parser *parser = calloc(1, sizeof(CBP_Parser));
  if (parser == NULL) {
    return NULL;
  }

  parser->title = NULL;

  parser->accounts = calloc(1, sizeof(CBP_HashMap));
  parser->journal_entries = calloc(1, sizeof(CBP_Array));
  parser->error_list = calloc(1, sizeof(CBP_Array));

  if (parser->accounts == NULL || parser->journal_entries == NULL ||
      parser->error_list == NULL) {
    goto cleanup_and_return_null;
  }

  if (CBP_HashMap_Init(parser->accounts) != HX_OK) {
    goto cleanup_and_return_null;
  }

  if (CBP_Array_Init(parser->journal_entries) != HX_OK) {
    goto cleanup_and_return_null;
  }

  if (CBP_Array_Init(parser->error_list) != HX_OK) {
    goto cleanup_and_return_null;
  }

  if (CBP_Parser_InitStates(&parser->states, working_dir, current_file_name) !=
      HX_OK) {
    goto cleanup_and_return_null;
  }

  if (CBP_Parser_InitCurrentTxnStates(&parser->current_txn_states) != HX_OK) {
    goto cleanup_and_return_null;
  }
  return parser;

cleanup_and_return_null:
  if (parser->accounts != NULL) {
    CBP_HashMap_Cleanup(parser->accounts);
  }
  if (parser->journal_entries != NULL) {
    CBP_Array_Cleanup(parser->journal_entries);
  }
  if (parser->error_list != NULL) {
    CBP_Array_Cleanup(parser->error_list);
  }
  CBP_Parser_CleanupStates(&parser->states);
  CBP_Parser_CleanupCurrentTxnStates(&parser->current_txn_states);
  free(parser);
  return NULL;
}

CBP_Parser *CBP_GetSubParser(const char *working_dir,
                             const char *current_file_name,
                             CBP_Parser *main_parser) {
  CBP_Parser *parser = calloc(1, sizeof(CBP_Parser));
  if (parser == NULL) {
    return NULL;
  }

  parser->title = NULL; // No need to assign a title here.
  parser->accounts = main_parser->accounts;
  parser->journal_entries = main_parser->journal_entries;
  parser->error_list = main_parser->error_list;

  if (CBP_Parser_InitStates(&parser->states, working_dir, current_file_name) !=
      HX_OK) {
    goto cleanup_and_return_null;
  }

  if (CBP_Parser_InitCurrentTxnStates(&parser->current_txn_states) != HX_OK) {
    goto cleanup_and_return_null;
  }

  return parser;

cleanup_and_return_null:
  CBP_Parser_CleanupStates(&parser->states);
  CBP_Parser_CleanupCurrentTxnStates(&parser->current_txn_states);
  free(parser);
  return NULL;
}

int CBP_CleanupParser(CBP_Parser *parser) {
  if (parser == NULL) {
    return HX_OK;
  }
  if (parser->title != NULL) {
    free(parser->title);
  }
  if (parser->accounts != NULL) {
    CBP_HashMap_Cleanup(parser->accounts);
  }
  if (parser->journal_entries != NULL) {
    CBP_Array_Cleanup(parser->journal_entries);
  }
  if (parser->error_list != NULL) {
    CBP_Array_Cleanup(parser->error_list);
  }
  CBP_Parser_CleanupStates(&parser->states);
  CBP_Parser_CleanupCurrentTxnStates(&parser->current_txn_states);
  free(parser);
  return HX_OK;
}

int CBP_CleanupSubParser(CBP_Parser *parser) {
  if (parser == NULL) {
    return HX_OK;
  }
  CBP_Parser_CleanupStates(&parser->states);
  CBP_Parser_CleanupCurrentTxnStates(&parser->current_txn_states);
  free(parser);
  return HX_OK;
}