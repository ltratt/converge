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
#include <math.h>
#include <stdarg.h>
#include <string.h>

#include "Core.h"
#include "Hash.h"
#include "Numbers.h"
#include "Object.h"
#include "Shortcuts.h"

#include "Builtins/Class/Atom.h"
#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/Float/Atom.h"
#include "Builtins/Float/Class.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



Con_Obj *_Con_Builtins_Float_Class_new_object(Con_Obj *);

Con_Obj *_Con_Builtins_Float_Class_to_str_func(Con_Obj *);
Con_Obj *_Con_Builtins_Float_Class_eq_func(Con_Obj *);
Con_Obj *_Con_Builtins_Float_Class_greater_than_func(Con_Obj *);
Con_Obj *_Con_Builtins_Float_Class_greater_than_equals_func(Con_Obj *);
Con_Obj *_Con_Builtins_Float_Class_less_than_func(Con_Obj *);
Con_Obj *_Con_Builtins_Float_Class_less_than_equals_func(Con_Obj *);
Con_Obj *_Con_Builtins_Float_Class_add_func(Con_Obj *);
Con_Obj *_Con_Builtins_Float_Class_subtract_func(Con_Obj *);
Con_Obj *_Con_Builtins_Float_Class_and_func(Con_Obj *);
Con_Obj *_Con_Builtins_Float_Class_div_func(Con_Obj *);
Con_Obj *_Con_Builtins_Float_Class_mul_func(Con_Obj *);
Con_Obj *_Con_Builtins_Float_Class_sqrt_func(Con_Obj *);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_Float_Class_bootstrap(Con_Obj *thread)
{
	Con_Obj *float_class = CON_BUILTIN(CON_BUILTIN_FLOAT_CLASS);

	Con_Builtins_Class_Atom *class_atom = (Con_Builtins_Class_Atom *) float_class->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (class_atom + 1);

	class_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;

	Con_Builtins_Class_Atom_init_atom(thread, class_atom, CON_NEW_STRING("Float"), CON_NEW_UNBOUND_C_FUNC(_Con_Builtins_Float_Class_new_object, "Float_new", CON_BUILTIN(CON_BUILTIN_NULL_OBJ)), CON_BUILTIN(CON_BUILTIN_NUMBER_CLASS), NULL);
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);

	Con_Memory_change_chunk_type(thread, float_class, CON_MEMORY_CHUNK_OBJ);

	CON_SET_FIELD(float_class, "to_str", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Float_Class_to_str_func, "to_str", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), float_class));
	CON_SET_FIELD(float_class, "==", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Float_Class_eq_func, "==", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), float_class));
	CON_SET_FIELD(float_class, ">", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Float_Class_greater_than_func, ">", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), float_class));
	CON_SET_FIELD(float_class, ">=", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Float_Class_greater_than_equals_func, ">=", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), float_class));
	CON_SET_FIELD(float_class, "<", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Float_Class_less_than_func, "<", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), float_class));
	CON_SET_FIELD(float_class, "<=", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Float_Class_less_than_equals_func, "<=", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), float_class));
	CON_SET_FIELD(float_class, "+", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Float_Class_add_func, "+", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), float_class));
	CON_SET_FIELD(float_class, "-", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Float_Class_subtract_func, "-", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), float_class));
	CON_SET_FIELD(float_class, "/", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Float_Class_div_func, "/", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), float_class));
	CON_SET_FIELD(float_class, "*", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Float_Class_mul_func, "*", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), float_class));
	CON_SET_FIELD(float_class, "sqrt", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Float_Class_sqrt_func, "sqrt", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), float_class));
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// func Float_new
//

Con_Obj *_Con_Builtins_Float_Class_new_object(Con_Obj *thread)
{
	Con_Obj *o, *self;
	CON_UNPACK_ARGS("CO", &self, &o);

	Con_Atom *atom = o->first_atom;
	while (atom != NULL) {
		if (atom->atom_type == CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT)) {
			char *c_string = Con_Builtins_String_Atom_to_c_string(thread, o);
			char *endptr;
			errno = 0;
			Con_Float num = strtod(c_string, &endptr);
			if (c_string == endptr || *endptr != 0)
				CON_RAISE_EXCEPTION("Number_Exception", o);
			if (errno != 0)
				CON_XXX;
			return Con_Builtins_Float_Atom_new(thread, num);
		}
	}

	CON_XXX;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Int class fields
//

//
// 'to_str()'.
//

Con_Obj *_Con_Builtins_Float_Class_to_str_func(Con_Obj *thread)
{
	Con_Obj *self;
	CON_UNPACK_ARGS("R", &self);

	Con_Builtins_Float_Atom *float_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_FLOAT_ATOM_DEF_OBJECT));

#	define MAX_FLOAT_SIZE_AS_STRING ((int) ((sizeof(Con_Int) * 8) / 2)) + 20

	char str[MAX_FLOAT_SIZE_AS_STRING];
	if (snprintf(str, MAX_FLOAT_SIZE_AS_STRING, "%.20lg", float_atom->val) >= MAX_FLOAT_SIZE_AS_STRING)
		CON_FATAL_ERROR("Unable to convert number to string");

	return Con_Builtins_String_Atom_new_copy(thread, str, strlen(str), CON_STR_UTF_8);
}



