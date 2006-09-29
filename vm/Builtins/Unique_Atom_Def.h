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


#ifndef _CON_BUILTINS_UNIQUE_ATOM_DEF_H
#define _CON_BUILTINS_UNIQUE_ATOM_DEF_H

#include "Core.h"

typedef struct Con_Builtins_Unique_Atom_Def Con_Builtins_Unique_Atom_Def;

typedef void Con_Builtins_Unique_Atom_Def_scan_gc(Con_Obj *, Con_Obj *, Con_Builtins_Unique_Atom_Def *);

#define CON_BUILTINS_UNIQUE_ATOM_HEAD \
	CON_ATOM_HEAD \
	Con_Builtins_Unique_Atom_Def_scan_gc *gc_scan_func;

struct Con_Builtins_Unique_Atom_Def {
	CON_BUILTINS_UNIQUE_ATOM_HEAD
};

void Con_Builtins_Unique_Atom_Def_bootstrap(Con_Obj *);

void Con_Builtins_Unique_Atom_Def_init_atom(Con_Obj *, Con_Builtins_Unique_Atom_Def *, Con_Builtins_Unique_Atom_Def_scan_gc *);

#endif
