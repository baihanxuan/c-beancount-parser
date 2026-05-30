// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#include "parser.h"
#include "arithmetics.h"
#include "array.h"
#include "date.h"
#include "errors.h"
#include "files.h"
#include "hashmap.h"
#include "macros.h"
#include "models.h"
#include "object.h"
#include <dirent.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int cbp_IsLeapYear(u64 year) {
  if (year < 1582) {
    return year % 4 == 0;
  }
  if (year % 400 == 0) {
    return true;
  }
  if (year % 4 == 0 && year % 100 != 0) {
    return true;
  }
  return false;
}

int cbp_IsValidDate(const char *token) {
  u64 y, m, d;
  if (sscanf(token, "%llu-%llu-%llu", &y, &m, &d) != 3) {
    return false;
  }

  // Be considerate for users under the reign of Pope Gregory XIII.
  // Calendar system switch can be a pain in the ass.
  if (y == 1582 && m == 10 && d >= 5 && d <= 14) {
    return false;
  }

  if (m > 12 || m == 0) {
    return false;
  }

  if (d == 0) {
    return false;
  }

  const int days_in_month[] = {0,  31, 28, 31, 30, 31, 30,
                               31, 31, 30, 31, 30, 31};

  int max_days = days_in_month[m];

  if (m == 2 && cbp_IsLeapYear(y)) {
    max_days = 29;
  }

  return d <= max_days;
}

int cbp_ParseOption(CBP_Parser *parser, CBP_Array *tokens) {
  if (parser == NULL) {
    return HX_ERR;
  }

  if (tokens->size < 3) {
    return HX_ERR;
  }
  // We only care about `title` and `operating_currency`.

  CBP_Object *option_name_obj = CBP_Array_GetValue(tokens, 1);
  if (option_name_obj == NULL || option_name_obj->type != STRING) {
    return HX_ERR;
  }

  CBP_Object *option_value_obj = CBP_Array_GetValue(tokens, 2);
  if (option_name_obj == NULL || option_value_obj->type != STRING) {
    return HX_ERR;
  }

  if (strcmp((const char *)option_name_obj->data, "title") == 0) {
    if (parser->title != NULL) {
      CBP_DestroyObject(parser->title);
      parser->title = NULL;
    }
    parser->title = CBP_CopyFromObject(option_value_obj);
    if (parser->title == NULL) {
      return HX_ERR;
    }
    return HX_OK;
  }

  if (strcmp((const char *)option_name_obj->data, "operating_currency") == 0) {
    if (parser->operating_currencies == NULL) {
      return HX_ERR;
    }
    CBP_Array_Push(parser->operating_currencies, option_value_obj);
  }

  return HX_OK;
}

int cbp_PreprocessInclude(CBP_Parser *parser, CBP_Array *tokens) {
  if (parser == NULL || tokens->size < 2 ||
      parser->states.files_to_include == NULL) {
    goto return_err;
  }
  CBP_Object *obj = CBP_Array_GetValue(tokens, 1);
  if (obj->type != STRING) {
    goto return_err;
  }

  // handle wildcard matching via asterisk
  u64 path_len, file_name_len;
  CBP_SeparatePathAndName(NULL, 256, NULL, 256, &path_len, &file_name_len,
                          obj->data);
  char *path = malloc(path_len);
  if (path == NULL) {
    goto return_err;
  }
  char *file_name = malloc(file_name_len);
  if (file_name == NULL) {
    goto return_err;
  }

  CBP_SeparatePathAndName(path, 256, file_name, 256, NULL, NULL, obj->data);

  if (parser->states.working_dir == NULL ||
      parser->states.working_dir->data == NULL) {
    goto return_err;
  }

  const char *asterisk_pos = strrchr(file_name, '*');
  int ignore_ext = false;
  char *full_working_dir =
      CBP_ConcatPaths(2, (char *)parser->states.working_dir->data, path);
  if (full_working_dir == NULL) {
    goto return_err;
  }
  if (asterisk_pos == NULL) {
    char *tmp_name = CBP_ConcatPaths(2, full_working_dir, file_name);
    if (tmp_name == NULL) {
      goto return_err;
    }
    CBP_Array_Push(parser->states.files_to_include, CBP_GetString(tmp_name));
    free(tmp_name);
  } else {
    const char *last_dot_pos = strrchr(file_name, '.');
    if (last_dot_pos == NULL) {
      // Then include everything
      ignore_ext = true;
    }
    struct dirent *entry;
    DIR *dp = opendir(full_working_dir);

    if (dp == NULL) {
      goto return_err;
    }

    while ((entry = readdir(dp)) != NULL) {
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
        continue;
      }
      const char *dir_entry_dot = strrchr(entry->d_name, '.');
      if (ignore_ext || (dir_entry_dot != NULL &&
                         strcmp(dir_entry_dot + 1, last_dot_pos + 1) == 0)) {
        char *tmp_name = CBP_ConcatPaths(2, full_working_dir, entry->d_name);
        if (tmp_name == NULL) {
          goto return_err;
        }
        CBP_Array_Push(parser->states.files_to_include,
                       CBP_GetString(tmp_name));
        free(tmp_name);
      }
    }

    closedir(dp);
  }

  free(full_working_dir);
  free(path);
  free(file_name);

  return HX_OK;
