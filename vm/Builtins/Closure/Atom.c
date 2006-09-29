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

#include "Core.h"
#include "Object.h"
#include "Shortcuts.h"

#include "Builtins/Atom_Def_Atom.h"
#include "Builtins/Class/Atom.h"
#include "Builtins/Closure/Atom.h"
#include "Builtins/Closure/Class.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



void _Con_Builtins_Closure_Class_gc_scan_func(Con_Obj *, Con_Obj *, Con_Atom *);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_Closure_Atom_bootstrap(Con_Obj *thread)
{
	Con_Obj *closure_atom_def = CON_BUILTIN(CON_BUILTIN_CLOSURE_ATOM_DEF_OBJECT);
	
	Con_Builtins_Atom_Def_Atom *atom_def_atom = (Con_Builtins_Atom_Def_Atom *) closure_atom_def->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (atom_def_atom + 1);

	atom_def_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;
	
	Con_Builtins_Atom_Def_Atom_init_atom(thread, atom_def_atom, _Con_Builtins_Closure_Class_gc_scan_func, NULL);
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	
	Con_Memory_change_chunk_type(thread, closure_atom_def, CON_MEMORY_CHUNK_OBJ);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// String creation functions
//

Con_Obj *Con_Builtins_Closure_Atom_new(Con_Obj *thread, Con_Int num_local_vars, Con_Obj *parent_closure)
{
	Con_Obj *new_closure;
	Con_Builtins_Closure_Atom *closure_atom;
	
	if (parent_closure == NULL) {
		new_closure = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(Con_Builtins_Closure_Atom) + num_local_vars * sizeof(Con_Obj *), CON_BUILTIN(CON_BUILTIN_CLOSURE_CLASS));
		closure_atom = (Con_Builtins_Closure_Atom *) new_closure->first_atom;
		closure_atom->next_atom = NULL;
		closure_atom->atom_type = CON_BUILTIN(CON_BUILTIN_CLOSURE_ATOM_DEF_OBJECT);
		
		closure_atom->num_parent_closures = 0;
	}
	else {
		Con_Builtins_Closure_Atom *parent_closure_atom = CON_GET_ATOM(parent_closure, CON_BUILTIN(CON_BUILTIN_CLOSURE_ATOM_DEF_OBJECT));
		
		CON_MUTEX_LOCK(&parent_closure->mutex);
		Con_Int num_parent_closures = parent_closure_atom->num_parent_closures + 1;
		CON_MUTEX_UNLOCK(&parent_closure->mutex);

		size_t new_closure_size = sizeof(Con_Obj) + sizeof(Con_Builtins_Closure_Atom) + num_parent_closures * sizeof(Con_Obj *) + num_local_vars * sizeof(Con_Obj *);
		new_closure = Con_Object_new_from_class(thread, new_closure_size, CON_BUILTIN(CON_BUILTIN_CLOSURE_CLASS));
		closure_atom = (Con_Builtins_Closure_Atom *) new_closure->first_atom;
		closure_atom->next_atom = NULL;
		closure_atom->atom_type = CON_BUILTIN(CON_BUILTIN_CLOSURE_ATOM_DEF_OBJECT);

		CON_MUTEX_LOCK(&parent_closure->mutex);
		assert(num_parent_closures == parent_closure_atom->num_parent_closures + 1);
		
		memmove(closure_atom->vars + 1, parent_closure_atom->vars, (num_parent_closures - 1) * sizeof(Con_Obj *));
		CON_MUTEX_UNLOCK(&parent_closure->mutex);
		closure_atom->vars[0] = parent_closure;
		
		closure_atom->num_parent_closures = num_parent_closures;
	}
	
	for (int i = 0; i < num_local_vars; i += 1)
		closure_atom->vars[closure_atom->num_parent_closures + i] = NULL;
	closure_atom->num_local_vars = num_local_vars;

	Con_Memory_change_chunk_type(thread, new_closure, CON_MEMORY_CHUNK_OBJ);

	return new_closure;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions
//


Con_Obj *Con_Builtins_Closure_Atom_get_var(Con_Obj *thread, Con_Obj *closure, Con_Int closures_offset, Con_Int var_num)
{
	Con_Builtins_Closure_Atom *closure_atom = CON_GET_ATOM(closure, CON_BUILTIN(CON_BUILTIN_CLOSURE_ATOM_DEF_OBJECT));

	if (closures_offset == 0) {
		CON_MUTEX_LOCK(&closure->mutex);
		assert(var_num < closure_atom->num_local_vars);
		Con_Obj *obj = closure_atom->vars[closure_atom->num_parent_closures + var_num];
		CON_MUTEX_UNLOCK(&closure->mutex);
		if (obj == NULL) {
			CON_RAISE_EXCEPTION("Unassigned_Var_Exception");
		}
		return obj;
	}
	else {
		CON_MUTEX_LOCK(&closure->mutex);
		assert((closures_offset > 0) && (closures_offset - 1 < closure_atom->num_parent_closures));
		Con_Obj *parent_closure = closure_atom->vars[closures_offset - 1];
		CON_MUTEX_UNLOCK(&closure->mutex);
		return Con_Builtins_Closure_Atom_get_var(thread, parent_closure, 0, var_num);
	}
}



void Con_Builtins_Closure_Atom_set_var(Con_Obj *thread, Con_Obj *closure, Con_Int closures_offset, Con_Int var_num, Con_Obj *obj)
{
	Con_Builtins_Closure_Atom *closure_atom = CON_GET_ATOM(closure, CON_BUILTIN(CON_BUILTIN_CLOSURE_ATOM_DEF_OBJECT));

	if (closures_offset == 0) {
		CON_MUTEX_LOCK(&closure->mutex);
		assert(var_num < closure_atom->num_local_vars);
		closure_atom->vars[closure_atom->num_parent_closures + var_num] = obj;
		CON_MUTEX_UNLOCK(&closure->mutex);
	}
	else {
		CON_MUTEX_LOCK(&closure->mutex);
		assert((closures_offset > 0) && (closures_offset - 1 < closure_atom->num_parent_closures));
		Con_Obj *parent_closure = closure_atom->vars[closures_offset - 1];
		CON_MUTEX_UNLOCK(&closure->mutex);
		return Con_Builtins_Closure_Atom_set_var(thread, parent_closure, 0, var_num, obj);
	}
}



bool Con_Builtins_Closure_Atom_is_assigned(Con_Obj *thread, Con_Obj *closure, Con_Int closures_offset, Con_Int var_num)
{
	Con_Builtins_Closure_Atom *closure_atom = CON_GET_ATOM(closure, CON_BUILTIN(CON_BUILTIN_CLOSURE_ATOM_DEF_OBJECT));

	if (closures_offset == 0) {
		CON_MUTEX_LOCK(&closure->mutex);
		assert(var_num < closure_atom->num_local_vars);
		Con_Obj *obj = closure_atom->vars[closure_atom->num_parent_closures + var_num];
		CON_MUTEX_UNLOCK(&closure->mutex);
		if (obj == NULL) 
			return false;
		else
			return true;
	}
	else {
		CON_MUTEX_LOCK(&closure->mutex);
		assert((closures_offset > 0) && (closures_offset - 1 < closure_atom->num_parent_closures));
		Con_Obj *parent_closure = closure_atom->vars[closures_offset - 1];
		CON_MUTEX_UNLOCK(&closure->mutex);
		return Con_Builtins_Closure_Atom_get_var(thread, parent_closure, 0, var_num);
	}
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Garbage collection
//

void _Con_Builtins_Closure_Class_gc_scan_func(Con_Obj *thread, Con_Obj *obj, Con_Atom *atom)
{
	Con_Builtins_Closure_Atom *closure_atom = (Con_Builtins_Closure_Atom *) atom;

	for (int i = 0; i < closure_atom->num_parent_closures + closure_atom->num_local_vars; i += 1) {
		if (closure_atom->vars[i] != NULL)
			Con_Memory_gc_push(thread, closure_atom->vars[i]);
	}
}
