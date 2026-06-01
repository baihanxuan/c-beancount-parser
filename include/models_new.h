// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#ifndef HX_CBP_MODELS_H
#define HX_CBP_MODELS_H

#include "array_new.h"
#include "date.h"
#include "hashmap_new.h"

typedef struct {
  char *name;
  CBP_Date opened_at;   // CBP_Date
  CBP_Array currencies; // Array<Cstring>
  CBP_HashMap balance;
} CBP_BeancountAccount;

typedef struct {
  char *account;
  long long amount;
  char *currency;
  long long quoted_amount;
  char *quoted_currency;
} CBP_BeancountPosting;

typedef struct {
  char *payee;
  char *remarks;
  CBP_Date posted_at; // CBP_Date
  CBP_Array postings; // CBP_BeancountPosting[]
} CBP_BeancountJournalEntry;

int CBP_Models_CopyPosting(CBP_BeancountPosting *dst_posting,
                           const CBP_BeancountPosting *src_posting);
int CBP_Models_CopyAccount(CBP_BeancountAccount *dst_account,
                           const CBP_BeancountAccount *src_account);
int CBP_Models_CopyEntry(CBP_BeancountJournalEntry *dst_entry,
                         const CBP_BeancountJournalEntry *src_entry);
int CBP_Models_CleanupPosting(CBP_BeancountPosting *posting);
int CBP_Models_CleanupAccount(CBP_BeancountAccount *account);
int CBP_Models_CleanupEntry(CBP_BeancountJournalEntry *entry);

#endif