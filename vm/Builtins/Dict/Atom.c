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
#include "Builtins/Dict/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



void _Con_Builtins_Dict_Atom_gc_scan_func(Con_Obj *, Con_Obj *, Con_Atom *);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_Dict_Atom_bootstrap(Con_Obj *thread)
{
	Con_Obj *dict_class = CON_BUILTIN(CON_BUILTIN_DICT_ATOM_DEF_OBJECT);

	Con_Builtins_Atom_Def_Atom *atom_def_atom = (Con_Builtins_Atom_Def_Atom *) dict_class->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (atom_def_atom + 1);

	atom_def_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;
	
	Con_Builtins_Atom_Def_Atom_init_atom(thread, atom_def_atom, _Con_Builtins_Dict_Atom_gc_scan_func, NULL);
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	
	Con_Memory_change_chunk_type(thread, dict_class, CON_MEMORY_CHUNK_OBJ);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Dictionary creation functions
//

Con_Obj *Con_Builtins_Dict_Atom_new(Con_Obj *thread)
{
	Con_Obj *new_dict = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(Con_Builtins_Dict_Atom) + sizeof(Con_Builtins_Slots_Atom), CON_BUILTIN(CON_BUILTIN_DICT_CLASS));
	Con_Builtins_Dict_Atom *dict_atom = (Con_Builtins_Dict_Atom *) new_dict->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (dict_atom + 1);
	dict_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;
	
	Con_Builtins_Dict_Atom_init_atom(thread, dict_atom);
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	
	Con_Memory_change_chunk_type(thread, new_dict, CON_MEMORY_CHUNK_OBJ);
	
	return new_dict;
}



void Con_Builtins_Dict_Atom_init_atom(Con_Obj *thread, Con_Builtins_Dict_Atom *dict_atom)
{
	dict_atom->atom_type = CON_BUILTIN(CON_BUILTIN_DICT_ATOM_DEF_OBJECT);
	
	dict_atom->entries = Con_Memory_malloc(thread, sizeof(Con_Builtins_Dict_Class_Hash_Entry) * CON_DEFAULT_DICT_NUM_ENTRIES_ALLOCATED, CON_MEMORY_CHUNK_OPAQUE);
	bzero(dict_atom->entries, sizeof(Con_Builtins_Dict_Class_Hash_Entry) * CON_DEFAULT_DICT_NUM_ENTRIES_ALLOCATED);
	dict_atom->num_entries = 0;
	dict_atom->num_entries_allocated = CON_DEFAULT_DICT_NUM_ENTRIES_ALLOCATED;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions
//

//
// Returns the offset in entries that key does / would occupy.
//
// mutex should be the mutex for the object dict is bound in. It should be locked on calling this
// function; it may be unlocked during this functions execution but will be locked when a value is
// returned.
//
// If the hash table is full, the results of calling this function are undefined. The hash table
// should always have at least 1 free entry.
//

Con_Int Con_Builtins_Dict_Atom_find_entry(Con_Obj *thread, Con_Mutex *mutex, Con_Builtins_Dict_Class_Hash_Entry *entries, Con_Int num_entries_allocated, Con_Obj *key, Con_Hash hash)
{
	CON_ASSERT_MUTEX_LOCKED(mutex);

	Con_Int i = hash % num_entries_allocated;
	// Since hash can be a negative number, i may be less than 0.
	if (i < 0)
		i = -i;
	while (1) {
		if (entries[i].key == NULL)
			return i;
	
		if (hash == entries[i].hash) {
			Con_Obj *key_orig_val = entries[i].key;
			CON_MUTEX_UNLOCK(mutex);
			bool keys_equals = Con_Object_eq(thread, key, entries[i].key);
			CON_MUTEX_LOCK(mutex);
			if ((i < num_entries_allocated) && (entries[i].key != key_orig_val)) {
				// The dictionaries contents have shuffled too much when we were checking object
				// equality.
				CON_XXX;
			}
			if (keys_equals)
				return i;
		}
				
		i = (i + 1) % num_entries_allocated;
	}
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Garbage collection
//

void _Con_Builtins_Dict_Atom_gc_scan_func(Con_Obj *thread, Con_Obj *obj, Con_Atom *atom)
{
	Con_Builtins_Dict_Atom *dict_atom = (Con_Builtins_Dict_Atom *) atom;

	Con_Memory_gc_push(thread, dict_atom->entries);
	
	for (int i = 0; i < dict_atom->num_entries_allocated; i += 1) {
		if (dict_atom->entries[i].key == NULL)
			continue;
		Con_Memory_gc_push(thread, dict_atom->entries[i].key);
		Con_Memory_gc_push(thread, dict_atom->entries[i].val);
	}
}
