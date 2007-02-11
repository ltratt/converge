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

#include "Bytecode.h"
#include "Core.h"
#include "Hash.h"
#include "Numbers.h"
#include "Object.h"
#include "Shortcuts.h"

#include "Builtins/Class/Atom.h"
#include "Builtins/Closure/Atom.h"
#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/Module/Class.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



Con_Obj *_Con_Builtins_Module_Class_to_str_func(Con_Obj *);
Con_Obj *_Con_Builtins_Module_Class_get_def_func(Con_Obj *);
Con_Obj *_Con_Builtins_Module_Class_get_slot_func(Con_Obj *);
Con_Obj *_Con_Builtins_Module_Class_def_names_func(Con_Obj *);
Con_Obj *_Con_Builtins_Module_Class_path_func(Con_Obj *);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_Module_Class_bootstrap(Con_Obj *thread)
{
	Con_Obj *module_class = CON_BUILTIN(CON_BUILTIN_MODULE_CLASS);
	
	Con_Builtins_Class_Atom *class_atom = (Con_Builtins_Class_Atom *) module_class->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (class_atom + 1);

	class_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;

	Con_Builtins_Class_Atom_init_atom(thread, class_atom, CON_NEW_STRING("Module"), NULL, CON_BUILTIN(CON_BUILTIN_OBJECT_CLASS), NULL);
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	
	Con_Memory_change_chunk_type(thread, module_class, CON_MEMORY_CHUNK_OBJ);
	
	CON_SET_FIELD(module_class, "to_str", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Module_Class_to_str_func, "to_str", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), module_class));
	CON_SET_FIELD(module_class, "def_names", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Module_Class_def_names_func, "def_names", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), module_class));
	CON_SET_FIELD(module_class, "get_def", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Module_Class_get_def_func, "get_def", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), module_class));
	CON_SET_FIELD(module_class, "get_slot", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Module_Class_get_slot_func, "get_slot", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), module_class));
	CON_SET_FIELD(module_class, "path", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Module_Class_path_func, "path", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), module_class));
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Module class fields
//

//
// 'to_str()'.
//

Con_Obj *_Con_Builtins_Module_Class_to_str_func(Con_Obj *thread)
{
	Con_Obj *self;
	CON_UNPACK_ARGS("M", &self);

	Con_Builtins_Module_Atom *self_module_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_MODULE_ATOM_DEF_OBJECT));
	
	CON_MUTEX_LOCK(&self->mutex);
	Con_Obj *identifier = self_module_atom->identifier;
	CON_MUTEX_UNLOCK(&self->mutex);
	
	Con_Obj *rtn = CON_NEW_STRING("<Module ");
	rtn = CON_ADD(rtn, identifier);
	rtn = CON_ADD(rtn, CON_NEW_STRING(">"));
	
	return rtn;
}



//
// 'def_names()' is a generator which returns the name of each definition in the module.
//

Con_Obj *_Con_Builtins_Module_Class_def_names_func(Con_Obj *thread)
{
	Con_Obj *self;
	CON_UNPACK_ARGS("M", &self);

	Con_Builtins_Module_Atom *module_atom = CON_FIND_ATOM(self, CON_BUILTIN(CON_BUILTIN_MODULE_ATOM_DEF_OBJECT));

	CON_MUTEX_LOCK(&self->mutex);
	Con_Obj *top_level_vars_map = module_atom->top_level_vars_map;
	CON_MUTEX_UNLOCK(&self->mutex);

	CON_PRE_GET_SLOT_APPLY_PUMP(top_level_vars_map, "keys");
	while (1) {
		Con_Obj *def_name = CON_APPLY_PUMP();
		if (def_name == NULL)
			break;
		CON_YIELD(def_name);
	}

	return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}



//
// 'get_def(name)' returns the definition 'name' in this module.
//

Con_Obj *_Con_Builtins_Module_Class_get_def_func(Con_Obj *thread)
{
	Con_Obj *name_obj, *self;
	CON_UNPACK_ARGS("MS", &self, &name_obj);

	Con_Builtins_String_Atom *name_string_atom = CON_GET_ATOM(name_obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	
	return Con_Builtins_Module_Atom_get_definition(thread, self, name_string_atom->str, name_string_atom->size);
}



//
// 'get_slot(name)'.
//

Con_Obj *_Con_Builtins_Module_Class_get_slot_func(Con_Obj *thread)
{
	Con_Obj *self, *slot_name;
	CON_UNPACK_ARGS("MO", &self, &slot_name);

	Con_Builtins_Module_Atom *self_module_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_MODULE_ATOM_DEF_OBJECT));

	if (CON_C_STRING_EQ("module_id", slot_name)) {
		return self_module_atom->identifier;
	}
	else
		return CON_APPLY(CON_EXBI(CON_BUILTIN(CON_BUILTIN_OBJECT_CLASS), "get_slot", self), slot_name);
}



//
// 'path(stop_at)' returns the path (including containers up to, but including, stop_at) of this
// function.
//

Con_Obj *_Con_Builtins_Module_Class_path_func(Con_Obj *thread)
{
	Con_Obj *self, *stop_at;
	CON_UNPACK_ARGS("MO", &self, &stop_at);
	
	if (self == stop_at)
		return CON_NEW_STRING("");

	Con_Builtins_Module_Atom *self_module_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_MODULE_ATOM_DEF_OBJECT));
	
	CON_MUTEX_LOCK(&self->mutex);
	Con_Obj *name = self_module_atom->name;
	Con_Obj *container = self_module_atom->container;
	CON_MUTEX_UNLOCK(&self->mutex);
	
	if ((container == CON_BUILTIN(CON_BUILTIN_NULL_OBJ)) || (container == stop_at))
		return name;
	else {
		Con_Obj *rtn = CON_GET_SLOT_APPLY(container, "path", stop_at);
		if (CON_GET_SLOT_APPLY_NO_FAIL(CON_BUILTIN(CON_BUILTIN_MODULE_CLASS), "instantiated", container) != NULL)
			rtn = CON_ADD(rtn, CON_NEW_STRING("::"));
		else
			rtn = CON_ADD(rtn, CON_NEW_STRING("."));
		rtn = CON_ADD(rtn, name);
		
		return rtn;
	}
}
