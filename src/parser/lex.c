// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#include "arithmetics.h"
#include "array.h"
#include "date.h"
#include "errors.h"
#include "files.h"
#include "hashmap.h"
#include "macros.h"
#include "models.h"
#include "parser.h"
#include <dirent.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// begin internal functions def

int cbp_Tokenize(CBP_Array *, const char *);
int cbp_ParseOption(CBP_Parser *, const CBP_Array *);
int cbp_ProcessWildcardInclude(CBP_Parser *, const char *, const char *);
int cbp_ProcessInclude(CBP_Parser *, const CBP_Array *);
int cbp_ProcessNormalDirective(CBP_Parser *, const CBP_Array *);
int cbp_SyncTransaction(CBP_Parser *);
int cbp_ParseIndentedLine(CBP_Parser *, const CBP_Array *);
int cbp_ParseUnindentedLine(CBP_Parser *, const CBP_Array *);
int cbp_FeedLineToParser(CBP_Parser *, const char *);
int cbp_StartParsing(CBP_Parser *, const char *);
int cbp_ParseRest(CBP_Parser *);
int cbp_ParseIndentedOne(CBP_Parser *, const char *);
int cbp_ParseIndentedLineHelper(CBP_Parser *, const CBP_Array *);
int cbp_EnsureBalanceToZero(CBP_Parser *);
int cbp_MaintainBalance(CBP_Parser *);
int cbp_ProcessTxnDefLine(CBP_Parser *, const CBP_Array *);
int cbp_ProcessOpenDirective(CBP_Parser *, const CBP_Array *);
int cbp_ProcessPadDirective(CBP_Parser *, const CBP_Array *);
int cbp_ProcessBalanceDirective(CBP_Parser *, const CBP_Array *);

// end internal functions def

CBP_Parser *CBP_Parse(const char *full_path) {
  CBP_Parser *parser = NULL;
  char *working_dir = NULL;
  uint64_t path_len, file_name_len;

  CBP_SeparatePathAndName(NULL, HX_CBP_FILES_NAME_CAP, NULL,
                          HX_CBP_FILES_NAME_CAP, &path_len, &file_name_len,
                          full_path);

  working_dir = calloc(path_len + 1, sizeof(char));
  if (working_dir == NULL) {
    goto cleanup_and_return_null;
  }
  CBP_SeparatePathAndName(working_dir, HX_CBP_FILES_NAME_CAP, NULL,
                          HX_CBP_FILES_NAME_CAP, NULL, NULL, full_path);

  parser = CBP_GetParser(working_dir, full_path);
  if (parser == NULL) {
    goto cleanup_and_return_null;
  }

  cbp_StartParsing(parser, full_path);

  return parser;

cleanup_and_return_null:
  if (working_dir != NULL) {
    free(working_dir);
  }
  if (parser != NULL) {
    free(parser);
  }
  return NULL;
}

// begin internal functions impl

int cbp_Tokenize(CBP_Array *arr, const char *line) {
  if (line == NULL) {
    return HX_ERR;
  }

  int is_in_doubleQuotes = 0;
  const char *start = NULL;

  for (uint64_t i = 0; line[i] != '\0'; i++) {
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
        uint64_t length = &line[i] - start;
        CBP_Array_PushStringFromRange(arr, start, length);
        start = NULL;
        is_in_doubleQuotes = 0;
      }
      continue;
    }

    if ((c == ' ' || c == '\t') && !is_in_doubleQuotes) {
      if (start != NULL) {
        uint64_t length = &line[i] - start;
        CBP_Array_PushStringFromRange(arr, start, length);
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
      return HX_ERR;
    }

    size_t length = strlen(start);
    CBP_Array_PushStringFromRange(arr, start, length);
  }

  return HX_OK;
}

int cbp_ParseOption(CBP_Parser *parser, const CBP_Array *tokens) {
  if (parser == NULL || tokens == NULL || tokens->size < 3) {
    return HX_ERR;
  }
  if (strcmp(tokens->contents[1].data.s, "title") == 0) {
    if (parser->title != NULL) {
      free(parser->title);
      parser->title = NULL;
    }
    parser->title = strdup(tokens->contents[2].data.s);
    if (parser->title == NULL) {
      return HX_ERR;
    }
  }
  return HX_OK;
}

