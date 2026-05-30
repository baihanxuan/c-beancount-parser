// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#include "array.h"
#include "errors.h"
#include "hashmap.h"
#include "macros.h"
#include "models.h"
#include "object.h"
#include "parser.h"
#include <dirent.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

int cbp_Parser_DestroyState(CBP_Parser *parser) {
  CBP_GracefulDestroy(CBP_Array_Destroy, parser->states.files_to_include);
  CBP_GracefulDestroy(CBP_DestroyObject, parser->states.nearest_error);
  CBP_GracefulDestroy(CBP_DestroyObject, parser->states.working_dir);
  CBP_GracefulDestroy(CBP_DestroyObject, parser->states.posted_at);
  CBP_GracefulDestroy(CBP_HashMap_Destroy, parser->states.pad_balance_map);
  CBP_GracefulDestroy(free, parser->states.current_working_file_name);
  CBP_GracefulDestroy(CBP_DestroyObject, parser->current_txn_states.payee);
  CBP_GracefulDestroy(CBP_DestroyObject, parser->current_txn_states.remarks);
  CBP_GracefulDestroy(CBP_HashMap_Destroy,
                      parser->current_txn_states.balance_map);
  // CBP_GracefulDestroy(CBP_Array_Destroy,
  //                     parser->current_txn_states.currencies_involved);
  CBP_GracefulDestroy(CBP_Array_Destroy, parser->current_txn_states.postings);

  // We don't free the catch_all_account pointer as it is designed to point to
  // the beancount account stored elsewhere. We just reset it to NULL.
  parser->current_txn_states.catch_all_account = NULL;
  return HX_OK;
}

int CBP_Parser_Destroy(CBP_Parser *parser) {
  if (parser == NULL) {
    return HX_OK;
  }
  CBP_GracefulDestroy(CBP_DestroyObject, parser->title);
  CBP_GracefulDestroy(CBP_HashMap_Destroy,
                      parser->beancount_accounts_balance_map);
  CBP_GracefulDestroy(CBP_HashMap_Destroy, parser->beancount_account_map);
  CBP_GracefulDestroy(CBP_Array_Destroy, parser->beancount_accounts);
  CBP_GracefulDestroy(CBP_Array_Destroy, parser->beancount_journal_entries);
  CBP_GracefulDestroy(CBP_Array_Destroy, parser->operating_currencies);
  cbp_Parser_DestroyState(parser);

  free(parser);

  return HX_OK;
}

int cbp_HasNonnullCurrentTxnStates(CBP_Parser *parser) {
  return parser->current_txn_states.balance_map != NULL &&
         parser->current_txn_states.payee != NULL &&
         parser->current_txn_states.postings != NULL &&
         parser->current_txn_states.remarks != NULL;
}

void cbp_DestroyCurrentTxnStates(CBP_Parser *parser) {
  CBP_GracefulDestroy(CBP_DestroyObject, parser->current_txn_states.payee);
  CBP_GracefulDestroy(CBP_DestroyObject, parser->current_txn_states.remarks);
  CBP_GracefulDestroy(CBP_HashMap_Destroy,
                      parser->current_txn_states.balance_map);
  CBP_GracefulDestroy(CBP_Array_Destroy, parser->current_txn_states.postings);
  parser->current_txn_states.catch_all_account = NULL;
}

int cbp_Parser_InitParserStates(CBP_Parser *parser, const char *working_dir,
                                const char *current_working_file_name) {

  if (current_working_file_name != NULL) {
    CBP_PtrSafeAssign(char, parser->states.current_working_file_name,
                      strdup(current_working_file_name), return_err);
  } else {
    CBP_Parser_RegisterError(parser, "Invalid file name for parsing.");
    goto return_err;
  }
  parser->states.files_to_include = CBP_GetArray();
  parser->states.nearest_error = NULL;
  parser->states.line = 0;
  parser->states.posted_at = NULL;
  if (working_dir != NULL) {
    parser->states.working_dir = CBP_GetString(working_dir);
  } else {
    parser->states.working_dir = CBP_GetString(".");
  }
  parser->states.pad_balance_map =
      CBP_GetHashMap(); // HashMap<const CBP_BeancountAccount*, const
                        // CBP_BeancountAccount*>
  parser->states.is_in_transaction = false;
  parser->current_txn_states.payee = NULL;
  parser->current_txn_states.remarks = NULL;
  parser->current_txn_states.postings = NULL;
  parser->current_txn_states.balance_map = NULL;
  // parser->current_txn_states.currencies_involved = NULL;
  parser->current_txn_states.catch_all_account = NULL;
  return HX_OK;
return_err:
  return HX_ERR;
}

