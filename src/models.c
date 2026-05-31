// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#include "models.h"
#include "array.h"
#include "macros.h"
#include "object.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <string.h>

int CBP_Models_DestroyBeancountJournalEntry(void *data) {
  if (data == NULL) {
    return HX_OK;
  }
  CBP_BeancountJournalEntry *entry = data;
  CBP_GracefulDestroy(free, entry->payee);
  CBP_GracefulDestroy(free, entry->remarks);
  CBP_GracefulDestroy(CBP_DestroyObject, entry->posted_at);
  CBP_GracefulDestroy(CBP_Array_Destroy, entry->postings);
  free(entry);
  return HX_OK;
}

int CBP_Models_CopyBeancountJournalEntry(void *dst, const void *src) {
  if (dst == NULL || src == NULL) {
    goto return_err;
  }
  const CBP_BeancountJournalEntry *entry = src;
  CBP_BeancountJournalEntry *target = dst;

  CBP_PtrSafeAllocate(target->payee, strlen(entry->payee) + 1, sizeof(char),
                      return_err);
  strcpy(target->payee, entry->payee);

  CBP_PtrSafeAllocate(target->remarks, strlen(entry->remarks) + 1, sizeof(char),
                      return_err);
  strcpy(target->remarks, entry->remarks);

  CBP_PtrSafeAssign(CBP_Object, target->posted_at,
                    CBP_CopyFromObject(entry->posted_at), return_err);

  CBP_PtrSafeAssign(CBP_Array, target->postings,
                    CBP_CopyFromArray(entry->postings), return_err);
  return HX_OK;
return_err:
  CBP_GracefulDestroy(free, target->payee);
  CBP_GracefulDestroy(free, target->remarks);
  CBP_GracefulDestroy(CBP_DestroyObject, target->posted_at);
  CBP_GracefulDestroy(CBP_Array_Destroy, target->postings);
  return HX_ERR;
}

int CBP_Models_InitBeancountPosting(CBP_BeancountPosting *data,
                                    const CBP_BeancountAccount *account,
                                    const char *posting_amount_string,
                                    const char *currency) {
  data->account = account;
  if (posting_amount_string != NULL) {
    data->amount =
        (i64)(atof(posting_amount_string) * pow(10.00, HX_CBP_PRECISION));
  } else {
    data->amount = 0;
  }
  data->quoted_amount = 0;
  data->quoted_currency = NULL;
  CBP_PtrSafeAssign(char, data->currency, strdup(currency),
                    free_and_return_err);
  return HX_OK;
free_and_return_err:
  CBP_GracefulDestroy(free, data->currency);
  return HX_ERR;
}

CBP_Object *CBP_Models_GetBeancountPosting(const CBP_BeancountAccount *account,
                                           const char *posting_amount_string,
                                           const char *currency) {
  CBP_Object *posting = NULL;
  CBP_PtrSafeAssign(CBP_Object, posting,
                    CBP_GetCustom(NULL, sizeof(CBP_BeancountPosting),
                                  CBP_Models_DestroyBeancountPosting,
                                  CBP_Models_CopyBeancountPosting, NULL),
                    return_null);
  CBP_BeancountPosting *posting_data = posting->data;
  if (CBP_Models_InitBeancountPosting(
          posting_data, account, posting_amount_string, currency) != HX_OK) {
    goto free_and_return_null;
  }
  return posting;
free_and_return_null:
  CBP_GracefulDestroy(CBP_DestroyObject, posting);
return_null:
  return NULL;
}

int CBP_Models_InitQuotedBeancountPosting(CBP_BeancountPosting *data,
                                          const CBP_BeancountAccount *account,
                                          const char *original_amount_string,
                                          const char *original_currency,
                                          const char *quoted_amount_string,
                                          const char *quoted_currency) {
  return HX_OK;
}

// This is minimum change. [TODO] Rewrite everything with CBP_Arith_*.

CBP_Object *CBP_Models_GetQuotedBeancountPostingByRawValue(
    const CBP_BeancountAccount *account, const char *original_amount_string,
    const char *original_currency, i64 quoted_amount_fixed_point_repr,
    const char *quoted_currency) {
  CBP_Object *posting = NULL;
  CBP_PtrSafeAssign(CBP_Object, posting,
                    CBP_Models_GetBeancountPosting(
                        account, original_amount_string, original_currency),
                    return_null);
  CBP_BeancountPosting *posting_data = posting->data;
  posting_data->quoted_amount = quoted_amount_fixed_point_repr;
  CBP_PtrSafeAssign(char, posting_data->quoted_currency,
                    strdup(quoted_currency), return_null);
  return posting;
return_null:
  CBP_GracefulDestroy(CBP_DestroyObject, posting);
  return NULL;
}

CBP_Object *CBP_Models_GetQuotedBeancountPosting(
    const CBP_BeancountAccount *account, const char *original_amount_string,
    const char *original_currency, const char *quoted_amount_string,
    const char *quoted_currency) {
  CBP_Object *posting = NULL;
  CBP_PtrSafeAssign(CBP_Object, posting,
                    CBP_Models_GetBeancountPosting(
                        account, original_amount_string, original_currency),
                    return_null);

  CBP_BeancountPosting *posting_data = posting->data;
  posting_data->quoted_amount =
      (i64)(atof(quoted_amount_string) * pow(10.00, HX_CBP_PRECISION) *
            ((original_amount_string[0] == '-') ? -1 : 1));
  CBP_PtrSafeAssign(char, posting_data->quoted_currency,
                    strdup(quoted_currency), free_and_return_null);
  return posting;
free_and_return_null:
  CBP_GracefulDestroy(CBP_DestroyObject, posting);
return_null:
  return NULL;
}

