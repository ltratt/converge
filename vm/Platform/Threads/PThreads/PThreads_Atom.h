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


#ifndef _CON_PLATFORM_THREADS_PTHREADS_PTHREADS_ATOM_H
#define _CON_PLATFORM_THREADS_PTHREADS_PTHREADS_ATOM_H

#include <setjmp.h>

#include "Core.h"

#include "Builtins/Unique_Atom_Def.h"


typedef enum {CON_BUILTINS_THREAD_CLASS_INITIALIZED, CON_BUILTINS_THREAD_CLASS_RUNNING, CON_BUILTINS_THREAD_CLASS_RUNNING_BLOCKED, CON_BUILTINS_THREAD_CLASS_AWAITING_GC, CON_BUILTINS_THREAD_CLASS_FINISHED} Con_Builtins_Thread_Class_State;

typedef struct {
	CON_BUILTINS_UNIQUE_ATOM_HEAD
	Con_Obj **threads;
	Con_Int num_threads_allocated, num_threads;
} Con_Builtins_Thread_Class_Unique_Atom;

typedef struct {
	CON_ATOM_HEAD
	Con_Builtins_Thread_Class_State state;
	u_char *c_stack_start, *c_stack_end;
	Con_Obj *start_func;
	Con_Obj *vm;
	Con_Obj *con_stack;
	Con_Obj *current_exception;
	pthread_t thread_id;
	sigjmp_buf last_env;
} Con_Builtins_Thread_Atom;

void Con_Builtins_Thread_Atom_add_thread(Con_Obj *, Con_Obj *);
void Con_Builtins_Thread_Atom_remove_thread(Con_Obj *, Con_Obj *);

#endif
