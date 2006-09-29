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

#include "Core.h"
#include "Shortcuts.h"

#include "Builtins/Atom_Def_Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/Unique_Atom_Def.h"
#include "Builtins/VM/Atom.h"



void _Con_Builtins_Unique_Atom_Def_gc_scan_func(Con_Obj *, Con_Obj *, Con_Atom *);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_Unique_Atom_Def_bootstrap(Con_Obj *thread)
{
	Con_Obj *unique_atom_def = CON_BUILTIN(CON_BUILTIN_UNIQUE_ATOM_DEF_OBJECT);
	
	Con_Builtins_Atom_Def_Atom *atom_def_atom = (Con_Builtins_Atom_Def_Atom *) unique_atom_def->first_atom;

	atom_def_atom->next_atom = NULL;
	
	Con_Builtins_Atom_Def_Atom_init_atom(thread, atom_def_atom, _Con_Builtins_Unique_Atom_Def_gc_scan_func, NULL);
	
	Con_Memory_change_chunk_type(thread, unique_atom_def, CON_MEMORY_CHUNK_OBJ);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Unique atom creation functions
//

void Con_Builtins_Unique_Atom_Def_init_atom(Con_Obj *thread, Con_Builtins_Unique_Atom_Def *unique_atom_atom, Con_Builtins_Unique_Atom_Def_scan_gc *gc_scan_func)
{
	unique_atom_atom->atom_type = CON_BUILTIN(CON_BUILTIN_UNIQUE_ATOM_DEF_OBJECT);
	unique_atom_atom->gc_scan_func = gc_scan_func;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Garbage collection
//

void _Con_Builtins_Unique_Atom_Def_gc_scan_func(Con_Obj *thread, Con_Obj *obj, Con_Atom *atom)
{
	Con_Builtins_Unique_Atom_Def *unique_atom_atom = (Con_Builtins_Unique_Atom_Def *) atom;

	unique_atom_atom->gc_scan_func(thread, obj, unique_atom_atom);
}
