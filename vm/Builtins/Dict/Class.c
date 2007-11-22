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

#include "Builtins/Class/Atom.h"
#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Dict/Atom.h"
#include "Builtins/Dict/Class.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



Con_Obj *_Con_Builtins_Dict_Class_to_str_func(Con_Obj *);
Con_Obj *_Con_Builtins_Dict_Class_del_func(Con_Obj *);
Con_Obj *_Con_Builtins_Dict_Class_extend_func(Con_Obj *);
Con_Obj *_Con_Builtins_Dict_Class_find_func(Con_Obj *);
Con_Obj *_Con_Builtins_Dict_Class_get_func(Con_Obj *);
Con_Obj *_Con_Builtins_Dict_Class_iter_func(Con_Obj *);
Con_Obj *_Con_Builtins_Dict_Class_iter_keys_func(Con_Obj *);
Con_Obj *_Con_Builtins_Dict_Class_len_func(Con_Obj *);
Con_Obj *_Con_Builtins_Dict_Class_set_func(Con_Obj *);
Con_Obj *_Con_Builtins_Dict_Class_scopy_func(Con_Obj *);
Con_Obj *_Con_Builtins_Dict_Class_iter_vals_func(Con_Obj *);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_Dict_Class_bootstrap(Con_Obj *thread)
{
	Con_Obj *dict_class = CON_BUILTIN(CON_BUILTIN_DICT_CLASS);

	Con_Builtins_Class_Atom *class_atom = (Con_Builtins_Class_Atom *) dict_class->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (class_atom + 1);

	class_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;
	
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	Con_Builtins_Class_Atom_init_atom(thread, class_atom, CON_NEW_STRING("Dict"), NULL, CON_BUILTIN(CON_BUILTIN_OBJECT_CLASS), NULL);
	
	Con_Memory_change_chunk_type(thread, dict_class, CON_MEMORY_CHUNK_OBJ);

	CON_SET_FIELD(dict_class, "to_str", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Dict_Class_to_str_func, "to_str", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), dict_class));
	CON_SET_FIELD(dict_class, "del", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Dict_Class_del_func, "del", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), dict_class));
	CON_SET_FIELD(dict_class, "extend", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Dict_Class_extend_func, "extend", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), dict_class));
	CON_SET_FIELD(dict_class, "find", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Dict_Class_find_func, "find", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), dict_class));
	CON_SET_FIELD(dict_class, "get", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Dict_Class_get_func, "get", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), dict_class));
	CON_SET_FIELD(dict_class, "iter", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Dict_Class_iter_func, "iter", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), dict_class));
	CON_SET_FIELD(dict_class, "iter_keys", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Dict_Class_iter_keys_func, "iter_keys", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), dict_class));
	CON_SET_FIELD(dict_class, "len", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Dict_Class_len_func, "len", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), dict_class));
	CON_SET_FIELD(dict_class, "set", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Dict_Class_set_func, "set", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), dict_class));
	CON_SET_FIELD(dict_class, "scopy", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Dict_Class_scopy_func, "scopy", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), dict_class));
	CON_SET_FIELD(dict_class, "iter_vals", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Dict_Class_iter_vals_func, "iter_vals", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), dict_class));
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Dict class fields
//

//
// 'to_str().
//

