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

#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include "Core.h"
#include "Hash.h"
#include "Numbers.h"
#include "Object.h"
#include "Shortcuts.h"

#include "Builtins/Class/Atom.h"
#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Float/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/Int/Class.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



Con_Obj *_Con_Builtins_Int_Class_new_object(Con_Obj *);

Con_Obj *_Con_Builtins_Int_Class_to_str_func(Con_Obj *);
Con_Obj *_Con_Builtins_Int_Class_eq_func(Con_Obj *);
Con_Obj *_Con_Builtins_Int_Class_less_than_func(Con_Obj *);
Con_Obj *_Con_Builtins_Int_Class_gt_func(Con_Obj *);
Con_Obj *_Con_Builtins_Int_Class_add_func(Con_Obj *);
Con_Obj *_Con_Builtins_Int_Class_subtract_func(Con_Obj *);
Con_Obj *_Con_Builtins_Int_Class_and_func(Con_Obj *);
Con_Obj *_Con_Builtins_Int_Class_div_func(Con_Obj *);
Con_Obj *_Con_Builtins_Int_Class_idiv_func(Con_Obj *);
Con_Obj *_Con_Builtins_Int_Class_modulo_func(Con_Obj *);
Con_Obj *_Con_Builtins_Int_Class_mul_func(Con_Obj *);
Con_Obj *_Con_Builtins_Int_Class_lsl_func(Con_Obj *);
Con_Obj *_Con_Builtins_Int_Class_lsr_func(Con_Obj *);
Con_Obj *_Con_Builtins_Int_Class_or_func(Con_Obj *);
Con_Obj *_Con_Builtins_Int_Class_iter_to_func(Con_Obj *);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_Int_Class_bootstrap(Con_Obj *thread)
{
	Con_Obj *int_class = CON_BUILTIN(CON_BUILTIN_INT_CLASS);
	
	Con_Builtins_Class_Atom *class_atom = (Con_Builtins_Class_Atom *) int_class->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (class_atom + 1);

	class_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;
	
	Con_Builtins_Class_Atom_init_atom(thread, class_atom, CON_NEW_STRING("Int"), CON_NEW_UNBOUND_C_FUNC(_Con_Builtins_Int_Class_new_object, "Int_new", CON_BUILTIN(CON_BUILTIN_NULL_OBJ)), CON_BUILTIN(CON_BUILTIN_NUMBER_CLASS), NULL);
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	
	Con_Memory_change_chunk_type(thread, int_class, CON_MEMORY_CHUNK_OBJ);
	
	CON_SET_FIELD(int_class, "to_str", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Int_Class_to_str_func, "to_str", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), int_class));
	CON_SET_FIELD(int_class, "==", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Int_Class_eq_func, "==", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), int_class));
	CON_SET_FIELD(int_class, "<", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Int_Class_less_than_func, "<", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), int_class));
	CON_SET_FIELD(int_class, ">", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Int_Class_gt_func, ">", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), int_class));
	CON_SET_FIELD(int_class, "+", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Int_Class_add_func, "+", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), int_class));
	CON_SET_FIELD(int_class, "-", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Int_Class_subtract_func, "-", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), int_class));
	CON_SET_FIELD(int_class, "/", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Int_Class_div_func, "/", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), int_class));
	CON_SET_FIELD(int_class, "idiv", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Int_Class_idiv_func, "idiv", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), int_class));
	CON_SET_FIELD(int_class, "%", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Int_Class_modulo_func, "%", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), int_class));
	CON_SET_FIELD(int_class, "*", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Int_Class_mul_func, "*", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), int_class));
	CON_SET_FIELD(int_class, "and", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Int_Class_and_func, "and", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), int_class));
	CON_SET_FIELD(int_class, "lsl", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Int_Class_lsl_func, "lsl", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), int_class));
	CON_SET_FIELD(int_class, "lsr", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Int_Class_lsr_func, "lsr", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), int_class));
	CON_SET_FIELD(int_class, "or", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Int_Class_or_func, "or", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), int_class));
	CON_SET_FIELD(int_class, "iter_to", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Int_Class_iter_to_func, "iter_to", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), int_class));
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// func Int_new
//

Con_Obj *_Con_Builtins_Int_Class_new_object(Con_Obj *thread)
{
	Con_Obj *o, *self;
	CON_UNPACK_ARGS("CO", &self, &o);
	
	Con_Atom *atom = o->first_atom;
	while (atom != NULL) {
		if (atom->atom_type == CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT)) {
#			if SIZEOF_LONG == SIZEOF_CON_INT
			char *c_string = Con_Builtins_String_Atom_to_c_string(thread, o);
			char *endptr;
			errno = 0;
			Con_Int num = strtol(c_string, &endptr, 0);
			if (c_string == endptr || *endptr != 0)
				CON_RAISE_EXCEPTION("Number_Exception", o);
			if (errno != 0)
				CON_XXX;
			return CON_NEW_INT(num);
#			else
			XXX;
#			endif
		}
		atom = atom->next_atom;
	}

	CON_RAISE_EXCEPTION("Number_Exception", o);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Int class fields
