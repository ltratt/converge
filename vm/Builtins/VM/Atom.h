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


#ifndef _CON_BUILTINS_VM_ATOM_H
#define _CON_BUILTINS_VM_ATOM_H

#include "Core.h"
#include "Memory.h"

#include <setjmp.h>


typedef enum {CON_VM_INITIALIZING, CON_VM_RUNNING} Con_Atoms_VM_State;

// Builtins/VM_Class.c and Platform/Threads/*Thread/*Thread_Class.c expect that VM objects have a
// Con_Builtins_VM_Atom as the first_atom in the object.

typedef struct {
	CON_ATOM_HEAD
	Con_Atoms_VM_State state;
	Con_Memory_Store *mem_store;
	int argc;
	char **argv;
	char *vm_path, *prog_path;
	Con_Obj *current_exception;
	Con_Obj *builtins[CON_NUMBER_OF_BUILTINS];
	Con_Obj *bootstrap_compiler_current_path, *bootstrap_compiler_old_path;
} Con_Builtins_VM_Atom;


void Con_Builtins_VM_Atom_bootstrap(Con_Obj *);

Con_Obj *Con_Builtins_VM_Atom_new(Con_Obj *, Con_Memory_Store *, int, char **, char *, char *);

Con_Atoms_VM_State Con_Builtins_VM_Atom_get_state(Con_Obj *);
Con_Memory_Store *Con_Builtins_VM_Atom_get_mem_store(Con_Obj *);
void Con_Builtins_VM_Atom_read_prog_args(Con_Obj *, int *, char ***);
void Con_Builtins_VM_Atom_get_paths(Con_Obj *, char **, char **);
Con_Obj *Con_Builtins_VM_Atom_get_builtin(Con_Obj*, Con_Int);
Con_Obj *Con_Builtins_VM_Atom_get_functions_module(Con_Obj *);
void Con_Builtins_VM_Atom_get_bootstrap_compiler_paths(Con_Obj *, Con_Obj **, Con_Obj **);
void Con_Builtins_VM_Atom_set_bootstrap_compiler_paths(Con_Obj *, Con_Obj *, Con_Obj *);

Con_Obj *Con_Builtins_VM_Atom_apply(Con_Obj *, Con_Obj *, bool, ...);
Con_Obj *Con_Builtins_VM_Atom_apply_with_closure(Con_Obj *, Con_Obj *, Con_Obj *, bool, ...);
Con_Obj *Con_Builtins_VM_Atom_get_slot_apply(Con_Obj *, Con_Obj *, const u_char *, Con_Int, bool, ...);
void Con_Builtins_VM_Atom_pre_apply_pump(Con_Obj *, Con_Obj *, Con_Int, Con_Obj *, Con_PC);
void Con_Builtins_VM_Atom_pre_get_slot_apply_pump(Con_Obj *, Con_Obj *, const u_char *, Con_Int, ...);
void _Con_Builtins_VM_Atom_apply_pump_restore_c_stack(Con_Obj *, u_char *, u_char *, size_t, JMP_BUF, int);
Con_Obj *Con_Builtins_VM_Atom_apply_pump(Con_Obj *, bool);
void Con_Builtins_VM_Atom_yield(Con_Obj *, Con_Obj *);

Con_Obj *Con_Builtins_VM_Atom_exbi(Con_Obj *, Con_Obj *, Con_Obj *, const u_char *, Con_Int, Con_Obj *);

void Con_Builtins_VM_Atom_raise(Con_Obj *, Con_Obj *);
Con_Obj *Con_Builtins_VM_Atom_get_current_exception(Con_Obj *);
void Con_Builtins_VM_Atom_reset_current_exception(Con_Obj *);
void Con_Builtins_VM_Atom_ensure_no_current_exception(Con_Obj *);

#endif
