// Copyright (c) 2003-2006 King's College London, created by Laurence Tratt
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.


#include "Config.h"

#include <string.h>

#include "Core.h"
#include "Hash.h"
#include "Misc.h"
#include "Numbers.h"
#include "Object.h"
#include "Shortcuts.h"

#include "Builtins/Class/Atom.h"
#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/String/Class.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



Con_Obj *_Con_Builtins_String_Class_eq_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_neq_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_less_than_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_greater_than_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_add_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_mul_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_to_str_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_find_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_find_index_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_get_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_get_slice_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_hash_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_int_val_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_iter_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_len_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_prefixed_by_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_replaced_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_rfind_index_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_lstripped_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_stripped_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_suffixed_by_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_lower_cased_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_upper_cased_func(Con_Obj *);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_String_Class_bootstrap(Con_Obj *thread)
{
	Con_Obj *string_class = CON_BUILTIN(CON_BUILTIN_STRING_CLASS);
	
	Con_Builtins_Class_Atom *class_atom = (Con_Builtins_Class_Atom *) string_class->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (class_atom + 1);

	class_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;
	
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	Con_Builtins_Class_Atom_init_atom(thread, class_atom, CON_NEW_STRING("String"), NULL, CON_BUILTIN(CON_BUILTIN_OBJECT_CLASS), NULL);
	
	Con_Memory_change_chunk_type(thread, string_class, CON_MEMORY_CHUNK_OBJ);
	
	CON_SET_FIELD(string_class, "==", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_eq_func, "==", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "!=", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_neq_func, "!=", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "<", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_less_than_func, "<", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, ">", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_greater_than_func, ">", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "+", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_add_func, "+", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "*", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_mul_func, "*", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "to_str", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_to_str_func, "to_str", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "find", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_find_func, "find", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "find_index", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_find_index_func, "find_index", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "get", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_get_func, "get", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "get_slice", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_get_slice_func, "get_slice", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "hash", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_hash_func, "hash", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
    CON_SET_FIELD(string_class, "int_val", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_int_val_func, "int_val", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "iter", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_iter_func, "iter", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "len", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_len_func, "len", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "prefixed_by", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_prefixed_by_func, "prefixed_by", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "rfind_index", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_rfind_index_func, "rfind_index", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "replaced", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_replaced_func, "replaced", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
    CON_SET_FIELD(string_class, "lstripped", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_lstripped_func, "lstripped", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "stripped", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_stripped_func, "stripped", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "suffixed_by", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_suffixed_by_func, "suffixed_by", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "lower_cased", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_lower_cased_func, "lower_cased", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "upper_cased", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_upper_cased_func, "upper_cased", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// String class fields
//

//
// '==(o)'.
//

Con_Obj *_Con_Builtins_String_Class_eq_func(Con_Obj *thread)
{
	Con_Obj *o, *self;
	CON_UNPACK_ARGS("SO", &self, &o);
	
	Con_Builtins_String_Atom *self_string_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	Con_Builtins_String_Atom *o_string_atom = CON_FIND_ATOM(o, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	
	bool equal = false;
	if (o_string_atom != NULL) {
		if ((self_string_atom->hash == o_string_atom->hash) && (self_string_atom->size == o_string_atom->size)) {
			if (self_string_atom->encoding != o_string_atom->encoding)
				CON_XXX;
			if (memcmp(self_string_atom->str, o_string_atom->str, self_string_atom->size) == 0)
				equal = true;
		}
	}
	
	if (equal)
		return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
	else
		return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}



//
// '!=(o)'.
//

Con_Obj *_Con_Builtins_String_Class_neq_func(Con_Obj *thread)
{
	Con_Obj *o, *self;
	CON_UNPACK_ARGS("SO", &self, &o);
	
	Con_Builtins_String_Atom *self_string_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	Con_Builtins_String_Atom *o_string_atom = CON_FIND_ATOM(o, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	
	bool equal = false;
	if (o_string_atom != NULL) {
		if ((self_string_atom->hash == o_string_atom->hash) && (self_string_atom->size == o_string_atom->size)) {
			if (self_string_atom->encoding != o_string_atom->encoding)
				CON_XXX;
			if (memcmp(self_string_atom->str, o_string_atom->str, self_string_atom->size) == 0)
				equal = true;
		}
	}
	
	if (!equal)
		return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
	else
		return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}



//
// '<(o)'.
//

Con_Obj *_Con_Builtins_String_Class_less_than_func(Con_Obj *thread)
{
	Con_Obj *o, *self;
	CON_UNPACK_ARGS("SO", &self, &o);
	
	Con_Builtins_String_Atom *self_string_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	Con_Builtins_String_Atom *o_string_atom = CON_FIND_ATOM(o, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	
	if (o_string_atom != NULL) {
		if (self_string_atom->hash == o_string_atom->hash && self_string_atom->size == o_string_atom->size && self_string_atom->encoding == o_string_atom->encoding && memcmp(self_string_atom->str, o_string_atom->str, self_string_atom->size) == 0)
			return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
		else {
			if (self_string_atom->encoding != o_string_atom->encoding)
				CON_XXX;

			for (Con_Int i = 0; i < self_string_atom->size && i < o_string_atom->size; i += 1) {
				if (self_string_atom->str[i] < o_string_atom->str[i])
					return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
				else if (self_string_atom->str[i] != o_string_atom->str[i])
					return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
			}

			// If we've got this far then the strings have compared equal so fail straight away.
			return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
		}
	}
	else
		CON_XXX;
}



//
// '>(o)'.
//

Con_Obj *_Con_Builtins_String_Class_greater_than_func(Con_Obj *thread)
{
	Con_Obj *o, *self;
	CON_UNPACK_ARGS("SO", &self, &o);
	
	Con_Builtins_String_Atom *self_string_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	Con_Builtins_String_Atom *o_string_atom = CON_FIND_ATOM(o, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	
	if (o_string_atom != NULL) {
		if (self_string_atom->hash == o_string_atom->hash && self_string_atom->size == o_string_atom->size && self_string_atom->encoding == o_string_atom->encoding && memcmp(self_string_atom->str, o_string_atom->str, self_string_atom->size) == 0)
			return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
		else {
			if (self_string_atom->encoding != o_string_atom->encoding)
				CON_XXX;

			for (Con_Int i = 0; i < self_string_atom->size && i < o_string_atom->size; i += 1) {
				if (self_string_atom->str[i] > o_string_atom->str[i])
					return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
				else if (self_string_atom->str[i] != o_string_atom->str[i])
					return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
			}

			// If we've got this far then the strings have compared equal so fail straight away.
			return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
		}
	}
	else
		CON_XXX;
}



//
// '+(o)'
//

Con_Obj *_Con_Builtins_String_Class_add_func(Con_Obj *thread)
{
	Con_Obj *obj, *self;
	CON_UNPACK_ARGS("SS", &self, &obj);
	
	Con_Builtins_String_Atom *self_string_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	Con_Builtins_String_Atom *obj_string_atom = CON_GET_ATOM(obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	
	if (self_string_atom->encoding != obj_string_atom->encoding)
		CON_XXX;
	
	u_char *str_mem = Con_Memory_malloc(thread, self_string_atom->size + obj_string_atom->size, CON_MEMORY_CHUNK_OPAQUE);
	memmove(str_mem, self_string_atom->str, self_string_atom->size);
	memmove(str_mem + self_string_atom->size, obj_string_atom->str, obj_string_atom->size);
	
	return Con_Builtins_String_Atom_new_no_copy(thread, str_mem, self_string_atom->size + obj_string_atom->size, CON_STR_UTF_8);
}



//
// '*(i)'
//

Con_Obj *_Con_Builtins_String_Class_mul_func(Con_Obj *thread)
{
	Con_Obj *i_obj, *self;
	CON_UNPACK_ARGS("SN", &self, &i_obj);
	
	Con_Int i = Con_Numbers_Number_to_Con_Int(thread, i_obj);
	if (i < 0)
		CON_XXX;
	
	Con_Builtins_String_Atom *self_string_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	
	if (self_string_atom->encoding != CON_STR_UTF_8)
		CON_XXX;
	
	u_char *str_mem = Con_Memory_malloc(thread, self_string_atom->size * i, CON_MEMORY_CHUNK_OPAQUE);
	for (Con_Int j = 0; j < i; j += 1) {
		memmove(str_mem + j * self_string_atom->size, self_string_atom->str, self_string_atom->size);
	}
	
	return Con_Builtins_String_Atom_new_no_copy(thread, str_mem, self_string_atom->size * i, CON_STR_UTF_8);
}



//
// 'to_str()".
//

Con_Obj *_Con_Builtins_String_Class_to_str_func(Con_Obj *thread)
{
	Con_Obj *self;
	CON_UNPACK_ARGS("S", &self);
	
	return CON_ADD(CON_NEW_STRING("\""), CON_ADD(self, CON_NEW_STRING("\"")));
}



//
// 'find(s)' returns each substring which matches 's' [in practice this means that 's' itself will
// be returned for each match].
//

Con_Obj *_Con_Builtins_String_Class_find_func(Con_Obj *thread)
{
	Con_Obj *o_obj, *self_obj;
	CON_UNPACK_ARGS("SS", &self_obj, &o_obj);
	
	Con_Builtins_String_Atom *self_string_atom = CON_GET_ATOM(self_obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	Con_Builtins_String_Atom *o_string_atom = CON_GET_ATOM(o_obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	
	if (self_string_atom->encoding != CON_STR_UTF_8 || o_string_atom->encoding != CON_STR_UTF_8)
		CON_XXX;
	
	Con_Int i = 0;
	while (i + o_string_atom->size < self_string_atom->size) {
		if (memcmp(self_string_atom->str + i, o_string_atom->str, o_string_atom->size) == 0)
			CON_YIELD(o_obj);
		
		i += 1;
	}
	
	return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}



//
// 'find_index(s)' returns the starting index of each substring which matches 's'.
//

Con_Obj *_Con_Builtins_String_Class_find_index_func(Con_Obj *thread)
{
	Con_Obj *o_obj, *self_obj;
	CON_UNPACK_ARGS("SS", &self_obj, &o_obj);
	
	Con_Builtins_String_Atom *self_string_atom = CON_GET_ATOM(self_obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	Con_Builtins_String_Atom *o_string_atom = CON_GET_ATOM(o_obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	
	if (self_string_atom->encoding != CON_STR_UTF_8 || o_string_atom->encoding != CON_STR_UTF_8)
		CON_XXX;
	
	Con_Int i = 0;
	while (i + o_string_atom->size < self_string_atom->size) {
		if (memcmp(self_string_atom->str + i, o_string_atom->str, o_string_atom->size) == 0)
			CON_YIELD(CON_NEW_INT(i));
		
		i += 1;
	}
	
	return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}



//
// 'get(i)' returns the character at position 'i'.
//

Con_Obj *_Con_Builtins_String_Class_get_func(Con_Obj *thread)
{
	Con_Obj *i_obj, *self;
	CON_UNPACK_ARGS("SN", &self, &i_obj);
	
	Con_Builtins_String_Atom *self_string_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	Con_Int i = Con_Misc_translate_index(thread, NULL, Con_Numbers_Number_to_Con_Int(thread, i_obj), self_string_atom->size);
	
	if (self_string_atom->encoding != CON_STR_UTF_8)
		CON_XXX;
	
	return Con_Builtins_String_Atom_new_no_copy(thread, self_string_atom->str + i, 1, self_string_atom->encoding);
}



//
// 'hash()' returns this strings hash.
//

Con_Obj *_Con_Builtins_String_Class_hash_func(Con_Obj *thread)
{
	Con_Obj *self;
	CON_UNPACK_ARGS("S", &self);
	
	Con_Builtins_String_Atom *self_string_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	
	return CON_NEW_INT(self_string_atom->hash);
}



//
// 'get_slice(lower, upper)" returns a new string consisting of (upper - lower) characters starting
// at position 'lower' of the current string.
//

Con_Obj *_Con_Builtins_String_Class_get_slice_func(Con_Obj *thread)
{
	Con_Obj *lower_obj, *self, *upper_obj;
	CON_UNPACK_ARGS("S;OO", &self, &lower_obj, &upper_obj);
	
	Con_Builtins_String_Atom *self_string_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	
	Con_Int lower, upper;
	Con_Misc_translate_slice_indices(thread, NULL, lower_obj, upper_obj, &lower, &upper, self_string_atom->size);

	if (self_string_atom->encoding != CON_STR_UTF_8)
		CON_XXX;
	
	return Con_Builtins_String_Atom_new_no_copy(thread, self_string_atom->str + lower, upper - lower, self_string_atom->encoding);
}



//
// 'int_val(i := 0)' returns the integer value of the character at position 'i'.
//

Con_Obj *_Con_Builtins_String_Class_int_val_func(Con_Obj *thread)
{
	Con_Obj *i_obj, *self;
	CON_UNPACK_ARGS("S;N", &self, &i_obj);
	
	Con_Builtins_String_Atom *self_string_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	Con_Int i;
    if (i_obj == NULL)
        i = Con_Misc_translate_index(thread, NULL, 0, self_string_atom->size);
    else
        i = Con_Misc_translate_index(thread, NULL, Con_Numbers_Number_to_Con_Int(thread, i_obj), self_string_atom->size);

	if (self_string_atom->encoding != CON_STR_UTF_8)
		CON_XXX;
	
	return CON_NEW_INT(self_string_atom->str[i]);
}



//
// 'iter(lower := 0, upper := -1)' is a generator which returns each character of the string in order.
//

Con_Obj *_Con_Builtins_String_Class_iter_func(Con_Obj *thread)
{
	Con_Obj *lower_obj, *self, *upper_obj;
	CON_UNPACK_ARGS("S;OO", &self, &lower_obj, &upper_obj);
	
	Con_Builtins_String_Atom *self_string_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	
	Con_Int lower, upper;
	Con_Misc_translate_slice_indices(thread, NULL, lower_obj, upper_obj, &lower, &upper, self_string_atom->size);

	for (Con_Int i = lower; i < upper; i += 1) {
		CON_YIELD(Con_Builtins_String_Atom_new_no_copy(thread, self_string_atom->str + i, 1, self_string_atom->encoding));
	}
	
	return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}



//
// 'len()' returns the number of characters in the string.
//

Con_Obj *_Con_Builtins_String_Class_len_func(Con_Obj *thread)
{
	Con_Obj *self;
	CON_UNPACK_ARGS("S", &self);
	
	Con_Builtins_String_Atom *self_string_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));

	if (self_string_atom->encoding != CON_STR_UTF_8)
		CON_XXX;

	return CON_NEW_INT(self_string_atom->size);
}



//
// 'prefixed_by(o, i := self.len())' succeeds if the string starting at position 'i' matches 's' in
// its entirety.
//

Con_Obj *_Con_Builtins_String_Class_prefixed_by_func(Con_Obj *thread)
{
	Con_Obj *i_obj, *o_obj, *self_obj;
	CON_UNPACK_ARGS("SS;N", &self_obj, &o_obj, &i_obj);
	
	Con_Builtins_String_Atom *self_string_atom = CON_GET_ATOM(self_obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	Con_Builtins_String_Atom *o_string_atom = CON_GET_ATOM(o_obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	
	Con_Int i;
	if (i_obj == NULL)
		i = 0;
	else
		i = Con_Misc_translate_slice_index(thread, NULL, Con_Numbers_Number_to_Con_Int(thread, i_obj), self_string_atom->size);
	
	if (self_string_atom->encoding != CON_STR_UTF_8 || o_string_atom->encoding != CON_STR_UTF_8)
		CON_XXX;
	
	if (self_string_atom->size - i >= o_string_atom->size && memcmp(self_string_atom->str + i, o_string_atom->str, o_string_atom->size) == 0)
		return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
	else
		return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}



//
// 'rfind_index(s)' is a generator which returns each index that the substring 's' matches in self,
// starting from the right hand side of the string.
//

Con_Obj *_Con_Builtins_String_Class_rfind_index_func(Con_Obj *thread)
{
	Con_Obj *o_obj, *self_obj;
	CON_UNPACK_ARGS("SS", &self_obj, &o_obj);
	
	Con_Builtins_String_Atom *self_string_atom = CON_GET_ATOM(self_obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	Con_Builtins_String_Atom *o_string_atom = CON_GET_ATOM(o_obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	
	if (self_string_atom->encoding != CON_STR_UTF_8 || o_string_atom->encoding != CON_STR_UTF_8)
		CON_XXX;
	
	Con_Int i = self_string_atom->size - o_string_atom->size;
	while (i >= 0) {
		if (memcmp(self_string_atom->str + i, o_string_atom->str, o_string_atom->size) == 0)
			CON_YIELD(CON_NEW_INT(i));
		
		i -= 1;
	}
	
	return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}



//
// 'replaced(old, new)' replaces each non-overlapping occurrence of substring 'old' with 'new'.
// Although an implementation detail, note that because of strings immutability this function will
// return 'self' if 'old' is not found within it.
//

Con_Obj *_Con_Builtins_String_Class_replaced_func(Con_Obj *thread)
{
	Con_Obj *old_obj, *new_obj, *self_obj;
	CON_UNPACK_ARGS("SSS", &self_obj, &old_obj, &new_obj);
	
	Con_Builtins_String_Atom *self_string_atom = CON_GET_ATOM(self_obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	Con_Builtins_String_Atom *old_string_atom = CON_GET_ATOM(old_obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	Con_Builtins_String_Atom *new_string_atom = CON_GET_ATOM(new_obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	
	if (self_string_atom->encoding != CON_STR_UTF_8 || old_string_atom->encoding != CON_STR_UTF_8 || new_string_atom->encoding != CON_STR_UTF_8)
		CON_XXX;

	// In order to replace substrings we do two passes. First we count how many substrings need to
	// be replaced.

	Con_Int i = 0;
	Con_Int matches = 0;
	while (i <= self_string_atom->size - old_string_atom->size) {
		if (memcmp(self_string_atom->str + i, old_string_atom->str, old_string_atom->size) == 0) {
			matches += 1;
			i += old_string_atom->size;
		}
		else
			i += 1;
	}
	
	if (matches == 0)
		 // If there are no substrings to be replaced, we return the self string immediately.
		return self_obj;

	// If there are strings to replace we can now accurately calculate the size of the resulting
	// string, which we then construct.

	Con_Int new_str_mem_size = self_string_atom->size + matches * (new_string_atom->size - old_string_atom->size);
	u_char *new_str_mem = Con_Memory_malloc(thread, new_str_mem_size, CON_MEMORY_CHUNK_OPAQUE);
	Con_Int new_str_mem_pos = 0;
	i = 0;
	Con_Int last_i = 0;
	while (i <= self_string_atom->size - old_string_atom->size) {
		if (memcmp(self_string_atom->str + i, old_string_atom->str, old_string_atom->size) == 0) {
			memmove(new_str_mem + new_str_mem_pos, self_string_atom->str + last_i, i - last_i);
			new_str_mem_pos += i - last_i;
			memmove(new_str_mem + new_str_mem_pos, new_string_atom->str, new_string_atom->size);
			new_str_mem_pos += new_string_atom->size;
			
			i += old_string_atom->size;
			last_i = i;
		}
		else
			i += 1;
	}
	if (last_i < i) {
		memmove(new_str_mem + new_str_mem_pos, self_string_atom->str + last_i, self_string_atom->size - last_i);
		new_str_mem_pos += self_string_atom->size - last_i;
	}
	
	if (new_str_mem == NULL)
		return self_obj;
	else
		return Con_Builtins_String_Atom_new_no_copy(thread, new_str_mem, new_str_mem_pos, CON_STR_UTF_8);
}



//
//
//

Con_Obj *_Con_Builtins_String_Class_lstripped_func(Con_Obj *thread)
{
	Con_Obj *self_obj;
	CON_UNPACK_ARGS("S", &self_obj);
	
	Con_Builtins_String_Atom *self_string_atom = CON_GET_ATOM(self_obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	
	if (self_string_atom->encoding != CON_STR_UTF_8)
		CON_XXX;
	
	Con_Int l = 0;
	while (l < self_string_atom->size) {
		if (self_string_atom->str[l] == ' ' || self_string_atom->str[l] == '\t' || self_string_atom->str[l] == '\n' || self_string_atom->str[l] == '\r')
			l += 1;
		else
			break;
	}
	
	return Con_Builtins_String_Atom_new_no_copy(thread, self_string_atom->str + l, self_string_atom->size - l, self_string_atom->encoding);
}



//
//
//

Con_Obj *_Con_Builtins_String_Class_stripped_func(Con_Obj *thread)
{
	Con_Obj *self_obj;
	CON_UNPACK_ARGS("S", &self_obj);
	
	Con_Builtins_String_Atom *self_string_atom = CON_GET_ATOM(self_obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	
	if (self_string_atom->encoding != CON_STR_UTF_8)
		CON_XXX;
	
	Con_Int l = 0;
	while (l < self_string_atom->size) {
		if (self_string_atom->str[l] == ' ' || self_string_atom->str[l] == '\t' || self_string_atom->str[l] == '\n' || self_string_atom->str[l] == '\r')
			l += 1;
		else
			break;
	}

	Con_Int r = self_string_atom->size - 1;
	while (r >= 0 && r > l) {
		if (self_string_atom->str[r] == ' ' || self_string_atom->str[r] == '\t' || self_string_atom->str[r] == '\n' || self_string_atom->str[r] == '\r')
			r -= 1;
		else
			break;
	}
	r += 1;
	
	return Con_Builtins_String_Atom_new_no_copy(thread, self_string_atom->str + l, (r - l), self_string_atom->encoding);
}



//
// 'suffixed_by(o, i := self.len())' succeeds if the string ending at position 'i' matches 's' in its
// entirety.
//

Con_Obj *_Con_Builtins_String_Class_suffixed_by_func(Con_Obj *thread)
{
	Con_Obj *i_obj, *o_obj, *self_obj;
	CON_UNPACK_ARGS("SS;N", &self_obj, &o_obj, &i_obj);
	
	Con_Builtins_String_Atom *self_string_atom = CON_GET_ATOM(self_obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	Con_Builtins_String_Atom *o_string_atom = CON_GET_ATOM(o_obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));

	Con_Int i;
	if (i_obj == NULL)
		i = self_string_atom->size;
	else
		i = Con_Misc_translate_slice_index(thread, NULL, Con_Numbers_Number_to_Con_Int(thread, i_obj), self_string_atom->size);
	
	if (self_string_atom->encoding != CON_STR_UTF_8 || o_string_atom->encoding != CON_STR_UTF_8)
		CON_XXX;
	
	if (i >= o_string_atom->size && memcmp(self_string_atom->str + i - o_string_atom->size, o_string_atom->str, o_string_atom->size) == 0)
		return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
	else
		return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}



//
// 'lower_cased()' returns a lower case version of the string.
//

Con_Obj *_Con_Builtins_String_Class_lower_cased_func(Con_Obj *thread)
{
	Con_Obj *self;
	CON_UNPACK_ARGS("S", &self);
	
	Con_Builtins_String_Atom *self_string_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	
	if (self_string_atom->encoding == CON_STR_UTF_8) {
		u_char *new_str_mem = Con_Memory_malloc(thread, self_string_atom->size, CON_MEMORY_CHUNK_OPAQUE);
		for (Con_Int i = 0; i < self_string_atom->size; i += 1) {
			char c = *(self_string_atom->str + i);
			if ((c >= 'A') && (c <= 'Z'))
				c -= 'A' - 'a';
			new_str_mem[i] = c;
		}
		return Con_Builtins_String_Atom_new_no_copy(thread, new_str_mem, self_string_atom->size, CON_STR_UTF_8);
	}
	else
		CON_XXX;
}



//
// 'upper_cased()' returns an upper case version of the string.
//

Con_Obj *_Con_Builtins_String_Class_upper_cased_func(Con_Obj *thread)
{
	Con_Obj *self;
	CON_UNPACK_ARGS("S", &self);
	
	Con_Builtins_String_Atom *self_string_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	
	if (self_string_atom->encoding == CON_STR_UTF_8) {
		u_char *new_str_mem = Con_Memory_malloc(thread, self_string_atom->size, CON_MEMORY_CHUNK_OPAQUE);
		for (Con_Int i = 0; i < self_string_atom->size; i += 1) {
			char c = *(self_string_atom->str + i);
			if ((c >= 'a') && (c <= 'z'))
				c -= 'a' - 'A';
			new_str_mem[i] = c;
		}
		return Con_Builtins_String_Atom_new_no_copy(thread, new_str_mem, self_string_atom->size, CON_STR_UTF_8);
	}
	else
		CON_XXX;
}
