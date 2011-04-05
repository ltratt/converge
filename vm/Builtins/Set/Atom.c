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
#include "Hash.h"
#include "Object.h"
#include "Shortcuts.h"

#include "Builtins/Atom_Def_Atom.h"
#include "Builtins/Class/Atom.h"
#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Set/Atom.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



void _Con_Builtins_Set_Atom_gc_scan_func(Con_Obj *, Con_Obj *, Con_Atom *);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_Set_Atom_bootstrap(Con_Obj *thread)
{
	Con_Obj *set_atom_def = CON_BUILTIN(CON_BUILTIN_SET_ATOM_DEF_OBJECT);

	Con_Builtins_Atom_Def_Atom *atom_def_atom = (Con_Builtins_Atom_Def_Atom *) set_atom_def->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (atom_def_atom + 1);

	atom_def_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;
	
	Con_Builtins_Atom_Def_Atom_init_atom(thread, atom_def_atom, _Con_Builtins_Set_Atom_gc_scan_func, NULL);
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	
	Con_Memory_change_chunk_type(thread, set_atom_def, CON_MEMORY_CHUNK_OBJ);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Set creation functions
//

Con_Obj *Con_Builtins_Set_Atom_new(Con_Obj *thread)
{
	return Con_Builtins_Set_Atom_new_sized(thread, CON_DEFAULT_SET_NUM_ENTRIES_ALLOCATED);
}



//
// Create a new set and initialize it with 'num_entries' objects from the stack.
//
// This is intended only for use by _Con_Builtins_VM_Atom_execute.
//

Con_Obj *Con_Builtins_Set_Atom_new_sized(Con_Obj *thread, Con_Int num_entries)
{
	Con_Obj *new_set = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(Con_Builtins_Set_Atom) + sizeof(Con_Builtins_Slots_Atom), CON_BUILTIN(CON_BUILTIN_SET_CLASS));
	Con_Builtins_Set_Atom *set_atom = (Con_Builtins_Set_Atom *) new_set->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (set_atom + 1);
	set_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;

	set_atom->atom_type = CON_BUILTIN(CON_BUILTIN_SET_ATOM_DEF_OBJECT);
	
	Con_Int num_entries_allocated = num_entries + num_entries / 4 + 1 > CON_DEFAULT_SET_NUM_ENTRIES_ALLOCATED ? num_entries + num_entries / 4 + 1 : CON_DEFAULT_SET_NUM_ENTRIES_ALLOCATED;
	set_atom->entries = Con_Memory_malloc(thread, sizeof(Con_Builtins_Set_Atom_Entry) * num_entries_allocated, CON_MEMORY_CHUNK_OPAQUE);
	for (Con_Int i = 0; i < num_entries_allocated; i += 1)
		set_atom->entries[i].obj = NULL;
	set_atom->num_entries = 0;
	set_atom->num_entries_allocated = num_entries_allocated;

	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	Con_Memory_change_chunk_type(thread, new_set, CON_MEMORY_CHUNK_OBJ);

	return new_set;
}



//
// Create a new set and initialize it with 'num_entries' objects from the stack.
//
// This is intended only for use by _Con_Builtins_VM_Atom_execute.
//

Con_Obj *Con_Builtins_Set_Atom_new_from_con_stack(Con_Obj *thread, Con_Obj *con_stack, Con_Int num_entries)
{
	Con_Obj *new_set = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(Con_Builtins_Set_Atom) + sizeof(Con_Builtins_Slots_Atom), CON_BUILTIN(CON_BUILTIN_SET_CLASS));
	Con_Builtins_Set_Atom *set_atom = (Con_Builtins_Set_Atom *) new_set->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (set_atom + 1);
	set_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;

	set_atom->atom_type = CON_BUILTIN(CON_BUILTIN_SET_ATOM_DEF_OBJECT);
	
	Con_Int num_entries_allocated = num_entries + num_entries / 4 + 1 > CON_DEFAULT_SET_NUM_ENTRIES_ALLOCATED ? num_entries + num_entries / 4 + 1 : CON_DEFAULT_SET_NUM_ENTRIES_ALLOCATED;
	set_atom->entries = Con_Memory_malloc(thread, sizeof(Con_Builtins_Set_Atom_Entry) * num_entries_allocated, CON_MEMORY_CHUNK_OPAQUE);
	for (Con_Int i = 0; i < num_entries_allocated; i += 1)
		set_atom->entries[i].obj = NULL;
	set_atom->num_entries = 0;
	set_atom->num_entries_allocated = num_entries_allocated;

	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	Con_Memory_change_chunk_type(thread, new_set, CON_MEMORY_CHUNK_OBJ);

	for (Con_Int i = 0; i < num_entries; i += 1) {
		CON_MUTEX_LOCK(&con_stack->mutex);
		Con_Obj *obj = Con_Builtins_Con_Stack_Atom_pop_object(thread, con_stack);
		CON_MUTEX_UNLOCK(&con_stack->mutex);
		
		Con_Hash hash = Con_Hash_get(thread, obj);
		
		CON_MUTEX_LOCK(&new_set->mutex);
		if (set_atom->num_entries == set_atom->num_entries_allocated)
			CON_XXX;
		
		Con_Int new_pos = Con_Builtins_Set_Atom_find_entry(thread, &new_set->mutex, set_atom->entries, set_atom->num_entries_allocated, obj, hash);
		
		if (set_atom->entries[new_pos].obj != NULL) {
			// If an object equal to 'obj' is already in the set, then we don't overwrite it with the
			// newly encountered one. This is because we're popping objects off the stack in reverse
			// order so that if there's a set along the lines of:
			//
			//   Set{"a", "b", "c", "a"}
			//
			// We'll have added the right-most "a" into the set first. If we were to overwrite it
			// with the left-hand "a", this would subvert the users expectations that Converge
			// evaluates strictly left-to-right. [Of course, anyone who uses a set initializer like
			// the above has probably made an unintentional mistake, but that's not the point.]
			
			CON_MUTEX_UNLOCK(&new_set->mutex);
			continue;
		}
		
		set_atom->entries[new_pos].hash = hash;
		set_atom->entries[new_pos].obj = obj;
		set_atom->num_entries += 1;
		CON_MUTEX_UNLOCK(&new_set->mutex);
	}
	
	return new_set;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions
