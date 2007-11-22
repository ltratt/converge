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

#include "Arch.h"
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
#include "Builtins/Unique_Atom_Def.h"
#include "Builtins/VM/Atom.h"



Con_Obj *_Con_Builtins_Thread_Atom_new_object(Con_Obj *, Con_Obj *, Con_Obj *);

void _Con_Builtins_Thread_Atom_gc_scan_func(Con_Obj *, Con_Obj *, Con_Atom *);
void _Con_Builtins_Thread_Atom_unique_atom_gc_scan_func(Con_Obj *, Con_Obj *, Con_Builtins_Unique_Atom_Def *);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_Thread_Atom_bootstrap(Con_Obj *thread)
{
	Con_Obj *thread_atom_def = CON_BUILTIN(CON_BUILTIN_THREAD_ATOM_DEF_OBJECT);
	
	Con_Builtins_Atom_Def_Atom *atom_def_atom = (Con_Builtins_Atom_Def_Atom *) thread_atom_def->first_atom;
	Con_Builtins_Thread_Class_Unique_Atom *unique_atom = (Con_Builtins_Thread_Class_Unique_Atom *) (atom_def_atom + 1);

	atom_def_atom->next_atom = (Con_Atom *) unique_atom;
	unique_atom->next_atom = NULL;
	
	Con_Builtins_Atom_Def_Atom_init_atom(thread, atom_def_atom, _Con_Builtins_Thread_Atom_gc_scan_func, NULL);
	Con_Builtins_Unique_Atom_Def_init_atom(thread, (Con_Builtins_Unique_Atom_Def *) unique_atom, _Con_Builtins_Thread_Atom_unique_atom_gc_scan_func);

	unique_atom->num_threads_allocated = CON_DEFAULT_VM_NUM_THREADS;
	unique_atom->threads = Con_Memory_malloc(thread, sizeof(Con_Obj *) * unique_atom->num_threads_allocated, CON_MEMORY_CHUNK_OPAQUE);
	unique_atom->num_threads = 0;
	
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
	thread_atom->start_func = NULL;
	thread_atom->c_stack_start = c_stack_start;
	thread_atom->vm = vm;
	thread_atom->con_stack = Con_Builtins_Con_Stack_Atom_new(thread);
	thread_atom->state = CON_BUILTINS_THREAD_CLASS_RUNNING;
	thread_atom->thread_id = pthread_self();
	thread_atom->current_exception = NULL;

	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	
	Con_Memory_change_chunk_type(thread, thread_obj, CON_MEMORY_CHUNK_OBJ);
	
	Con_Builtins_Thread_Atom_add_thread(thread, thread_obj);
		
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



//
// Mark the current thread as being finished.
//
// Intended only to be called by the 'main' function.
//

void Con_Builtins_Thread_Atom_mark_as_finished(Con_Obj *thread)
{
	CON_MUTEX_LOCK(&thread->mutex);
	((Con_Builtins_Thread_Atom *) thread->first_atom)->state = CON_BUILTINS_THREAD_CLASS_FINISHED;
	CON_MUTEX_UNLOCK(&thread->mutex);
}



//
// Add 'thread' to the list of threads being tracked.
//

void Con_Builtins_Thread_Atom_add_thread(Con_Obj *thread, Con_Obj *new_thread)
{
	Con_Obj *thread_def = CON_BUILTIN(CON_BUILTIN_THREAD_ATOM_DEF_OBJECT);
	Con_Builtins_Thread_Class_Unique_Atom *thread_unique_atom = CON_GET_ATOM(thread_def, CON_BUILTIN(CON_BUILTIN_UNIQUE_ATOM_DEF_OBJECT));
	
	CON_MUTEX_LOCK(&thread_def->mutex);
	Con_Memory_make_array_room(thread, (void **) &thread_unique_atom->threads, &thread_def->mutex, &thread_unique_atom->num_threads_allocated, &thread_unique_atom->num_threads, 1, sizeof(Con_Obj *));
	thread_unique_atom->threads[thread_unique_atom->num_threads++] = new_thread;
	CON_MUTEX_UNLOCK(&thread_def->mutex);
}



//
// Remove 'thread' from the list of threads being tracked.
//

void Con_Builtins_Thread_Atom_remove_thread(Con_Obj *thread, Con_Obj *dead_thread)
{
	Con_Obj *thread_def = CON_BUILTIN(CON_BUILTIN_THREAD_ATOM_DEF_OBJECT);
	Con_Builtins_Thread_Class_Unique_Atom *thread_unique_atom = CON_GET_ATOM(thread_def, CON_BUILTIN(CON_BUILTIN_UNIQUE_ATOM_DEF_OBJECT));
	
	CON_MUTEX_LOCK(&thread_def->mutex);
	int i;
	for (i = 0; i < thread_unique_atom->num_threads; i += 1) {
		if (thread_unique_atom->threads[i] == dead_thread)
			break;
	}
	assert(i != thread_unique_atom->num_threads);
	memmove(thread_unique_atom->threads + i, thread_unique_atom->threads + i + 1, (thread_unique_atom->num_threads - (i + 1)) * sizeof(Con_Obj *));
	thread_unique_atom->num_threads -= 1;
	CON_MUTEX_UNLOCK(&thread_def->mutex);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions relating to synchronising threads around garbage collection
//

//
// Mark the current thread as being about to enter a "blocking" call of some sort. "Blocking" isn't a
// strictly accurate term; it really means that the current thread is about to enter a period where
// no new pointers will be added to the stack / registers.
//
// While the thread is marked as being in a "blocked" state, it can be garbage collected. Careful use
// of this function via the CON_THREAD_START_BLOCKING_CALL macro can significantly improve
// performance, but be careful.
//

void Con_Builtins_Thread_Atom_start_blocking_call(Con_Obj *thread)
{
	CON_MUTEX_LOCK(&thread->mutex);
	Con_Builtins_Thread_Atom *thread_atom = (Con_Builtins_Thread_Atom *) thread->first_atom;

	assert(thread_atom->state == CON_BUILTINS_THREAD_CLASS_RUNNING);
	CON_ARCH_GET_STACKP(thread_atom->c_stack_end);
	setjmp(thread_atom->last_env);
	thread_atom->state = CON_BUILTINS_THREAD_CLASS_RUNNING_BLOCKED;
	CON_MUTEX_UNLOCK(&thread->mutex);
}


//
// Mark the current thread as having finished its blocking call, and where normal garbage collection
// rules apply.
//

void Con_Builtins_Thread_Atom_end_blocking_call(Con_Obj *thread)
{
	CON_MUTEX_LOCK(&thread->mutex);
	Con_Builtins_Thread_Atom *thread_atom = (Con_Builtins_Thread_Atom *) thread->first_atom;

	assert(thread_atom->state == CON_BUILTINS_THREAD_CLASS_RUNNING_BLOCKED);
	thread_atom->state = CON_BUILTINS_THREAD_CLASS_RUNNING;
	CON_MUTEX_UNLOCK(&thread->mutex);
}



//
// The current thread is going to be the thread carrying out garbage collection, so this function
// waits until all other threads are in an appropriate state to garbage collect before returning.
//
// Note that other threads MAY be running when this function returns if they are in the
// RUNNING_BLOCKED state or if they are in the CON_BUILTINS_THREAD_CLASS_AWAITING_GC state but were
// interrupted in betweening setting this state and waiting on 'mem_store->gc_condition'. In both
// cases such threads will be in a safe state to garbage collect.
//

void Con_Builtins_Thread_Atom_wait_until_safe_to_gc(Con_Obj *thread)
{
	Con_Obj *thread_def = CON_BUILTIN(CON_BUILTIN_THREAD_ATOM_DEF_OBJECT);
	Con_Builtins_Thread_Class_Unique_Atom *thread_unique_atom = CON_GET_ATOM(thread_def, CON_BUILTIN(CON_BUILTIN_UNIQUE_ATOM_DEF_OBJECT));
	
	while (1) {
		CON_MUTEX_LOCK(&thread_def->mutex);
		bool still_waiting = false;
		// XXX there is a possible race condition here if thread_unique_atom->threads is altered
		// during the for loops execution.
		for (int i = 0; i < thread_unique_atom->num_threads; i += 1) {
			Con_Obj *other_thread = thread_unique_atom->threads[i];
			if (other_thread == thread)
				continue;
			CON_MUTEX_UNLOCK(&thread_def->mutex);
			
			CON_MUTEX_LOCK(&thread->mutex);
			Con_Builtins_Thread_Atom *other_thread_atom = (Con_Builtins_Thread_Atom *) other_thread->first_atom;
			Con_Builtins_Thread_Class_State other_thread_state = other_thread_atom->state;
			CON_MUTEX_UNLOCK(&thread->mutex);
			if (other_thread_state == CON_BUILTINS_THREAD_CLASS_RUNNING) {
				still_waiting = true;
#				ifdef CON_HAVE_PTHREAD_YIELD
				pthread_yield();
#				endif
				CON_MUTEX_LOCK(&thread_def->mutex);
				break;
			}
			CON_MUTEX_LOCK(&thread_def->mutex);
		}
		CON_MUTEX_UNLOCK(&thread_def->mutex);
		
		if (!still_waiting)
			break;
	}
}



//
// Suspend the current thread so that garbage collection can go ahead in another thread. 'condition'
// should be a pointer to 'mem_store->gc_condition' and 'mutex' a pointer to 'mem_store->mutex'.
//

void Con_Builtins_Thread_Atom_suspend_for_gc(Con_Obj *thread, Con_Thread_Condition *condition, Con_Mutex *mutex)
{
	Con_Builtins_Thread_Atom *thread_atom = (Con_Builtins_Thread_Atom *) thread->first_atom;

	// Update the thread state.

	CON_MUTEX_LOCK(&thread->mutex);
	CON_ARCH_GET_STACKP(thread_atom->c_stack_end);
	
	setjmp(thread_atom->last_env);
	Con_Builtins_Thread_Class_State prev_thread_state = thread_atom->state;
	thread_atom->state = CON_BUILTINS_THREAD_CLASS_AWAITING_GC;
	CON_MUTEX_UNLOCK(&thread->mutex);

	// Note that although this thread could get interrupted inbetween unlocking 'thread->mutex' and
	// 'mutex', but that doesn't effect garbage collection.

	CON_MUTEX_LOCK(mutex);
	Con_Builtins_Thread_Atom_condition_wait(thread, condition, mutex);
	CON_MUTEX_UNLOCK(mutex);

	CON_MUTEX_LOCK(&thread->mutex);
	thread_atom->state = prev_thread_state;
	CON_MUTEX_UNLOCK(&thread->mutex);
}



//
// Returns when all threads have finished.
//
// This is only intended to be called by the "main" function once it has finished. If called by any
// thread other than the main thread, or if it is called while the main thread is still running,
// the results of calling this function are undefined.
//

void Con_Builtins_Thread_Atom_join_all(Con_Obj *thread)
{
	Con_Obj *thread_def = CON_BUILTIN(CON_BUILTIN_THREAD_ATOM_DEF_OBJECT);
	Con_Builtins_Thread_Class_Unique_Atom *thread_unique_atom = CON_GET_ATOM(thread_def, CON_BUILTIN(CON_BUILTIN_UNIQUE_ATOM_DEF_OBJECT));
	
	CON_MUTEX_LOCK(&thread_def->mutex);
	while (thread_unique_atom->num_threads > 1) {
		// XXX there is a possible race condition here if thread_unique_atom->threads is altered
		// during the for loops execution.
		for (int i = 0; i < thread_unique_atom->num_threads; i += 1) {
			Con_Obj *other_thread = thread_unique_atom->threads[i];
			if (other_thread == thread)
				// Skip the main thread.
				continue;
			CON_MUTEX_UNLOCK(&thread_def->mutex);

			CON_MUTEX_LOCK(&thread->mutex);
			Con_Builtins_Thread_Atom *other_thread_atom = (Con_Builtins_Thread_Atom *) other_thread->first_atom;
			Con_Builtins_Thread_Class_State other_thread_state = other_thread_atom->state;
			CON_MUTEX_UNLOCK(&thread->mutex);
			if (other_thread_state == CON_BUILTINS_THREAD_CLASS_RUNNING) {
				CON_GET_SLOT_APPLY(other_thread, "join");
			}
				
			CON_MUTEX_LOCK(&thread_def->mutex);
		}
	}
	CON_MUTEX_UNLOCK(&thread_def->mutex);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Mutex functions
//

//
// Initializes 'mutex'.
//
// Generally one should use Con_Memory_mutex_init to initialise a mutex from a counting mutex.
//

void Con_Builtins_Thread_Atom_mutex_init(Con_Obj *thread, Con_Mutex *mutex)
{
	int rtn;
	if ((rtn = pthread_mutex_init(mutex, NULL)) == 0)
		return;

	printf("%s: %s", __func__, strerror(rtn));
	abort();
}



void Con_Builtins_Thread_Atom_mutex_lock(Con_Obj *thread, Con_Mutex *mutex)
{
	assert(mutex != NULL);

	int rtn;
	if ((rtn = pthread_mutex_lock(mutex)) == 0)
		return;
	
	printf("%s: %s", __func__, strerror(rtn));
	abort();
}



bool Con_Builtins_Thread_Atom_mutex_trylock(Con_Obj *thread, Con_Mutex *mutex)
{
	assert(mutex != NULL);

	int rtn = pthread_mutex_trylock(mutex);
	if (rtn == 0)
		return true;
	else if (rtn == EBUSY)
		return false;

	printf("%s: %s", __func__, strerror(rtn));
	abort();
}



void Con_Builtins_Thread_Atom_mutex_unlock(Con_Obj *thread, Con_Mutex *mutex)
{
	assert(mutex != NULL);

	int rtn;
	if ((rtn = pthread_mutex_unlock(mutex)) == 0)
		return;
	
	printf("%s: %s", __func__, strerror(rtn));
	abort();
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions on lists of mutexes
//


int _Con_Builtins_Thread_Atom_mutex_compare(const void *, const void *);

#define MAX_NUM_MUTEXES 10

int _Con_Builtins_Thread_Atom_mutex_compare(const void *mutex1, const void *mutex2)
{
	Con_Mutex m1 = **((Con_Mutex **) mutex1);
	Con_Mutex m2 = **((Con_Mutex **) mutex2);

	if (m1 < m2)
		return -1;
	else if (m1 > m2)
		return 1;
	return 0;
}



void Con_Builtins_Thread_Atom_mutex_add_lock(Con_Obj *thread, ...)
{
	Con_Mutex *mutexes[MAX_NUM_MUTEXES];

	va_list ap;
	va_start(ap, thread);
	int i = 0;
	Con_Mutex *mutex;
	while ((mutex = va_arg(ap, Con_Mutex *)) != NULL) {
		if (i == MAX_NUM_MUTEXES)
			CON_XXX;
		mutexes[i++] = mutex;
	}
	va_end(ap);
	
	assert(i >= 2);
	
	Con_Mutex *last_mutex = mutexes[i - 1];
	
	for (int j = 0; j < i - 1; j += 1) {
		if (*mutexes[j] == *last_mutex) {
			// The mutex is already locked so we don't need to do anything else.
			return;
		}
	}
	
	// For efficiencies sake, we want to unlock and relock the minimal number of locks possible
	// without deadlocking.
	//
	// What we do is to sort the list of mutexes so that it looks like (low = left, high = right):
	//
	//   <mutex a>, <mutex b>, <mutex of interest>, <mutex c>
	//
	// and then unlock all mutexes right of <mutex of interest>. We then lock <mutex of interest>
	// and then relock the locks just unlocked.
	
	qsort(&mutexes, i, sizeof(Con_Mutex *), _Con_Builtins_Thread_Atom_mutex_compare);
	
	int j;
	for (j = i - 1; j >= 0; j -= 1) {
		if ((j > 0) && (*mutexes[j] == *mutexes[j - 1]))
			continue;
		if (*mutexes[j] == *last_mutex)
			break;
		CON_MUTEX_UNLOCK(mutexes[j]);
	}

	for (int k = j; k < i; k += 1) {
		if ((k > 0) && (*mutexes[k] == *mutexes[k - 1]))
			continue;
		CON_MUTEX_LOCK(mutexes[k]);
	}
}



void Con_Builtins_Thread_Atom_mutexes_lock(Con_Obj *thread, ...)
{
	Con_Mutex *mutexes[MAX_NUM_MUTEXES];

	va_list ap;
	va_start(ap, thread);
	int i = 0;
	Con_Mutex *mutex;
	while ((mutex = va_arg(ap, Con_Mutex *)) != NULL) {
		if (i == MAX_NUM_MUTEXES)
			CON_XXX;
		mutexes[i++] = mutex;
	}
	va_end(ap);
	
	qsort(&mutexes, i, sizeof(Con_Mutex *), _Con_Builtins_Thread_Atom_mutex_compare);
	
	for (int j = 0; j < i; j += 1) {
		if ((j > 0) && (*mutexes[j] == *mutexes[j - 1]))
			continue;
		CON_MUTEX_LOCK(mutexes[j]);
	}
}



void Con_Builtins_Thread_Atom_mutexes_unlock(Con_Obj *thread, ...)
{
	Con_Mutex *mutexes[MAX_NUM_MUTEXES];

	va_list ap;
	va_start(ap, thread);
	int i = 0;
	Con_Mutex *mutex;
	while ((mutex = va_arg(ap, Con_Mutex *)) != NULL) {
		if (i == MAX_NUM_MUTEXES)
			CON_XXX;
		mutexes[i++] = mutex;
	}
	va_end(ap);
	
	qsort(&mutexes, i, sizeof(Con_Mutex *), _Con_Builtins_Thread_Atom_mutex_compare);
	
	for (int j = 0; j < i; j += 1) {
		if ((j > 0) && (*mutexes[j] == *mutexes[j - 1]))
			continue;
		CON_MUTEX_UNLOCK(mutexes[j]);
	}
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Condition functions
//

void Con_Builtins_Thread_Atom_condition_init(Con_Obj *thread, Con_Thread_Condition *cond)
{
	int rtn;
	if ((rtn = pthread_cond_init(cond, NULL)) == 0)
		return;

	printf("%s: %s", __func__, strerror(rtn));
	abort();
}



void Con_Builtins_Thread_Atom_condition_wait(Con_Obj *thread, Con_Thread_Condition *cond, Con_Mutex *mutex)
{
	int rtn;
	if ((rtn = pthread_cond_wait(cond, mutex)) == 0)
		return;

	printf("%s: %s", __func__, strerror(rtn));
	abort();
}



void Con_Builtins_Thread_Atom_condition_unblock_all(Con_Obj *thread, Con_Thread_Condition *cond)
{
	int rtn;
	if ((rtn = pthread_cond_broadcast(cond)) == 0)
		return;

	printf("%s: %s", __func__, strerror(rtn));
	abort();
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Garbage collection
//

void _Con_Builtins_Thread_Atom_gc_scan_func(Con_Obj *thread, Con_Obj *obj, Con_Atom *atom)
{
	Con_Builtins_Thread_Atom *thread_atom = (Con_Builtins_Thread_Atom *) atom;

	if (thread_atom->start_func != NULL)
		Con_Memory_gc_push(thread, thread_atom->start_func);
	Con_Memory_gc_push(thread, thread_atom->vm);
	Con_Memory_gc_push(thread, thread_atom->con_stack);
	if (thread_atom->current_exception != NULL)
		Con_Memory_gc_push(thread, thread_atom->current_exception);

	// There's no C stack to check if this thread isn't running, awaiting garbage collection, or
	// running blocked.

	if (!((thread_atom->state == CON_BUILTINS_THREAD_CLASS_RUNNING) || (thread_atom->state == CON_BUILTINS_THREAD_CLASS_AWAITING_GC) || (thread_atom->state == CON_BUILTINS_THREAD_CLASS_RUNNING_BLOCKED)))
		return;

#ifdef CON_C_STACK_GROWS_DOWN
	if (thread == obj) {
		u_char *c_stack_bottom;
		CON_ARCH_GET_STACKP(c_stack_bottom);
		Con_Memory_gc_scan_conservative(thread, c_stack_bottom, thread_atom->c_stack_start - c_stack_bottom);
	}
	else {
		Con_Memory_gc_scan_conservative(thread, thread_atom->last_env, sizeof(sigjmp_buf));
		Con_Memory_gc_scan_conservative(thread, thread_atom->c_stack_end, thread_atom->c_stack_start - thread_atom->c_stack_end);
	}
#else
	XXX
#endif
}



void _Con_Builtins_Thread_Atom_unique_atom_gc_scan_func(Con_Obj *thread, Con_Obj *obj, Con_Builtins_Unique_Atom_Def *unique_atom)
{
	Con_Builtins_Thread_Class_Unique_Atom *thread_unique_atom = (Con_Builtins_Thread_Class_Unique_Atom *) unique_atom;
	Con_Memory_gc_push(thread, thread_unique_atom->threads);

	for (int i = 0; i < thread_unique_atom->num_threads; i += 1)
		Con_Memory_gc_push(thread, thread_unique_atom->threads[i]);
}