int cbp_ProcessWildcardInclude(CBP_Parser *parser, const char *full_working_dir,
                               const char *file_name) {
  bool ignore_extension = false;
  const char *last_dot_pos = strrchr(file_name, '.');
  if (last_dot_pos == NULL) {
    ignore_extension = true;
  }
  struct dirent *entry;
  DIR *dp = opendir(full_working_dir);
  if (dp == NULL) {
    return HX_ERR;
  }
  while ((entry = readdir(dp)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }
    const char *dir_entry_dot_pos = strrchr(entry->d_name, '.');
    if (ignore_extension ||
        (dir_entry_dot_pos != NULL &&
         strcmp(dir_entry_dot_pos + 1, last_dot_pos + 1) == 0)) {
      char *tmp_file_name = CBP_ConcatPaths(2, full_working_dir, entry->d_name);
      if (tmp_file_name == NULL) {
        closedir(dp);
        return HX_ERR;
      }
      CBP_Array_PushString(&parser->states.files_to_include, tmp_file_name);
      free(tmp_file_name);
    }
  }
  closedir(dp);
  return HX_OK;
}

int cbp_ProcessInclude(CBP_Parser *parser, const CBP_Array *tokens) {
  if (parser == NULL || tokens == NULL || tokens->size < 2) {
    return HX_ERR;
  }
  uint64_t path_len, file_name_len;
  CBP_SeparatePathAndName(NULL, HX_CBP_FILES_NAME_CAP, NULL,
                          HX_CBP_FILES_NAME_CAP, &path_len, &file_name_len,
                          tokens->contents[1].data.s);
  char *path = calloc(path_len + 1, sizeof(char));
  char *file_name = calloc(file_name_len + 1, sizeof(char));
  char *full_working_dir = NULL;
  if (path == NULL || file_name == NULL) {
    goto cleanup_and_return_err;
  }
  CBP_SeparatePathAndName(path, HX_CBP_FILES_NAME_CAP, file_name,
                          HX_CBP_FILES_NAME_CAP, NULL, NULL,
                          tokens->contents[1].data.s);

  const char *asterisk_pos = strrchr(file_name, '*');
  full_working_dir = CBP_ConcatPaths(2, parser->states.working_dir, path);
  if (full_working_dir == NULL) {
    goto cleanup_and_return_err;
  }
  if (asterisk_pos == NULL) {
    char *tmp_file_name = CBP_ConcatPaths(2, full_working_dir, file_name);
    if (tmp_file_name == NULL) {
      goto cleanup_and_return_err;
    }
    CBP_Array_PushString(&parser->states.files_to_include, tmp_file_name);
    free(tmp_file_name);
  } else {
    if (cbp_ProcessWildcardInclude(parser, full_working_dir, file_name) !=
        HX_OK) {
      goto cleanup_and_return_err;
    }
  }
  return HX_OK;
cleanup_and_return_err:
  if (path != NULL) {
    free(path);
  }
  if (file_name != NULL) {
    free(file_name);
  }
  if (full_working_dir != NULL) {
    free(full_working_dir);
  }
  return HX_ERR;
}

int cbp_ProcessNormalDirective(CBP_Parser *parser, const CBP_Array *tokens) {
  if (tokens->size < 3) {
    CBP_Parser_RegisterUnmatchedArgumentSizeError(
        parser, "open/pad/balance directive or transaction definition line",
        ">= 3", tokens->size);
    return HX_ERR;
  }
  const char *directive_type = tokens->contents[1].data.s;
  if (strcmp(directive_type, "*") == 0) {
    return cbp_ProcessTxnDefLine(parser, tokens);
  }
  if (strcmp(directive_type, "open") == 0) {
    return cbp_ProcessOpenDirective(parser, tokens);
  }
  if (strcmp(directive_type, "pad") == 0) {
    return cbp_ProcessPadDirective(parser, tokens);
  }
  if (strcmp(directive_type, "balance") == 0) {
    return cbp_ProcessBalanceDirective(parser, tokens);
  }
  if (strcmp(directive_type, "custom") == 0) {
    return HX_OK;
  }
  CBP_Parser_RegisterUnknownArgumentError(parser, "directive_type",
                                          "* | open | pad | balance | custom",
                                          directive_type);
  return HX_ERR;
}

