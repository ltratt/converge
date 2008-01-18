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
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



void _Con_Builtins_List_Class_gc_scan_func(Con_Obj *, Con_Obj *, Con_Atom *);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_List_Atom_bootstrap(Con_Obj *thread)
{
	Con_Obj *list_atom_def = CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT);
	
	Con_Builtins_Atom_Def_Atom *atom_def_atom = (Con_Builtins_Atom_Def_Atom *) list_atom_def->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (atom_def_atom + 1);

	atom_def_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;
	
	Con_Builtins_Atom_Def_Atom_init_atom(thread, atom_def_atom, _Con_Builtins_List_Class_gc_scan_func, NULL);
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	
	Con_Memory_change_chunk_type(thread, list_atom_def, CON_MEMORY_CHUNK_OBJ);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// List creation functions
//

Con_Obj *Con_Builtins_List_Atom_new(Con_Obj *thread)
{
	return Con_Builtins_List_Atom_new_sized(thread, CON_DEFAULT_LIST_NUM_ENTRIES_ALLOCATED);
}



//
// Create a new list of at least 'num_entries_to_allocate' long.
//

Con_Obj *Con_Builtins_List_Atom_new_sized(Con_Obj *thread, Con_Int num_entries_to_allocate)
{
	assert(num_entries_to_allocate > 0);

	Con_Obj *new_list = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(Con_Builtins_List_Atom) + sizeof(Con_Builtins_Slots_Atom), CON_BUILTIN(CON_BUILTIN_LIST_CLASS));
	Con_Builtins_List_Atom *list_atom = (Con_Builtins_List_Atom *) new_list->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (list_atom + 1);
	list_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;
	
	Con_Builtins_List_Atom_init_atom(thread, list_atom, num_entries_to_allocate);
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	
	Con_Memory_change_chunk_type(thread, new_list, CON_MEMORY_CHUNK_OBJ);
	
	return new_list;
}



//
// Create a new list and initialize it with 'num_entries' objects from the stack.
//
// This is intended only for use by _Con_Builtins_VM_Atom_execute.
//

Con_Obj *Con_Builtins_List_Atom_new_from_con_stack(Con_Obj *thread, Con_Obj *con_stack, Con_Int num_entries)
{
	Con_Obj *new_list = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(Con_Builtins_List_Atom) + sizeof(Con_Builtins_Slots_Atom), CON_BUILTIN(CON_BUILTIN_LIST_CLASS));
	Con_Builtins_List_Atom *list_atom = (Con_Builtins_List_Atom *) new_list->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (list_atom + 1);
	list_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;

	// We have to duplicate Con_Builtins_List_Atom_init_atom, because we can't make any guarantees
	// about the locks that Con_Builtins_Con_Stack_Atom_pop_object might use, and we don't want to
	// be continually locking / unlocking the new lists mutex.

	list_atom->atom_type = CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT);
	Con_Int num_entries_allocated = num_entries > CON_DEFAULT_LIST_NUM_ENTRIES_ALLOCATED ? num_entries : CON_DEFAULT_LIST_NUM_ENTRIES_ALLOCATED;
	list_atom->entries = (Con_Obj **) Con_Memory_malloc(thread, sizeof(Con_Obj *) * num_entries_allocated, CON_MEMORY_CHUNK_CONSERVATIVE);
	list_atom->num_entries = num_entries;
	list_atom->num_entries_allocated = num_entries_allocated;

	CON_MUTEX_LOCK(&con_stack->mutex);
	for (Con_Int i = 0; i < num_entries; i += 1) {
		// Notice we're poping elements off the stack in reverse order.
		list_atom->entries[num_entries - i - 1] = Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack);
	}
	CON_MUTEX_UNLOCK(&con_stack->mutex);

	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);

	Con_Memory_change_chunk_type(thread, list_atom->entries, CON_MEMORY_CHUNK_OPAQUE);
	Con_Memory_change_chunk_type(thread, new_list, CON_MEMORY_CHUNK_OBJ);
	
	return new_list;
}



//
// Create a new list populated by '...' which should be terminated by NULL.
//

Con_Obj *Con_Builtins_List_Atom_new_va(Con_Obj *thread, ...)
{
	va_list ap;
	va_start(ap, thread);
	Con_Obj *new_list = Con_Builtins_List_Atom_new(thread);
	
	Con_Builtins_List_Atom *list_atom = CON_GET_ATOM(new_list, CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT));

	CON_MUTEX_LOCK(&new_list->mutex);

	Con_Obj *obj;
	while ((obj = va_arg(ap, Con_Obj *)) != NULL) {
		Con_Memory_make_array_room(thread, (void **) &list_atom->entries, &new_list->mutex, &list_atom->num_entries_allocated, &list_atom->num_entries, 1, sizeof(Con_Obj *));
		list_atom->entries[list_atom->num_entries++] = obj;
	}
	va_end(ap);

	CON_MUTEX_UNLOCK(&new_list->mutex);
	
	return new_list;
}



void Con_Builtins_List_Atom_init_atom(Con_Obj *thread, Con_Builtins_List_Atom *list_atom, Con_Int num_entries_to_allocate)
{
	list_atom->atom_type = CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT);
	
	list_atom->entries = (Con_Obj **) Con_Memory_malloc(thread, sizeof(Con_Obj *) * num_entries_to_allocate, CON_MEMORY_CHUNK_OPAQUE);
	list_atom->num_entries = 0;
	list_atom->num_entries_allocated = num_entries_to_allocate;
}




////////////////////////////////////////////////////////////////////////////////////////////////////
// Garbage collection
//

void _Con_Builtins_List_Class_gc_scan_func(Con_Obj *thread, Con_Obj *obj, Con_Atom *atom)
{
	Con_Builtins_List_Atom *list_atom = (Con_Builtins_List_Atom *) atom;

	Con_Memory_gc_push(thread, list_atom->entries);
	
	for (int i = 0; i < list_atom->num_entries; i += 1) {
		Con_Memory_gc_push(thread, list_atom->entries[i]);
	}
}
