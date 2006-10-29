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
#include "Object.h"
#include "Shortcuts.h"

#include "Builtins/Class/Atom.h"
#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Exception/Atom.h"
#include "Builtins/Exception/Class.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



Con_Obj *_Con_Builtins_Exception_Class_new_object(Con_Obj *);

Con_Obj *_Con_Builtins_Exception_Class_init_func(Con_Obj *);
Con_Obj *_Con_Builtins_Exception_Class_to_str_func(Con_Obj *);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_Exception_Class_bootstrap(Con_Obj *thread)
{
	// Exception class

	Con_Obj *exception_class = CON_BUILTIN(CON_BUILTIN_EXCEPTION_CLASS);
	
	Con_Builtins_Class_Atom *class_atom = (Con_Builtins_Class_Atom *) exception_class->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (class_atom + 1);

	class_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;

	Con_Builtins_Class_Atom_init_atom(thread, class_atom, CON_NEW_STRING("Exception"), 	CON_NEW_UNBOUND_C_FUNC(_Con_Builtins_Exception_Class_new_object, "Exception_new", CON_BUILTIN(CON_BUILTIN_NULL_OBJ)), CON_BUILTIN(CON_BUILTIN_OBJECT_CLASS), NULL);	
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);

	Con_Memory_change_chunk_type(thread, exception_class, CON_MEMORY_CHUNK_OBJ);
	
	CON_SET_FIELD(exception_class, "init", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Exception_Class_init_func, "init", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), exception_class));
	CON_SET_FIELD(exception_class, "to_str", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Exception_Class_to_str_func, "to_str", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), exception_class));
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// func Exception_new
//

Con_Obj *_Con_Builtins_Exception_Class_new_object(Con_Obj *thread)
{
	Con_Obj *self, *var_args;
	CON_UNPACK_ARGS("Cv", &self, &var_args);
	
	Con_Obj *new_exception = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(Con_Builtins_Slots_Atom) + sizeof(Con_Builtins_Exception_Atom), self);
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) new_exception->first_atom;
	Con_Builtins_Exception_Atom *exception_atom = (Con_Builtins_Exception_Atom *) (slots_atom + 1);
	slots_atom->next_atom = (Con_Atom *) exception_atom;
	exception_atom->next_atom = NULL;
	
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);

	exception_atom->atom_type = CON_BUILTIN(CON_BUILTIN_EXCEPTION_ATOM_DEF_OBJECT);
	exception_atom->call_chain = NULL;
	exception_atom->num_call_chain_entries = 0;
#	ifndef CON_THREADS_SINGLE_THREADED
	exception_atom->exception_thread = NULL;
#	endif

	Con_Memory_change_chunk_type(thread, new_exception, CON_MEMORY_CHUNK_OBJ);
	
	CON_GET_SLOT_APPLY(CON_GET_SLOT(new_exception, "init"), "apply", var_args);
	
	return new_exception;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Exception class fields
//

//
// 'init(msg := "")'.
//

Con_Obj *_Con_Builtins_Exception_Class_init_func(Con_Obj *thread)
{
	Con_Obj *msg, *self;
	CON_UNPACK_ARGS("O;O", &self, &msg);

	if (msg == NULL)
		CON_SET_SLOT(self, "msg", CON_NEW_STRING(""));
	else
		CON_SET_SLOT(self, "msg", msg);

	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'to_str()'.
//

Con_Obj *_Con_Builtins_Exception_Class_to_str_func(Con_Obj *thread)
{
	Con_Obj *self;
	CON_UNPACK_ARGS("O", &self);

	// Format the output as '<self.name>: <self.msg>'.
	
	return CON_ADD(CON_ADD(CON_GET_SLOT(CON_GET_SLOT(self, "instance_of"), "name"), CON_NEW_STRING(": ")), CON_GET_SLOT(self, "msg"));
}