int cbp_EnsureBalanceToZero(CBP_Parser *parser) {
  if (parser == NULL) {
    return HX_ERR;
  }
  for (uint64_t i = 0; i < parser->current_txn_states.balance_map.keys.size;
       i++) {
    const char *currency =
        parser->current_txn_states.balance_map.keys.contents[i].data.s;
    int64_t balance = CBP_HashMap_RetrieveByKey(
                          &parser->current_txn_states.balance_map, currency)
                          ->data.i;
    if (parser->current_txn_states.catch_all_account == NULL) {
      if (balance != 0) {
        CBP_Parser_RegisterError(parser,
                                 "Postings in a transaction do not balance to "
                                 "zero: Current Balance = %.2lf %s",
                                 CBP_Arith_RawRepresentationToDouble(balance),
                                 currency);
        return HX_ERR;
      }
    } else {
      CBP_BeancountPosting posting_object = (CBP_BeancountPosting){
          .account = strdup(parser->current_txn_states.catch_all_account),
          .amount = -balance,
          .currency = strdup(currency),
          .quoted_amount = 0,
          .quoted_currency = NULL};
      if (posting_object.account == NULL || posting_object.currency == NULL) {
        if (posting_object.account != NULL) {
          free(posting_object.account);
        }
        if (posting_object.currency != NULL) {
          free(posting_object.currency);
        }
        return HX_ERR;
      }
      CBP_Array_PushPosting(&parser->current_txn_states.postings,
                            (void *)&posting_object);
      free(posting_object.account);
      free(posting_object.currency);
    }
  }
  return HX_OK;
}

int cbp_MaintainBalance(CBP_Parser *parser) {
  if (parser == NULL) {
    return HX_ERR;
  }
  for (uint64_t i = 0; i < parser->current_txn_states.postings.size; i++) {
    const CBP_BeancountPosting *posting =
        parser->current_txn_states.postings.contents[i].data.ptr;
    CBP_BeancountAccount *account =
        CBP_HashMap_RetrieveByKey(parser->accounts, posting->account)->data.ptr;
    CBP_Data *current_balance =
        CBP_HashMap_RetrieveByKey(&account->balance, posting->currency);
    int64_t current_balance_i64 = 0;
    if (current_balance != NULL) {
      current_balance_i64 = current_balance->data.i;
    }
    if (CBP_HashMap_Upsert(
            &account->balance, posting->currency,
            (CBP_Data){.type = INT,
                       .data = {.i = posting->amount + current_balance_i64}}) !=
        HX_OK) {
      return HX_ERR;
    }
  }
  return HX_OK;
}

int cbp_SyncTransaction(CBP_Parser *parser) {
  if (parser == NULL) {
    return HX_ERR;
  }

  if (cbp_EnsureBalanceToZero(parser) != HX_OK) {
    return HX_ERR;
  }
  if (cbp_MaintainBalance(parser) != HX_OK) {
    return HX_ERR;
  }

  CBP_BeancountJournalEntry entry = (CBP_BeancountJournalEntry){
      .payee = parser->current_txn_states.payee != NULL
                   ? strdup(parser->current_txn_states.payee)
                   : strdup(""),
      .remarks = strdup(parser->current_txn_states.remarks),
  };

  if (CBP_Array_InitByCopy(&entry.postings,
                           &parser->current_txn_states.postings) != HX_OK) {
    goto cleanup_and_return_err;
  }

  if (CBP_Array_PushEntry(parser->journal_entries, (void *)&entry) != HX_OK) {
    goto cleanup_and_return_err;
  }
  if (entry.payee != NULL) {
    free(entry.payee);
  }
  if (entry.remarks != NULL) {
    free(entry.remarks);
  }

  CBP_Parser_CleanupCurrentTxnStates(&parser->current_txn_states);

  return HX_OK;
cleanup_and_return_err:
  if (entry.payee != NULL) {
    free(entry.payee);
  }
  if (entry.remarks != NULL) {
    free(entry.remarks);
  }
  CBP_Parser_CleanupCurrentTxnStates(&parser->current_txn_states);
  return HX_ERR;
}

int cbp_ParseIndentedOne(CBP_Parser *parser, const char *account_name) {
  if (parser == NULL || account_name == NULL) {
    return HX_ERR;
  }
  if (parser->current_txn_states.catch_all_account != NULL) {
    CBP_Parser_RegisterError(parser,
                             "There can't be two catch-all accounts in a "
                             "single transaction. You already had one: %s.",
                             parser->current_txn_states.catch_all_account);
  }
  parser->current_txn_states.catch_all_account = strdup(account_name);
  if (parser->current_txn_states.catch_all_account == NULL) {
    return HX_ERR;
  }
  return HX_OK;
}

