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

#include "Arch.h"
#include "Core.h"
#include "Object.h"
#include "Shortcuts.h"

#include "Builtins/Class/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"
#include "Builtins/VM/Class.h"



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_VM_Class_bootstrap(Con_Obj *thread)
{
	Con_Obj *vm_class = CON_BUILTIN(CON_BUILTIN_VM_CLASS);
	
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) vm_class->first_atom;
	Con_Builtins_Class_Atom *class_atom = (Con_Builtins_Class_Atom *) (slots_atom + 1);

	slots_atom->next_atom = (Con_Atom *) class_atom;
	class_atom->next_atom = NULL;
	
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	Con_Builtins_Class_Atom_init_atom(thread, class_atom, CON_NEW_STRING("VM"), NULL, CON_BUILTIN(CON_BUILTIN_OBJECT_CLASS), NULL);
	
	Con_Memory_change_chunk_type(thread, vm_class, CON_MEMORY_CHUNK_OBJ);
}