return_err:
  if (full_working_dir != NULL) {
    free(full_working_dir);
  }
  if (path != NULL) {
    free(path);
  }
  if (file_name != NULL) {
    free(file_name);
  }
  return HX_ERR;
}

void cbp_DestroyCurrentTxnStates(CBP_Parser *parser) {
  CBP_GracefulDestroy(CBP_DestroyObject, parser->current_txn_states.payee);
  CBP_GracefulDestroy(CBP_DestroyObject, parser->current_txn_states.remarks);
  CBP_GracefulDestroy(CBP_HashMap_Destroy,
                      parser->current_txn_states.balance_map);
  CBP_GracefulDestroy(CBP_Array_Destroy, parser->current_txn_states.postings);
  parser->current_txn_states.catch_all_account = NULL;
}

int cbp_HasNonnullCurrentTxnStates(CBP_Parser *parser) {
  return parser->current_txn_states.balance_map != NULL &&
         parser->current_txn_states.payee != NULL &&
         parser->current_txn_states.postings != NULL &&
         parser->current_txn_states.remarks != NULL;
}

// Checks whether the transaction balances to zero, and if so, syncs.
//
// This will free everything in parser->current_txn_states.
int cbp_SyncTransaction(CBP_Parser *parser) {
  if (parser == NULL || !cbp_HasNonnullCurrentTxnStates(parser)) {
    goto return_err;
  }

  // Checks whether the transaction balances to zero.
  for (u64 i = 0; i < parser->current_txn_states.balance_map->keys->size; i++) {
    CBP_Object *current_currency = NULL;

    CBP_Array_TypeCheckedSafeGetValue(
        current_currency, parser, STRING, "currency",
        parser->current_txn_states.balance_map->keys, i, return_err);

    CBP_Object *balance = CBP_HashMap_RetrieveByKey(
        parser->current_txn_states.balance_map, current_currency);
    if (balance == NULL) {
      CBP_Parser_RegisterError(
          parser, "Got a null object as the balance for currency %s",
          (char *)current_currency->data);
      goto return_err;
    }

    if (parser->current_txn_states.catch_all_account == NULL) {
      if (!CBP_EqCint(balance, 0)) {
        CBP_Parser_RegisterError(
            parser,
            "Postings in a transaction do not balance to zero: %.2lf %s",
            CBP_Arith_RawRepresentationToDouble(*(i64 *)balance->data),
            (char *)current_currency->data);
        goto return_err;
      }
    } else {
      // Add transactions respectively.
      CBP_Object *posting_object = NULL;
      CBP_PtrSafeAssign(CBP_Object, posting_object,
                        CBP_Models_GetBeancountPostingByRawValue(
                            parser->current_txn_states.catch_all_account->data,
                            -(*(i64 *)balance->data),
                            (char *)current_currency->data),
                        return_err);
      if (CBP_Array_Push(parser->current_txn_states.postings, posting_object) !=
          HX_OK) {
        CBP_GracefulDestroy(CBP_DestroyObject, posting_object);
        goto return_err;
      }
      CBP_GracefulDestroy(CBP_DestroyObject, posting_object);
    }
  }

  for (u64 i = 0; i < parser->current_txn_states.postings->size; i++) {
    CBP_Object *posting_object = NULL;
    CBP_Array_TypeCheckedSafeGetValue(
        posting_object, parser, CUSTOM, "posting_object",
        parser->current_txn_states.postings, i, return_err);
    CBP_BeancountPosting *posting_data = posting_object->data;
    CBP_Object *account_view = NULL;
    CBP_PtrSafeAssign(CBP_Object, account_view,
                      CBP_GetConstPtrView((void *)posting_data->account,
                                          sizeof(CBP_BeancountAccount));
                      , return_err);

    CBP_Object *account_balance_map = CBP_HashMap_RetrieveByKey(
        parser->beancount_accounts_balance_map, account_view);

    CBP_Object *target_account_balance_map = NULL;
    CBP_Object *scoped_target_account_balance_map = NULL;

    if (account_balance_map == NULL) {
      scoped_target_account_balance_map = CBP_GetCustom(
          NULL, sizeof(CBP_HashMap), CBP_HashMap_ObjectCompatibleDestroy,
          CBP_HashMap_ObjectCompatibleCopy, NULL);
      CBP_HashMap_Initialize(scoped_target_account_balance_map->data);
      target_account_balance_map = scoped_target_account_balance_map;
    } else {
      target_account_balance_map = CBP_CopyFromObject(account_balance_map);
    }

    CBP_HashMap *account_balance_hashmap = target_account_balance_map->data;

    CBP_Object *current_currency = NULL;

    char *currency_to_update = posting_data->quoted_currency
                                   ? posting_data->quoted_currency
                                   : posting_data->currency;

    CBP_PtrSafeAssign(CBP_Object, current_currency,
                      CBP_GetString(currency_to_update), return_err);

    CBP_Object *existing_balance =
        CBP_HashMap_RetrieveByKey(account_balance_hashmap, current_currency);

    CBP_Object *balance_to_update = NULL;

    CBP_PtrSafeAssign(CBP_Object, balance_to_update, CBP_GetInt(0), return_err);

    i64 delta = posting_data->quoted_currency ? posting_data->quoted_amount
                                              : posting_data->amount;

    if (CBP_IsNullObj(existing_balance)) {
      *(i64 *)balance_to_update->data = delta;
    } else {
      *(i64 *)balance_to_update->data = *(i64 *)existing_balance->data + delta;
    }

    CBP_HashMap_Upsert(account_balance_hashmap, current_currency,
                       balance_to_update);

    CBP_HashMap_Upsert(parser->beancount_accounts_balance_map, account_view,
                       target_account_balance_map);

    CBP_GracefulDestroy(CBP_DestroyObject, balance_to_update);

    CBP_GracefulDestroy(CBP_DestroyObject, current_currency);

    CBP_GracefulDestroy(CBP_DestroyObject, account_view);

    CBP_GracefulDestroy(CBP_DestroyObject, target_account_balance_map);
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

// Now, raw CBP_Array objects are considered Array_Refs.
// No need to think about when to free.
int cbp_ProcessTransactionDefLine(CBP_Parser *parser, CBP_Array tokens) {
  if (parser == NULL || tokens.contents == NULL) {
    goto return_err;
  }
  if (tokens.size < 1 || tokens.size > 2) {
    CBP_Parser_RegisterUnmatchedArgumentSizeError(
        parser, "transaction definition line", "1 or 2", tokens.size);
    goto return_err;
  }
  for (u64 i = 0; i < tokens.size; i++) {
    if (CBP_IsNullObj(&tokens.contents[i])) {
      goto return_null;
    } else if (tokens.contents[i].type != STRING) {
      CBP_Parser_RegisterInvalidTypeError(
          parser, "transaction definition line :: payee or remarks", STRING,
          tokens.contents[i].type);
      goto return_err;
    }
  }
  const char *payee = NULL;
  const char *remarks = NULL;
  if (tokens.size == 1) {
    remarks = (const char *)tokens.contents[0].data;
  } else {
    payee = (const char *)tokens.contents[0].data;
    remarks = (const char *)tokens.contents[1].data;
  }

  if (remarks == NULL) {
    goto return_null;
  }

  CBP_PtrSafeAssign(CBP_Object, parser->current_txn_states.payee,
                    CBP_GetString(payee == NULL ? "" : payee), return_err);
  CBP_PtrSafeAssign(CBP_Object, parser->current_txn_states.remarks,
                    CBP_GetString(remarks), return_err);
  CBP_PtrSafeAssign(CBP_HashMap, parser->current_txn_states.balance_map,
                    CBP_GetHashMap(), return_err);
  CBP_PtrSafeAssign(CBP_Array, parser->current_txn_states.postings,
                    CBP_GetArray(), return_err);

  parser->states.is_in_transaction = true;
  return HX_OK;
return_null:
  CBP_Parser_RegisterInvalidTypeError(
      parser, "transaction definition line :: payee or remarks", STRING,
      NULLOBJ);
return_err:
  return HX_ERR;
}

int cbp_ProcessOpenDirective(CBP_Parser *parser, CBP_Array tokens) {
  if (tokens.size < 1) {
    CBP_Parser_RegisterUnmatchedArgumentSizeError(parser, "open directive",
                                                  ">= 1", tokens.size);
    goto return_err;
  }
  CBP_Object *account_name = NULL;
  CBP_Array_TypeCheckedSafeGetValue(account_name, parser, STRING,
                                    "open_directive::account_name", &tokens, 0,
                                    return_err);
  CBP_Object *object_to_push = NULL;
  CBP_Object *account_view_to_push = NULL;
  CBP_PtrSafeAssign(
      CBP_Object, object_to_push,
      CBP_Models_GetBeancountAccount((const char *)account_name->data,
                                     parser->states.posted_at),
      return_err);
  CBP_Array_Push(parser->beancount_accounts, object_to_push);

  CBP_PtrSafeAssign(
      CBP_Object, account_view_to_push,
      CBP_GetConstView(parser->beancount_accounts
                           ->contents[parser->beancount_accounts->size - 1]
                           .data),
      return_err);
  CBP_HashMap_Upsert(parser->beancount_account_map, account_name,
                     account_view_to_push);

  CBP_GracefulDestroy(CBP_DestroyObject, account_view_to_push);
  CBP_GracefulDestroy(CBP_DestroyObject, object_to_push);
  return HX_OK;
return_err:
  return HX_ERR;
}

int cbp_ProcessNormalDirective(CBP_Parser *parser, CBP_Array *tokens) {
  if (tokens->size < 3) {
    CBP_Parser_RegisterUnmatchedArgumentSizeError(
        parser, "open/pad/balance directive or transaction definition line",
        ">= 3", tokens->size);
    return HX_ERR;
  }

  CBP_Object *date = CBP_Array_GetValue(tokens, 0);
  CBP_Object *obj = CBP_Array_GetValue(tokens, 1);

  if (CBP_IsNullObj(obj)) {
    CBP_Parser_RegisterError(parser, "Got null object as directive type");
    return HX_ERR;
  }

  if (obj->type != STRING) {
    CBP_Parser_RegisterInvalidTypeError(parser, "directive type", STRING,
                                        obj->type);
    return HX_ERR;
  }

  CBP_PtrSafeAssign(CBP_Object, parser->states.posted_at, CBP_GetDate(date),
                    return_err);

  if (CBP_EqCstring(obj, "*")) {
    // Transaction.
    // +2 and -2: omitted date and '*'
    return cbp_ProcessTransactionDefLine(
        parser, CBP_GetArrayRef(tokens->contents + 2, tokens->size - 2));
  }

  if (CBP_EqCstring(obj, "open")) {
    // open directive.
    return cbp_ProcessOpenDirective(
        parser, CBP_GetArrayRef(tokens->contents + 2, tokens->size - 2));
  }

  if (CBP_EqCstring(obj, "pad")) {
    // pad directive.
    return HX_OK;
  }

  if (CBP_EqCstring(obj, "balance")) {
    // balance directive.
    return HX_OK;
  }

  if (CBP_EqCstring(obj, "custom")) {
    // custom directive - dummy.
    return HX_OK;
  }

  CBP_Parser_RegisterUnknownArgumentError(parser, "directive type",
                                          "* | open | pad | balance | custom",
                                          (const char *)obj->data);
return_err:
  return HX_ERR;
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

int CBP_Parser_DestroyState(CBP_Parser *parser) {
  CBP_GracefulDestroy(CBP_Array_Destroy, parser->states.files_to_include);
  CBP_GracefulDestroy(CBP_DestroyObject, parser->states.nearest_error);
  CBP_GracefulDestroy(CBP_DestroyObject, parser->states.working_dir);
  CBP_GracefulDestroy(CBP_DestroyObject, parser->states.posted_at);
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
  CBP_Parser_DestroyState(parser);

  free(parser);

  return HX_OK;
}

int CBP_Parser_ParseUnindentedLine(CBP_Parser *parser, CBP_Array *tokens) {
  if (tokens == NULL || tokens->contents == NULL || tokens->size == 0) {
    return HX_ERR;
  }

  // determine directive type
  CBP_Object *obj = CBP_Array_GetValue(tokens, 0);
  if (obj->type == STRING) {
    if (strcmp((const char *)obj->data, "option") == 0) {
      if (cbp_ParseOption(parser, tokens) != HX_OK) {
        // for debugging only. to be changed in production build.
        perror("Option parsing error");
      }
      return HX_OK;
    }
    if (strcmp((const char *)obj->data, "include") == 0) {
      if (cbp_PreprocessInclude(parser, tokens) != HX_OK) {
        // for debugging only. to be changed in production build.
        perror("Include preprocessing error");
      }
      return HX_OK;
    }
    if (cbp_IsValidDate((const char *)obj->data)) {
      if (cbp_ProcessNormalDirective(parser, tokens) != HX_OK) {
        // cbp_ProcessNormalDirective has already registered a detailed error.
        goto return_err;
      }
      return HX_OK;
    }
  } else {
    goto return_err;
  }
return_err:
  return HX_ERR;
}

// Parse indented line
int CBP_Parser_ParseIndentedLine(CBP_Parser *parser, CBP_Array *tokens) {
  char buffer[BUFFER_SIZE + 1];
  if (parser == NULL || tokens == NULL || tokens->size == 0 ||
      !cbp_HasNonnullCurrentTxnStates(parser)) {
    CBP_Parser_RegisterError(
        parser,
        "Parser is not ready. It is null, the tokens "
        "are null, or the current_txn_states are, in part or full, null.");
    goto return_err;
  }
  if (!parser->states.is_in_transaction) {
    CBP_Parser_RegisterError(parser, "Indented line without a transaction.");
    goto return_err;
  }
  CBP_Object *account_name = NULL;

  CBP_Array_TypeCheckedSafeGetValue(account_name, parser, STRING,
                                    "account_name", tokens, 0, return_err);

  CBP_Object *account =
      CBP_HashMap_RetrieveByKey(parser->beancount_account_map, account_name);

  if (CBP_IsNullObj(account)) {
    snprintf(buffer, BUFFER_SIZE, "Account \"%s\" does not exist.",
             (char *)account_name->data);
    CBP_Parser_RegisterError(parser, buffer);
    goto return_err;
  }

  // If this is the only token, mark as the catch-all account
  // If not, add the amount to the respective balance map to track whether they
  // come to zero in the end.

  if (tokens->size != 1 && tokens->size != 3 && tokens->size != 6) {
    CBP_Parser_RegisterUnmatchedArgumentSizeError(
        parser, "Indented Posting Line", "1 | 3 | 6", tokens->size);
    goto return_err;
  }

  if (tokens->size == 1) {
    if (!CBP_IsNullObj(parser->current_txn_states.catch_all_account)) {
      const char *account_name =
          ((CBP_BeancountAccount *)
               parser->current_txn_states.catch_all_account->data)
              ->name;
      if (account_name == NULL) {
        snprintf(buffer, BUFFER_SIZE,
                 "Unknown error. Check CBP_Parser_ParseIndentedLine for more "
                 "details.");
      } else {
        snprintf(buffer, BUFFER_SIZE,
                 "A catch-all account for this transaction has already been "
                 "specified: \"%llx\".",
                 (u64)account_name);
      }
      CBP_Parser_RegisterError(parser, buffer);
      goto return_err;
    }
    parser->current_txn_states.catch_all_account = account;
    return HX_OK;
  }

  CBP_Object *currency_to_push = NULL;
  i64 value_to_push = 0;
  CBP_Object *posting_amount = NULL;
  CBP_Object *currency = NULL;
  CBP_Object *posting_object_to_push = NULL;

  CBP_Array_TypeCheckedSafeGetValue(posting_amount, parser, STRING,
                                    "posting::amount", tokens, 1, return_err);
  CBP_Array_TypeCheckedSafeGetValue(currency, parser, STRING,
                                    "posting::currency", tokens, 2, return_err);
  value_to_push =
      (i64)(atof(posting_amount->data) * pow(10.00, HX_CBP_PRECISION));
  currency_to_push = currency;
  if (tokens->size == 3) {
    posting_object_to_push = CBP_Models_GetBeancountPosting(
        (const CBP_BeancountAccount *)(account->data),
        (const char *)posting_amount->data, (const char *)currency->data);
  } else if (tokens->size == 6) {
    CBP_Object *at = NULL;
    CBP_Object *target_amount = NULL;
    CBP_Object *target_currency = NULL;

    CBP_Array_TypeCheckedSafeGetValue(at, parser, STRING, "posting::quote::at",
                                      tokens, 3, return_err);
    CBP_Array_TypeCheckedSafeGetValue(target_amount, parser, STRING,
                                      "posting::quote::quoted_amount", tokens,
                                      4, return_err);
    CBP_Array_TypeCheckedSafeGetValue(target_currency, parser, STRING,
                                      "posting::quote::quoted_currency", tokens,
                                      5, return_err);

    if (CBP_EqCstring(at, "@")) {
      double per_unit_fx_rate = atof(target_amount->data);
      double posting_amount_double = atof(posting_amount->data);
      value_to_push = (i64)(per_unit_fx_rate * pow(10.00, HX_CBP_PRECISION) *
                            posting_amount_double);
      char at_buffer[BUFFER_SIZE + 1];
      snprintf(at_buffer, BUFFER_SIZE, "%.2lf",
               (double)value_to_push / pow(10.00, HX_CBP_PRECISION));
      posting_object_to_push = CBP_Models_GetQuotedBeancountPosting(
          (const CBP_BeancountAccount *)account->data,
          (const char *)posting_amount->data, (const char *)currency->data,
          at_buffer, (const char *)target_currency->data);
    } else if (CBP_EqCstring(at, "@@")) {
      value_to_push =
          (i64)(atof(target_amount->data) * pow(10.00, HX_CBP_PRECISION));
      posting_object_to_push = CBP_Models_GetQuotedBeancountPosting(
          (const CBP_BeancountAccount *)(account->data),
          (const char *)posting_amount->data, (const char *)currency->data,
          (const char *)target_amount->data,
          (const char *)target_currency->data);
    } else {
      CBP_Parser_RegisterUnknownArgumentError(parser, "posting::quote::at",
                                              "@ | @@", at->data);
      goto return_err;
    }
    currency_to_push = target_currency;
  }

  CBP_Object *target_value_obj = NULL;

  CBP_Object *existing_balance = CBP_HashMap_RetrieveByKey(
      parser->current_txn_states.balance_map, currency_to_push);

  if (existing_balance != NULL) {
    value_to_push += *(i64 *)existing_balance->data;
  }

  CBP_PtrSafeAssign(CBP_Object, target_value_obj, CBP_GetInt(value_to_push),
                    return_err);
  CBP_Array_Push(parser->current_txn_states.postings, posting_object_to_push);
  CBP_HashMap_Upsert(parser->current_txn_states.balance_map, currency_to_push,
                     target_value_obj);
  CBP_GracefulDestroy(CBP_DestroyObject, target_value_obj);
  CBP_GracefulDestroy(CBP_DestroyObject, posting_object_to_push);

  return HX_OK;
return_err:
  return HX_ERR;
}

CBP_Array *CBP_Parser_ParseFields(const char *line) {
  if (line == NULL) {
    return NULL;
  }

  CBP_Array *arr = CBP_GetArray();
  int is_in_doubleQuotes = 0;
  const char *start = NULL;

  for (u64 i = 0; line[i] != '\0'; i++) {
    char c = line[i];

    if (c == ';') {
      if (!is_in_doubleQuotes) {
        break;
      }
    }

    if (c == '"') {
      if (!is_in_doubleQuotes) {
        is_in_doubleQuotes = 1;
        start = &line[i + 1];
      } else {
        u64 length = &line[i] - start;
        CBP_Array_Push(arr, CBP_GetStringFromRange(start, length));
        start = NULL;
        is_in_doubleQuotes = 0;
      }
      continue;
    }

    if ((c == ' ' || c == '\t') && !is_in_doubleQuotes) {
      if (start != NULL) {
        u64 length = &line[i] - start;
        CBP_Array_Push(arr, CBP_GetStringFromRange(start, length));
        start = NULL;
      }
      continue;
    }

    if (start == NULL) {
      start = &line[i];
    }
  }

  if (start != NULL) {
    if (is_in_doubleQuotes) {
      CBP_Array_Destroy(arr);
      return NULL;
    }

    size_t length = strlen(start);
    CBP_Array_Push(arr, CBP_GetStringFromRange(start, length));
  }

  return arr;
}

int CBP_Parser_Feed(CBP_Parser *parser, const char *line) {
  if (line == NULL) {
    return HX_ERR;
  }
  CBP_Array *tokens = CBP_Parser_ParseFields(line);
  if (tokens == NULL) {
    return HX_ERR;
  }
  if (line[0] == ' ' || line[0] == '\t') {
    // unimplemented
    CBP_Parser_ParseIndentedLine(parser, tokens);
  } else {
    if (parser->states.is_in_transaction) {
      cbp_SyncTransaction(parser);
    }
    CBP_Parser_ParseUnindentedLine(parser, tokens);
  }
  CBP_Array_Destroy(tokens);
  return HX_OK;
}

int CBP_Parser_Parse(CBP_Parser *parser, const char *file) {
  if (file == NULL) {
    goto return_err;
  }
  FILE *fd = fopen(file, "r");
  if (fd == NULL) {
    char *error_string =
        malloc(strlen("Cannot open file: ") + strlen(file) + 1);
    sprintf(error_string, "Cannot open file: %s", file);
    CBP_Parser_RegisterError(parser, error_string);
    free(error_string);
    goto return_err;
  }

  char buffer[BUFFER_SIZE + 1];
  while (fgets(buffer, BUFFER_SIZE, fd) != NULL) {
    i64 j = strlen(buffer) - 1;
    if (j <= 0) {
      continue;
    }
    while (j >= 0 && buffer[j] == '\n') {
      buffer[j] = '\0';
      j--;
    }
    parser->states.line++;
    if (CBP_Parser_Feed(parser, buffer) != HX_OK) {
      goto return_err;
    }
  }

  if (parser->states.is_in_transaction) {
    cbp_SyncTransaction(parser);
    parser->states.is_in_transaction = false;
  }

  fclose(fd);

  if (parser->states.files_to_include == NULL ||
      parser->states.files_to_include->contents == NULL) {
    goto return_ok;
  }
  for (u64 i = 0; i < parser->states.files_to_include->size; i++) {
    const CBP_Object *include_file_name =
        CBP_Array_GetValue(parser->states.files_to_include, i);
    if (include_file_name->type != STRING) {
      CBP_Parser_RegisterInvalidTypeError(parser, "include_file_name", STRING,
                                          include_file_name->type);
      goto return_err;
    }
    char *sub_working_dir = CBP_GetFilePath(include_file_name->data);
    if (sub_working_dir == NULL) {
      goto return_err;
    }
    CBP_Parser *sub_parser =
        CBP_GetSubParser(sub_working_dir, include_file_name->data, parser);
    int sub_result = CBP_Parser_Parse(sub_parser, include_file_name->data);
    free(sub_working_dir);
    CBP_Parser_DestroyState(sub_parser);
    free(sub_parser);
    if (sub_result != HX_OK) {
      goto return_err;
    }
  }

return_ok:
  return HX_OK;

return_err:
  return HX_ERR;
}

CBP_Parser *CBP_Parser_ParseStart(const char *file) {
  char *working_dir = CBP_GetFilePath(file);
  if (working_dir == NULL) {
    goto return_null;
  }
  CBP_Parser *parser = CBP_GetParser(working_dir, file);
  free(working_dir);
  if (parser == NULL) {
    goto return_null;
  }
  if (CBP_Parser_Parse(parser, file) != HX_OK) {
    goto return_err;
  }

  return parser;
return_null:
  return NULL;
return_err:
  if (parser->states.nearest_error != NULL &&
      parser->states.nearest_error->data != NULL &&
      parser->states.nearest_error->type == STRING) {
    fprintf(stderr, "Panicked: %s\n",
            (char *)parser->states.nearest_error->data);
  } else {
    fprintf(stderr, "Panicked: Unknown error!\n");
  }
  if (parser != NULL) {
    CBP_Parser_Destroy(parser);
  }
  return NULL;
}