Con_Obj *_Con_Builtins_Dict_Class_to_str_func(Con_Obj *thread)
{
	Con_Obj *default_val, *key, *self;
	CON_UNPACK_ARGS("D", &self, &key, &default_val);

#	define DICT_TO_STR_START "Dict{"
#	define DICT_TO_STR_START_SIZE ((Con_Int) (sizeof(DICT_TO_STR_START) - 1))
#	define DICT_TO_STR_MAP " : "
#	define DICT_TO_STR_MAP_SIZE ((Con_Int) (sizeof(DICT_TO_STR_MAP) - 1))
#	define DICT_TO_STR_SEPERATOR ", "
#	define DICT_TO_STR_SEPERATOR_SIZE ((Con_Int) (sizeof(DICT_TO_STR_SEPERATOR) - 1))
#	define DICT_TO_STR_END "}"
#	define DICT_TO_STR_END_SIZE ((Con_Int) (sizeof(DICT_TO_STR_END) - 1))

	Con_Builtins_Dict_Atom *dict_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_DICT_ATOM_DEF_OBJECT));

	CON_MUTEX_LOCK(&self->mutex);

	// We have to make some sort of guess about how much memory we're going to need. Unfortunately
	// the penalty for guessing wrong either way is likely to be quite high.

	Con_Int str_mem_size_allocated;
	if (dict_atom->num_entries == 0)
		// This is the easy case; if there are no entries we only need to allocate constant size
		// memory to hold "Dict{" and "}".
		str_mem_size_allocated = DICT_TO_STR_START_SIZE + DICT_TO_STR_END_SIZE;
	else
		str_mem_size_allocated = dict_atom->num_entries * 20;
	
	u_char *str_mem = Con_Memory_malloc_no_gc(thread, str_mem_size_allocated, CON_MEMORY_CHUNK_OPAQUE);
	if (str_mem == NULL)
		CON_XXX;

	assert(str_mem_size_allocated > DICT_TO_STR_START_SIZE);
	memmove(str_mem, DICT_TO_STR_START, DICT_TO_STR_START_SIZE);
	Con_Int str_mem_size = DICT_TO_STR_START_SIZE;

	Con_Int num_processed = 0;
	for (Con_Int i = 0; i < dict_atom->num_entries_allocated; i += 1) {
		Con_Obj *key = dict_atom->entries[i].key;
		Con_Obj *val = dict_atom->entries[i].val;
		if (key == NULL)
			continue;
		CON_MUTEX_UNLOCK(&self->mutex);
		
		Con_Obj *key_to_str = CON_GET_SLOT_APPLY(key, "to_str");
		Con_Obj *val_to_str = CON_GET_SLOT_APPLY(val, "to_str");

		Con_Builtins_String_Atom *key_str_atom = CON_GET_ATOM(key_to_str, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
		Con_Builtins_String_Atom *val_str_atom = CON_GET_ATOM(val_to_str, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
		if ((key_str_atom->encoding != CON_STR_UTF_8) || (val_str_atom->encoding != CON_STR_UTF_8)) 
			CON_XXX;
		
		Con_Int extra_size_needed;
		if (num_processed == 0)
			extra_size_needed = key_str_atom->size + DICT_TO_STR_MAP_SIZE + val_str_atom->size;
		else
			extra_size_needed = key_str_atom->size + DICT_TO_STR_MAP_SIZE + val_str_atom->size + DICT_TO_STR_SEPERATOR_SIZE;

		if (str_mem_size + extra_size_needed > str_mem_size_allocated) {
			Con_Int new_str_mem_size_allocated = str_mem_size_allocated + str_mem_size_allocated / 2;
			if (new_str_mem_size_allocated < str_mem_size + extra_size_needed)
				new_str_mem_size_allocated = str_mem_size + extra_size_needed;
			str_mem = Con_Memory_realloc(thread, str_mem, new_str_mem_size_allocated);
			str_mem_size_allocated = new_str_mem_size_allocated;
		}
		
		if (num_processed > 0) {
			memmove(str_mem + str_mem_size, DICT_TO_STR_SEPERATOR, DICT_TO_STR_SEPERATOR_SIZE);
			str_mem_size += DICT_TO_STR_SEPERATOR_SIZE;
		}
		num_processed += 1;

		memmove(str_mem + str_mem_size, key_str_atom->str, key_str_atom->size);
		str_mem_size += key_str_atom->size;
		memmove(str_mem + str_mem_size, DICT_TO_STR_MAP, DICT_TO_STR_MAP_SIZE);
		str_mem_size += DICT_TO_STR_MAP_SIZE;
		memmove(str_mem + str_mem_size, val_str_atom->str, val_str_atom->size);
		str_mem_size += val_str_atom->size;
		CON_MUTEX_LOCK(&self->mutex);
	}
	CON_MUTEX_UNLOCK(&self->mutex);
	
	if (str_mem_size + DICT_TO_STR_END_SIZE != str_mem_size_allocated) {
		// Since keeping round unused memory for strings is incredibly wasteful, we now realloc the
		// allocated memory to exactly the right size.
	
		str_mem = Con_Memory_realloc(thread, str_mem, str_mem_size + DICT_TO_STR_END_SIZE);
	}
	
	memmove(str_mem + str_mem_size, DICT_TO_STR_END, DICT_TO_STR_END_SIZE);
	str_mem_size += DICT_TO_STR_END_SIZE;
	
	return Con_Builtins_String_Atom_new_no_copy(thread, str_mem, str_mem_size, CON_STR_UTF_8);
}



//
// 'del(key)' deletes 'key' from the dictionary, raising an exception if 'key' is not found.
//

Con_Obj *_Con_Builtins_Dict_Class_del_func(Con_Obj *thread)
{
	Con_Obj *key, *self;
	CON_UNPACK_ARGS("DO", &self, &key);

	Con_Builtins_Dict_Atom *dict_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_DICT_ATOM_DEF_OBJECT));

	Con_Hash hash = Con_Hash_get(thread, key);
	CON_MUTEX_LOCK(&self->mutex);
	Con_Int i = Con_Builtins_Dict_Atom_find_entry(thread, &self->mutex, dict_atom->entries, dict_atom->num_entries_allocated, key, hash);
	if (dict_atom->entries[i].key == NULL)
		CON_RAISE_EXCEPTION("Key_Exception", key);

	Con_Int j = i;
	while (1) {
		j = (j + 1) % dict_atom->num_entries_allocated;
		if (dict_atom->entries[j].key == NULL)
			break;

		Con_Int real_i = dict_atom->entries[j].hash % dict_atom->num_entries_allocated;
		if ((j > i && (real_i <= i || real_i > j)) || (j < i && real_i <= i && real_i > j)) {
			dict_atom->entries[i] = dict_atom->entries[j];
			i = j;
		}
	}
	dict_atom->entries[i].key = NULL;
	dict_atom->num_entries -= 1;
	CON_MUTEX_UNLOCK(&self->mutex);

	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'find(key)' returns the value of 'key'. If 'key' is not found it fails. This function only
