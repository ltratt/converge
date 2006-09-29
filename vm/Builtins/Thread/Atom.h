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


#ifndef _CON_BUILTINS_THREAD_ATOM_H
#define _CON_BUILTINS_THREAD_ATOM_H

void Con_Builtins_Thread_Atom_bootstrap(Con_Obj *);

Con_Obj *Con_Builtins_Thread_Atom_new_from_self(Con_Obj *, u_char *, Con_Obj *);

Con_Obj *Con_Builtins_Thread_Atom_get_vm(Con_Obj *);
Con_Obj *Con_Builtins_Thread_Atom_get_con_stack(Con_Obj *);

#ifndef CON_THREADS_SINGLE_THREADED
void Con_Builtins_Thread_Atom_mutex_init(Con_Obj *, Con_Mutex *);
void Con_Builtins_Thread_Atom_mutex_lock(Con_Obj *, Con_Mutex *);
void Con_Builtins_Thread_Atom_mutex_add_lock(Con_Obj *, ...);
bool Con_Builtins_Thread_Atom_mutex_trylock(Con_Obj *, Con_Mutex *);
void Con_Builtins_Thread_Atom_mutex_unlock(Con_Obj *, Con_Mutex *);

void Con_Builtins_Thread_Atom_mutexes_lock(Con_Obj *, ...);
void Con_Builtins_Thread_Atom_mutexes_unlock(Con_Obj *, ...);

void Con_Builtins_Thread_Atom_condition_init(Con_Obj *, Con_Thread_Condition *);
void Con_Builtins_Thread_Atom_condition_wait(Con_Obj *, Con_Thread_Condition *, Con_Mutex *);
void Con_Builtins_Thread_Atom_condition_unblock_all(Con_Obj *, Con_Thread_Condition *);

void Con_Builtins_Thread_Atom_mark_as_finished(Con_Obj *);

void Con_Builtins_Thread_Atom_start_blocking_call(Con_Obj *);
void Con_Builtins_Thread_Atom_end_blocking_call(Con_Obj *);
void Con_Builtins_Thread_Atom_suspend_for_gc(Con_Obj *, Con_Thread_Condition *, Con_Mutex *);
void Con_Builtins_Thread_Atom_wait_until_safe_to_gc(Con_Obj *);
void Con_Builtins_Thread_Atom_join_all(Con_Obj *);
#endif


#if defined(CON_THREADS_SINGLE_THREADED)
#	include "Platform/Threads/Single_Thread/Single_Thread_Atom.h"
#elif defined(CON_THREADS_PTHREADS)
#	include "Platform/Threads/PThreads/PThreads_Atom.h"
#else
#	error "Unknown thread library"
#endif

#endif