CBP_Object *
CBP_Models_GetBeancountPostingByRawValue(const CBP_BeancountAccount *account,
                                         long long raw_amount,
                                         const char *currency) {
  CBP_Object *posting = NULL;
  CBP_PtrSafeAssign(CBP_Object, posting,
                    CBP_Models_GetBeancountPosting(account, NULL, currency),
                    return_null);
  CBP_BeancountPosting *posting_data = posting->data;
  posting_data->amount = raw_amount;
  return posting;
return_null:
  return NULL;
}

int CBP_Models_DestroyBeancountPosting(void *data) {
  if (data == NULL) {
    return HX_OK;
  }
  CBP_BeancountPosting *posting = data;
  CBP_GracefulDestroy(free, posting->currency);
  CBP_GracefulDestroy(free, posting->quoted_currency);
  CBP_GracefulDestroy(free, posting);
  return HX_OK;
}

int CBP_Models_CopyBeancountPosting(void *dst, const void *src) {
  if (dst == NULL || src == NULL) {
    goto return_err;
  }
  CBP_BeancountPosting *posting = dst;
  const CBP_BeancountPosting *original = src;

  posting->account = original->account;
  posting->amount = original->amount;
  posting->quoted_amount = original->quoted_amount;
  CBP_PtrSafeAssign(char, posting->currency, strdup(original->currency),
                    free_and_return_err);
  if (original->quoted_currency != NULL) {
    CBP_PtrSafeAssign(char, posting->quoted_currency,
                      strdup(original->quoted_currency), free_and_return_err);
  }
  return HX_OK;
free_and_return_err:
  CBP_GracefulDestroy(free, posting->currency);
  CBP_GracefulDestroy(free, posting->quoted_currency);
return_err:
  return HX_ERR;
}

CBP_Object *CBP_Models_GetBeancountAccount(const char *name,
                                           const CBP_Object *opened_at) {
  CBP_Object *account = NULL;
  CBP_PtrSafeAssign(CBP_Object, account,
                    CBP_GetCustom(NULL, sizeof(CBP_BeancountAccount),
                                  CBP_Models_DestroyBeancountAccount,
                                  CBP_Models_CopyBeancountAccount, NULL),
                    return_null);

  CBP_BeancountAccount *account_data = account->data;
  account_data->currencies = NULL; // [TODO] to be implemented
  CBP_PtrSafeAssign(char, account_data->name, strdup(name),
                    free_and_return_null);
  CBP_PtrSafeAssign(CBP_Object, account_data->opened_at,
                    CBP_CopyFromObject(opened_at), free_and_return_null);
  return account;
free_and_return_null:
  CBP_GracefulDestroy(free, account_data->name);
  CBP_GracefulDestroy(CBP_DestroyObject, account_data->opened_at);
  CBP_GracefulDestroy(free, account);
return_null:
  return NULL;
}

int CBP_Models_CopyBeancountAccount(void *dst, const void *src) {
  if (dst == NULL || src == NULL) {
    goto return_err;
  }
  const CBP_BeancountAccount *src_data = src;
  CBP_BeancountAccount *dst_data = dst;

  dst_data->currencies = NULL; // [TODO] to be implemented
  CBP_PtrSafeAssign(char, dst_data->name, strdup(src_data->name),
                    free_and_return_err);
  CBP_PtrSafeAssign(CBP_Object, dst_data->opened_at,
                    CBP_CopyFromObject(src_data->opened_at),
                    free_and_return_err);
  return HX_OK;
free_and_return_err:
  CBP_GracefulDestroy(free, dst_data->name);
  CBP_GracefulDestroy(CBP_DestroyObject, dst_data->opened_at);
return_err:
  return HX_ERR;
}

int CBP_Models_DestroyBeancountAccount(void *data) {
  if (data == NULL) {
    return HX_OK;
  }
  CBP_BeancountAccount *account_data = data;
  account_data->currencies = NULL; // [TODO] to be implemented
  CBP_GracefulDestroy(free, account_data->name);
  CBP_GracefulDestroy(CBP_DestroyObject, account_data->opened_at);
  return HX_OK;
}

CBP_Object *CBP_Models_GetBeancountJournalEntry() {
  CBP_Object *entry_obj = NULL;
  CBP_PtrSafeAssign(CBP_Object, entry_obj,
                    CBP_GetCustom(NULL, sizeof(CBP_BeancountJournalEntry),
                                  CBP_Models_DestroyBeancountJournalEntry,
                                  CBP_Models_CopyBeancountJournalEntry, NULL),
                    return_null);
  CBP_BeancountJournalEntry *entry = entry_obj->data;
  entry->payee = NULL;
  entry->posted_at = NULL;
  entry->postings = CBP_GetArray();
  entry->remarks = NULL;
  return entry_obj;
return_null:
  return NULL;
}