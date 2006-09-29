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
#include "Core.h"
#include "Hash.h"
#include "Memory.h"
#include "Object.h"
#include "Shortcuts.h"
#include "Slots.h"

#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



Con_Int _Con_Slots_find_pos(Con_Obj *, Con_Slots_Hash_Entry *, Con_Int, u_char *, const char *, Con_Int);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Slots functions
//

//
// Initialize a Con_Slots. This must be called before any other slots functions are called.
//
// This function does not assume that any mutexes are held relating to 'slots'.
//

void Con_Slots_init(Con_Obj *thread, Con_Slots *slots)
{
	slots->num_entries = 0;
	slots->num_hash_entries_allocated = 0;
	slots->hash_entries = NULL;
	slots->full_entries_size = slots->full_entries_size_allocated = 0;
	slots->full_entries = NULL;
}



//
// Returns the offset in hash_entries that 'slot_name' does / would occupy.
//
// If the hash table is full, the results of calling this function are undefined. The hash table
// should always have at least 1 free entry.
//

Con_Int _Con_Slots_find_pos(Con_Obj *thread, Con_Slots_Hash_Entry *hash_entries, Con_Int num_hash_entries_allocated, u_char *full_entries, const char *slot_name, Con_Int slot_name_size)
{
	Con_Int hash = Con_Hash_calc_string_hash(thread, slot_name, slot_name_size);
	Con_Int i = hash % num_hash_entries_allocated;
	while (1) {
		// Since hash can be a negative number, i may be less than 0.
		if (i < 0)
			i = -i;
		if (hash_entries[i].full_entry_offset == -1)
			return i;
		
		if (hash == hash_entries[i].hash) {
			Con_Slots_Full_Entry *full_entry = (Con_Slots_Full_Entry *) (full_entries + hash_entries[i].full_entry_offset);
			if ((slot_name_size == full_entry->slot_name_size) && (memcmp(slot_name, full_entry->slot_name, slot_name_size) == 0))
				return i;
		}
		
		i = (i + 1) % num_hash_entries_allocated;
	}
}



//
// Returns the value of slot 'slot_name' in 'slots' or NULL if no such is found.
//
// Assumes the relevant mutex for 'slots' is locked by the caller if necessary.
//

Con_Obj *Con_Slots_get_slot(Con_Obj *thread, Con_Slots *slots, const char *slot_name, Con_Int slot_name_size)
{
	if (slots->hash_entries == NULL)
		return NULL;

	Con_Int pos = _Con_Slots_find_pos(thread, slots->hash_entries, slots->num_hash_entries_allocated, slots->full_entries, slot_name, slot_name_size);
	Con_Int offset = slots->hash_entries[pos].full_entry_offset;
	if (offset != -1)
		return ((Con_Slots_Full_Entry *) (slots->full_entries + offset))->value;
	else
		return NULL;
}



//
// Sets the value of slot 'slot_name' in 'slots' to val.
//
// If the Con_Slots reference is contained within a live object, 'mutex' should point to the objects'
// mutex which must be held when this function is called. This function may unlock 'mutex' during its
// operation, but will lock it again before returning.
//

