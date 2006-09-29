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


#include "Config.h"

#include <stdarg.h>
#include <string.h>

#include "Core.h"
#include "Hash.h"
#include "Object.h"
#include "Shortcuts.h"

#include "Builtins/Atom_Def_Atom.h"
#include "Builtins/Class/Atom.h"
#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



void _Con_Builtins_Int_Class_unique_atom_gc_scan_func(Con_Obj *, Con_Obj *, Con_Builtins_Unique_Atom_Def *);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_Int_Atom_bootstrap(Con_Obj *thread)
{
	Con_Obj *int_atom_def = CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT);
	
	Con_Builtins_Atom_Def_Atom *atom_def_atom = (Con_Builtins_Atom_Def_Atom *) int_atom_def->first_atom;
	Con_Builtins_Int_Class_Unique_Atom *unique_atom = (Con_Builtins_Int_Class_Unique_Atom *) (atom_def_atom + 1);
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (unique_atom + 1);

	atom_def_atom->next_atom = (Con_Atom *) unique_atom;
	unique_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;
	
	Con_Builtins_Atom_Def_Atom_init_atom(thread, atom_def_atom, NULL, NULL);
	Con_Builtins_Unique_Atom_Def_init_atom(thread, (Con_Builtins_Unique_Atom_Def *) unique_atom, _Con_Builtins_Int_Class_unique_atom_gc_scan_func);
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);

	for (int i = 0; i < CON_DEFAULT_FIRST_N_INTS_TO_INTERN; i += 1) {
		unique_atom->first_n_interned_ints[i] = NULL;
	}
	
	Con_Memory_change_chunk_type(thread, int_atom_def, CON_MEMORY_CHUNK_OBJ);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Int creation functions
//

Con_Obj *Con_Builtins_Int_Atom_new(Con_Obj *thread, Con_Int val)
{
	// As a speed hack, we pull the unique atom out of the int class directly; this can be a
	// surprisingly large saving but does rely on the unique atom being at a fixed position
	// within the int class object.

	Con_Builtins_Int_Class_Unique_Atom *int_unique_atom = (Con_Builtins_Int_Class_Unique_Atom *) CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT)->first_atom->next_atom;
	assert(int_unique_atom = CON_GET_ATOM(CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT), CON_BUILTIN(CON_BUILTIN_UNIQUE_ATOM_DEF_OBJECT)));

	// If 'val' is one of the possibly interned values, see if we can return an interned object.

	if ((val >= 0) && (val < CON_DEFAULT_FIRST_N_INTS_TO_INTERN)) {
		CON_MUTEX_LOCK(&CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT)->mutex);
		Con_Obj *interned_val = int_unique_atom->first_n_interned_ints[val];
		CON_MUTEX_UNLOCK(&CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT)->mutex);
		if (interned_val != NULL)
			return interned_val;
	}

	// Create a new integer object.

	Con_Obj *new_int = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(Con_Builtins_Int_Atom), CON_BUILTIN(CON_BUILTIN_INT_CLASS));
	Con_Builtins_Int_Atom_init_atom(thread, (Con_Builtins_Int_Atom *) new_int->first_atom, val);
	new_int->first_atom->next_atom = NULL;
	
	Con_Memory_change_chunk_type(thread, new_int, CON_MEMORY_CHUNK_OBJ);

	if ((val >= 0) && (val < CON_DEFAULT_FIRST_N_INTS_TO_INTERN)) {
		// 'val' is one of the possibly interned values, so intern it.
		
		CON_MUTEX_LOCK(&CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT)->mutex);
		int_unique_atom->first_n_interned_ints[val] = new_int;
		CON_MUTEX_UNLOCK(&CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT)->mutex);
	}
	
	return new_int;
}



void Con_Builtins_Int_Atom_init_atom(Con_Obj *thread, Con_Builtins_Int_Atom *int_atom, Con_Int val)
{
	int_atom->atom_type = CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT);
	int_atom->val = val;
}



//
// Returns the Con_Int value of 'obj'.
//

Con_Int Con_Builtins_Int_Atom_get_int(Con_Obj *thread, Con_Obj *obj)
{
	return ((Con_Builtins_Int_Atom *) CON_GET_ATOM(obj, CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT)))->val;
}



//
//
//

int Con_Builtins_Int_Atom_get_c_int(Con_Obj *thread, Con_Obj *obj)
{
#	if SIZEOF_INT == SIZEOF_CON_INT
	// if sizeof(int) == sizeof(Con_Int) then the two types are equivalent and there is no need to
	// check for overflow etc.
	return ((Con_Builtins_Int_Atom *) CON_GET_ATOM(obj, CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT)))->val;
#	else
	CON_XXX;
#	endif
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Garbage collection
//


void _Con_Builtins_Int_Class_unique_atom_gc_scan_func(Con_Obj *thread, Con_Obj *obj, Con_Builtins_Unique_Atom_Def *unique_atom)
{
	Con_Builtins_Int_Class_Unique_Atom *int_unique_atom = (Con_Builtins_Int_Class_Unique_Atom *) unique_atom;

	for (int i = 0; i < CON_DEFAULT_FIRST_N_INTS_TO_INTERN; i += 1) {
		if (int_unique_atom->first_n_interned_ints[i] != NULL)
			Con_Memory_gc_push(thread, int_unique_atom->first_n_interned_ints[i]);
	}
}