//

//
// Returns the offset in entries that 'obj' does / would occupy.
//
// mutex should be the mutex for the object set is bound in. It should be locked on calling this
// function; it may be unlocked during this functions execution but will be locked when a value is
// returned.
//
// If the hash table is full, the results of calling this function are undefined. The hash table
// should always have at least 1 free entry.
//

Con_Int Con_Builtins_Set_Atom_find_entry(Con_Obj *thread, Con_Mutex *mutex, Con_Builtins_Set_Atom_Entry *entries, Con_Int num_entries_allocated, Con_Obj *obj, Con_Hash hash)
{
	CON_ASSERT_MUTEX_LOCKED(mutex);

	Con_Int i = hash % num_entries_allocated;
	while (1) {
		// Since hash can be a negative number, i may be less than 0.
		if (i < 0)
			i = -i;
		if (entries[i].obj == NULL)
			return i;
	
		if (hash == entries[i].hash) {
			Con_Obj *obj_orig_val = entries[i].obj;
			CON_MUTEX_UNLOCK(mutex);
			bool objs_equals = Con_Object_eq(thread, obj, entries[i].obj);
			CON_MUTEX_LOCK(mutex);
			if ((i < num_entries_allocated) && (entries[i].obj != obj_orig_val)) {
				// The sets contents have shuffled too much when we were checking object equality.
				CON_XXX;
			}
			if (objs_equals)
				return i;
		}
				
		i = (i + 1) % hash % num_entries_allocated;
	}
}



//
// Add 'obj' with hash 'hash' into 'set'. The mutex for 'set' must be held when this function is
// called; it may be unlocked during this functions execution but will be locked upon return.
// 

void Con_Builtins_Set_Atom_add_entry(Con_Obj *thread, Con_Obj *set, Con_Obj *obj, Con_Hash hash)
{
	CON_ASSERT_MUTEX_LOCKED(&set->mutex);
	
	Con_Builtins_Set_Atom *set_atom = CON_GET_ATOM(set, CON_BUILTIN(CON_BUILTIN_SET_ATOM_DEF_OBJECT));
	
	Con_Int pos = Con_Builtins_Set_Atom_find_entry(thread, &set->mutex, set_atom->entries, set_atom->num_entries_allocated, obj, hash);
	if (set_atom->entries[pos].obj == NULL) {
		if (set_atom->num_entries + (set_atom->num_entries / 4) + 1 >= set_atom->num_entries_allocated) {
			Con_Int new_num_entries_allocated = set_atom->num_entries + (set_atom->num_entries / 4) + 2;
			Con_Builtins_Set_Atom_Entry *new_entries = Con_Memory_malloc_no_gc(thread, sizeof(Con_Builtins_Set_Atom_Entry) * new_num_entries_allocated, CON_MEMORY_CHUNK_OPAQUE);
			if (new_entries == NULL)
				CON_XXX;
			
			for (Con_Int i = 0; i < new_num_entries_allocated; i += 1)
				new_entries[i].obj = NULL;
			
			for (Con_Int i = 0; i < set_atom->num_entries_allocated; i += 1) {
				if (set_atom->entries[i].obj == NULL)
					continue;
				
				Con_Int new_pos = Con_Builtins_Set_Atom_find_entry(thread, &set->mutex, new_entries, new_num_entries_allocated, set_atom->entries[i].obj, set_atom->entries[i].hash);
				new_entries[new_pos] = set_atom->entries[i];
			}
			
			set_atom->entries = new_entries;
			set_atom->num_entries_allocated = new_num_entries_allocated;
			pos = Con_Builtins_Set_Atom_find_entry(thread, &set->mutex, set_atom->entries, set_atom->num_entries_allocated, obj, hash);
		}
		set_atom->num_entries += 1;
	}

	set_atom->entries[pos].hash = hash;
	set_atom->entries[pos].obj = obj;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Garbage collection
//

void _Con_Builtins_Set_Atom_gc_scan_func(Con_Obj *thread, Con_Obj *obj, Con_Atom *atom)
{
	Con_Builtins_Set_Atom *set_atom = (Con_Builtins_Set_Atom *) atom;

	Con_Memory_gc_push(thread, set_atom->entries);
	
	for (int i = 0; i < set_atom->num_entries_allocated; i += 1) {
		if (set_atom->entries[i].obj != NULL)
			Con_Memory_gc_push(thread, set_atom->entries[i].obj);
	}
}
