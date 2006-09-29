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
#include "Object.h"
#include "Shortcuts.h"

#include "Builtins/Atom_Def_Atom.h"
#include "Builtins/Class/Atom.h"
#include "Builtins/Closure/Atom.h"
#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Partial_Application/Atom.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



void _Con_Builtins_Partial_Application_Class_gc_scan_func(Con_Obj *, Con_Obj *, Con_Atom *);

Con_Obj *_Con_Builtins_Partial_Application_Class_apply_func(Con_Obj *);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_Partial_Application_Atom_bootstrap(Con_Obj *thread)
{
	Con_Obj *partial_application_atom_def = CON_BUILTIN(CON_BUILTIN_PARTIAL_APPLICATION_ATOM_DEF_OBJECT);
	
	Con_Builtins_Atom_Def_Atom *atom_def_atom = (Con_Builtins_Atom_Def_Atom *) partial_application_atom_def->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (atom_def_atom + 1);

	atom_def_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;
	
	Con_Builtins_Atom_Def_Atom_init_atom(thread, atom_def_atom, _Con_Builtins_Partial_Application_Class_gc_scan_func, NULL);
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	
	Con_Memory_change_chunk_type(thread, partial_application_atom_def, CON_MEMORY_CHUNK_OBJ);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions
//

//
// Push the partial applications' arguments onto 'con_stack'; returns the number of arguments pushed.
//

Con_Int Con_Builtins_Partial_Application_Atom_push_args_onto_stack(Con_Obj *thread, Con_Obj *partial_application, Con_Obj *con_stack, Con_Int num_objs_deep)
{
	Con_Builtins_Partial_Application_Atom *partial_application_atom = CON_GET_ATOM(partial_application, CON_BUILTIN(CON_BUILTIN_PARTIAL_APPLICATION_ATOM_DEF_OBJECT));
	
	CON_MUTEX_LOCK(&con_stack->mutex);
	for (Con_Int i = 0; i < partial_application_atom->num_args; i += 1)
		Con_Builtins_Con_Stack_Atom_push_n_object(thread, con_stack, partial_application_atom->args[i], num_objs_deep + i);
	CON_MUTEX_UNLOCK(&con_stack->mutex);
	
	return partial_application_atom->num_args;
}



//
// Get the partial applicatios function.
//

Con_Obj *Con_Builtins_Partial_Application_Atom_get_func(Con_Obj *thread, Con_Obj *partial_application)
{
	return ((Con_Builtins_Partial_Application_Atom *) CON_GET_ATOM(partial_application, CON_BUILTIN(CON_BUILTIN_PARTIAL_APPLICATION_ATOM_DEF_OBJECT)))->func;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Garbage collection
//

void _Con_Builtins_Partial_Application_Class_gc_scan_func(Con_Obj *thread, Con_Obj *obj, Con_Atom *atom)
{
	Con_Builtins_Partial_Application_Atom *partial_application_atom = (Con_Builtins_Partial_Application_Atom *) atom;
	
	Con_Memory_gc_push(thread, partial_application_atom->func);
	for (Con_Int i = 0; i < partial_application_atom->num_args; i += 1)
		Con_Memory_gc_push(thread, partial_application_atom->args[i]);
}
