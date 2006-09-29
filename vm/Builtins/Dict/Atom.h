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


#ifndef _CON_ATOMS_BUILTINS_DICT_ATOM_H
#define _CON_ATOMS_BUILTINS_DICT_ATOM_H

#include "Core.h"

// Empty hash entries have key == NULL.

typedef struct {
	Con_Hash hash;
	Con_Obj *key;
	Con_Obj *val;
} Con_Builtins_Dict_Class_Hash_Entry;

typedef struct {
	CON_ATOM_HEAD
	Con_Int num_entries, num_entries_allocated;
	Con_Builtins_Dict_Class_Hash_Entry *entries;
} Con_Builtins_Dict_Atom;


void Con_Builtins_Dict_Atom_bootstrap(Con_Obj *);

Con_Obj *Con_Builtins_Dict_Class_new(Con_Obj *);
void Con_Builtins_Dict_Class_init_atom(Con_Obj *, Con_Builtins_Dict_Atom *);

Con_Int Con_Builtins_Dict_Class_find_entry(Con_Obj *, Con_Mutex *, Con_Builtins_Dict_Class_Hash_Entry *, Con_Int, Con_Obj *, Con_Hash);

#endif
