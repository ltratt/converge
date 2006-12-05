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


#ifndef _CON_ATOMS_BUILTINS_STRING_ATOM_H
#define _CON_ATOMS_BUILTINS_STRING_ATOM_H

#include "Core.h"

#include "Builtins/Unique_Atom_Def.h"


typedef struct {
	Con_Int num_users;
	Con_Obj *str;
} Con_Builtins_String_Class_Unique_Atom_Cache_Entry;

typedef struct {
	CON_BUILTINS_UNIQUE_ATOM_HEAD
	Con_Int num_cache_entries, num_cache_entries_allocated;
	Con_Builtins_String_Class_Unique_Atom_Cache_Entry *cache;
} Con_Builtins_String_Class_Unique_Atom;

// Strings and their contents are immutable throughout their lifetimes. You do not need to hold the
// strings mutex in order to read any of its contents.

typedef struct {
	CON_ATOM_HEAD
	Con_String_Encoding encoding;
	Con_Int hash;
	Con_Int size; // In bytes.
	// str can be an arbitrary pointer. It may not necessarily point to a chunk of memory allocated
	// by Con_Memory_malloc; it might not even be a pointer to within such a chunk. All that is
	// required is that it point to a valid memory location.
	const char *str;
} Con_Builtins_String_Atom;


void Con_Builtins_String_Atom_bootstrap(Con_Obj *);

Con_Obj *Con_Builtins_String_Atom_new_copy(Con_Obj *, const char *, Con_Int, Con_String_Encoding);
Con_Obj *Con_Builtins_String_Atom_new_no_copy(Con_Obj *, const char *, Con_Int, Con_String_Encoding);
void Con_Builtins_String_Atom_init_atom(Con_Obj *, Con_Builtins_String_Atom *, const char *, Con_Int, Con_String_Encoding, Con_Int);

char *Con_Builtins_String_Atom_to_c_string(Con_Obj *, Con_Obj *);
bool Con_Builtins_String_Atom_c_string_eq(Con_Obj *, const char *, Con_Int, Con_Obj *);

#endif