int cbp_ParseIndentedLineHelper(CBP_Parser *parser, const CBP_Array *tokens) {
  char *account = tokens->contents[0].data.s;
  char *posting_amount = tokens->contents[1].data.s;
  char *currency = tokens->contents[2].data.s;
  int64_t value_to_push = CBP_Arith_GetFixedPointRepr(posting_amount);

  CBP_BeancountPosting posting_object_to_push = (CBP_BeancountPosting){
      .account = strdup(account),
      .amount = CBP_Arith_GetFixedPointRepr(posting_amount),
      .currency = strdup(currency),
      .quoted_amount = 0,
      .quoted_currency = NULL};

  if (posting_object_to_push.account == NULL ||
      posting_object_to_push.currency == NULL) {
    goto cleanup_and_return_err;
  }

  if (tokens->size == 6) {
    int64_t number_after_at =
        CBP_Arith_GetFixedPointRepr(tokens->contents[4].data.s);
    posting_object_to_push.quoted_currency = strdup(tokens->contents[5].data.s);
    if (posting_object_to_push.quoted_currency == NULL) {
      goto cleanup_and_return_err;
    }
    if (strcmp(tokens->contents[3].data.s, "@") == 0) {
      posting_object_to_push.quoted_amount = CBP_Arith_GetQuotedFixedPointRepr(
          posting_object_to_push.amount, number_after_at);
    } else if (strcmp(tokens->contents[3].data.s, "@@") == 0) {
      posting_object_to_push.quoted_amount = number_after_at;
    } else {
      CBP_Parser_RegisterUnknownArgumentError(
          parser, "quoted_posting::at", "@ | @@", tokens->contents[3].data.s);
      goto cleanup_and_return_err;
    }
  }

  CBP_Array_PushPosting(&parser->current_txn_states.postings,
                        (void *)&posting_object_to_push);

  free(posting_object_to_push.account);
  free(posting_object_to_push.currency);
  if (posting_object_to_push.quoted_currency != NULL) {
    free(posting_object_to_push.quoted_currency);
  }

  return HX_OK;

cleanup_and_return_err:
  if (posting_object_to_push.account != NULL) {
    free(posting_object_to_push.account);
  }
  if (posting_object_to_push.currency != NULL) {
    free(posting_object_to_push.currency);
  }
  if (posting_object_to_push.quoted_currency != NULL) {
    free(posting_object_to_push.quoted_currency);
  }
  return HX_ERR;
}

int cbp_ParseIndentedLine(CBP_Parser *parser, const CBP_Array *tokens) {
  if (parser == NULL || tokens->contents == NULL || tokens->size == 0) {
    return HX_ERR;
  }
  if (!parser->states.is_in_transaction) {
    CBP_Parser_RegisterError(
        parser, "Indented line without a previous transaction definition.");
    return HX_ERR;
  }
  if (tokens->size != 1 && tokens->size != 3 && tokens->size != 6) {
    CBP_Parser_RegisterUnmatchedArgumentSizeError(
        parser, "indented_posting_line", "1 | 3 | 6", tokens->size);
    return HX_ERR;
  }

  if (tokens->size == 1) {
    return cbp_ParseIndentedOne(parser, tokens->contents[0].data.s);
  }

  return cbp_ParseIndentedLineHelper(parser, tokens);
}

int cbp_ParseUnindentedLine(CBP_Parser *parser, const CBP_Array *tokens) {
  if (parser == NULL || tokens == NULL || tokens->contents == NULL) {
    return HX_ERR;
  }

  if (tokens->size == 0) {
    return HX_OK;
  }

  // skip checking. `tokens` passed in is definitely Array<Cstring>.
  if (strcmp(tokens->contents[0].data.s, "option") == 0) {
    return cbp_ParseOption(parser, tokens);
  }

  if (strcmp(tokens->contents[0].data.s, "include") == 0) {
    return cbp_ProcessInclude(parser, tokens);
  }

  if (CBP_Date_IsValidDateString(tokens->contents[0].data.s)) {
    return cbp_ProcessNormalDirective(parser, tokens);
  }

  return HX_ERR;
}

