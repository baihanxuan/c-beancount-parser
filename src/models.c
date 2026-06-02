// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#include "models.h"
#include "macros.h"
#include <stdlib.h>
#include <string.h>

int CBP_Models_CopyPosting(CBP_BeancountPosting *dst_posting,
                           const CBP_BeancountPosting *src_posting) {
  if (dst_posting == NULL) {
    return HX_ERR;
  }
  dst_posting->account = strdup(src_posting->account);
  dst_posting->amount = src_posting->amount;
  dst_posting->currency = strdup(src_posting->currency);
  dst_posting->quoted_amount = src_posting->quoted_amount;
  if (src_posting->quoted_currency != NULL) {
    dst_posting->quoted_currency = strdup(src_posting->quoted_currency);
    if (dst_posting->quoted_currency == NULL) {
      goto cleanup_and_return_err;
    }
  } else {
    dst_posting->quoted_currency = NULL;
  }

  if (dst_posting->account == NULL || dst_posting->currency == NULL) {
    goto cleanup_and_return_err;
  }
  return HX_OK;
cleanup_and_return_err:
  if (dst_posting->account != NULL) {
    free(dst_posting->account);
  }
  if (dst_posting->currency != NULL) {
    free(dst_posting->currency);
  }
  if (dst_posting->quoted_currency != NULL) {
    free(dst_posting->quoted_currency);
  }
  return HX_ERR;
}

int CBP_Models_CopyAccount(CBP_BeancountAccount *dst_account,
                           const CBP_BeancountAccount *src_account) {
  dst_account->opened_at.day = src_account->opened_at.day;
  dst_account->opened_at.month = src_account->opened_at.month;
  dst_account->opened_at.year = src_account->opened_at.year;
  CBP_HashMap_Copy(&dst_account->balance, &src_account->balance);
  CBP_Array_InitByCopy(&dst_account->currencies, &src_account->currencies);
  dst_account->name = strdup(src_account->name);
  return HX_OK;
}

int CBP_Models_CopyEntry(CBP_BeancountJournalEntry *dst_entry,
                         const CBP_BeancountJournalEntry *src_entry) {
  dst_entry->payee = strdup(src_entry->payee);
  dst_entry->posted_at.day = src_entry->posted_at.day;
  dst_entry->posted_at.month = src_entry->posted_at.month;
  dst_entry->posted_at.year = src_entry->posted_at.year;
  CBP_Array_InitByCopy(&dst_entry->postings, &src_entry->postings);
  dst_entry->remarks = strdup(src_entry->remarks);
  return HX_OK;
}

int CBP_Models_CleanupPosting(CBP_BeancountPosting *posting) {
  if (posting == NULL) {
    return HX_OK;
  }
  if (posting->account != NULL) {
    free(posting->account);
  }
  if (posting->currency != NULL) {
    free(posting->currency);
  }
  if (posting->quoted_currency != NULL) {
    free(posting->quoted_currency);
  }
  free(posting);
  return HX_OK;
}

int CBP_Models_CleanupAccount(CBP_BeancountAccount *account) {
  if (account->name != NULL) {
    free(account->name);
  }
  CBP_HashMap_Cleanup(&account->balance);
  CBP_Array_Cleanup(&account->currencies);
  free(account);
  return HX_OK;
}

int CBP_Models_CleanupEntry(CBP_BeancountJournalEntry *entry) {
  if (entry->payee != NULL) {
    free(entry->payee);
  }
  if (entry->remarks != NULL) {
    free(entry->remarks);
  }
  CBP_Array_Cleanup(&entry->postings);
  free(entry);
  return HX_OK;
}