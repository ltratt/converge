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


#ifndef _CON_BUILTINS_FUNC_ATOM_H
#define _CON_BUILTINS_FUNC_ATOM_H

#include "Core.h"

typedef struct {
	CON_ATOM_HEAD
	bool is_bound; // Immutable.
	Con_PC pc; // Immutable.
	Con_Int num_closure_vars; // Immutable.
	
	Con_Obj *container_closure; // Currently immutable for efficiency, but that should not be relied
	                            // on.
} Con_Builtins_Func_Atom;


void Con_Builtins_Func_Atom_bootstrap(Con_Obj *thread);

Con_Obj *Con_Builtins_Func_Atom_new(Con_Obj *, bool, Con_PC, Con_Int, Con_Int, Con_Obj *, Con_Obj *, Con_Obj *);

Con_PC Con_Builtins_Func_Atom_get_pc(Con_Obj *thread, Con_Obj *);
Con_Int Con_Builtins_Func_Atom_get_num_closure_vars(Con_Obj *, Con_Obj *);
Con_Obj *Con_Builtins_Func_Atom_get_container_closure(Con_Obj *, Con_Obj *);
bool Con_Builtins_Func_Atom_is_bound(Con_Obj *, Con_Obj *);

Con_PC Con_Builtins_Func_Atom_make_con_pc_c_function(Con_Obj *, Con_Obj *, Con_C_Function *);
Con_PC Con_Builtins_Func_Atom_make_con_pc_bytecode(Con_Obj *, Con_Obj *, Con_Int);
Con_PC Con_Builtins_Func_Atom_make_con_pc_null(Con_Obj *);

#endif