void Con_Slots_set_slot(Con_Obj *thread, Con_Mutex *mutex, Con_Slots *slots, const char *slot_name, Con_Int slot_name_size, Con_Obj *val)
{
	if (slots->hash_entries == NULL) {
		assert(mutex != NULL);
		CON_MUTEX_UNLOCK(mutex);
		
		Con_Slots_Hash_Entry *hash_entries = Con_Memory_malloc_no_gc(thread, sizeof(Con_Slots_Hash_Entry) * CON_DEFAULT_NUM_SLOTS, CON_MEMORY_CHUNK_OPAQUE);
		if (hash_entries == NULL)
			CON_XXX;
		u_char *full_entries = Con_Memory_malloc_no_gc(thread, CON_DEFAULT_FULL_ENTRIES_SIZE, CON_MEMORY_CHUNK_OPAQUE);
		if (full_entries == NULL)
			CON_XXX;
		for (int i = 0; i < CON_DEFAULT_NUM_SLOTS; i += 1)
			hash_entries[i].full_entry_offset = -1;
		
		CON_MUTEX_LOCK(mutex);
		if (slots->hash_entries != NULL)
			CON_XXX;
		slots->hash_entries = hash_entries;
		slots->num_hash_entries_allocated = CON_DEFAULT_NUM_SLOTS;
		slots->full_entries = full_entries;
		slots->full_entries_size_allocated = CON_DEFAULT_FULL_ENTRIES_SIZE;
	}
	
	Con_Int pos = _Con_Slots_find_pos(thread, slots->hash_entries, slots->num_hash_entries_allocated, slots->full_entries, slot_name, slot_name_size);
	Con_Int offset = slots->hash_entries[pos].full_entry_offset;
	if (offset != -1) {
		// Overwrite the previous value this slot had.
		((Con_Slots_Full_Entry *) (slots->full_entries + offset))->value = val;
	}
	else {
		// Create a new entry.
		if (slots->num_entries + (slots->num_entries / 4) + 1 >= slots->num_hash_entries_allocated) {
			// We're running out of room, so allocate more entries in the hash table.
			
			Con_Int num_hash_entries_allocated = slots->num_entries + (slots->num_entries / 2) + 2;
			Con_Slots_Hash_Entry *hash_entries = Con_Memory_malloc_no_gc(thread, sizeof(Con_Slots_Hash_Entry) * num_hash_entries_allocated, CON_MEMORY_CHUNK_OPAQUE);
			if (hash_entries == NULL)
				CON_XXX;
			
			for (Con_Int i = 0; i < num_hash_entries_allocated; i += 1)
				hash_entries[i].full_entry_offset = -1;

			// Transfer the old hash table to the new hash table.
			//
			// Note that we only need to transfer the hash entries themselves; the "full" entries can
			// stay as is.
			
			for (Con_Int i = 0; i < slots->num_hash_entries_allocated; i += 1) {
				Con_Slots_Hash_Entry *hash_entry = &slots->hash_entries[i];
				if (hash_entry->full_entry_offset == -1)
					continue;
				Con_Slots_Full_Entry *full_entry = (Con_Slots_Full_Entry *) (slots->full_entries + hash_entry->full_entry_offset);
				Con_Int new_pos = _Con_Slots_find_pos(thread, hash_entries, num_hash_entries_allocated, slots->full_entries, full_entry->slot_name, full_entry->slot_name_size);
				assert(hash_entries[new_pos].full_entry_offset == -1);
				hash_entries[new_pos].hash = hash_entry->hash;
				hash_entries[new_pos].full_entry_offset = hash_entry->full_entry_offset;
			}
			
			slots->hash_entries = hash_entries;
			slots->num_hash_entries_allocated = num_hash_entries_allocated;
			
			// Since the entries in the new hash table are likely to be different than the old, we
			// have to recalculate the 'pos' that the new entry can occupy.
			
			pos = _Con_Slots_find_pos(thread, slots->hash_entries, slots->num_hash_entries_allocated, slots->full_entries, slot_name, slot_name_size);
		}
		
		Con_Int new_offset = slots->full_entries_size;
		if (Con_Arch_align(thread, new_offset + ((Con_Int) sizeof(Con_Slots_Full_Entry)) + slot_name_size) > slots->full_entries_size_allocated) {
			// As a heuristic, we allocate space for 5 slots of the currently required size.
			Con_Int new_full_entries_size_allocated = slots->full_entries_size_allocated + (((Con_Int) sizeof(Con_Slots_Full_Entry)) + slot_name_size) * 5;
			u_char *new_full_entries = Con_Memory_realloc_no_gc(thread, slots->full_entries, new_full_entries_size_allocated);
			if (new_full_entries == NULL)
				CON_XXX;
			slots->full_entries = new_full_entries;
			slots->full_entries_size_allocated = new_full_entries_size_allocated;
		}
		slots->hash_entries[pos].hash = Con_Hash_calc_string_hash(thread, slot_name, slot_name_size);
		slots->hash_entries[pos].full_entry_offset = new_offset;
		Con_Slots_Full_Entry *full_entry = (Con_Slots_Full_Entry *) (slots->full_entries + new_offset);
		full_entry->slot_name_size = slot_name_size;
		full_entry->value = val;
		memmove(full_entry->slot_name, slot_name, slot_name_size);
		slots->full_entries_size += Con_Arch_align(thread, sizeof(Con_Slots_Full_Entry) + slot_name_size);
		slots->num_entries += 1;
	}
}



//
// Read all values in a Con_Slots.
//
// Read the value of slot num 'i' into 'slot_name', 'slot_name_size', and 'val'. 'i' should point to
// an integer whose initial value is 0; after each call of Con_Slots_read_slot this will be updated
// so that the next call reads the next slot in the hash table. If there is no slot to read, false is
// returned and the values of 'slot_name' etc are undefined.
//

bool Con_Slots_read_slot(Con_Obj *thread, Con_Slots *slots, Con_Int *i, const char **slot_name, Con_Int *slot_name_size, Con_Obj **val)
{
	while (*i < slots->num_hash_entries_allocated) {
		if (slots->hash_entries[*i].full_entry_offset == -1) {
			*i += 1;
			continue;
		}
		
		Con_Slots_Full_Entry *full_entry = (Con_Slots_Full_Entry *) (slots->full_entries + slots->hash_entries[*i].full_entry_offset);
		*slot_name = full_entry->slot_name;
		*slot_name_size = full_entry->slot_name_size;
		*val = full_entry->value;
		
		*i += 1;
		return true;
	}
	
	return false;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Garbage collection
//

void Con_Slots_gc_scan_slots(Con_Obj *thread, Con_Slots *slots)
{
	if (slots->hash_entries != NULL)
		Con_Memory_gc_push(thread, slots->hash_entries);

	if (slots->full_entries != NULL)
		Con_Memory_gc_push(thread, slots->full_entries);

	for (int i = 0; i < slots->num_hash_entries_allocated; i += 1) {
		Con_Slots_Hash_Entry *hash_entry = &slots->hash_entries[i];
		if (hash_entry->full_entry_offset == -1)
			continue;
		Con_Memory_gc_push(thread, ((Con_Slots_Full_Entry *) (slots->full_entries + hash_entry->full_entry_offset))->value);
	}
}
