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

#include <stdarg.h>
#include <string.h>

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
#include "Builtins/Partial_Application/Atom.h"
#include "Builtins/Partial_Application/Class.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"


Con_Obj *_Con_Builtins_Partial_Application_Class_get_slot_func(Con_Obj *);

Con_Obj *_Con_Builtins_Partial_Application_Class_apply_func(Con_Obj *);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_Partial_Application_Class_bootstrap(Con_Obj *thread)
{
	Con_Obj *partial_application_class = CON_BUILTIN(CON_BUILTIN_PARTIAL_APPLICATION_CLASS);
	
	Con_Builtins_Class_Atom *class_atom = (Con_Builtins_Class_Atom *) partial_application_class->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (class_atom + 1);

	class_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;

	Con_Builtins_Class_Atom_init_atom(thread, class_atom, CON_NEW_STRING("Partial_Application"), NULL, CON_BUILTIN(CON_BUILTIN_OBJECT_CLASS), NULL);
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	
	Con_Memory_change_chunk_type(thread, partial_application_class, CON_MEMORY_CHUNK_OBJ);
	
	CON_SET_FIELD(partial_application_class, "get_slot", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Partial_Application_Class_get_slot_func, "get_slot", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), partial_application_class));
	
	CON_SET_FIELD(partial_application_class, "apply", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Partial_Application_Class_apply_func, "apply", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), partial_application_class));
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Partial application creation functions
//

Con_Obj *Con_Builtins_Partial_Application_Class_new(Con_Obj *thread, Con_Obj *func, ...)
{
	if (CON_FIND_ATOM(func, CON_BUILTIN(CON_BUILTIN_PARTIAL_APPLICATION_ATOM_DEF_OBJECT)) != NULL)
		CON_XXX;

	Con_Int num_args = 0;
	va_list ap;
	va_start(ap, func);
	while (va_arg(ap, Con_Obj *) != NULL)
		num_args += 1;
	va_end(ap);

	Con_Obj *new_partial_application = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(Con_Builtins_Partial_Application_Atom) + sizeof(Con_Obj *) * num_args, CON_BUILTIN(CON_BUILTIN_PARTIAL_APPLICATION_CLASS));
	new_partial_application->first_atom->next_atom = NULL;
	
	Con_Builtins_Partial_Application_Atom *partial_application_atom = (Con_Builtins_Partial_Application_Atom *) new_partial_application->first_atom;
	partial_application_atom->atom_type = CON_BUILTIN(CON_BUILTIN_PARTIAL_APPLICATION_ATOM_DEF_OBJECT);
	partial_application_atom->num_args = num_args;
	partial_application_atom->func = func;
	
	va_start(ap, func);
	Con_Obj *arg;
	Con_Int i = 0;
	while ((arg = va_arg(ap, Con_Obj *)) != NULL) {
		partial_application_atom->args[i] = arg;
	}
	va_end(ap);
	
	Con_Memory_change_chunk_type(thread, new_partial_application, CON_MEMORY_CHUNK_OBJ);
	
	return new_partial_application;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Partial_Application class fields
//

//
// get_slot(name)
//

Con_Obj *_Con_Builtins_Partial_Application_Class_get_slot_func(Con_Obj *thread)
{
	Con_Obj *self, *slot_name;
	CON_UNPACK_ARGS("US", CON_BUILTIN(CON_BUILTIN_PARTIAL_APPLICATION_ATOM_DEF_OBJECT), &self, &slot_name);
	
	Con_Builtins_Partial_Application_Atom *partial_application_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_PARTIAL_APPLICATION_ATOM_DEF_OBJECT));

	if (CON_C_STRING_EQ("func_", slot_name)) {
		CON_MUTEX_LOCK(&self->mutex);
		Con_Obj *func = partial_application_atom->func;
		CON_MUTEX_UNLOCK(&self->mutex);
		return func;
	}
	else
		return CON_APPLY(CON_EXBI(CON_BUILTIN(CON_BUILTIN_OBJECT_CLASS), "get_slot", self), slot_name);
}



//
// apply(args_list) applies the list of argument 'args_list' to the ones already applied to
// 'self.func_'.
//

Con_Obj *_Con_Builtins_Partial_Application_Class_apply_func(Con_Obj *thread)
{
	Con_Obj *self, *args_list;
	CON_UNPACK_ARGS("OO", &self, &args_list);

	Con_Builtins_Partial_Application_Atom *partial_application_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_PARTIAL_APPLICATION_ATOM_DEF_OBJECT));

	Con_Obj *con_stack = Con_Builtins_Thread_Atom_get_con_stack(thread);
	
	CON_MUTEX_LOCK(&con_stack->mutex);
	Con_Int num_args = 0;
	for (Con_Int i = 0; i < partial_application_atom->num_args; i += 1) {
		Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, partial_application_atom->args[i]);
		num_args += 1;
	}
	CON_MUTEX_UNLOCK(&con_stack->mutex);
		
	Con_Int list_size = Con_Numbers_Number_to_Con_Int(thread, CON_GET_SLOT_APPLY(args_list, "len"));
	for (Con_Int i = 0; i < list_size; i += 1) {
		Con_Obj *arg = CON_GET_SLOT_APPLY(args_list, "get", CON_NEW_INT(i));
		CON_MUTEX_LOCK(&con_stack->mutex);
		Con_Builtins_Con_Stack_Atom_push_object(thread, con_stack, arg);
		CON_MUTEX_UNLOCK(&con_stack->mutex);
		num_args += 1;
	}

	Con_Int num_closure_vars = Con_Builtins_Func_Atom_get_num_closure_vars(thread, partial_application_atom->func);
	Con_Obj *container_closure = Con_Builtins_Func_Atom_get_container_closure(thread, partial_application_atom->func);
	Con_Obj *closure = NULL;
	if ((num_closure_vars > 0) || (container_closure != NULL))
		closure = Con_Builtins_Closure_Atom_new(thread, num_closure_vars, container_closure);

	Con_Builtins_VM_Atom_pre_apply_pump(thread, partial_application_atom->func, num_args, closure, Con_Builtins_Func_Atom_make_con_pc_null(thread));
	while (1) {
		Con_Obj *val = CON_APPLY_PUMP();
		if (val == NULL)
			break;
		CON_YIELD(val);
	}
	
	return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}
