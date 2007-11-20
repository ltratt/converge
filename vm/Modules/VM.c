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
#include "Modules.h"
#include "Object.h"
#include "Shortcuts.h"
#include "Slots.h"

#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



Con_Obj *Con_Module_VM_init(Con_Obj *, Con_Obj *);
Con_Obj *Con_Module_VM_import(Con_Obj *, Con_Obj *);

Con_Obj *_Con_Module_VM_add_modules_func(Con_Obj *);
Con_Obj *_Con_Module_VM_find_module_func(Con_Obj *);
Con_Obj *_Con_Module_VM_import_module_func(Con_Obj *);
Con_Obj *_Con_Module_VM_set_bootstrap_compiler_paths_func(Con_Obj *);



Con_Obj *Con_Module_VM_init(Con_Obj *thread, Con_Obj *identifier)
{
	const char* defn_names[] = {"add_modules", "find_module", "import_module", "set_bootstrap_compiler_paths", "vm", NULL};

	return Con_Builtins_Module_Atom_new_c(thread, identifier, CON_NEW_STRING("VM"), defn_names, CON_BUILTIN(CON_BUILTIN_NULL_OBJ));
}



Con_Obj *Con_Module_VM_import(Con_Obj *thread, Con_Obj *vm_mod)
{
	CON_SET_MOD_DEFN(vm_mod, "add_modules", CON_NEW_UNBOUND_C_FUNC(_Con_Module_VM_add_modules_func, "add_modules", vm_mod));
	CON_SET_MOD_DEFN(vm_mod, "find_module", CON_NEW_UNBOUND_C_FUNC(_Con_Module_VM_find_module_func, "find_module", vm_mod));
	CON_SET_MOD_DEFN(vm_mod, "import_module", CON_NEW_UNBOUND_C_FUNC(_Con_Module_VM_import_module_func, "import_module", vm_mod));
	CON_SET_MOD_DEFN(vm_mod, "set_bootstrap_compiler_paths", CON_NEW_UNBOUND_C_FUNC(_Con_Module_VM_set_bootstrap_compiler_paths_func, "set_bootstrap_compiler_paths", vm_mod));
	
	CON_SET_MOD_DEFN(vm_mod, "vm", Con_Builtins_Thread_Atom_get_vm(thread));
	
	return vm_mod;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions in VM module
//


//
// 'add_modules(modules)' adds / replaces modules in the running VM. 'modules' should be a list of
// modules. Note that this function does *not* import modules.
//

Con_Obj *_Con_Module_VM_add_modules_func(Con_Obj *thread)
{
	Con_Obj *mods;
	CON_UNPACK_ARGS("L", &mods);
	
	CON_PRE_GET_SLOT_APPLY_PUMP(mods, "iter");
	while (1) {
		Con_Obj *mod = CON_APPLY_PUMP();
		if (mod == NULL)
			break;
		
		Con_Obj *mod_id = CON_GET_SLOT(mod, "mod_id");
		CON_GET_SLOT_APPLY(CON_GET_SLOT(Con_Builtins_Thread_Atom_get_vm(thread), "modules"), "set", mod_id, mod);
	}
	
	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'find_module(mod_id)' attempts to find the module with id 'mod_id'.
//

Con_Obj *_Con_Module_VM_find_module_func(Con_Obj *thread)
{
	Con_Obj *mod_id;
	CON_UNPACK_ARGS("S", &mod_id);
	
	Con_Obj *mod = Con_Modules_find(thread, mod_id);
	if (mod == NULL)
		return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
	else
		return mod;
}



//
// 'import_module(mod)' imports the module 'mod'.
//

Con_Obj *_Con_Module_VM_import_module_func(Con_Obj *thread)
{
	Con_Obj *mod;
	CON_UNPACK_ARGS("M", &mod);
	
	return Con_Modules_import(thread, mod);
}



//
// 'set_bootstrap_compiler_paths(current_path, old_path)' is an internal function to tell the VM that
// the compiler is operating in bootstrapping mode, and needs Con_Modules_find to do the same.
//

Con_Obj *_Con_Module_VM_set_bootstrap_compiler_paths_func(Con_Obj *thread)
{
	Con_Obj *current_path, *old_path;
	CON_UNPACK_ARGS("SS", &current_path, &old_path);
	
	Con_Builtins_VM_Atom_set_bootstrap_compiler_paths(thread, current_path, old_path);
	
	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}
