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
#include "Builtins/Float/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_Float_Atom_bootstrap(Con_Obj *thread)
{
	Con_Obj *float_atom_def = CON_BUILTIN(CON_BUILTIN_FLOAT_ATOM_DEF_OBJECT);

	Con_Builtins_Atom_Def_Atom *atom_def_atom = (Con_Builtins_Atom_Def_Atom *) float_atom_def->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (atom_def_atom + 1);

	atom_def_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;

	Con_Builtins_Atom_Def_Atom_init_atom(thread, atom_def_atom, NULL, NULL);
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);

	Con_Memory_change_chunk_type(thread, float_atom_def, CON_MEMORY_CHUNK_OBJ);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Int creation functions
//

Con_Obj *Con_Builtins_Float_Atom_new(Con_Obj *thread, Con_Float val)
{
	Con_Obj *new_float = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(Con_Builtins_Float_Atom), CON_BUILTIN(CON_BUILTIN_FLOAT_CLASS));
	Con_Builtins_Float_Atom_init_atom(thread, (Con_Builtins_Float_Atom *) new_float->first_atom, val);
	new_float->first_atom->next_atom = NULL;

	Con_Memory_change_chunk_type(thread, new_float, CON_MEMORY_CHUNK_OBJ);

	return new_float;
}



void Con_Builtins_Float_Atom_init_atom(Con_Obj *thread, Con_Builtins_Float_Atom *float_atom, Con_Float val)
{
	float_atom->atom_type = CON_BUILTIN(CON_BUILTIN_FLOAT_ATOM_DEF_OBJECT);
	float_atom->val = val;
}



//
// Returns the Con_Float value of 'obj'.
//

Con_Float Con_Builtins_Int_Atom_get_float(Con_Obj *thread, Con_Obj *obj)
{
	return ((Con_Builtins_Float_Atom *) CON_GET_ATOM(obj, CON_BUILTIN(CON_BUILTIN_FLOAT_ATOM_DEF_OBJECT)))->val;
}