// returns a single value.
//

Con_Obj *_Con_Builtins_Dict_Class_find_func(Con_Obj *thread)
{
	Con_Obj *key, *self;
	CON_UNPACK_ARGS("DO", &self, &key);
	
	Con_Builtins_Dict_Atom *dict_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_DICT_ATOM_DEF_OBJECT));

	Con_Hash hash = Con_Hash_get(thread, key);
	CON_MUTEX_LOCK(&self->mutex);
	Con_Int i = Con_Builtins_Dict_Atom_find_entry(thread, &self->mutex, dict_atom->entries, dict_atom->num_entries_allocated, key, hash);
	Con_Obj *val = NULL;
	if (dict_atom->entries[i].key != NULL)
		val = dict_atom->entries[i].val;
	else {
		val = CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
	}
	CON_MUTEX_UNLOCK(&self->mutex);
	
	return val;
}



//
// 'extend(o)' adds all the elements in the dictionary 'o' to. Elements in 'o' will overwrite
// their equivalents in 'self'.
//

Con_Obj *_Con_Builtins_Dict_Class_extend_func(Con_Obj *thread)
{
	Con_Obj *o, *self;
	CON_UNPACK_ARGS("DO", &self, &o);

	Con_Builtins_Dict_Atom *o_dict_atom = CON_FIND_ATOM(o, CON_BUILTIN(CON_BUILTIN_DICT_ATOM_DEF_OBJECT));
	if (o_dict_atom != NULL) {
		CON_MUTEX_LOCK(&o->mutex);
		for (Con_Int i = 0; i < o_dict_atom->num_entries_allocated; i += 1) {
			if (o_dict_atom->entries[i].key == NULL)
				continue;

			Con_Obj *key = o_dict_atom->entries[i].key;
			Con_Obj *val = o_dict_atom->entries[i].val;
			CON_MUTEX_UNLOCK(&o->mutex);

			CON_GET_SLOT_APPLY(self, "set", key, val);

			CON_MUTEX_LOCK(&o->mutex);
		}
		CON_MUTEX_UNLOCK(&o->mutex);

		return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
	}

	CON_RAISE_EXCEPTION("Type_Exception", CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_DICT_CLASS), "path"), o);
}



