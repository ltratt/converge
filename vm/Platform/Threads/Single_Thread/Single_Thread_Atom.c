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
#ifdef CON_HAVE_NATIVE_ERR
#include <err.h>
#else
#include "Platform/err.h"
#endif
#include <string.h>

#include "Arch.h"
#include "Memory.h"
#include "Object.h"
#include "Shortcuts.h"

#include "Builtins/Atom_Def_Atom.h"
#include "Builtins/Class/Atom.h"
#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



void _Con_Builtins_Thread_Class_gc_scan_func(Con_Obj *, Con_Obj *, Con_Atom *);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_Thread_Atom_bootstrap(Con_Obj *thread)
{
	Con_Obj *thread_atom_def = CON_BUILTIN(CON_BUILTIN_THREAD_ATOM_DEF_OBJECT);
	Con_Builtins_Atom_Def_Atom *atom_def_atom = (Con_Builtins_Atom_Def_Atom *) thread_atom_def->first_atom;

	atom_def_atom->next_atom = NULL;
	
	Con_Builtins_Atom_Def_Atom_init_atom(thread, atom_def_atom, _Con_Builtins_Thread_Class_gc_scan_func, NULL);
	
	Con_Memory_change_chunk_type(thread, thread_atom_def, CON_MEMORY_CHUNK_OBJ);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Thread / object creation / initialization functions
//

Con_Obj *Con_Builtins_Thread_Atom_new_from_self(Con_Obj *thread, u_char *c_stack_start, Con_Obj *vm)
{
	Con_Obj *thread_obj = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(Con_Builtins_Thread_Atom) + sizeof(Con_Builtins_Slots_Atom), CON_BUILTIN(CON_BUILTIN_THREAD_CLASS));
	Con_Builtins_Thread_Atom *thread_atom = (Con_Builtins_Thread_Atom *) thread_obj->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (thread_atom + 1);
	
	thread_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;

	thread_atom->atom_type = CON_BUILTIN(CON_BUILTIN_THREAD_ATOM_DEF_OBJECT);
	thread_atom->c_stack_start = c_stack_start;
	thread_atom->vm = vm;
	thread_atom->con_stack = Con_Builtins_Con_Stack_Atom_new(thread);
	thread_atom->current_exception = NULL;

	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	
	Con_Memory_change_chunk_type(thread, thread_obj, CON_MEMORY_CHUNK_OBJ);
		
	return thread_obj;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions
//

Con_Obj *Con_Builtins_Thread_Atom_get_vm(Con_Obj *thread)
{
	return ((Con_Builtins_Thread_Atom *) thread->first_atom)->vm;
}



Con_Obj *Con_Builtins_Thread_Atom_get_con_stack(Con_Obj *thread)
{
	return ((Con_Builtins_Thread_Atom *) thread->first_atom)->con_stack;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Garbage collection
//

void _Con_Builtins_Thread_Class_gc_scan_func(Con_Obj *thread, Con_Obj *obj, Con_Atom *atom)
{
	Con_Builtins_Thread_Atom *thread_atom = (Con_Builtins_Thread_Atom *) atom;

	Con_Memory_gc_push(thread, thread_atom->vm);
	Con_Memory_gc_push(thread, thread_atom->con_stack);
	if (thread_atom->current_exception != NULL)
		Con_Memory_gc_push(thread, thread_atom->current_exception);

#ifdef CON_C_STACK_GROWS_DOWN
	u_char *c_stack_bottom;
	CON_ARCH_GET_STACKP(c_stack_bottom);
	Con_Memory_gc_scan_conservative(thread, c_stack_bottom, thread_atom->c_stack_start - c_stack_bottom);
#else
	XXX
#endif
}
