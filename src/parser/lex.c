// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#include "arithmetics.h"
#include "array.h"
#include "date.h"
#include "errors.h"
#include "hashmap.h"
#include "macros.h"
#include "models.h"
#include "object.h"
#include "parser.h"
#include <dirent.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int cbp_ProcessPadDirective(CBP_Parser *parser, CBP_Array tokens) {
  if (parser == NULL || tokens.contents == NULL) {
    goto return_err;
  }
  if (tokens.size != 2) {
    CBP_Parser_RegisterUnmatchedArgumentSizeError(parser, "pad_directive", "2",
                                                  tokens.size);
    goto return_err;
  }

  CBP_Object *reconciled_account_string = NULL, *contra_account_string = NULL;

  CBP_Array_TypeCheckedSafeGetValue(reconciled_account_string, parser, STRING,
                                    "pad_directive::reconciled_account",
                                    &tokens, 0, return_err);
  CBP_Array_TypeCheckedSafeGetValue(contra_account_string, parser, STRING,
                                    "pad_directive::contra_account", &tokens, 1,
                                    return_err);

  CBP_Object *reconciled_account_obj = CBP_HashMap_RetrieveByKey(
      parser->beancount_account_map, reconciled_account_string);
  CBP_Object *contra_account_obj = CBP_HashMap_RetrieveByKey(
      parser->beancount_account_map, contra_account_string);

  if (CBP_IsNullObj(reconciled_account_obj) ||
      CBP_IsNullObj(contra_account_obj)) {
    CBP_Parser_RegisterInvalidTypeError(parser,
                                        "pad_directive::reconciled_account | "
                                        "pad_directive::contra_account",
                                        STRING, NULLOBJ);
    goto return_err;
  }

  return CBP_HashMap_Upsert(parser->states.pad_balance_map,
                            reconciled_account_obj, contra_account_obj);
return_err:
  return HX_ERR;
}

