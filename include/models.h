// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#ifndef HX_CBP_MODELS_H
#define HX_CBP_MODELS_H

#define HX_CBP_PRECISION 2

#include "array.h"
#include "object.h"
typedef struct {
  char *name;
  CBP_Object *opened_at; // CBP_Date
  char **currencies;
} CBP_BeancountAccount;

typedef struct {
  const CBP_BeancountAccount *account;
  long long amount;
  char *currency;
  long long quoted_amount;
  char *quoted_currency;
} CBP_BeancountPosting;

typedef struct {
  char *payee;
  char *remarks;
  CBP_Object *posted_at; // CBP_Date
  CBP_Array *postings;   // CBP_BeancountPosting[]
} CBP_BeancountJournalEntry;

int CBP_Models_DestroyBeancountJournalEntry(void *);
int CBP_Models_CopyBeancountJournalEntry(void *dst, const void *src);

CBP_Object *CBP_Models_GetBeancountPosting(const CBP_BeancountAccount *account,
                                           const char *posting_amount_string,
                                           const char *currency);
CBP_Object *CBP_Models_GetQuotedBeancountPosting(
    const CBP_BeancountAccount *account, const char *original_amount_string,
    const char *original_currency, const char *quoted_amount_string,
    const char *quoted_currency);
CBP_Object *
CBP_Models_GetBeancountPostingByRawValue(const CBP_BeancountAccount *account,
                                         long long raw_amount,
                                         const char *currency);
int CBP_Models_CopyBeancountPosting(void *dst, const void *src);
int CBP_Models_DestroyBeancountPosting(void *);

CBP_Object *CBP_Models_GetBeancountAccount(const char *name,
                                           const CBP_Object *opened_at);

int CBP_Models_CopyBeancountAccount(void *dst, const void *src);
int CBP_Models_DestroyBeancountAccount(void *);

#endif