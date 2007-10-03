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
#include "Hash.h"
#include "Modules.h"
#include "Numbers.h"
#include "Object.h"
#include "Shortcuts.h"
#include "Target.h"

#include "Builtins/Atom_Def_Atom.h"
#include "Builtins/Class/Atom.h"
#include "Builtins/Closure/Atom.h"
#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Dict/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/Module/Class.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



Con_Obj *Con_Modules_import_mod_from_bytecode(Con_Obj *thread, Con_Obj *module, Con_Int import_num)
{
	Con_Builtins_Module_Atom *module_atom = CON_GET_ATOM(module, CON_BUILTIN(CON_BUILTIN_MODULE_ATOM_DEF_OBJECT));
	
	CON_MUTEX_LOCK(&module->mutex);
	Con_Obj *imports = module_atom->imports;
	CON_MUTEX_UNLOCK(&module->mutex);

	return Con_Modules_import(thread, CON_GET_SLOT_APPLY(imports, "get", CON_NEW_INT(import_num)));
}



extern Con_Obj *Con_Modules_Builtins_init(Con_Obj *, Con_Obj*);
extern Con_Obj *Con_Modules_Exceptions_init(Con_Obj *, Con_Obj*);
extern Con_Obj *Con_Modules_Sys_init(Con_Obj *, Con_Obj*);
extern Con_Obj *Con_Modules_POSIX_File_Module_init(Con_Obj *, Con_Obj*);
extern Con_Obj *Con_Modules_PThreads_init(Con_Obj *, Con_Obj*);
extern Con_Obj *Con_Modules_PCRE_init(Con_Obj *, Con_Obj*);
extern Con_Obj *Con_Modules_Array_init(Con_Obj *, Con_Obj*);
extern Con_Obj *Con_Modules_C_Earley_Parser_init(Con_Obj *, Con_Obj*);
extern Con_Obj *Con_Modules_C_Strings_init(Con_Obj *, Con_Obj*);
extern Con_Obj *Con_Modules_VM_init(Con_Obj *, Con_Obj*);
extern Con_Obj *Con_Modules_Thread_init(Con_Obj *, Con_Obj*);
extern Con_Obj *Con_Modules_libXML2_init(Con_Obj *, Con_Obj*);
extern Con_Obj *Con_Modules_Random_init(Con_Obj *, Con_Obj*);
extern Con_Obj *Con_Modules_C_Platform_Properties_init(Con_Obj *, Con_Obj*);
extern Con_Obj *Con_Modules_C_Platform_Env_init(Con_Obj *, Con_Obj*);
extern Con_Obj *Con_Modules_C_Platform_Exec_init(Con_Obj *, Con_Obj*);