int cbp_ProcessBalanceDirective(CBP_Parser *parser, CBP_Array tokens) {
  if (parser == NULL || tokens.contents == NULL) {
    goto return_err;
  }
  if (tokens.size != 3) {
    CBP_Parser_RegisterUnmatchedArgumentSizeError(parser, "balance_directive",
                                                  "3", tokens.size);
    goto return_err;
  }

  const CBP_Object *reconciled_account_string = NULL, *target_balance = NULL,
                   *target_currency = NULL;

  CBP_Object *target_account_balance_map = NULL;
  CBP_Object *scoped_target_account_balance_map = NULL;
  CBP_Object *reconciled_account_obj_const_view = NULL;

  CBP_Object *delta_obj = NULL;

  CBP_Object *posting_obj = NULL;

  CBP_Array_TypeCheckedSafeGetValue(reconciled_account_string, parser, STRING,
                                    "balance_directive::reconciled_account",
                                    &tokens, 0, return_err);
  CBP_Array_TypeCheckedSafeGetValue(target_balance, parser, STRING,
                                    "balance_directive::target_balance",
                                    &tokens, 1, return_err);
  CBP_Array_TypeCheckedSafeGetValue(target_currency, parser, STRING,
                                    "balance_directive::target_balance",
                                    &tokens, 2, return_err);

  const CBP_Object *reconciled_account_obj = CBP_HashMap_RetrieveByKey(
      parser->beancount_account_map, reconciled_account_string);
  const CBP_Object *contra_account_obj = CBP_HashMap_RetrieveByKey(
      parser->states.pad_balance_map, reconciled_account_obj);

  if (CBP_IsNullObj(reconciled_account_obj)) {
    CBP_Parser_RegisterError(parser, "Reconciled Account is empty");
    goto return_err;
  }

  if (cbp_HasNonnullCurrentTxnStates(parser)) {
    CBP_Parser_RegisterError(
        parser,
        "In processing balance directive: has nonnull current txn states!");
    goto return_err;
  }

  parser->current_txn_states.postings = CBP_GetArray();
  parser->current_txn_states.balance_map = CBP_GetHashMap();
  parser->current_txn_states.payee = CBP_GetString("");
  parser->current_txn_states.remarks = CBP_GetString("Pad-balance");

  reconciled_account_obj_const_view = CBP_GetConstPtrView(
      reconciled_account_obj->data, sizeof(CBP_BeancountAccount));

  CBP_Object *reconciled_account_balance_map =
      CBP_HashMap_RetrieveByKey(parser->beancount_accounts_balance_map,
                                reconciled_account_obj_const_view);

  if (reconciled_account_balance_map == NULL) {
    scoped_target_account_balance_map = CBP_GetCustom(
        NULL, sizeof(CBP_HashMap), CBP_HashMap_ObjectCompatibleDestroy,
        CBP_HashMap_ObjectCompatibleCopy, NULL);
    CBP_HashMap_Initialize(scoped_target_account_balance_map->data);
    target_account_balance_map = scoped_target_account_balance_map;
  } else {
    target_account_balance_map = reconciled_account_balance_map;
  }

  CBP_HashMap *account_balance_hashmap = target_account_balance_map->data;

  CBP_Object *reconciled_account_current_balance =
      CBP_HashMap_RetrieveByKey(account_balance_hashmap, target_currency);

      /*
  double current_balance_f64 = 0.00;

  if (reconciled_account_current_balance == NULL) {
    current_balance_f64 = 0.00;
  } else {
    current_balance_f64 =
        (double)(*(i64 *)reconciled_account_current_balance->data) /
        pow(10.00, HX_CBP_PRECISION);
  }

  char buffer[BUFFER_SIZE + 1];

  double delta = atof(target_balance->data) - current_balance_f64;
  */

  i64 current_balance_i64 = 0;

  if (reconciled_account_current_balance == NULL) {
    current_balance_i64 = 0;
  } else {
    current_balance_i64 = *(i64 *)reconciled_account_current_balance->data;
  }

  i64 delta = CBP_Arith_GetFixedPointRepr(target_balance->data) - current_balance_i64;

  if (delta == 0) {
    goto free_and_return_ok;
  } else {
    if (CBP_IsNullObj(contra_account_obj)) {
      goto return_err;
    }
  }


  CBP_PtrSafeAssign(CBP_Object, delta_obj,
                    CBP_GetInt(delta),
                    return_err);
  CBP_HashMap_Upsert(parser->current_txn_states.balance_map, target_currency,
                     delta_obj);

  CBP_PtrSafeAssign(CBP_Object, posting_obj,
                    CBP_Models_GetBeancountPostingByRawValue(reconciled_account_obj->data,
                                                   delta,
                                                   target_currency->data),
                    return_err);

  CBP_Array_Push(parser->current_txn_states.postings, posting_obj);
  parser->current_txn_states.catch_all_account =
      (CBP_Object *)contra_account_obj;

  cbp_SyncTransaction(parser);
free_and_return_ok:
  CBP_GracefulDestroy(CBP_DestroyObject, delta_obj);
  CBP_GracefulDestroy(CBP_DestroyObject, posting_obj);

  CBP_GracefulDestroy(CBP_DestroyObject, reconciled_account_obj_const_view);
  cbp_DestroyCurrentTxnStates(parser);

  return HX_OK;
return_err:
  CBP_GracefulDestroy(CBP_DestroyObject, delta_obj);
  CBP_GracefulDestroy(CBP_DestroyObject, posting_obj);

  CBP_GracefulDestroy(CBP_DestroyObject, reconciled_account_obj_const_view);
  cbp_DestroyCurrentTxnStates(parser);
  return HX_ERR;
}

int cbp_ProcessTransactionDefLine(CBP_Parser *parser, CBP_Array tokens) {
  if (parser == NULL || tokens.contents == NULL) {
    goto return_err;
  }
  if (tokens.size < 1) {
    CBP_Parser_RegisterUnmatchedArgumentSizeError(
        parser, "transaction definition line", ">1", tokens.size);
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
    return cbp_ProcessPadDirective(
        parser, CBP_GetArrayRef(tokens->contents + 2, tokens->size - 2));
  }

  if (CBP_EqCstring(obj, "balance")) {
    return cbp_ProcessBalanceDirective(
        parser, CBP_GetArrayRef(tokens->contents + 2, tokens->size - 2));
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
