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


#ifndef _CON_ATOMS_BUILTINS_EXCEPTION_ATOM_H
#define _CON_ATOMS_BUILTINS_EXCEPTION_ATOM_H

typedef struct {
	Con_Obj *func;
	Con_PC pc;
} Con_Builtins_Exception_Class_Call_Chain_Entry;

typedef struct {
	CON_ATOM_HEAD
	// The call_chain can be NULL which means the call chain has not yet been set.
	//
	// The call chain is expected to store entries in strict descending order of age i.e.:
	//   [<newest call>, ... , <oldest call>]
	Con_Builtins_Exception_Class_Call_Chain_Entry *call_chain;
	Con_Int num_call_chain_entries, num_call_chain_entries_allocated;
#	ifndef CON_THREADS_SINGLE_THREADED
	Con_Obj *exception_thread;
#	endif
} Con_Builtins_Exception_Atom;


void Con_Builtins_Exception_Atom_bootstrap(Con_Obj *);

Con_Obj *Con_Builtins_Exception_Atom_strerror(Con_Obj *, int);
void Con_Builtins_Exception_Atom_get_call_chain(Con_Obj *, Con_Obj *, Con_Builtins_Exception_Class_Call_Chain_Entry **, Con_Int *, Con_Int *);
void Con_Builtins_Exception_Atom_set_call_chain(Con_Obj *, Con_Obj *, Con_Builtins_Exception_Class_Call_Chain_Entry *, Con_Int, Con_Int);
#ifndef CON_THREADS_SINGLE_THREADED
void Con_Builtins_Exception_Atom_set_exception_thread(Con_Obj *, Con_Obj *, Con_Obj *);
#endif

#endif