//

//
// 'to_str()'.
//

Con_Obj *_Con_Builtins_Int_Class_to_str_func(Con_Obj *thread)
{
	Con_Obj *self;
	CON_UNPACK_ARGS("I", &self);
	
	Con_Builtins_Int_Atom *self_int_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT));

#	define MAX_INTEGER_SIZE_AS_STRING ((int) ((sizeof(Con_Int) * 8) / 2))

	char str[MAX_INTEGER_SIZE_AS_STRING];
	if (snprintf(str, MAX_INTEGER_SIZE_AS_STRING, CON_INT_FORMAT, self_int_atom->val) >= MAX_INTEGER_SIZE_AS_STRING)
		CON_FATAL_ERROR("Unable to convert number to string");
	
	return Con_Builtins_String_Atom_new_copy(thread, (u_char *) str, strlen(str), CON_STR_UTF_8);
}



//
// '==(o)'.
//

Con_Obj *_Con_Builtins_Int_Class_eq_func(Con_Obj *thread)
{
	Con_Obj *self, *o;
	CON_UNPACK_ARGS("IN", &self, &o);
	
	Con_Builtins_Int_Atom *self_int_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT));
	Con_Int o_val = Con_Numbers_Number_to_Con_Int(thread, o);
	
	if (self_int_atom->val == o_val)
		return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
	else
		return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}



//
// '<(o)'.
//

Con_Obj *_Con_Builtins_Int_Class_less_than_func(Con_Obj *thread)
{
	Con_Obj *self, *o;
	CON_UNPACK_ARGS("IN", &self, &o);
	
	Con_Builtins_Int_Atom *self_int_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT));
	Con_Int o_val = Con_Numbers_Number_to_Con_Int(thread, o);
	
	if (self_int_atom->val < o_val)
		return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
	else
		return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}



//
// '>(o)'.
//

Con_Obj *_Con_Builtins_Int_Class_gt_func(Con_Obj *thread)
{
	Con_Obj *self, *o;
	CON_UNPACK_ARGS("IN", &self, &o);
	
	Con_Builtins_Int_Atom *self_int_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT));
	Con_Int o_val = Con_Numbers_Number_to_Con_Int(thread, o);
	
	if (self_int_atom->val > o_val)
		return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
	else
		return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}



//
// '+(o)'.
//

Con_Obj *_Con_Builtins_Int_Class_add_func(Con_Obj *thread)
{
	Con_Obj *self, *o;
	CON_UNPACK_ARGS("IN", &self, &o);
	
	Con_Builtins_Int_Atom *self_int_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT));
	Con_Builtins_Int_Atom *o_int_atom = CON_FIND_ATOM(o, CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT));
	
	if (o_int_atom != NULL)
		return CON_NEW_INT(self_int_atom->val + o_int_atom->val);
	else {
		Con_Builtins_Float_Atom *o_float_atom = CON_GET_ATOM(o, CON_BUILTIN(CON_BUILTIN_FLOAT_ATOM_DEF_OBJECT));
		return Con_Builtins_Float_Atom_new(thread, (Con_Float) self_int_atom->val + o_float_atom->val);
	}
}



//
// '-(o)'.
//

Con_Obj *_Con_Builtins_Int_Class_subtract_func(Con_Obj *thread)
{
	Con_Obj *self, *o;
	CON_UNPACK_ARGS("IN", &self, &o);
	
	Con_Builtins_Int_Atom *self_int_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT));
	Con_Builtins_Int_Atom *o_int_atom = CON_FIND_ATOM(o, CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT));
	
	if (o_int_atom != NULL)
		return CON_NEW_INT(self_int_atom->val - o_int_atom->val);
	else {
		Con_Builtins_Float_Atom *o_float_atom = CON_GET_ATOM(o, CON_BUILTIN(CON_BUILTIN_FLOAT_ATOM_DEF_OBJECT));
		return Con_Builtins_Float_Atom_new(thread, (Con_Float) self_int_atom->val - o_float_atom->val);
	}
}



//
// '/(o)'.
//

Con_Obj *_Con_Builtins_Int_Class_div_func(Con_Obj *thread)
{
	Con_Obj *self, *o_obj;
	CON_UNPACK_ARGS("IN", &self, &o_obj);

	Con_Builtins_Int_Atom *self_int_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT));
	Con_Builtins_Int_Atom *o_int_atom = CON_FIND_ATOM(o_obj, CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT));
	
	if (o_int_atom == NULL) {
		CON_XXX;
	}
	else {
		Con_Int o = o_int_atom->val;
		if (o == 0)
			CON_XXX;

		if (self_int_atom->val % o == 0)
			return CON_NEW_INT(self_int_atom->val / o);
		else
			return Con_Builtins_Float_Atom_new(thread, ((Con_Float) self_int_atom->val) / ((Con_Float) o));
	}
}



//
// 'idiv(o)'.
//

