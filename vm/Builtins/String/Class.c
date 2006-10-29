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
Con_Obj *_Con_Builtins_String_Class_add_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_mul_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_to_str_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_find_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_find_index_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_get_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_get_slice_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_hash_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_iterate_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_len_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_prefixed_by_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_rfind_index_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_stripped_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_suffixed_by_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_to_lower_case_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_to_upper_case_func(Con_Obj *);



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
	CON_SET_FIELD(string_class, "+", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_add_func, "+", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "*", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_mul_func, "*", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "to_str", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_to_str_func, "to_str", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "find", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_find_func, "find", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "find_index", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_find_index_func, "find_index", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "get", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_get_func, "get", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "get_slice", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_get_slice_func, "get_slice", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "hash", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_hash_func, "hash", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "iterate", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_iterate_func, "iterate", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "len", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_len_func, "len", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "prefixed_by", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_prefixed_by_func, "prefixed_by", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "rfind_index", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_rfind_index_func, "rfind_index", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "stripped", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_stripped_func, "stripped", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "suffixed_by", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_suffixed_by_func, "suffixed_by", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "to_lower_case", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_to_lower_case_func, "to_lower_case", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
	CON_SET_FIELD(string_class, "to_upper_case", CON_NEW_BOUND_C_FUNC(_Con_Builtins_String_Class_to_upper_case_func, "to_upper_case", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), string_class));
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
	
	return Con_Builtins_String_Atom_new_no_copy(thread, (const char *) str_mem, self_string_atom->size + obj_string_atom->size, CON_STR_UTF_8);
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
	
	return Con_Builtins_String_Atom_new_no_copy(thread, (const char *) str_mem, self_string_atom->size * i, CON_STR_UTF_8);
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
// 'iterate(lower := 0, upper := -1)' is a generator which returns each character of the string in order.
//

Con_Obj *_Con_Builtins_String_Class_iterate_func(Con_Obj *thread)
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
// 'prefixed_by(o)' returns null if 'self' starts with the string 's', or fails otherwise.
//

Con_Obj *_Con_Builtins_String_Class_prefixed_by_func(Con_Obj *thread)
{
	Con_Obj *o_obj, *self_obj;
	CON_UNPACK_ARGS("SS", &self_obj, &o_obj);
	
	Con_Builtins_String_Atom *self_string_atom = CON_GET_ATOM(self_obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	Con_Builtins_String_Atom *o_string_atom = CON_GET_ATOM(o_obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	
	if (self_string_atom->encoding != CON_STR_UTF_8 || o_string_atom->encoding != CON_STR_UTF_8)
		CON_XXX;
	
	if (self_string_atom->size >= o_string_atom->size && memcmp(self_string_atom->str, o_string_atom->str, o_string_atom->size) == 0)
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
// 'suffixed_by(o)' returns null if 'self' ends with the string 's', or fails otherwise.
//

Con_Obj *_Con_Builtins_String_Class_suffixed_by_func(Con_Obj *thread)
{
	Con_Obj *o_obj, *self_obj;
	CON_UNPACK_ARGS("SS", &self_obj, &o_obj);
	
	Con_Builtins_String_Atom *self_string_atom = CON_GET_ATOM(self_obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	Con_Builtins_String_Atom *o_string_atom = CON_GET_ATOM(o_obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	
	if (self_string_atom->encoding != CON_STR_UTF_8 || o_string_atom->encoding != CON_STR_UTF_8)
		CON_XXX;
	
	if (self_string_atom->size >= o_string_atom->size && memcmp(self_string_atom->str + self_string_atom->size - o_string_atom->size, o_string_atom->str, o_string_atom->size) == 0)
		return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
	else
		return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}



//
// 'to_lower_case()' returns a lower case version of the string.
//

Con_Obj *_Con_Builtins_String_Class_to_lower_case_func(Con_Obj *thread)
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
// 'to_upper_case()' returns an upper case version of the string.
//

Con_Obj *_Con_Builtins_String_Class_to_upper_case_func(Con_Obj *thread)
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