int cbp_FeedLineToParser(CBP_Parser *parser, const char *line_buffer) {
  if (parser == NULL || line_buffer == NULL) {
    return HX_ERR;
  }
  CBP_Array tokens;
  CBP_Array_Init(&tokens);
  if (cbp_Tokenize(&tokens, line_buffer) != HX_OK) {
    CBP_Parser_RegisterError(
        parser,
        "Tokenization failed: Found a string literal that is not closed.");
    goto cleanup_and_return_err;
  }
  if (tokens.size != 0) {
    if (line_buffer[0] == ' ' || line_buffer[0] == '\t') {
      if (cbp_ParseIndentedLine(parser, &tokens) != HX_OK) {
        goto cleanup_and_return_err;
      }
    } else {
      if (parser->states.is_in_transaction) {
        if (cbp_SyncTransaction(parser) != HX_OK) {
          goto cleanup_and_return_err;
        }
        parser->states.is_in_transaction = false;
      }
      if (cbp_ParseUnindentedLine(parser, &tokens) != HX_OK) {
        goto cleanup_and_return_err;
      }
    }
  }
  CBP_Array_Cleanup(&tokens);
  return HX_OK;
cleanup_and_return_err:
  CBP_Array_Cleanup(&tokens);
  return HX_ERR;
}

int cbp_StartParsing(CBP_Parser *parser, const char *full_path) {
  if (parser == NULL || full_path == NULL) {
    return HX_ERR;
  }
  FILE *fd = fopen(full_path, "r");

  if (fd == NULL) {
    CBP_Parser_RegisterError(parser, "Cannot open file: %s", full_path);
    return HX_ERR;
  }

  char buffer[BUFFER_SIZE + 1];
  while (fgets(buffer, BUFFER_SIZE, fd) != NULL) {
    parser->states.line++;
    if (strlen(buffer) == 0 || strcmp(buffer, "\n") == 0 ||
        strcmp(buffer, "\r\n") == 0) {
      continue;
    }
    for (int64_t i = strlen(buffer) - 1; i >= 0; i--) {
      if (buffer[i] == '\n' || buffer[i] == '\r') {
        buffer[i] = '\0';
      }
    }
    if (cbp_FeedLineToParser(parser, buffer) != HX_OK) {
      goto cleanup_and_return_err;
    }
  }

  if (parser->states.is_in_transaction) {
    cbp_SyncTransaction(parser);
    parser->states.is_in_transaction = false;
  }

  fclose(fd);

  if (parser->states.files_to_include.size == 0 ||
      parser->states.files_to_include.contents == NULL) {
    return HX_OK;
  }

  return cbp_ParseRest(parser);

cleanup_and_return_err:
  fclose(fd);
  return HX_ERR;
}

int cbp_ParseRest(CBP_Parser *parser) {
  for (uint64_t i = 0; i < parser->states.files_to_include.size; i++) {
    char *current_file = parser->states.files_to_include.contents[i].data.s;
    char *sub_working_dir = CBP_GetFilePath(current_file);
    if (sub_working_dir == NULL) {
      return HX_ERR;
    }
    CBP_Parser *subparser =
        CBP_GetSubParser(sub_working_dir, current_file, parser);
    int sub_result = cbp_StartParsing(subparser, current_file);
    free(sub_working_dir);
    CBP_CleanupSubParser(subparser);
    if (sub_result != HX_OK) {
      return HX_ERR;
    }
  }
  return HX_OK;
}

int cbp_ProcessOpenDirective(CBP_Parser *parser, const CBP_Array *tokens) {
  if (parser == NULL || tokens == NULL) {
    return HX_ERR;
  }
  if (tokens->size < 3) {
    CBP_Parser_RegisterUnmatchedArgumentSizeError(parser, "open_directive",
                                                  ">= 3", tokens->size);
    return HX_ERR;
  }
  CBP_BeancountAccount *account_obj = calloc(1, sizeof(CBP_BeancountAccount));
  account_obj->name = strdup(tokens->contents[2].data.s);
  if (account_obj->name == NULL) {
    free(account_obj);
    return HX_ERR;
  }
  CBP_HashMap_Init(&account_obj->balance);
  CBP_Array_Init(&account_obj->currencies);
  if (CBP_Date_Init(&account_obj->opened_at, tokens->contents[0].data.s) !=
      HX_OK) {
    goto cleanup_and_return_err;
  }
  if (CBP_HashMap_Upsert(parser->accounts, tokens->contents[2].data.s,
                         (CBP_Data){.type = ACCOUNT,
                                    .data = {
                                        .ptr = (void *)account_obj,
                                    }}) != HX_OK) {
    goto cleanup_and_return_err;
  }
  return HX_OK;
cleanup_and_return_err:
  free(account_obj->name);
  CBP_HashMap_Cleanup(&account_obj->balance);
  CBP_Array_Cleanup(&account_obj->currencies);
  free(account_obj);
  return HX_ERR;
}