//
// '==(o)'.
//

Con_Obj *_Con_Builtins_Float_Class_eq_func(Con_Obj *thread)
{
	Con_Obj *self, *o_obj;
	CON_UNPACK_ARGS("RN", &self, &o_obj);

	Con_Builtins_Float_Atom *self_float_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_FLOAT_ATOM_DEF_OBJECT));
	Con_Float o = Con_Numbers_Number_to_Con_Float(thread, o_obj);

	if (self_float_atom->val == o)
		return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
	else
		return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}



//
// '>(o)'.
//

Con_Obj *_Con_Builtins_Float_Class_greater_than_func(Con_Obj *thread)
{
	Con_Obj *self, *o_obj;
	CON_UNPACK_ARGS("RN", &self, &o_obj);

	Con_Builtins_Float_Atom *self_float_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_FLOAT_ATOM_DEF_OBJECT));
	Con_Float o = Con_Numbers_Number_to_Con_Float(thread, o_obj);

	if (self_float_atom->val > o)
		return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
	else
		return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}



//
// '>=(o)'.
//

Con_Obj *_Con_Builtins_Float_Class_greater_than_equals_func(Con_Obj *thread)
{
	Con_Obj *self, *o_obj;
	CON_UNPACK_ARGS("RN", &self, &o_obj);

	Con_Builtins_Float_Atom *self_float_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_FLOAT_ATOM_DEF_OBJECT));
	Con_Float o = Con_Numbers_Number_to_Con_Float(thread, o_obj);

	if (self_float_atom->val >= o)
		return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
	else
		return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}



//
// '<(o)'.
//

Con_Obj *_Con_Builtins_Float_Class_less_than_func(Con_Obj *thread)
{
	Con_Obj *self, *o_obj;
	CON_UNPACK_ARGS("RN", &self, &o_obj);

	Con_Builtins_Float_Atom *self_float_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_FLOAT_ATOM_DEF_OBJECT));
	Con_Float o = Con_Numbers_Number_to_Con_Float(thread, o_obj);

	if (self_float_atom->val < o)
		return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
	else
		return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}



//
// '<=(o)'.
//

Con_Obj *_Con_Builtins_Float_Class_less_than_equals_func(Con_Obj *thread)
{
	Con_Obj *self, *o_obj;
	CON_UNPACK_ARGS("RN", &self, &o_obj);

	Con_Builtins_Float_Atom *self_float_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_FLOAT_ATOM_DEF_OBJECT));
	Con_Float o = Con_Numbers_Number_to_Con_Float(thread, o_obj);

	if (self_float_atom->val <= o)
		return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
	else
		return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}



//
// '+(o)'.
//

Con_Obj *_Con_Builtins_Float_Class_add_func(Con_Obj *thread)
{
	Con_Obj *self, *o_obj;
	CON_UNPACK_ARGS("RN", &self, &o_obj);

	Con_Builtins_Float_Atom *float_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_FLOAT_ATOM_DEF_OBJECT));
	Con_Float o = Con_Numbers_Number_to_Con_Float(thread, o_obj);

	return Con_Builtins_Float_Atom_new(thread, float_atom->val + o);
}



//
// '-(o)'.
//

Con_Obj *_Con_Builtins_Float_Class_subtract_func(Con_Obj *thread)
{
	Con_Obj *self, *o_obj;
	CON_UNPACK_ARGS("RN", &self, &o_obj);

	Con_Builtins_Float_Atom *float_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_FLOAT_ATOM_DEF_OBJECT));
	Con_Float o = Con_Numbers_Number_to_Con_Float(thread, o_obj);

	return Con_Builtins_Float_Atom_new(thread, float_atom->val - o);
}



//
// '/(o)'.
//

Con_Obj *_Con_Builtins_Float_Class_div_func(Con_Obj *thread)
{
	Con_Obj *self, *o_obj;
	CON_UNPACK_ARGS("RN", &self, &o_obj);

	Con_Builtins_Float_Atom *float_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_FLOAT_ATOM_DEF_OBJECT));
	Con_Float o = Con_Numbers_Number_to_Con_Float(thread, o_obj);

	if (o == 0)
		CON_XXX;

	return Con_Builtins_Float_Atom_new(thread, float_atom->val / o);
}



//
// '*(o)'.
//

Con_Obj *_Con_Builtins_Float_Class_mul_func(Con_Obj *thread)
{
	Con_Obj *self, *o_obj;
	CON_UNPACK_ARGS("RN", &self, &o_obj);

	Con_Builtins_Float_Atom *float_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_FLOAT_ATOM_DEF_OBJECT));
	Con_Float o = Con_Numbers_Number_to_Con_Float(thread, o_obj);

	return Con_Builtins_Float_Atom_new(thread, float_atom->val * o);
}



//
// 'sqrt()'.
//

Con_Obj *_Con_Builtins_Float_Class_sqrt_func(Con_Obj *thread)
{
	Con_Obj *self;
	CON_UNPACK_ARGS("R", &self);

	Con_Builtins_Float_Atom *float_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_FLOAT_ATOM_DEF_OBJECT));

	return Con_Builtins_Float_Atom_new(thread, sqrt(float_atom->val));
}
