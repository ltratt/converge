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
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"


Con_Int _Con_Builtins_String_Atom_find_cached_pos(Con_Obj *, Con_Builtins_String_Class_Unique_Atom *, const char *, Con_Int, Con_String_Encoding, Con_Int);
void _Con_Builtins_String_Atom_set_cached(Con_Obj *, Con_Builtins_String_Class_Unique_Atom *, Con_Int, Con_Obj *);

void _Con_Builtins_String_Class_unique_atom_gc_scan_func(Con_Obj *, Con_Obj *, Con_Builtins_Unique_Atom_Def *);

void _Con_Builtins_String_Class_gc_scan_func(Con_Obj *, Con_Obj *, Con_Atom *);

Con_Obj *_Con_Builtins_String_Class_eq_func(Con_Obj *);
Con_Obj *_Con_Builtins_String_Class_add_func(Con_Obj *);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_String_Atom_bootstrap(Con_Obj *thread)
{
	Con_Obj *string_atom_def = CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT);
	
	Con_Builtins_Atom_Def_Atom *atom_def_atom = (Con_Builtins_Atom_Def_Atom *) string_atom_def->first_atom;
	Con_Builtins_String_Class_Unique_Atom *unique_atom = (Con_Builtins_String_Class_Unique_Atom *) (atom_def_atom + 1);
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (unique_atom + 1);

	atom_def_atom->next_atom = (Con_Atom *) unique_atom;
	unique_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;
	
	Con_Builtins_Atom_Def_Atom_init_atom(thread, atom_def_atom, _Con_Builtins_String_Class_gc_scan_func, NULL);
	Con_Builtins_Unique_Atom_Def_init_atom(thread, (Con_Builtins_Unique_Atom_Def *) unique_atom, _Con_Builtins_String_Class_unique_atom_gc_scan_func);
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);

	unique_atom->num_cache_entries_allocated = CON_DEFAULT_STRING_NUM_CACHE_ENTRIES;
	unique_atom->num_cache_entries = 0;
	unique_atom->cache = Con_Memory_malloc(thread, unique_atom->num_cache_entries_allocated * sizeof(Con_Builtins_String_Class_Unique_Atom_Cache_Entry), CON_MEMORY_CHUNK_OPAQUE);
	for (Con_Int i = 0; i < unique_atom->num_cache_entries_allocated; i += 1) {
		unique_atom->cache[i].str = NULL;
	}
	
	Con_Memory_change_chunk_type(thread, string_atom_def, CON_MEMORY_CHUNK_OBJ);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// String creation functions
//

Con_Obj *Con_Builtins_String_Atom_new_copy(Con_Obj *thread, const char *str, Con_Int size, Con_String_Encoding enc)
{
	// As a speed hack, we pull the unique atom out of the String class directly; this can be a
	// surprisingly large saving but does rely on the unique atom being at a fixed position
	// within the int class object.

	Con_Builtins_String_Class_Unique_Atom *unique_atom = (Con_Builtins_String_Class_Unique_Atom *) CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT)->first_atom->next_atom;
	assert(unique_atom == CON_GET_ATOM(CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT), CON_BUILTIN(CON_BUILTIN_UNIQUE_ATOM_DEF_OBJECT)));

	Con_Int hash = Con_Hash_calc_string_hash(thread, str, size);
	
	Con_Int i = _Con_Builtins_String_Atom_find_cached_pos(thread, unique_atom, str, size, enc, hash);
	if (unique_atom->cache[i].str != NULL) {
		unique_atom->cache[i].num_users += 1;
		return unique_atom->cache[i].str;
	}

	Con_Obj *new_str = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(Con_Builtins_String_Atom), CON_BUILTIN(CON_BUILTIN_STRING_CLASS));
	new_str->first_atom->next_atom = NULL;
	
	u_char *str_mem = Con_Memory_malloc(thread, size, CON_MEMORY_CHUNK_OPAQUE);
	memmove(str_mem, str, size);
	Con_Builtins_String_Atom_init_atom(thread, (Con_Builtins_String_Atom *) new_str->first_atom, str_mem, size, enc, hash);
	
	Con_Memory_change_chunk_type(thread, new_str, CON_MEMORY_CHUNK_OBJ);
	
	_Con_Builtins_String_Atom_set_cached(thread, unique_atom, i, new_str);
	
	return new_str;
}



