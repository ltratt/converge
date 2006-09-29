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


#ifndef _CON_ATOMS_BUILTINS_INT_ATOM_H
#define _CON_ATOMS_BUILTINS_INT_ATOM_H

#include "Core.h"

#include "Builtins/Unique_Atom_Def.h"

// The root Int object is used to intern the first CON_DEFAULT_FIRST_N_INTS_TO_INTERN integers, to
// ensure that the VM doesn't spend half of its life allocating new integer objects.

typedef struct {
	CON_BUILTINS_UNIQUE_ATOM_HEAD
	Con_Obj *first_n_interned_ints[CON_DEFAULT_FIRST_N_INTS_TO_INTERN];
} Con_Builtins_Int_Class_Unique_Atom;


// Int atoms are immutable.

typedef struct {
	CON_ATOM_HEAD
	Con_Int val;
} Con_Builtins_Int_Atom;


void Con_Builtins_Int_Atom_bootstrap(Con_Obj *);

Con_Obj *Con_Builtins_Int_Atom_new(Con_Obj *, Con_Int);
void Con_Builtins_Int_Atom_init_atom(Con_Obj *, Con_Builtins_Int_Atom *, Con_Int);

Con_Int Con_Builtins_Int_Atom_get_int(Con_Obj *, Con_Obj *);
int Con_Builtins_Int_Atom_get_c_int(Con_Obj *, Con_Obj *);
int Con_Builtins_Int_Atom_Con_Int_to_c_int(Con_Obj *, Con_Int);

#endif
