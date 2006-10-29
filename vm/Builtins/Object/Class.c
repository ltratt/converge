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
#include "Object.h"
#include "Shortcuts.h"

#include "Builtins/Atom_Def_Atom.h"
#include "Builtins/Class/Atom.h"
#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Object/Class.h"
#include "Builtins/Partial_Application/Class.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



Con_Obj *_Con_Builtins_Object_Class_new_object(Con_Obj *);

Con_Obj *_Con_Builtins_Object_Class_init_func(Con_Obj *);
Con_Obj *_Con_Builtins_Object_Class_get_slot_func(Con_Obj *);
Con_Obj *_Con_Builtins_Object_Class_find_slot_func(Con_Obj *);
Con_Obj *_Con_Builtins_Object_Class_to_str_func(Con_Obj *);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_Object_Class_bootstrap(Con_Obj *thread)
{
	Con_Obj *object_class = CON_BUILTIN(CON_BUILTIN_OBJECT_CLASS);
	
	Con_Builtins_Class_Atom *class_atom = (Con_Builtins_Class_Atom *) object_class->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (class_atom + 1);

	class_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;
	
	Con_Builtins_Class_Atom_init_atom(thread, class_atom, CON_NEW_STRING("Object"), CON_NEW_UNBOUND_C_FUNC(_Con_Builtins_Object_Class_new_object, "Object_new", CON_BUILTIN(CON_BUILTIN_NULL_OBJ)), NULL);
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	
	Con_Memory_change_chunk_type(thread, object_class, CON_MEMORY_CHUNK_OBJ);
	
	CON_SET_FIELD(object_class, "init", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Object_Class_init_func, "init", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), object_class));
	CON_SET_FIELD(object_class, "get_slot", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Object_Class_get_slot_func, "get_slot", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), object_class));
	CON_SET_FIELD(object_class, "find_slot", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Object_Class_find_slot_func, "find_slot", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), object_class));
	CON_SET_FIELD(object_class, "to_str", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Object_Class_to_str_func, "to_str", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), object_class));
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// func Object_new
//

Con_Obj *_Con_Builtins_Object_Class_new_object(Con_Obj *thread)
{
	Con_Obj *class_, *var_args;
	CON_UNPACK_ARGS("Ov", &class_, &var_args);
	
	Con_Obj *new_obj = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(Con_Builtins_Slots_Atom), class_);
	
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) new_obj->first_atom;
	slots_atom->next_atom = NULL;
	
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);

	Con_Memory_change_chunk_type(thread, new_obj, CON_MEMORY_CHUNK_OBJ);
	
	CON_GET_SLOT_APPLY(CON_GET_SLOT(new_obj, "init"), "apply", var_args);
	
	return new_obj;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Object class
//

//
// 'init(...)' is effectively a no-op.
//

Con_Obj *_Con_Builtins_Object_Class_init_func(Con_Obj *thread)
{
	Con_Obj *self, *var_args;
	CON_UNPACK_ARGS("Ov", &self, &var_args);

	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'find_slot(name, caller)' returns the slot named 'name' in 'self'.
//

Con_Obj *_Con_Builtins_Object_Class_get_slot_func(Con_Obj *thread)
{
	// This 'get_slot' function is rather unusual: as the "root" get_slot, it has a deep knowledge of
	// how this functionality works in Converge.

	Con_Obj *self, *slot_name;
	CON_UNPACK_ARGS("OS", &self, &slot_name);
	
	Con_Builtins_String_Atom *slot_name_string_atom = CON_GET_ATOM(slot_name, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	assert(slot_name_string_atom->encoding == CON_STR_UTF_8);

	CON_MUTEX_LOCK(&self->mutex);
	Con_Obj *slot_val = Con_Object_get_slot_no_custom(thread, self, slot_name_string_atom->str, slot_name_string_atom->size);
	CON_MUTEX_UNLOCK(&self->mutex);
	
	if ((CON_FIND_ATOM(slot_val, CON_BUILTIN(CON_BUILTIN_FUNC_ATOM_DEF_OBJECT)) != NULL) && Con_Builtins_Func_Atom_is_bound(thread, slot_val))
		slot_val = Con_Builtins_Partial_Application_Class_new(thread, slot_val, self, NULL);
	
	return slot_val;
}



//
// 'get_slot(name, caller)' returns the slot named 'name' in 'self'.
//

Con_Obj *_Con_Builtins_Object_Class_find_slot_func(Con_Obj *thread)
{
	// This 'get_slot' function is rather unusual: as the "root" get_slot, it has a deep knowledge of
	// how this functionality works in Converge.

	Con_Obj *self, *slot_name;
	CON_UNPACK_ARGS("OS", &self, &slot_name);
	
	Con_Builtins_String_Atom *slot_name_string_atom = CON_GET_ATOM(slot_name, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	assert(slot_name_string_atom->encoding == CON_STR_UTF_8);

	CON_MUTEX_LOCK(&self->mutex);
	Con_Obj *slot_val = Con_Object_find_slot_no_custom(thread, self, slot_name_string_atom->str, slot_name_string_atom->size);
	CON_MUTEX_UNLOCK(&self->mutex);
	
	if (slot_val == NULL)
		slot_val = CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
	else {	
		if ((CON_FIND_ATOM(slot_val, CON_BUILTIN(CON_BUILTIN_FUNC_ATOM_DEF_OBJECT)) != NULL) && Con_Builtins_Func_Atom_is_bound(thread, slot_val))
			slot_val = Con_Builtins_Partial_Application_Class_new(thread, slot_val, self, NULL);
	}
	
	return slot_val;
}



//
// 'to_str()'.
//

Con_Obj *_Con_Builtins_Object_Class_to_str_func(Con_Obj *thread)
{
	Con_Obj *self;
	CON_UNPACK_ARGS("O", &self);

#	define TO_STR_WIDTH ((Con_Int) (sizeof(void*) * 2 + sizeof("<Object@0x>") + 1))

	char str[TO_STR_WIDTH];
	if (snprintf(str, TO_STR_WIDTH, "<Object@%p>", self) >= TO_STR_WIDTH)
		CON_FATAL_ERROR("Unable to print object ID.");

	size_t str_size = strlen(str);

	return Con_Builtins_String_Atom_new_copy(thread, str, str_size, CON_STR_UTF_8);
}