Con_Obj *Con_Builtins_String_Atom_new_no_copy(Con_Obj *thread, const char *str, Con_Int size, Con_String_Encoding enc)
{
	// As a speed hack, we pull the unique atom out of the String class directly; this can be a
	// surprisingly large saving but does rely on the unique atom being at a fixed position
	// within the int class object.

	Con_Builtins_String_Class_Unique_Atom *unique_atom = (Con_Builtins_String_Class_Unique_Atom *) CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT)->first_atom->next_atom;
	assert(unique_atom == CON_GET_ATOM(CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT), CON_BUILTIN(CON_BUILTIN_UNIQUE_ATOM_DEF_OBJECT)));

	Con_Int hash = Con_Hash_calc_string_hash(thread, str, size);
	
	Con_Int i = _Con_Builtins_String_Atom_find_cached_pos(thread, unique_atom, str, size, enc, hash);
	if (unique_atom->cache[i].str != NULL) {
		unique_atom->cache[i].num_users += 1;
		return unique_atom->cache[i].str;
	}

	Con_Obj *new_str = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(Con_Builtins_String_Atom), CON_BUILTIN(CON_BUILTIN_STRING_CLASS));
	new_str->first_atom->next_atom = NULL;
	
	Con_Builtins_String_Atom_init_atom(thread, (Con_Builtins_String_Atom *) new_str->first_atom, str, size, enc, hash);
	
	Con_Memory_change_chunk_type(thread, new_str, CON_MEMORY_CHUNK_OBJ);
	
	_Con_Builtins_String_Atom_set_cached(thread, unique_atom, i, new_str);
	
	return new_str;
}



void Con_Builtins_String_Atom_init_atom(Con_Obj *thread, Con_Builtins_String_Atom *str_atom, const char *str, Con_Int size, Con_String_Encoding enc, Con_Int hash)
{
	if (enc != CON_STR_UTF_8)
		CON_XXX;

	str_atom->atom_type = CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT);

	str_atom->encoding = enc;
	str_atom->size = size;
	str_atom->str = str;
	str_atom->hash = hash;
}



//
// Find the position in the hashtable of a string.
//

