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



void _Con_Builtins_Func_Class_gc_scan_func(Con_Obj *, Con_Obj *, Con_Atom *);

Con_Obj *_Con_Builtins_Func_Class_path_func(Con_Obj *);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_Func_Atom_bootstrap(Con_Obj *thread)
{
	Con_Obj *func_atom_def = CON_BUILTIN(CON_BUILTIN_FUNC_ATOM_DEF_OBJECT);

	Con_Builtins_Atom_Def_Atom *atom_def_atom = (Con_Builtins_Atom_Def_Atom *) func_atom_def->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (atom_def_atom + 1);

	atom_def_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;
	
	Con_Builtins_Atom_Def_Atom_init_atom(thread, atom_def_atom, _Con_Builtins_Func_Class_gc_scan_func, NULL);
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	
	Con_Memory_change_chunk_type(thread, func_atom_def, CON_MEMORY_CHUNK_OBJ);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Function creation functions
//

Con_Obj *Con_Builtins_Func_Atom_new(Con_Obj *thread, bool is_bound, Con_PC pc, Con_Int num_params, Con_Int num_closure_vars, Con_Obj *container_closure, Con_Obj *name, Con_Obj *container)
{
	Con_Obj *new_func = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(Con_Builtins_Func_Atom) + sizeof(Con_Builtins_Slots_Atom), CON_BUILTIN(CON_BUILTIN_FUNC_CLASS));
	Con_Builtins_Func_Atom *func_atom = (Con_Builtins_Func_Atom *) new_func->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (func_atom + 1);
	func_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;
	
	func_atom->atom_type = CON_BUILTIN(CON_BUILTIN_FUNC_ATOM_DEF_OBJECT);
	func_atom->is_bound = is_bound;
	func_atom->pc = pc;
	func_atom->num_closure_vars = num_closure_vars;
	func_atom->container_closure = container_closure;
	
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	
	Con_Memory_change_chunk_type(thread, new_func, CON_MEMORY_CHUNK_OBJ);
	
	CON_SET_SLOT(new_func, "name", name);
	CON_SET_SLOT(new_func, "num_params", CON_NEW_INT(num_params));
	if (container == NULL)
		container = CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
	CON_SET_SLOT(new_func, "container", container);
	
	return new_func;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Function functions
//

Con_PC Con_Builtins_Func_Atom_get_pc(Con_Obj *thread, Con_Obj *func)
{
	Con_Builtins_Func_Atom *func_atom = CON_GET_ATOM(func, CON_BUILTIN(CON_BUILTIN_FUNC_ATOM_DEF_OBJECT));
	
	return func_atom->pc;
}



Con_Int Con_Builtins_Func_Atom_get_num_closure_vars(Con_Obj *thread, Con_Obj *func)
{
	Con_Builtins_Func_Atom *func_atom = CON_GET_ATOM(func, CON_BUILTIN(CON_BUILTIN_FUNC_ATOM_DEF_OBJECT));
	
	return func_atom->num_closure_vars;
}



Con_Obj *Con_Builtins_Func_Atom_get_container_closure(Con_Obj *thread, Con_Obj *func)
{
	Con_Builtins_Func_Atom *func_atom = CON_GET_ATOM(func, CON_BUILTIN(CON_BUILTIN_FUNC_ATOM_DEF_OBJECT));
	
	return func_atom->container_closure;
}



bool Con_Builtins_Func_Atom_is_bound(Con_Obj *thread, Con_Obj *func)
{
	Con_Builtins_Func_Atom *func_atom = CON_GET_ATOM(func, CON_BUILTIN(CON_BUILTIN_FUNC_ATOM_DEF_OBJECT));
	
	return func_atom->is_bound;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions
//

Con_PC Con_Builtins_Func_Atom_make_con_pc_c_function(Con_Obj *thread, Con_Obj *module, Con_C_Function *c_function)
{
	Con_PC pc;
	
	pc.type = PC_TYPE_C_FUNCTION;
	pc.module = module;
	pc.pc.c_function = c_function;
	
	return pc;
}



Con_PC Con_Builtins_Func_Atom_make_con_pc_bytecode(Con_Obj *thread, Con_Obj *module, Con_Int offset)
{
	Con_PC pc;
	
	pc.type = PC_TYPE_BYTECODE;
	pc.module = module;
	pc.pc.bytecode_offset = offset;
	
	return pc;
}



Con_PC Con_Builtins_Func_Atom_make_con_pc_null(Con_Obj *thread)
{
	Con_PC pc;
	
	pc.type = PC_TYPE_NULL;
	
	return pc;
}




////////////////////////////////////////////////////////////////////////////////////////////////////
// Garbage collection
//

void _Con_Builtins_Func_Class_gc_scan_func(Con_Obj *thread, Con_Obj *obj, Con_Atom *atom)
{
	Con_Builtins_Func_Atom *func_atom = (Con_Builtins_Func_Atom *) atom;

	Con_Memory_gc_scan_pc(thread, func_atom->pc);

	if (func_atom->container_closure != NULL)
		Con_Memory_gc_push(thread, func_atom->container_closure);
}
