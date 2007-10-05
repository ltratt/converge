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

#include <string.h>

#include "Arch.h"
#include "Bytecode.h"
#include "Core.h"
#include "Memory.h"
#include "Object.h"
#include "Shortcuts.h"
#include "Slots.h"
#include "Target.h"

#include "Builtins/Dict/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



//
// Import an exec file from memory. bytecode should be a chunk of memory that will not be written to
// and which will not be freed until after this function has completed. Note that holding a lock on
// a segment while calling this function may lead to deadlock.
//

Con_Obj *Con_Bytecode_add_executable(Con_Obj *thread, u_char *bytecode)
{
	int i, num_modules, module_offset;
	Con_Obj *main_module_identifier;

#	define ID_BYTECODE_GET_WORD(x) (*(Con_Int*) (bytecode + (x)))

	num_modules = ID_BYTECODE_GET_WORD(CON_BYTECODE_NUMBER_OF_MODULES);
	
	main_module_identifier = NULL;
	for (i = 0; i < num_modules; i += 1) {
		module_offset = ID_BYTECODE_GET_WORD(CON_BYTECODE_MODULES + i * sizeof(Con_Int));

		Con_Obj *module = Con_Builtins_Module_Atom_new_from_bytecode(thread, bytecode + module_offset);
		Con_Obj *identifier = CON_GET_SLOT(module, "module_id");
		CON_GET_SLOT_APPLY(CON_GET_SLOT(Con_Builtins_Thread_Atom_get_vm(thread), "modules"), "set", identifier, module);

		if (i == 0)
			main_module_identifier = identifier;
	}
	
	if (main_module_identifier == NULL)
		CON_XXX;

	return main_module_identifier;
}
