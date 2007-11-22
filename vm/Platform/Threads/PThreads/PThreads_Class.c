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
#include <pthread.h>
#include <string.h>
#include <stdarg.h>

#include "Memory.h"
#include "Object.h"
#include "Shortcuts.h"

#include "Builtins/Atom_Def_Atom.h"
#include "Builtins/Class/Atom.h"
#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/Thread/Class.h"
#include "Builtins/Unique_Atom_Def.h"
#include "Builtins/VM/Atom.h"



Con_Obj *_Con_Builtins_Thread_Class_new_object(Con_Obj *);

Con_Obj *_Con_Builtins_Thread_Class_start_func(Con_Obj *);
Con_Obj *_Con_Builtins_Thread_Class_join_func(Con_Obj *);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_Thread_Class_bootstrap(Con_Obj *thread)
{
	// Exception class

	Con_Obj *thread_class = CON_BUILTIN(CON_BUILTIN_THREAD_CLASS);
	Con_Builtins_Class_Atom *class_atom = (Con_Builtins_Class_Atom *) thread_class->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (class_atom + 1);

	class_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;
	
	Con_Builtins_Class_Atom_init_atom(thread, class_atom, CON_NEW_STRING("Thread"), CON_NEW_UNBOUND_C_FUNC(_Con_Builtins_Thread_Class_new_object, "Thread_new", CON_BUILTIN(CON_BUILTIN_NULL_OBJ)), CON_BUILTIN(CON_BUILTIN_OBJECT_CLASS), NULL);
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);

	Con_Memory_change_chunk_type(thread, thread_class, CON_MEMORY_CHUNK_OBJ);
	
	CON_SET_FIELD(thread_class, "start", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Thread_Class_start_func, "start", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), thread_class));
	CON_SET_FIELD(thread_class, "join", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Thread_Class_join_func, "join", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), thread_class));
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// func Thread_new
//

Con_Obj *_Con_Builtins_Thread_Class_new_object(Con_Obj *thread)
{
	Con_Obj *class_, *start_func;
	CON_UNPACK_ARGS("CO", &class_, &start_func);

	Con_Obj *thread_obj = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(Con_Builtins_Thread_Atom) + sizeof(Con_Builtins_Slots_Atom), class_);
	Con_Builtins_Thread_Atom *thread_atom = (Con_Builtins_Thread_Atom *) thread_obj->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (thread_atom + 1);
	
	thread_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;

	thread_atom->atom_type = CON_BUILTIN(CON_BUILTIN_THREAD_ATOM_DEF_OBJECT);
	thread_atom->start_func = start_func;
	thread_atom->c_stack_start = NULL;
	thread_atom->vm = Con_Builtins_Thread_Atom_get_vm(thread);
	thread_atom->con_stack = Con_Builtins_Con_Stack_Atom_new(thread);
	thread_atom->state = CON_BUILTINS_THREAD_CLASS_INITIALIZED;
	thread_atom->current_exception = NULL;

	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	
	Con_Memory_change_chunk_type(thread, thread_obj, CON_MEMORY_CHUNK_OBJ);
	
	Con_Builtins_Thread_Atom_add_thread(thread, thread_obj);
		
	return thread_obj;
}




////////////////////////////////////////////////////////////////////////////////////////////////////
// Thread class fields

//
// 'Thread.start()' creates a new thread and executes 'start_func'.
//

void *_Con_Builtins_Thread_Class_start_func_start_routine(void *);

Con_Obj *_Con_Builtins_Thread_Class_start_func(Con_Obj *thread)
{
	Con_Obj *self;
	CON_UNPACK_ARGS("T", &self);

	Con_Builtins_Thread_Atom *self_thread_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_THREAD_ATOM_DEF_OBJECT));
	
	CON_MUTEX_LOCK(&self->mutex);
	// A thread can only be started once.
	if (self_thread_atom->state != CON_BUILTINS_THREAD_CLASS_INITIALIZED)
		CON_XXX;
	CON_MUTEX_UNLOCK(&self->mutex);

	// Create the thread. We don't need to record the thread ID at this point.

	pthread_t dummy_thread_id;
	pthread_create(&dummy_thread_id, NULL, _Con_Builtins_Thread_Class_start_func_start_routine, self);

	while (1) {
		CON_MUTEX_LOCK(&self->mutex);
		if (self_thread_atom->state != CON_BUILTINS_THREAD_CLASS_INITIALIZED) {
			CON_MUTEX_UNLOCK(&self->mutex);
			break;
		}
		CON_MUTEX_UNLOCK(&self->mutex);
#		ifdef CON_HAVE_PTHREAD_YIELD
		pthread_yield();
#		endif
	}

	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}




//
// This function is called when the new thread is run; it then calls 'start_func'.
//

void *_Con_Builtins_Thread_Class_start_func_start_routine(void *thread_obj)
{
	Con_Obj *thread = (Con_Obj *) thread_obj;
	Con_Builtins_Thread_Atom *thread_atom = (Con_Builtins_Thread_Atom *) thread->first_atom;

	CON_MUTEX_LOCK(&thread->mutex);

	thread_atom->state = CON_BUILTINS_THREAD_CLASS_RUNNING;
	thread_atom->thread_id = pthread_self();
#ifdef __GNUC__
	thread_atom->c_stack_start = __builtin_frame_address(0);
#else
	CON_FATAL_ERROR("Unable to determine starting stack address.");
#endif

	Con_Obj *start_func = thread_atom->start_func;

	CON_MUTEX_UNLOCK(&thread->mutex);

	// Run start_func.

	Con_Obj *exception;
	CON_TRY {
		CON_APPLY(start_func);
	}
	CON_CATCH(exception) {
		if (CON_GET_SLOT_APPLY_NO_FAIL(CON_GET_SLOT(CON_BUILTIN(CON_BUILTIN_EXCEPTIONS_MODULE), "System_Exit_Exception"), "instantiated", exception) != NULL) {
			CON_XXX;
		}
		else {
			Con_Obj *backtrace = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_EXCEPTIONS_MODULE), "backtrace", exception);
			CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_SYS_MODULE), "println", backtrace);
		}
	}
	CON_TRY_END

	CON_MUTEX_LOCK(&thread->mutex);
	thread_atom->state = CON_BUILTINS_THREAD_CLASS_FINISHED;
	CON_MUTEX_UNLOCK(&thread->mutex);

	Con_Builtins_Thread_Atom_remove_thread(thread, thread);
	
	pthread_exit(NULL);
	
	// pthread_exit can't return but since GCC complains if a return value isn't specified:
	
	return NULL;
}



//
// 'PThread.join()' means that the current thread waits until the self thread finished.
//
// Note that this can return immediately if 'self' has already finished.
//

Con_Obj *_Con_Builtins_Thread_Class_join_func(Con_Obj *thread)
{
	Con_Obj *self;
	CON_UNPACK_ARGS("T", &self);

	Con_Builtins_Thread_Atom *self_thread_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_THREAD_ATOM_DEF_OBJECT));
	
	CON_MUTEX_LOCK(&self->mutex);
	pthread_t thread_id = self_thread_atom->thread_id;
	CON_MUTEX_UNLOCK(&self->mutex);
	
	CON_THREAD_START_BLOCKING_CALL();
	pthread_join(thread_id, NULL);
	CON_THREAD_END_BLOCKING_CALL();

	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}