Con_Obj *Con_Modules_import(Con_Obj *thread, Con_Obj *identifier)
{
	Con_Obj *modules = CON_GET_SLOT(Con_Builtins_Thread_Atom_get_vm(thread), "modules");

	Con_Obj *module = CON_GET_SLOT_APPLY_NO_FAIL(modules, "get", identifier, CON_BUILTIN(CON_BUILTIN_FAIL_OBJ));
	if (module == NULL) {
		if (CON_C_STRING_EQ("Sys", identifier)) {
			module = Con_Modules_Sys_init(thread, CON_NEW_STRING("Sys"));
			CON_GET_SLOT_APPLY(modules, "set", CON_NEW_STRING("Sys"), module);
		}
		else if (CON_C_STRING_EQ("Exceptions", identifier)) {
			module = Con_Modules_Exceptions_init(thread, CON_NEW_STRING("Exceptions"));
			CON_GET_SLOT_APPLY(modules, "set", CON_NEW_STRING("Exceptions"), module);
		}
		else if (CON_C_STRING_EQ("Builtins", identifier)) {
			module = Con_Modules_Builtins_init(thread, CON_NEW_STRING("Builtins"));
			CON_GET_SLOT_APPLY(modules, "set", CON_NEW_STRING("Builtins"), module);
		}
		else if (CON_C_STRING_EQ("POSIX_File", identifier)) {
			module = Con_Modules_POSIX_File_Module_init(thread, CON_NEW_STRING("POSIX_File"));
			CON_GET_SLOT_APPLY(modules, "set", CON_NEW_STRING("POSIX_File"), module);
		}
		else if (CON_C_STRING_EQ("PCRE", identifier)) {
			module = Con_Modules_PCRE_init(thread, CON_NEW_STRING("PCRE"));
			CON_GET_SLOT_APPLY(modules, "set", CON_NEW_STRING("PCRE"), module);
		}
		else if (CON_C_STRING_EQ("Array", identifier)) {
			module = Con_Modules_Array_init(thread, CON_NEW_STRING("Array"));
			CON_GET_SLOT_APPLY(modules, "set", CON_NEW_STRING("Array"), module);
		}
		else if (CON_C_STRING_EQ("VM", identifier)) {
			module = Con_Modules_VM_init(thread, CON_NEW_STRING("VM"));
			CON_GET_SLOT_APPLY(modules, "set", CON_NEW_STRING("VM"), module);
		}
		else if (CON_C_STRING_EQ("C_Earley_Parser", identifier)) {
			module = Con_Modules_C_Earley_Parser_init(thread, CON_NEW_STRING("C_Earley_Parser"));
			CON_GET_SLOT_APPLY(modules, "set", CON_NEW_STRING("C_Earley_Parser"), module);
		}
		else if (CON_C_STRING_EQ("C_Strings", identifier)) {
			module = Con_Modules_C_Strings_init(thread, CON_NEW_STRING("C_Strings"));
			CON_GET_SLOT_APPLY(modules, "set", CON_NEW_STRING("C_Strings"), module);
		}
		else if (CON_C_STRING_EQ("Thread", identifier)) {
			module = Con_Modules_Thread_init(thread, CON_NEW_STRING("Thread"));
			CON_GET_SLOT_APPLY(modules, "set", CON_NEW_STRING("Thread"), module);
		}
		else if (CON_C_STRING_EQ("libXML2", identifier)) {
			module = Con_Modules_libXML2_init(thread, CON_NEW_STRING("libXML2"));
			CON_GET_SLOT_APPLY(modules, "set", CON_NEW_STRING("libXML2"), module);
		}
		else if (CON_C_STRING_EQ("Random", identifier)) {
			module = Con_Modules_Random_init(thread, CON_NEW_STRING("Random"));
			CON_GET_SLOT_APPLY(modules, "set", CON_NEW_STRING("Random"), module);
		}
		else if (CON_C_STRING_EQ("C_Platform_Properties", identifier)) {
			module = Con_Modules_C_Platform_Properties_init(thread, CON_NEW_STRING("C_Platform_Properties"));
			CON_GET_SLOT_APPLY(modules, "set", CON_NEW_STRING("C_Platform_Properties"), module);
		}
		else if (CON_C_STRING_EQ("C_Platform_Env", identifier)) {
			module = Con_Modules_C_Platform_Env_init(thread, CON_NEW_STRING("C_Platform_Env"));
			CON_GET_SLOT_APPLY(modules, "set", CON_NEW_STRING("C_Platform_Env"), module);
		}
		else if (CON_C_STRING_EQ("C_Platform_Exec", identifier)) {
			module = Con_Modules_C_Platform_Exec_init(thread, CON_NEW_STRING("C_Platform_Exec"));
			CON_GET_SLOT_APPLY(modules, "set", CON_NEW_STRING("C_Platform_Exec"), module);
		}
#		ifdef CON_THREADS_PTHREADS
		else if (CON_C_STRING_EQ("PThreads", identifier)) {
			module = Con_Modules_PThreads_init(thread, CON_NEW_STRING("PThreads"));
			CON_GET_SLOT_APPLY(modules, "set", CON_NEW_STRING("PThreads"), module);
		}
#		endif
		else
			CON_RAISE_EXCEPTION("Import_Exception", identifier);
		Con_Builtins_Module_Atom *module_atom = CON_GET_ATOM(module, CON_BUILTIN(CON_BUILTIN_MODULE_ATOM_DEF_OBJECT));
		CON_MUTEX_LOCK(&module->mutex);
		module_atom->state = CON_MODULE_INITIALIZED;
		CON_MUTEX_UNLOCK(&module->mutex);
		return module;
	}
	
	Con_Builtins_Module_Atom *module_atom = CON_GET_ATOM(module, CON_BUILTIN(CON_BUILTIN_MODULE_ATOM_DEF_OBJECT));
	CON_MUTEX_LOCK(&module->mutex);
	if (module_atom->state == CON_MODULE_UNINITIALIZED) {
		module_atom->state = CON_MODULE_INITIALIZING;
		Con_Obj *init_func = module_atom->init_func;
		CON_MUTEX_UNLOCK(&module->mutex);
		
		if (init_func == NULL)
			CON_XXX;
		
		Con_Int num_closure_vars = Con_Builtins_Func_Atom_get_num_closure_vars(thread, init_func);
		Con_Obj *container_closure = Con_Builtins_Func_Atom_get_container_closure(thread, init_func);
		Con_Obj *closure = NULL;
		if ((num_closure_vars > 0) || (container_closure != NULL))
			closure = Con_Builtins_Closure_Atom_new(thread, num_closure_vars, container_closure);
		
		CON_MUTEX_LOCK(&module->mutex);
		module_atom->closure = closure;
		CON_MUTEX_UNLOCK(&module->mutex);
		
		Con_Builtins_VM_Atom_apply_with_closure(thread, init_func, closure, false, module, NULL);

		CON_MUTEX_LOCK(&module->mutex);
		module_atom->state = CON_MODULE_INITIALIZED;
		CON_MUTEX_UNLOCK(&module->mutex);
	}
	else
		CON_MUTEX_UNLOCK(&module->mutex);
	
	return module;
}
