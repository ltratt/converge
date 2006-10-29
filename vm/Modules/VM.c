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

#include "Bytecode.h"
#include "Core.h"
#include "Memory.h"
#include "Object.h"
#include "Shortcuts.h"

#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"

#include "Modules/VM.h"



Con_Obj *_Con_Modules_VM_add_modules_func(Con_Obj *);
Con_Obj *_Con_Modules_VM_import_module_func(Con_Obj *);



Con_Obj *Con_Modules_VM_init(Con_Obj *thread, Con_Obj *identifier)
{
	Con_Obj *vm_mod = Con_Builtins_Module_Atom_new_c(thread, identifier, CON_NEW_STRING("VM"), CON_BUILTIN(CON_BUILTIN_NULL_OBJ));
	
	CON_SET_SLOT(vm_mod, "add_modules", CON_NEW_UNBOUND_C_FUNC(_Con_Modules_VM_add_modules_func, "add_modules", CON_BUILTIN(CON_BUILTIN_NULL_OBJ)));
	CON_SET_SLOT(vm_mod, "import_module", CON_NEW_UNBOUND_C_FUNC(_Con_Modules_VM_import_module_func, "import_module", CON_BUILTIN(CON_BUILTIN_NULL_OBJ)));
	
	CON_SET_SLOT(vm_mod, "vm", Con_Builtins_Thread_Atom_get_vm(thread));
	
	return vm_mod;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions in VM module
//


//
// 'add_modules(modules)' adds / replaces modules in the running VM. 'modules' should be a list of
// strings. A list of identifiers (one for each module in 'modules') will be returned.
//
// Note that this function does *not* import module objects, it merely adds them into the VM so that
// they can then be imported; see the 'import_module' func.
//

Con_Obj *_Con_Modules_VM_add_modules_func(Con_Obj *thread)
{
	Con_Obj *modules_bytecode;
	CON_UNPACK_ARGS("L", &modules_bytecode);
	
	Con_Obj *module_identifiers = Con_Builtins_List_Atom_new(thread);

	CON_PRE_GET_SLOT_APPLY_PUMP(modules_bytecode, "iterate");
	while (1) {
		Con_Obj *val = CON_APPLY_PUMP();
		if (val == NULL)
			break;
		
		Con_Builtins_String_Atom *val_string_atom = CON_FIND_ATOM(val, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
		if (val_string_atom == NULL)
			CON_XXX;
		
		Con_Obj *identifier = Con_Bytecode_add_module(thread, (u_char*) val_string_atom->str);
		CON_GET_SLOT_APPLY(module_identifiers, "append", identifier);
	}
	
	return module_identifiers;
}



//
// 'import_module(identifier)' imports the module 'identifier'.
//

Con_Obj *_Con_Modules_VM_import_module_func(Con_Obj *thread)
{
	Con_Obj *identifier;
	CON_UNPACK_ARGS("O", &identifier);
	
	return Con_Builtins_Module_Atom_import(thread, identifier);
}