//
// 'get(key)' returns the value of 'key'. If 'key' is not found an exception is raised.
//

Con_Obj *_Con_Builtins_Dict_Class_get_func(Con_Obj *thread)
{
	Con_Obj *key, *self;
	CON_UNPACK_ARGS("DO", &self, &key);

	Con_Builtins_Dict_Atom *dict_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_DICT_ATOM_DEF_OBJECT));

	Con_Hash hash = Con_Hash_get(thread, key);
	CON_MUTEX_LOCK(&self->mutex);
	Con_Int i = Con_Builtins_Dict_Atom_find_entry(thread, &self->mutex, dict_atom->entries, dict_atom->num_entries_allocated, key, hash);
	Con_Obj *val = NULL;
	if (dict_atom->entries[i].key != NULL)
		val = dict_atom->entries[i].val;
	else
		CON_RAISE_EXCEPTION("Key_Exception", key);
	CON_MUTEX_UNLOCK(&self->mutex);
	
	return val;
}


//
// 'iter()' .
//

Con_Obj *_Con_Builtins_Dict_Class_iter_func(Con_Obj *thread)
{
	Con_Obj *self_obj;
	CON_UNPACK_ARGS("D", &self_obj);
	
	Con_Builtins_Dict_Atom *self_dict_atom = CON_GET_ATOM(self_obj, CON_BUILTIN(CON_BUILTIN_DICT_ATOM_DEF_OBJECT));

	CON_MUTEX_LOCK(&self_obj->mutex);
	for (Con_Int i = 0; i < self_dict_atom->num_entries_allocated; i += 1) {
		if (self_dict_atom->entries[i].key == NULL)
			continue;
		
		Con_Obj *key = self_dict_atom->entries[i].key;
		Con_Obj *val = self_dict_atom->entries[i].val;
		CON_MUTEX_UNLOCK(&self_obj->mutex);
		
		CON_YIELD(Con_Builtins_List_Atom_new_va(thread, key, val, NULL));
		
		CON_MUTEX_LOCK(&self_obj->mutex);
	}
	CON_MUTEX_UNLOCK(&self_obj->mutex);
	
	return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}



//
// 'keys()' .
//

Con_Obj *_Con_Builtins_Dict_Class_iter_keys_func(Con_Obj *thread)
{
	Con_Obj *self_obj;
	CON_UNPACK_ARGS("D", &self_obj);
	
	Con_Builtins_Dict_Atom *self_dict_atom = CON_GET_ATOM(self_obj, CON_BUILTIN(CON_BUILTIN_DICT_ATOM_DEF_OBJECT));

	CON_MUTEX_LOCK(&self_obj->mutex);
	for (Con_Int i = 0; i < self_dict_atom->num_entries_allocated; i += 1) {
		if (self_dict_atom->entries[i].key == NULL)
			continue;
		
		Con_Obj *key = self_dict_atom->entries[i].key;
		CON_MUTEX_UNLOCK(&self_obj->mutex);
		
		CON_YIELD(key);
		
		CON_MUTEX_LOCK(&self_obj->mutex);
	}
	CON_MUTEX_UNLOCK(&self_obj->mutex);
	
	return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}



//
// 'len()' returns number of (key, value) pairs in the dictionary.
//

Con_Obj *_Con_Builtins_Dict_Class_len_func(Con_Obj *thread)
{
	Con_Obj *self;
	CON_UNPACK_ARGS("D", &self);
	
	Con_Builtins_Dict_Atom *dict_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_DICT_ATOM_DEF_OBJECT));

	CON_MUTEX_LOCK(&self->mutex);
	Con_Int len = dict_atom->num_entries;
	CON_MUTEX_UNLOCK(&self->mutex);
	
	return CON_NEW_INT(len);
}



