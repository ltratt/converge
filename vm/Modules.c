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
#include <sys/stat.h>

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



extern Con_Modules_Spec Con_Builtin_Modules[];



Con_Obj *Con_Modules_find(Con_Obj *thread, Con_Obj *mod_id)
{
	Con_Obj *modules = CON_GET_SLOT(Con_Builtins_Thread_Atom_get_vm(thread), "modules");

	Con_Obj *mod = CON_GET_SLOT_APPLY_NO_FAIL(modules, "find", mod_id);
	if (mod == NULL) {
		int i;
		for (i = 0; Con_Builtin_Modules[i].mod_name != NULL; i += 1) {
			if (Con_Builtins_String_Atom_c_string_eq(thread, Con_Builtin_Modules[i].mod_name, strlen((char *) Con_Builtin_Modules[i].mod_name), mod_id)) {
				mod = Con_Builtin_Modules[i].init_func(thread, mod_id);
				break;
			}
		}
		if (Con_Builtin_Modules[i].mod_name == NULL)
			return NULL;
		CON_GET_SLOT_APPLY(modules, "set", mod_id, mod);
	}
		
	return mod;
}



Con_Obj *Con_Modules_get(Con_Obj *thread, Con_Obj *mod_id)
{
	Con_Obj *mod = Con_Modules_find(thread, mod_id);
	
	if (mod == NULL)
		CON_RAISE_EXCEPTION("Import_Exception", mod_id);
		
	return mod;
}



//
// 'Con_Modules_get_stdlib'
//
// Hohoho. Con_Modules_get_stdlib can get called by main.c outside of a continuation. This causes
// all sorts of problems because it violates some of CON_APPLY_PUMP's assumptions. So we hack around
// this by creating a temporary function which we call to do the pumping.
//

Con_Obj *_Con_Modules_get_stdlib_hack(Con_Obj *);

Con_Obj *Con_Modules_get_stdlib(Con_Obj *thread, const char *ptl_mod_id)
{
    Con_Obj *w = CON_NEW_BOUND_C_FUNC(_Con_Modules_get_stdlib_hack, "woo", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), CON_BUILTIN(CON_BUILTIN_NULL_OBJ));
    return CON_APPLY(w, Con_Builtins_String_Atom_new_copy(thread, ptl_mod_id, strlen(ptl_mod_id), CON_STR_UTF_8));
}


Con_Obj *_Con_Modules_get_stdlib_hack(Con_Obj *thread)
{
    Con_Obj *ptl_mod_id_obj;
    CON_UNPACK_ARGS("S", &ptl_mod_id_obj);

	//if (memcmp(ptl_mod_id, CON_DIR_SEP, strlen(CON_DIR_SEP)) != 0) {
    if (CON_GET_SLOT_APPLY_NO_FAIL(ptl_mod_id_obj, "suffixed_by", CON_NEW_STRING(CON_DIR_SEP)) != NULL) {
		return Con_Modules_get(thread, ptl_mod_id_obj);
	}
	else {
		Con_Obj *modules = CON_GET_SLOT(Con_Builtins_Thread_Atom_get_vm(thread), "modules");
		CON_PRE_GET_SLOT_APPLY_PUMP(modules, "iter_keys");
		while (1) {
			Con_Obj *orig_key = CON_APPLY_PUMP();
			if (orig_key == NULL)
				break;

            // XXX. The next two operations are pure evil and are basically a poor-man's
            // bootstrapping. They're intended to convert between module IDs across different
            // platforms. The potential problems with the below are fairly obvious, although
            // unlikely to actually manifest themselves in real life.

#ifndef CON_FILENAMES_CASE_SENSITIVE
            Con_Obj *cmp_key = CON_GET_SLOT_APPLY(orig_key, "lower_cased");
#else
            Con_Obj *cmp_key = orig_key;
#endif

            if (strcmp(CON_DIR_SEP, "/") == 0)
                cmp_key = CON_GET_SLOT_APPLY(cmp_key, "replaced", CON_NEW_STRING("\\"), CON_NEW_STRING("/"));
            else if (strcmp(CON_DIR_SEP, "\\") == 0)
                cmp_key = CON_GET_SLOT_APPLY(cmp_key, "replaced", CON_NEW_STRING("/"), CON_NEW_STRING("\\"));
            else
                CON_XXX;

			if (CON_GET_SLOT_APPLY_NO_FAIL(cmp_key, "suffixed_by", ptl_mod_id_obj) != NULL)
				return Con_Modules_get(thread, orig_key);
		}
		
        CON_RAISE_EXCEPTION("Import_Exception", ptl_mod_id_obj);
	}
}



Con_Obj *Con_Modules_import_mod_from_bytecode(Con_Obj *thread, Con_Obj *module, Con_Int import_num)
{
	Con_Builtins_Module_Atom *module_atom = CON_GET_ATOM(module, CON_BUILTIN(CON_BUILTIN_MODULE_ATOM_DEF_OBJECT));
	
	CON_MUTEX_LOCK(&module->mutex);
	Con_Obj *imports = module_atom->imports;
	CON_MUTEX_UNLOCK(&module->mutex);

	Con_Obj *mod = Con_Modules_get(thread, CON_GET_SLOT_APPLY(imports, "get", CON_NEW_INT(import_num)));

	return Con_Modules_import(thread, mod);
}



Con_Obj *Con_Modules_import(Con_Obj *thread, Con_Obj *module)
{
	Con_Builtins_Module_Atom *module_atom = CON_GET_ATOM(module, CON_BUILTIN(CON_BUILTIN_MODULE_ATOM_DEF_OBJECT));
	CON_MUTEX_LOCK(&module->mutex);
	if (module_atom->state == CON_MODULE_UNINITIALIZED) {
		module_atom->state = CON_MODULE_INITIALIZING;
		CON_MUTEX_UNLOCK(&module->mutex);
		
		if (module_atom->module_bytecode == NULL) {
			for (int i = 0; Con_Builtin_Modules[i].mod_name != NULL; i += 1) {
				if (Con_Builtins_String_Atom_c_string_eq(thread, Con_Builtin_Modules[i].mod_name, strlen((char *) Con_Builtin_Modules[i].mod_name), module_atom->identifier)) {
					Con_Builtin_Modules[i].import_func(thread, module);
					break;
				}
			}
		}
		else {
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
		}

		CON_MUTEX_LOCK(&module->mutex);
		module_atom->state = CON_MODULE_INITIALIZED;
		CON_MUTEX_UNLOCK(&module->mutex);
	}
	else
		CON_MUTEX_UNLOCK(&module->mutex);
	
	return module;
}