Con_Obj *_Con_Builtins_Int_Class_idiv_func(Con_Obj *thread)
{
	Con_Obj *self, *o_obj;
	CON_UNPACK_ARGS("IN", &self, &o_obj);

	Con_Builtins_Int_Atom *self_int_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT));
	Con_Builtins_Int_Atom *o_int_atom = CON_FIND_ATOM(o_obj, CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT));

	if (o_int_atom == NULL) {
		CON_XXX;
	}
	else {
		Con_Int o = o_int_atom->val;
		if (o == 0)
			CON_XXX;

		return CON_NEW_INT(self_int_atom->val / o);
	}
}



//
// '%(o)'.
//

Con_Obj *_Con_Builtins_Int_Class_modulo_func(Con_Obj *thread)
{
	Con_Obj *self, *o_obj;
	CON_UNPACK_ARGS("IN", &self, &o_obj);
	
	Con_Builtins_Int_Atom *self_int_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT));
	Con_Int o = Con_Numbers_Number_to_Con_Int(thread, o_obj);
	
	return CON_NEW_INT(self_int_atom->val % o);
}



//
// '*(o)'.
//

Con_Obj *_Con_Builtins_Int_Class_mul_func(Con_Obj *thread)
{
	Con_Obj *self, *o;
	CON_UNPACK_ARGS("IN", &self, &o);
	
	Con_Builtins_Int_Atom *self_int_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT));
	Con_Builtins_Int_Atom *o_int_atom = CON_FIND_ATOM(o, CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT));
	
	if (o_int_atom != NULL)
		return CON_NEW_INT(self_int_atom->val * o_int_atom->val);
	else {
		Con_Builtins_Float_Atom *o_float_atom = CON_GET_ATOM(o, CON_BUILTIN(CON_BUILTIN_FLOAT_ATOM_DEF_OBJECT));
		return Con_Builtins_Float_Atom_new(thread, (Con_Float) self_int_atom->val * o_float_atom->val);
	}
}



//
// 'and(n)'.
//

Con_Obj *_Con_Builtins_Int_Class_and_func(Con_Obj *thread)
{
	Con_Obj *self, *n_obj;
	CON_UNPACK_ARGS("IN", &self, &n_obj);
	
	Con_Builtins_Int_Atom *self_int_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT));
	Con_Int n = Con_Numbers_Number_to_Con_Int(thread, n_obj);
	
	return CON_NEW_INT(self_int_atom->val & n);
}



//
// 'lsl(n)' Logical Shift Left.
//

Con_Obj *_Con_Builtins_Int_Class_lsl_func(Con_Obj *thread)
{
	Con_Obj *self, *n_obj;
	CON_UNPACK_ARGS("IN", &self, &n_obj);
	
	Con_Builtins_Int_Atom *self_int_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT));
	Con_Int n = Con_Numbers_Number_to_Con_Int(thread, n_obj);
	
	if (n >= SIZEOF_CON_INT * 8) {
		// Overflow.
		CON_XXX;
	}
	
	return CON_NEW_INT(self_int_atom->val << n);
}



//
// 'lsr(n)' Logical Shift Right.
//

Con_Obj *_Con_Builtins_Int_Class_lsr_func(Con_Obj *thread)
{
	Con_Obj *self, *n_obj;
	CON_UNPACK_ARGS("IN", &self, &n_obj);
	
	Con_Builtins_Int_Atom *self_int_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT));
	Con_Int n = Con_Numbers_Number_to_Con_Int(thread, n_obj);
	
	if (n >= SIZEOF_CON_INT * 8) {
		// Overflow.
		CON_XXX;
	}
	
	return CON_NEW_INT(self_int_atom->val >> n);
}



//
// 'or(n)'.
//

Con_Obj *_Con_Builtins_Int_Class_or_func(Con_Obj *thread)
{
	Con_Obj *self, *n_obj;
	CON_UNPACK_ARGS("IN", &self, &n_obj);
	
	Con_Builtins_Int_Atom *self_int_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT));
	Con_Int n = Con_Numbers_Number_to_Con_Int(thread, n_obj);
	
	return CON_NEW_INT(self_int_atom->val | n);
}



//
// 'iter_to(n)'.
//

Con_Obj *_Con_Builtins_Int_Class_iter_to_func(Con_Obj *thread)
{
	Con_Obj *self, *step_obj, *to_obj;
	CON_UNPACK_ARGS("IN;N", &self, &to_obj, &step_obj);
	
	Con_Builtins_Int_Atom *self_int_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT));
	
	Con_Int to = Con_Numbers_Number_to_Con_Int(thread, to_obj);
	
	Con_Int step;
	if (step_obj == NULL)
		step = 1;
	else
		step = Con_Numbers_Number_to_Con_Int(thread, step_obj);

	if (step >= 0) {
		for (Con_Int i = self_int_atom->val; i < to; i += step) {
			CON_YIELD(CON_NEW_INT(i));
		}
	}
	else {
		for (Con_Int i = self_int_atom->val; i > to; i += step) {
			CON_YIELD(CON_NEW_INT(i));
		}
	}
	
	return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}
