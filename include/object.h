// Copyright (C) 2026 Bai Hanxuan
// This file is part of c-beancount-parser, licensed under GNU GPLv2 Only.
// See the COPYING file for details.

#ifndef HX_CBP_OBJECT_H
#define HX_CBP_OBJECT_H

enum CBP_ObjectType { NULLOBJ, CONSTVIEW, INT, UINT, CHAR, STRING, CUSTOM };

typedef struct CBP_Object {
  enum CBP_ObjectType type;
  void *data;
  unsigned long long element_size;
  // stores a reference to an external, unified
  // destroy function of the custom object
  int (*destroy_function)(void *data);
  // stores a reference to an external, unified
  // copy function of the custom object
  int (*copy_function)(void *dst, const void *src);
  // stores a reference to an external, unified compare function of the custom
  // object
  int (*compare_function)(const struct CBP_Object *this_data, const struct CBP_Object *that_data);
} CBP_Object;

typedef int (*CBP_DestroyFn)(void *);
typedef int (*CBP_CopyFn)(void *, const void *);
typedef int (*CBP_CompareFn)(const CBP_Object *, const CBP_Object *);

// Get an int64 object
CBP_Object *CBP_GetInt(long long value);

// Get an uint64 object
CBP_Object *CBP_GetUint(unsigned long long value);

// Get a char object
CBP_Object *CBP_GetChar(char value);

// Get a string object
CBP_Object *CBP_GetString(const char *value);

// Get a string object from start_ptr and length
CBP_Object *CBP_GetStringFromRange(const char *start_ptr,
                                   unsigned long long length);

// Get a custom object
CBP_Object *CBP_GetCustom(const void *value, unsigned long long size,
                          const CBP_DestroyFn destroy_function,
                          const CBP_CopyFn copy_function,
                          const CBP_CompareFn compare_function);

// Copy from an existing object
CBP_Object *CBP_CopyFromObject(const CBP_Object *src);

// Get a const view (= pointer) to an existing object
CBP_Object *CBP_GetConstView(const CBP_Object *src);

CBP_Object *CBP_GetConstPtrView(void *ptr, unsigned long long size);

// Destroy an object.
// frees its data and itself.
//
// This function will free the CBP_Object passed in.
int CBP_DestroyObject(CBP_Object *obj);

// Nullify an object.
// Resets the object type to NULLOBJ and frees its data.
//
// This function will *NOT* free the CBP_Object passed in.
int CBP_Nullify(CBP_Object *obj);

// Destroy a custom object.
int CBP_DestroyCustomObject(CBP_Object *obj, CBP_DestroyFn destroy_function);

// Initialize a null object
int CBP_InitNullObj(CBP_Object *obj);

// compare byte-by-byte and check if the two objects are equal.
// Returns 1 if they are equal, and 0 otherwise.
int CBP_Eq(const CBP_Object *lhs, const CBP_Object *rhs);

// compare intrinsic CBP Objects with intrinsic C values.
int CBP_EqCint(const CBP_Object *lhs, const long long value);
int CBP_EqCuint(const CBP_Object *lhs, const unsigned long long value);
int CBP_EqCchar(const CBP_Object *lhs, const char value);
int CBP_EqCstring(const CBP_Object *lhs, const char *rhs);

// Check if a CBP_Object is null. (NULLOBJ / == NULL / data == NULL)
//
// Returns 1 if the object pointer is a nullptr, 2 if the object is a nullobj, 3
// if the object has a null data, 0 otherwise.
int CBP_IsNullObj(const CBP_Object *obj);

#endif