//
// 'set(key, val)' sets the value of 'key' to 'val'. If 'key' was previously associated with a value,
// 'val' overwrites the previous association. Returns null.
//

Con_Obj *_Con_Builtins_Dict_Class_set_func(Con_Obj *thread)
{
	Con_Obj *key, *self, *val;
	CON_UNPACK_ARGS("DOO", &self, &key, &val);
	
	Con_Builtins_Dict_Atom *dict_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_DICT_ATOM_DEF_OBJECT));

	Con_Hash hash = Con_Hash_get(thread, key);

	CON_MUTEX_LOCK(&self->mutex);
	Con_Int pos = Con_Builtins_Dict_Atom_find_entry(thread, &self->mutex, dict_atom->entries, dict_atom->num_entries_allocated, key, hash);
	if (dict_atom->entries[pos].key == NULL) {
		if (dict_atom->num_entries + (dict_atom->num_entries / 4) + 1 >= dict_atom->num_entries_allocated) {
			Con_Int new_num_entries_allocated = dict_atom->num_entries + (dict_atom->num_entries / 2) + 2;
			Con_Builtins_Dict_Class_Hash_Entry *new_entries = Con_Memory_malloc_no_gc(thread, sizeof(Con_Builtins_Dict_Class_Hash_Entry) * new_num_entries_allocated, CON_MEMORY_CHUNK_OPAQUE);
			if (new_entries == NULL)
				CON_XXX;
			
			for (Con_Int i = 0; i < new_num_entries_allocated; i += 1)
				new_entries[i].key = NULL;
			
			for (Con_Int i = 0; i < dict_atom->num_entries_allocated; i += 1) {
				if (dict_atom->entries[i].key == NULL)
					continue;
				
				Con_Int new_pos = Con_Builtins_Dict_Atom_find_entry(thread, &self->mutex, new_entries, new_num_entries_allocated, dict_atom->entries[i].key, dict_atom->entries[i].hash);
				new_entries[new_pos] = dict_atom->entries[i];
			}
			
			dict_atom->entries = new_entries;
			dict_atom->num_entries_allocated = new_num_entries_allocated;
			pos = Con_Builtins_Dict_Atom_find_entry(thread, &self->mutex, dict_atom->entries, dict_atom->num_entries_allocated, key, hash);
		}
		dict_atom->num_entries += 1;
	}

	dict_atom->entries[pos].hash = hash;
	dict_atom->entries[pos].key = key;
	dict_atom->entries[pos].val = val;
	CON_MUTEX_UNLOCK(&self->mutex);
	
	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'scopy()'.
//

Con_Obj *_Con_Builtins_Dict_Class_scopy_func(Con_Obj *thread)
{
	Con_Obj *self_obj;
	CON_UNPACK_ARGS("D", &self_obj);
	
	Con_Obj *copy = Con_Builtins_Dict_Atom_new(thread);
	CON_GET_SLOT_APPLY(copy, "extend", self_obj);
	
	return copy;
}



//
// 'vals()'.
//

Con_Obj *_Con_Builtins_Dict_Class_iter_vals_func(Con_Obj *thread)
{
	Con_Obj *self_obj;
	CON_UNPACK_ARGS("D", &self_obj);
	
	Con_Builtins_Dict_Atom *self_dict_atom = CON_GET_ATOM(self_obj, CON_BUILTIN(CON_BUILTIN_DICT_ATOM_DEF_OBJECT));

	CON_MUTEX_LOCK(&self_obj->mutex);
	for (Con_Int i = 0; i < self_dict_atom->num_entries_allocated; i += 1) {
		if (self_dict_atom->entries[i].key == NULL)
			continue;
		
		Con_Obj *val = self_dict_atom->entries[i].val;
		CON_MUTEX_UNLOCK(&self_obj->mutex);
		
		CON_YIELD(val);
		
		CON_MUTEX_LOCK(&self_obj->mutex);
	}
	CON_MUTEX_UNLOCK(&self_obj->mutex);
	
	return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}
