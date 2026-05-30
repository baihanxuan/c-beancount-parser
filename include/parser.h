// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#ifndef HX_CBP_PARSER_H
#define HX_CBP_PARSER_H

#include "array.h"
#include "hashmap.h"

struct CBP_Parser {
  CBP_Array *operating_currencies;
  CBP_Object *title;
  CBP_Array *beancount_accounts;
  CBP_HashMap *beancount_account_map;
  CBP_HashMap
      *beancount_accounts_balance_map; // HashMap<const CBP_BeancountAccount*,
                                       // HashMap<string, int64>>
  CBP_Array *beancount_journal_entries;
  struct {
    CBP_Array *files_to_include;
    CBP_Object *nearest_error;
    CBP_Object *working_dir;
    unsigned long long line;
    int is_in_transaction;
    CBP_Object *posted_at;
    char *current_working_file_name;
    CBP_HashMap* pad_balance_map;
  } states;
  struct {
    CBP_Object *payee;
    CBP_Object *remarks;
    CBP_Array *postings;
    CBP_HashMap *balance_map;
    CBP_Object *catch_all_account;
  } current_txn_states;
};

typedef struct CBP_Parser CBP_Parser;

// Get a beancount parser
CBP_Parser *CBP_GetParser(const char *working_dir,
                          const char *working_file_name);

// Get a subparser
CBP_Parser *CBP_GetSubParser(const char *working_dir,
                             const char *working_file_name,
                             CBP_Parser *main_parser);

// Destroy a beancount parser
extern int CBP_Parser_Destroy(CBP_Parser *parser);

// Feed a line and parse
int CBP_Parser_Feed(CBP_Parser *parser, const char *line);

// Recursively parse a file
int CBP_Parser_Parse(CBP_Parser *parser, const char *file);

extern CBP_Parser *CBP_Parser_ParseStart(const char *file);

// Parse beancount entries into fields, keeping double-quoted string literals
// intact
CBP_Array *CBP_Parser_ParseFields(const char *line);

/* Start CBP_Parser internal functions */

int cbp_EnsureBalanceToZero(CBP_Parser *parser);

int cbp_MaintainBalance(CBP_Parser *parser);

int cbp_ParseIndentedOne(CBP_Parser *parser, CBP_Object *account);

CBP_Object *cbp_ParseIndentedQuoted(CBP_Parser *, CBP_Array *, CBP_Object *,
                                    long long *, CBP_Object *, CBP_Object *,
                                    CBP_Object *);

int cbp_ParseIndentedHelper(CBP_Parser *parser, CBP_Array *tokens,
                            CBP_Object *account);

int cbp_ProcessTransactionDefLine(CBP_Parser *parser, CBP_Array tokens);

int cbp_ProcessOpenDirective(CBP_Parser *parser, CBP_Array tokens);

int cbp_ProcessNormalDirective(CBP_Parser *parser, CBP_Array *tokens);

int cbp_HasNonnullCurrentTxnStates(CBP_Parser *parser);

int cbp_Parser_DestroyState(CBP_Parser *parser);

int cbp_SyncTransaction(CBP_Parser *parser);

void cbp_DestroyCurrentTxnStates(CBP_Parser *parser);
/* End CBP_Parser internal functions */

#endif