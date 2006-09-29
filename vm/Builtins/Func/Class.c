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

#include "Core.h"
#include "Numbers.h"
#include "Object.h"
#include "Shortcuts.h"

#include "Builtins/Atom_Def_Atom.h"
#include "Builtins/Class/Atom.h"
#include "Builtins/Closure/Atom.h"
#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Func/Class.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



Con_Obj *_Con_Builtins_Func_Class_apply_func(Con_Obj *);
Con_Obj *_Con_Builtins_Func_Class_path_func(Con_Obj *);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_Func_Class_bootstrap(Con_Obj *thread)
{
	Con_Obj *func_class = CON_BUILTIN(CON_BUILTIN_FUNC_CLASS);

	Con_Builtins_Class_Atom *class_atom = (Con_Builtins_Class_Atom *) func_class->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (class_atom + 1);

	class_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;
	
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	Con_Builtins_Class_Atom_init_atom(thread, class_atom, CON_NEW_STRING("Func"), NULL, CON_BUILTIN(CON_BUILTIN_OBJECT_CLASS), NULL);
	
	Con_Memory_change_chunk_type(thread, func_class, CON_MEMORY_CHUNK_OBJ);
	
	CON_SET_FIELD(func_class, "apply", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Func_Class_apply_func, "apply", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), func_class));
	CON_SET_FIELD(func_class, "path", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Func_Class_path_func, "path", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), func_class));
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Func class fields
//

//
// 'path(stop_at)' returns the path (including containers up to, but including, stop_at) of this
// function.
//

Con_Obj *_Con_Builtins_Func_Class_apply_func(Con_Obj *thread)
{
	Con_Obj *self, *args_list;
	CON_UNPACK_ARGS("FO", &self, &args_list);

	Con_Builtins_Func_Atom *func_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_FUNC_ATOM_DEF_OBJECT));

	Con_Obj *con_stack = Con_Builtins_Thread_Atom_get_con_stack(thread);
	
	Con_Int args_len = Con_Numbers_Number_to_Con_Int(thread, CON_GET_SLOT_APPLY(args_list, "len"));
	for (Con_Int i = 0; i < args_len; i += 1) {
		Con_Obj *arg = CON_GET_SLOT_APPLY(args_list, "get", CON_NEW_INT(i));
		CON_MUTEX_LOCK(&con_stack->mutex);
		Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, arg);
		CON_MUTEX_UNLOCK(&con_stack->mutex);
	}

	Con_Obj *closure = NULL;
	if ((func_atom->num_closure_vars > 0) || (func_atom->container_closure != NULL))
		closure = Con_Builtins_Closure_Atom_new(thread, func_atom->num_closure_vars, func_atom->container_closure);

	Con_Builtins_VM_Atom_pre_apply_pump(thread, self, args_len, closure, Con_Builtins_Func_Atom_make_con_pc_null(thread));
	while (1) {
		Con_Obj *val = CON_APPLY_PUMP();
		if (val == NULL)
			break;
		CON_YIELD(val);
	}
	
	CON_RETURN(CON_BUILTIN(CON_BUILTIN_FAIL_OBJ));
}



//
// 'path(stop_at)' returns the path (including containers up to, but including, stop_at) of this
// function.
//

Con_Obj *_Con_Builtins_Func_Class_path_func(Con_Obj *thread)
{
	Con_Obj *self, *stop_at;
	CON_UNPACK_ARGS("FO", &self, &stop_at);

	if (self == stop_at)
		CON_RETURN(CON_NEW_STRING(""));

	Con_Obj *container = CON_GET_SLOT(self, "container");

	if ((container == CON_BUILTIN(CON_BUILTIN_NULL_OBJ)) || (container == stop_at))
		CON_RETURN(CON_GET_SLOT(self, "name"));
	else {
		Con_Obj *rtn = CON_GET_SLOT_APPLY(container, "path", stop_at);
		rtn = CON_ADD(rtn, CON_NEW_STRING("."));
		rtn = CON_ADD(rtn, CON_GET_SLOT(self, "name"));
		
		CON_RETURN(rtn);
	}
}