int cbp_ProcessTxnDefLine(CBP_Parser *parser, const CBP_Array *tokens) {
  if (parser == NULL || tokens == NULL) {
    return HX_ERR;
  }
  if (tokens->size < 3) {
    CBP_Parser_RegisterUnmatchedArgumentSizeError(
        parser, "transaction_definition", ">= 3", tokens->size);
    return HX_ERR;
  }
  const char *payee = NULL, *remarks = NULL;
  if (tokens->size == 3) {
    remarks = tokens->contents[2].data.s;
  } else {
    payee = tokens->contents[2].data.s;
    remarks = tokens->contents[3].data.s;
  }

  CBP_Parser_InitCurrentTxnStates(&parser->current_txn_states);
  parser->current_txn_states.remarks = strdup(remarks);
  if (parser->current_txn_states.remarks == NULL) {
    goto cleanup_and_return_err;
  }

  if (payee != NULL) {
    parser->current_txn_states.payee = strdup(payee);
    if (parser->current_txn_states.payee == NULL) {
      goto cleanup_and_return_err;
    }
  }

  parser->states.is_in_transaction = true;

  return HX_OK;
cleanup_and_return_err:
  CBP_Parser_CleanupCurrentTxnStates(&parser->current_txn_states);
  return HX_ERR;
}

int cbp_ProcessPadDirective(CBP_Parser *parser, const CBP_Array *tokens) {
  if (parser == NULL || tokens == NULL) {
    return HX_ERR;
  }
  if (tokens->size != 4) {
    CBP_Parser_RegisterUnmatchedArgumentSizeError(parser, "pad_directive", "2",
                                                  tokens->size);
    return HX_ERR;
  }
  CBP_Data contra_account_data = (CBP_Data){
      .type = STRING, .data = {.s = strdup(tokens->contents[3].data.s)}};
  if (contra_account_data.data.s == NULL) {
    return HX_ERR;
  }
  return CBP_HashMap_Upsert(&parser->states.pad_balance_map,
                            tokens->contents[2].data.s, contra_account_data);
}

int cbp_ProcessBalanceDirective(CBP_Parser *parser, const CBP_Array *tokens) {
  if (parser == NULL || tokens == NULL) {
    return HX_ERR;
  }
  if (tokens->size != 5) {
    CBP_Parser_RegisterUnmatchedArgumentSizeError(parser, "balance_directive",
                                                  "5", tokens->size);
    return HX_ERR;
  }
  const char *reconciled_account_string = tokens->contents[2].data.s,
             *target_currency = tokens->contents[4].data.s;
  int64_t target_balance =
      CBP_Arith_GetFixedPointRepr(tokens->contents[3].data.s);
  CBP_BeancountAccount *reconciled_account =
      CBP_HashMap_RetrieveByKey(parser->accounts, reconciled_account_string)
          ->data.ptr;
  CBP_Data *current_balance =
      CBP_HashMap_RetrieveByKey(&reconciled_account->balance, target_currency);
  int64_t current_balance_i64 = 0;
  if (current_balance != NULL) {
    current_balance_i64 = current_balance->data.i;
  }
  int64_t delta = target_balance - current_balance_i64;
  if (delta == 0) {
    return HX_OK;
  }

  int result = HX_OK;

  CBP_Parser_InitCurrentTxnStates(&parser->current_txn_states);

  const char *contra_account_string =
      CBP_HashMap_RetrieveByKey(&parser->states.pad_balance_map,
                                reconciled_account_string)
          ->data.s;

  CBP_BeancountPosting posting_obj = {.account =
                                          strdup(reconciled_account_string),
                                      .amount = delta,
                                      .currency = strdup(target_currency),
                                      .quoted_amount = 0,
                                      .quoted_currency = NULL};

  if (posting_obj.account == NULL || posting_obj.currency == NULL) {
    result = HX_ERR;
    goto cleanup;
  }

  CBP_Array_PushPosting(&parser->current_txn_states.postings,
                        (void *)&posting_obj);

  parser->current_txn_states.catch_all_account = strdup(contra_account_string);

  if (parser->current_txn_states.catch_all_account == NULL) {
    result = HX_ERR;
    goto cleanup;
  }

  if ((result = cbp_SyncTransaction(parser)) != HX_OK) {
    goto cleanup;
  }

cleanup:
  CBP_Parser_CleanupCurrentTxnStates(&parser->current_txn_states);
  if (posting_obj.account != NULL) {
    free(posting_obj.account);
  }
  if (posting_obj.currency != NULL) {
    free(posting_obj.currency);
  }
  return result;
}

// end internal functions impl