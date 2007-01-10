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

#include <limits.h>
#include <string.h>

#include "Core.h"
#include "Hash.h"
#include "Object.h"
#include "Shortcuts.h"

#include "Builtins/Atom_Def_Atom.h"
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



void _Con_Builtins_Exception_Class_gc_scan_func(Con_Obj *, Con_Obj *, Con_Atom *);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_Exception_Atom_bootstrap(Con_Obj *thread)
{
	Con_Obj *exception_atom_def = CON_BUILTIN(CON_BUILTIN_EXCEPTION_ATOM_DEF_OBJECT);
	
	Con_Builtins_Atom_Def_Atom *atom_def_atom = (Con_Builtins_Atom_Def_Atom *) exception_atom_def->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (atom_def_atom + 1);

	atom_def_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;
	
	Con_Builtins_Atom_Def_Atom_init_atom(thread, atom_def_atom, _Con_Builtins_Exception_Class_gc_scan_func, NULL);
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	
	Con_Memory_change_chunk_type(thread, exception_atom_def, CON_MEMORY_CHUNK_OBJ);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions
//

//
// Return a string object representing the error 'errnum'.
//

Con_Obj *Con_Builtins_Exception_Atom_strerror(Con_Obj *thread, int errnum)
{
#	ifndef NL_TEXTMAX
#		define NL_TEXTMAX 512
#	endif

#	if STRERROR_R_CHAR_P
	char strerrbuf_tmp[NL_TEXTMAX];
	char *strerrbuf;
	strerrbuf = strerror_r(errnum, strerrbuf_tmp, NL_TEXTMAX);
#	else
	char strerrbuf[NL_TEXTMAX];
	if (strerror_r(errnum, strerrbuf, NL_TEXTMAX) != 0)
		CON_XXX;
#	endif
	
	return Con_Builtins_String_Atom_new_copy(thread, (u_char *) strerrbuf, strlen(strerrbuf), CON_STR_UTF_8);
}



//
// Set the call chain of this exception. 'call_chain' should be a pointer to a memory chunk
// containing 'num_call_chain_entries' entries.
//

void Con_Builtins_Exception_Atom_set_call_chain(Con_Obj *thread, Con_Obj *exception, Con_Builtins_Exception_Class_Call_Chain_Entry *call_chain, Con_Int num_call_chain_entries)
{
	Con_Builtins_Exception_Atom *exception_atom = CON_GET_ATOM(exception, CON_BUILTIN(CON_BUILTIN_EXCEPTION_ATOM_DEF_OBJECT));
	
	CON_MUTEX_LOCK(&exception->mutex);
	exception_atom->call_chain = call_chain;
	exception_atom->num_call_chain_entries = num_call_chain_entries;
	CON_MUTEX_UNLOCK(&exception->mutex);
}



#ifndef CON_THREADS_SINGLE_THREADED
//
// Set the thread that initially raised this exception to 'exception_thread'. Note that this may be
// a different thread than both the thread that created the exception object, and the current running
// thread.
//

void Con_Builtins_Exception_Atom_set_exception_thread(Con_Obj *thread, Con_Obj *exception, Con_Obj *exception_thread)
{
	Con_Builtins_Exception_Atom *exception_atom = CON_GET_ATOM(exception, CON_BUILTIN(CON_BUILTIN_EXCEPTION_ATOM_DEF_OBJECT));

	CON_MUTEX_LOCK(&exception->mutex);
	exception_atom->exception_thread = exception_thread;
	CON_MUTEX_UNLOCK(&exception->mutex);
}
#endif



////////////////////////////////////////////////////////////////////////////////////////////////////
// Garbage collection
//

void _Con_Builtins_Exception_Class_gc_scan_func(Con_Obj *thread, Con_Obj *obj, Con_Atom *atom)
{
	Con_Builtins_Exception_Atom *exception_atom = (Con_Builtins_Exception_Atom *) atom;

	if (exception_atom->call_chain != NULL)
		Con_Memory_gc_push(thread, exception_atom->call_chain);
	
	for (int i = 0; i < exception_atom->num_call_chain_entries; i += 1) {
		Con_Memory_gc_push(thread, exception_atom->call_chain[i].func);
		Con_Memory_gc_scan_pc(thread, exception_atom->call_chain[i].pc);
	}

#	ifndef CON_THREADS_SINGLE_THREADED
	if (exception_atom->exception_thread != NULL)
		Con_Memory_gc_push(thread, exception_atom->exception_thread);
#	endif
}