CBP_Parser *CBP_GetParser(const char *working_dir,
                          const char *working_file_name) {
  CBP_Parser *parser = calloc(1, sizeof(CBP_Parser));
  parser->title = NULL;
  parser->beancount_accounts = CBP_GetArray();
  parser->beancount_account_map = CBP_GetHashMap();
  parser->beancount_accounts_balance_map = CBP_GetHashMap();
  parser->beancount_journal_entries = CBP_GetArray();
  parser->operating_currencies = CBP_GetArray();
  cbp_Parser_InitParserStates(parser, working_dir, working_file_name);
  return parser;
}

CBP_Parser *CBP_GetSubParser(const char *working_dir,
                             const char *working_file_name,
                             CBP_Parser *main_parser) {
  CBP_Parser *parser = malloc(sizeof(CBP_Parser));
  parser->title = main_parser->title;
  parser->beancount_accounts = main_parser->beancount_accounts;
  parser->beancount_account_map = main_parser->beancount_account_map;
  parser->beancount_accounts_balance_map =
      main_parser->beancount_accounts_balance_map;
  parser->beancount_journal_entries = main_parser->beancount_journal_entries;
  parser->operating_currencies = main_parser->operating_currencies;

  cbp_Parser_InitParserStates(parser, working_dir, working_file_name);
  return parser;
}

// Checks whether the transaction balances to zero, and if so, syncs.
//
// This will free everything in parser->current_txn_states.
int cbp_SyncTransaction(CBP_Parser *parser) {
  if (parser == NULL || !cbp_HasNonnullCurrentTxnStates(parser)) {
    goto return_err;
  }

  // Checks whether the transaction balances to zero.
  if (cbp_EnsureBalanceToZero(parser) != HX_OK) {
    goto return_err;
  }

  if (cbp_MaintainBalance(parser) != HX_OK) {
    goto return_err;
  }

  CBP_Object *journal_entry_object = NULL;
  CBP_PtrSafeAssign(CBP_Object, journal_entry_object,
                    CBP_GetCustom(NULL, sizeof(CBP_BeancountJournalEntry),
                                  CBP_Models_DestroyBeancountJournalEntry,
                                  CBP_Models_CopyBeancountJournalEntry, NULL),
                    return_err);

  CBP_BeancountJournalEntry *entry = journal_entry_object->data;
  CBP_PtrSafeAssign(char, entry->payee,
                    strdup(parser->current_txn_states.payee->data), return_err);
  CBP_PtrSafeAssign(char, entry->remarks,
                    strdup(parser->current_txn_states.remarks->data),
                    return_err);
  CBP_PtrSafeAssign(CBP_Object, entry->posted_at,
                    CBP_CopyFromObject(parser->states.posted_at), return_err);

  CBP_PtrSafeAssign(CBP_Array, entry->postings,
                    CBP_CopyFromArray(parser->current_txn_states.postings),
                    return_err);

  CBP_Array_Push(parser->beancount_journal_entries, journal_entry_object);

  CBP_GracefulDestroy(CBP_DestroyObject, parser->states.posted_at);
  cbp_DestroyCurrentTxnStates(parser);
  CBP_GracefulDestroy(CBP_DestroyObject, journal_entry_object);
  parser->states.is_in_transaction = false;
  return HX_OK;
return_err:
  return HX_ERR;
}