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


#ifndef _CON_BUILTINS_ATOMS_ATOM_H
#define _CON_BUILTINS_ATOMS_ATOM_H

#include "Core.h"

typedef enum {CON_MODULE_UNINITIALIZED, CON_MODULE_INITIALIZING, CON_MODULE_INITIALIZED} Con_Builtins_Module_Class_State;

typedef struct {
	CON_ATOM_HEAD
	Con_Builtins_Module_Class_State state;    // Sequence is strictly: uninitialized -> initializing -> initialized.

	// module_bytecode is an immutable reference. It is set to NULL for builtin modules. For user
	// modules it points to a readable, but not writeable, bytecode. Ideally the read only nature of
	// this memory should be enforced by the OS / architecture.
	u_char *module_bytecode;
	Con_Int module_bytecode_offset;
	Con_Obj *identifier, *name, *closure, *imports, *init_func, *top_level_vars_map, *container;
	
	Con_Int num_constants;
	Con_Int *constants_create_offsets; // Can be NULL if num_constants == 0
	Con_Obj **constants;               // Can be NULL if num_constants == 0
} Con_Builtins_Module_Atom;


void Con_Builtins_Module_Atom_bootstrap(Con_Obj *);

Con_Obj *Con_Builtins_Module_Atom_new(Con_Obj *, Con_Obj *, u_char *, Con_Int, Con_Obj *, Con_Obj *, Con_Int, Con_Int *, Con_Obj *, Con_Obj *);
Con_Obj *Con_Builtins_Module_Atom_new_c(Con_Obj *, Con_Obj *, Con_Obj *, Con_Obj *);
void Con_Builtins_Module_Atom_init_atom(Con_Obj *, Con_Builtins_Module_Atom *, Con_Obj *, u_char *, Con_Int, Con_Obj *, Con_Obj *, Con_Int, Con_Int *, Con_Obj *, Con_Obj *);

Con_Obj *Con_Builtins_Module_Atom_get_identifier(Con_Obj *, Con_Obj *);
void Con_Builtins_Module_Atom_set_init_func(Con_Obj *, Con_Obj *, Con_Obj *);
Con_Int Con_Builtins_Module_Atom_get_instruction(Con_Obj *, Con_Obj *, Con_Int);
Con_Obj *Con_Builtins_Module_Atom_get_string(Con_Obj *, Con_Obj *, Con_Int, Con_Int);
void Con_Builtins_Module_Atom_read_bytes(Con_Obj *, Con_Obj *, Con_Int, void *, Con_Int);
Con_Obj *Con_Builtins_Module_Atom_get_constant(Con_Obj *, Con_Obj *, Con_Int);
void Con_Builtins_Module_Atom_set_constant(Con_Obj *, Con_Obj *, Con_Int, Con_Obj *);
Con_Int Con_Builtins_Module_Atom_get_constant_create_offset(Con_Obj *, Con_Obj *, Con_Int);
Con_Obj *Con_Builtins_Module_Atom_pc_to_src_locations(Con_Obj *, Con_PC);
void Con_Builtins_Module_Atom_src_offset_to_line_column(Con_Obj *, Con_Obj *, Con_Int, Con_Int *, Con_Int *);
Con_Obj	*Con_Builtins_Module_Atom_get_definition(Con_Obj *, Con_Obj *, const char *, Con_Int);

Con_Obj *Con_Builtins_Module_Atom_import_mod_from_bytecode(Con_Obj *, Con_Obj *, Con_Int);
Con_Obj *Con_Builtins_Module_Atom_import(Con_Obj *, Con_Obj *);

#endif
