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
#include "Numbers.h"
#include "Object.h"
#include "Shortcuts.h"
#include "Slots.h"

#include "Builtins/Exception/Atom.h"
#include "Builtins/Float/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"




//
// Convert a number object into a Con_Int. An exception is raised if 'val' can not be represented
// as a Con_Int.
//

Con_Int Con_Numbers_Number_to_Con_Int(Con_Obj *thread, Con_Obj *number)
{
	// First of all we go through atom looking for the first atom of a Number type.
	Con_Atom *atom = number->first_atom;
	while (atom != NULL) {
		if (atom->atom_type == CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT)) {
			return ((Con_Builtins_Int_Atom *) atom)->val;
		}
		else if (atom->atom_type == CON_BUILTIN(CON_BUILTIN_FLOAT_ATOM_DEF_OBJECT)) {
			return (Con_Int) ((Con_Builtins_Float_Atom *) atom)->val;
		}
		atom = atom->next_atom;
	}

	CON_RAISE_EXCEPTION("Type_Exception", CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_NUMBER_CLASS), "path"), number);
}



//
// Convert a number object into a Con_Float. An exception is raised if 'val' can not be represented
// as a Con_Int.
//

Con_Float Con_Numbers_Number_to_Con_Float(Con_Obj *thread, Con_Obj *number)
{
	// First of all we go through atom looking for the first atom of a Number type.
	Con_Atom *atom = number->first_atom;
	while (atom != NULL) {
		if (atom->atom_type == CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT))
			return (Con_Float) ((Con_Builtins_Int_Atom *) atom)->val;
		else if (atom->atom_type == CON_BUILTIN(CON_BUILTIN_FLOAT_ATOM_DEF_OBJECT)) {
			return ((Con_Builtins_Float_Atom *) atom)->val;
		}
		atom = atom->next_atom;
	}

	CON_RAISE_EXCEPTION("Type_Exception", CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_NUMBER_CLASS), "path"), number);
}



//
// Convert a number object into a int. An exception is raised if 'val' can not be represented
// as a Con_Int.
//

int Con_Numbers_Number_to_c_Int(Con_Obj *thread, Con_Obj *number)
{
	// First of all we go through atom looking for the first atom of a Number type.
	Con_Atom *atom = number->first_atom;
	while (atom != NULL) {
		if (atom->atom_type == CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT)) {
			return Con_Numbers_Con_Int_to_c_int(thread, ((Con_Builtins_Int_Atom *) atom)->val);
		}
		else if (atom->atom_type == CON_BUILTIN(CON_BUILTIN_FLOAT_ATOM_DEF_OBJECT)) {
			return (int) ((Con_Builtins_Float_Atom *) atom)->val;
		}
		atom = atom->next_atom;
	}

	CON_RAISE_EXCEPTION("Type_Exception", CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_NUMBER_CLASS), "path"), number);
}



//
// Convert a Con_Int into a C int. An exception is raised if 'val' can not be represented as an
// int.
//

int Con_Numbers_Con_Int_to_c_int(Con_Obj *thread, Con_Int val)
{
#	if SIZEOF_INT == SIZEOF_CON_INT
	// if sizeof(int) == sizeof(Con_Int) then the two types are equivalent and there is no need to
	// check for overflow etc.
	return val;
#	elif SIZEOF_INT == 4 && SIZEOF_CON_INT == 8
	if ((val & 0xFFFFFFFF00000000) != 0)
		CON_XXX;
	return val;
#	endif
}