Con_Int _Con_Builtins_String_Atom_find_cached_pos(Con_Obj *thread, Con_Builtins_String_Class_Unique_Atom *unique_atom, const char *str, Con_Int size, Con_String_Encoding enc, Con_Int hash)
{
	Con_Int i = hash % unique_atom->num_cache_entries_allocated;
	// Since hash can be a negative number, i may be less than 0.
	if (i < 0)
		i = -i;
	while (1) {
		if (unique_atom->cache[i].str == NULL)
			return i;
		
		Con_Builtins_String_Atom *string_atom = CON_GET_ATOM(unique_atom->cache[i].str, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
		if (string_atom->hash == hash && string_atom->size == size && string_atom->encoding == enc && bcmp(str, string_atom->str, size) == 0)
			return i;
		
		i = (i + 1) % unique_atom->num_cache_entries_allocated;
	}
	
	return NULL;
}



//
// Set entry 'i' in the hashtable to 'str'. If the hashtable is full, it will be rebalanced, and the
// string may be placed at a location other than 'i'. After calling this function, all information
// previously known about the hashtable and its entries should be considered out of date, and
// recalculated if necessary.
//

void _Con_Builtins_String_Atom_set_cached(Con_Obj *thread, Con_Builtins_String_Class_Unique_Atom *unique_atom, Con_Int i, Con_Obj *str)
{
	if (unique_atom->num_cache_entries + (unique_atom->num_cache_entries / 4) > unique_atom->num_cache_entries_allocated) {
		// The hash table is getting full, so we need to create a new one.
		//
		// This is muddied by the fact that while we try to purge "old" entries (those with few
		// apparent users), sometimes nothing in the cache will appear to be old enough to purge.
		// Since that would mean creating a new cache that was identical to the old cache, in such
		// cases we somewhat arbitrarily don't copy over some entries, in order to be sure that the
		// new cache has some spare slots.
	
		Con_Builtins_String_Class_Unique_Atom_Cache_Entry *old_cache = unique_atom->cache;
	
		unique_atom->cache = Con_Memory_malloc(thread, unique_atom->num_cache_entries_allocated * sizeof(Con_Builtins_String_Class_Unique_Atom_Cache_Entry), CON_MEMORY_CHUNK_OPAQUE);
		unique_atom->num_cache_entries = 0;
		Con_Int j;
		for (j = 0; j < unique_atom->num_cache_entries_allocated; j += 1) {
			unique_atom->cache[j].str = NULL;
		}

		for (j = 0; j < unique_atom->num_cache_entries_allocated; j += 1) {
			if (unique_atom->num_cache_entries + (unique_atom->num_cache_entries / 3) > unique_atom->num_cache_entries_allocated) {
				// Bugger. Since we weren't able to purge enough old records, we now stop copying
				// from the old to the new cache, to make sure that there is at least some room
				// in the new hash table.
				break;
			}
		
			if (old_cache[j].str == NULL || old_cache[j].num_users < 2)
				continue;
			
			Con_Builtins_String_Atom *new_string_atom = CON_GET_ATOM(old_cache[j].str, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
			Con_Int new_i = _Con_Builtins_String_Atom_find_cached_pos(thread, unique_atom, new_string_atom->str, new_string_atom->size, new_string_atom->encoding, new_string_atom->hash);
			
			// When we copy over the entry we deliberately reset the number of apparent users. This
			// is to ensure that strings that were frequently created at one point, but are no longer
			// created frequently, don't clog up the cache.
			
			Con_Builtins_String_Class_Unique_Atom_Cache_Entry *entry = &unique_atom->cache[new_i];
			entry->num_users = 1;
			entry->str = str;
			unique_atom->num_cache_entries += 1;
		}
		
		assert(unique_atom->num_cache_entries + (unique_atom->num_cache_entries / 4) <= unique_atom->num_cache_entries_allocated);
		
		Con_Builtins_String_Atom *string_atom = CON_GET_ATOM(str, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
		i = _Con_Builtins_String_Atom_find_cached_pos(thread, unique_atom, string_atom->str, string_atom->size, string_atom->encoding, string_atom->hash);
	}
	
	Con_Builtins_String_Class_Unique_Atom_Cache_Entry *entry = &unique_atom->cache[i];
	
	entry->num_users = 1;
	entry->str = str;
	unique_atom->num_cache_entries += 1;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions
//

//
// Given a Converge string 'string', return a pointer to a NULL-terminated C string.
//
// The pointer returned must not be explicitly freed; if memory is allocated, it will be freed by
// the garbage collector.
//

char *Con_Builtins_String_Atom_to_c_string(Con_Obj *thread, Con_Obj *string)
{
	Con_Builtins_String_Atom *string_atom = CON_GET_ATOM(string, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	
	assert(string_atom->encoding == CON_STR_UTF_8);
	
	char *c_str = Con_Memory_malloc(thread, string_atom->size + 1, CON_MEMORY_CHUNK_OPAQUE);
	memmove(c_str, string_atom->str, string_atom->size);
	c_str[string_atom->size] = 0;
	
	return c_str;
}



//
// Compare the UTF-8 string 'c_string' of size 'c_string_size' with 'string_obj' for equality.
//

bool Con_Builtins_String_Atom_c_string_eq(Con_Obj *thread, const char *c_string, Con_Int c_string_size, Con_Obj *string_obj)
{
	Con_Builtins_String_Atom *string_obj_atom = CON_FIND_ATOM(string_obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	
	if (string_obj_atom == NULL)
		return false;
	
	if (string_obj_atom->encoding == CON_STR_UTF_8) {
		if (string_obj_atom->size != c_string_size)
			return false;
			
		if (memcmp(c_string, string_obj_atom->str, c_string_size) == 0)
			return true;
		else
			return false;
	}
	else
		CON_XXX;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Garbage collection
//

void _Con_Builtins_String_Class_unique_atom_gc_scan_func(Con_Obj *thread, Con_Obj *obj, Con_Builtins_Unique_Atom_Def *unique_atom)
{
	Con_Builtins_String_Class_Unique_Atom *string_unique_atom = (Con_Builtins_String_Class_Unique_Atom *) unique_atom;
	
	Con_Memory_gc_push_ptr(thread, string_unique_atom->cache);

	for (Con_Int i = 0; i < string_unique_atom->num_cache_entries_allocated; i += 1) {
		if (string_unique_atom->cache[i].str != NULL) {
			Con_Memory_gc_push_ptr(thread, string_unique_atom->cache[i].str);
		}
	}
}



void _Con_Builtins_String_Class_gc_scan_func(Con_Obj *thread, Con_Obj *obj, Con_Atom *atom)
{
	Con_Builtins_String_Atom * str_atom = (Con_Builtins_String_Atom *) atom;

	Con_Memory_gc_push_ptr(thread, str_atom->str);
}
