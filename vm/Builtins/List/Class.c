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
#include "Misc.h"
#include "Numbers.h"
#include "Object.h"
#include "Shortcuts.h"

#include "Builtins/Atom_Def_Atom.h"
#include "Builtins/Class/Atom.h"
#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/List/Class.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"


Con_Obj *_Con_Builtins_List_Class_new_object(Con_Obj *);

Con_Obj *_Con_Builtins_List_Class_init_func(Con_Obj *);
Con_Obj *_Con_Builtins_List_Class_add_func(Con_Obj *);
Con_Obj *_Con_Builtins_List_Class_mult_func(Con_Obj *);
Con_Obj *_Con_Builtins_List_Class_to_str_func(Con_Obj *);
Con_Obj *_Con_Builtins_List_Class_eq_func(Con_Obj *);
Con_Obj *_Con_Builtins_List_Class_neq_func(Con_Obj *);
Con_Obj *_Con_Builtins_List_Class_append_func(Con_Obj *);
Con_Obj *_Con_Builtins_List_Class_del_func(Con_Obj *);
Con_Obj *_Con_Builtins_List_Class_extend_func(Con_Obj *);
Con_Obj *_Con_Builtins_List_Class_find_func(Con_Obj *);
Con_Obj *_Con_Builtins_List_Class_find_index_func(Con_Obj *);
Con_Obj *_Con_Builtins_List_Class_flattened_func(Con_Obj *);
Con_Obj *_Con_Builtins_List_Class_get_func(Con_Obj *);
Con_Obj *_Con_Builtins_List_Class_get_slice_func(Con_Obj *);
Con_Obj *_Con_Builtins_List_Class_insert_func(Con_Obj *);
Con_Obj *_Con_Builtins_List_Class_iterate_func(Con_Obj *);
Con_Obj *_Con_Builtins_List_Class_len_func(Con_Obj *);
Con_Obj *_Con_Builtins_List_Class_pop_func(Con_Obj *);
Con_Obj *_Con_Builtins_List_Class_riterate_func(Con_Obj *);
Con_Obj *_Con_Builtins_List_Class_set_func(Con_Obj *);
Con_Obj *_Con_Builtins_List_Class_set_slice_func(Con_Obj *);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_List_Class_bootstrap(Con_Obj *thread)
{
	Con_Obj *list_class = CON_BUILTIN(CON_BUILTIN_LIST_CLASS);
	
	Con_Builtins_Class_Atom *class_atom = (Con_Builtins_Class_Atom *) list_class->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (class_atom + 1);

	class_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;
	
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	Con_Builtins_Class_Atom_init_atom(thread, class_atom, CON_NEW_STRING("List"), CON_NEW_UNBOUND_C_FUNC(_Con_Builtins_List_Class_new_object, "List_new", CON_BUILTIN(CON_BUILTIN_NULL_OBJ)), CON_BUILTIN(CON_BUILTIN_OBJECT_CLASS), NULL);
	
	Con_Memory_change_chunk_type(thread, list_class, CON_MEMORY_CHUNK_OBJ);

	CON_SET_FIELD(list_class, "init", CON_NEW_BOUND_C_FUNC(_Con_Builtins_List_Class_init_func, "init", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), list_class));	
	CON_SET_FIELD(list_class, "+", CON_NEW_BOUND_C_FUNC(_Con_Builtins_List_Class_add_func, "add", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), list_class));
	CON_SET_FIELD(list_class, "*", CON_NEW_BOUND_C_FUNC(_Con_Builtins_List_Class_mult_func, "*", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), list_class));
	CON_SET_FIELD(list_class, "to_str", CON_NEW_BOUND_C_FUNC(_Con_Builtins_List_Class_to_str_func, "to_str", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), list_class));
	CON_SET_FIELD(list_class, "==", CON_NEW_BOUND_C_FUNC(_Con_Builtins_List_Class_eq_func, "==", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), list_class));
	CON_SET_FIELD(list_class, "!=", CON_NEW_BOUND_C_FUNC(_Con_Builtins_List_Class_neq_func, "!=", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), list_class));
	CON_SET_FIELD(list_class, "append", CON_NEW_BOUND_C_FUNC(_Con_Builtins_List_Class_append_func, "append", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), list_class));
	CON_SET_FIELD(list_class, "del", CON_NEW_BOUND_C_FUNC(_Con_Builtins_List_Class_del_func, "del", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), list_class));
	CON_SET_FIELD(list_class, "extend", CON_NEW_BOUND_C_FUNC(_Con_Builtins_List_Class_extend_func, "extend", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), list_class));
	CON_SET_FIELD(list_class, "find", CON_NEW_BOUND_C_FUNC(_Con_Builtins_List_Class_find_func, "find", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), list_class));
	CON_SET_FIELD(list_class, "find_index", CON_NEW_BOUND_C_FUNC(_Con_Builtins_List_Class_find_index_func, "find_index", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), list_class));
	CON_SET_FIELD(list_class, "flattened", CON_NEW_BOUND_C_FUNC(_Con_Builtins_List_Class_flattened_func, "flattened", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), list_class));
	CON_SET_FIELD(list_class, "get", CON_NEW_BOUND_C_FUNC(_Con_Builtins_List_Class_get_func, "get", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), list_class));
	CON_SET_FIELD(list_class, "get_slice", CON_NEW_BOUND_C_FUNC(_Con_Builtins_List_Class_get_slice_func, "get_slice", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), list_class));
	CON_SET_FIELD(list_class, "insert", CON_NEW_BOUND_C_FUNC(_Con_Builtins_List_Class_insert_func, "insert", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), list_class));
	CON_SET_FIELD(list_class, "iterate", CON_NEW_BOUND_C_FUNC(_Con_Builtins_List_Class_iterate_func, "iterate", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), list_class));
	CON_SET_FIELD(list_class, "len", CON_NEW_BOUND_C_FUNC(_Con_Builtins_List_Class_len_func, "len", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), list_class));
	CON_SET_FIELD(list_class, "pop", CON_NEW_BOUND_C_FUNC(_Con_Builtins_List_Class_pop_func, "pop", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), list_class));
	CON_SET_FIELD(list_class, "riterate", CON_NEW_BOUND_C_FUNC(_Con_Builtins_List_Class_riterate_func, "riterate", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), list_class));
	CON_SET_FIELD(list_class, "set", CON_NEW_BOUND_C_FUNC(_Con_Builtins_List_Class_set_func, "set", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), list_class));
	CON_SET_FIELD(list_class, "set_slice", CON_NEW_BOUND_C_FUNC(_Con_Builtins_List_Class_set_slice_func, "set_slice", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), list_class));
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// func List_new
//

Con_Obj *_Con_Builtins_List_Class_new_object(Con_Obj *thread)
{
	Con_Obj *class_, *var_args;
	CON_UNPACK_ARGS("Ov", &class_, &var_args);
	
	Con_Int num_entries_to_allocate;
	if (Con_Numbers_Number_to_Con_Int(thread, CON_GET_SLOT_APPLY(var_args, "len")) == 0)
		num_entries_to_allocate = 0;
	else {
		Con_Obj *first_elem = CON_GET_SLOT_APPLY(var_args, "get", CON_NEW_INT(0));
		num_entries_to_allocate = Con_Numbers_Number_to_Con_Int(thread, CON_GET_SLOT_APPLY(first_elem, "len"));
	}
	
	Con_Obj *new_list = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(Con_Builtins_List_Atom) + sizeof(Con_Builtins_Slots_Atom), class_);
	Con_Builtins_List_Atom *list_atom = (Con_Builtins_List_Atom *) new_list->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (list_atom + 1);
	list_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;
	
	Con_Builtins_List_Atom_init_atom(thread, list_atom, num_entries_to_allocate);
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	
	Con_Memory_change_chunk_type(thread, new_list, CON_MEMORY_CHUNK_OBJ);

	CON_GET_SLOT_APPLY(CON_GET_SLOT(new_list, "init"), "apply", var_args);

	return new_list;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// List class fields
//

//
// 'init(o)'.
//

Con_Obj *_Con_Builtins_List_Class_init_func(Con_Obj *thread)
{
	Con_Obj *o, *self;
	CON_UNPACK_ARGS("LO", &self, &o);
	
	CON_GET_SLOT_APPLY(self, "extend", o);

	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// '+(o)'.
//

Con_Obj *_Con_Builtins_List_Class_add_func(Con_Obj *thread)
{
	Con_Obj *o, *self;
	CON_UNPACK_ARGS("LO", &self, &o);
	
	Con_Builtins_List_Atom *self_list_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT));

	CON_MUTEX_LOCK(&o->mutex);
	if (o->virgin && o->first_atom->atom_type == CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT)) {
		// The fast case: we're adding together two lists, where 'o' is a virgin object. We can
		// use memmove without penalty.
		
		Con_Builtins_List_Atom *o_list_atom = (Con_Builtins_List_Atom *) o->first_atom;
		CON_MUTEX_ADD_LOCK(&o->mutex, &self->mutex);
		Con_Int new_list_size = self_list_atom->num_entries + o_list_atom->num_entries;
		CON_MUTEXES_UNLOCK(&self->mutex, &o->mutex);
		
		Con_Obj *new_list = Con_Builtins_List_Atom_new_sized(thread, new_list_size);
		Con_Builtins_List_Atom *new_list_list_atom = CON_FIND_ATOM(new_list, CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT));

		CON_MUTEXES_LOCK(&self->mutex, &o->mutex, &new_list->mutex);
		if (self_list_atom->num_entries + o_list_atom->num_entries > new_list_size)
			CON_XXX;
		
		memmove(new_list_list_atom->entries, self_list_atom->entries, self_list_atom->num_entries * sizeof(Con_Obj *));
		memmove(new_list_list_atom->entries + self_list_atom->num_entries, o_list_atom->entries, o_list_atom->num_entries * sizeof(Con_Obj *));
		new_list_list_atom->num_entries = self_list_atom->num_entries + o_list_atom->num_entries;
		CON_MUTEXES_UNLOCK(&self->mutex, &o->mutex, &new_list->mutex);

		return new_list;
	}
	else {
		// The slow case: 'o' isn't a list or is a list but isn't a virgin object. We can use memmove
		// on 'self', but have to resort to iterating over 'o'.
		
		CON_MUTEX_UNLOCK(&o->mutex);
		Con_Int o_num_entries = Con_Numbers_Number_to_Con_Int(thread, CON_GET_SLOT_APPLY(o, "len"));

		CON_MUTEX_LOCK(&self->mutex);
		Con_Int new_list_size = self_list_atom->num_entries + o_num_entries;
		CON_MUTEXES_UNLOCK(&self->mutex);

		Con_Obj *new_list = Con_Builtins_List_Atom_new_sized(thread, new_list_size);
		Con_Builtins_List_Atom *new_list_list_atom = CON_FIND_ATOM(new_list, CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT));

		CON_MUTEXES_LOCK(&self->mutex, &new_list->mutex);
		if (self_list_atom->num_entries > new_list_size)
			CON_XXX;
		memmove(new_list_list_atom->entries, self_list_atom->entries, self_list_atom->num_entries * sizeof(Con_Obj *));
		new_list_list_atom->num_entries += self_list_atom->num_entries;
		CON_MUTEXES_UNLOCK(&self->mutex, &new_list->mutex);

		CON_PRE_GET_SLOT_APPLY_PUMP(o, "iterate");
		while (1) {
			Con_Obj *val = CON_APPLY_PUMP();
			if (val == NULL)
				break;
				
			CON_MUTEX_LOCK(&new_list->mutex);
			Con_Memory_make_array_room(thread, (void **) &new_list_list_atom->entries, &new_list->mutex, &new_list_list_atom->num_entries_allocated, &new_list_list_atom->num_entries, 1, sizeof(Con_Obj *));
			new_list_list_atom->entries[new_list_list_atom->num_entries] = val;
			new_list_list_atom->num_entries += 1;
			CON_MUTEX_UNLOCK(&new_list->mutex);
		}
		
		return new_list;
	}
}



//
// '*(i)'.
//

Con_Obj *_Con_Builtins_List_Class_mult_func(Con_Obj *thread)
{
	Con_Obj *i_obj, *self;
	CON_UNPACK_ARGS("LN", &self, &i_obj);
	
	Con_Builtins_List_Atom *self_list_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT));
	Con_Int i = Con_Numbers_Number_to_Con_Int(thread, i_obj);

	CON_MUTEX_LOCK(&self->mutex);
	Con_Int new_list_size = self_list_atom->num_entries * i;
	CON_MUTEX_UNLOCK(&self->mutex);

	Con_Obj *new_list = Con_Builtins_List_Atom_new_sized(thread, new_list_size);
	Con_Builtins_List_Atom *new_list_list_atom = CON_FIND_ATOM(new_list, CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT));

	CON_MUTEXES_LOCK(&self->mutex, &new_list->mutex);
	if (self_list_atom->num_entries * i > new_list_size)
		CON_XXX;

	for (Con_Int j = 0; j < i; j += 1) {
		memmove(new_list_list_atom->entries + self_list_atom->num_entries * j, self_list_atom->entries, self_list_atom->num_entries * sizeof(Con_Obj *));
	}
	new_list_list_atom->num_entries = new_list_size;
	CON_MUTEXES_UNLOCK(&self->mutex, &new_list->mutex);

	return new_list;
}



//
// Pretty print the list in the form "[<entry 1>, <entry 2>, ..., <entry n>]"
//

Con_Obj *_Con_Builtins_List_Class_to_str_func(Con_Obj *thread)
{
	Con_Obj *self;
	CON_UNPACK_ARGS("L", &self);

#	define LIST_TO_STR_START "["
#	define LIST_TO_STR_START_SIZE ((Con_Int) (sizeof(LIST_TO_STR_START) - 1))
#	define LIST_TO_STR_SEPERATOR ", "
#	define LIST_TO_STR_SEPERATOR_SIZE ((Con_Int) (sizeof(LIST_TO_STR_SEPERATOR) - 1))
#	define LIST_TO_STR_END "]"
#	define LIST_TO_STR_END_SIZE ((Con_Int) (sizeof(LIST_TO_STR_END) - 1))
	
	Con_Builtins_List_Atom *list_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT));

	CON_MUTEX_LOCK(&self->mutex);

	// We have to make some sort of guess about how much memory we're going to need. Unfortunately
	// the penalty for guessing wrong either way is likely to be quite high.

	Con_Int str_mem_size_allocated;
	if (list_atom->num_entries == 0)
		// This is the easy case; if there are no entries we only need to allocate constant size
		// memory to hold "[" and "]".
		str_mem_size_allocated = LIST_TO_STR_START_SIZE + LIST_TO_STR_END_SIZE;
	else
		str_mem_size_allocated = LIST_TO_STR_START_SIZE + LIST_TO_STR_END_SIZE + list_atom->num_entries * (8 + LIST_TO_STR_SEPERATOR_SIZE);
	
	char *str_mem = Con_Memory_malloc_no_gc(thread, str_mem_size_allocated, CON_MEMORY_CHUNK_OPAQUE);
	if (str_mem == NULL)
		CON_XXX;

	assert(str_mem_size_allocated > LIST_TO_STR_START_SIZE);
	memmove(str_mem, LIST_TO_STR_START, LIST_TO_STR_START_SIZE);
	Con_Int str_mem_size = LIST_TO_STR_START_SIZE;

	for (int i = 0; i < list_atom->num_entries; i += 1) {
		Con_Obj *entry = list_atom->entries[i];
		CON_MUTEX_UNLOCK(&self->mutex);
		
		Con_Obj *to_str = CON_GET_SLOT_APPLY(entry, "to_str");
		Con_Builtins_String_Atom *str_atom = CON_GET_ATOM(to_str, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
		assert(str_atom->encoding == CON_STR_UTF_8);
		
		Con_Int extra_size_needed;
		if (i == 0)
			extra_size_needed = str_atom->size;
		else
			extra_size_needed = str_atom->size + LIST_TO_STR_SEPERATOR_SIZE;

		if (str_mem_size + extra_size_needed > str_mem_size_allocated) {
			Con_Int new_str_mem_size_allocated = str_mem_size_allocated + str_mem_size_allocated / 2;
			if (new_str_mem_size_allocated < str_mem_size + extra_size_needed)
				new_str_mem_size_allocated = str_mem_size + extra_size_needed;
			str_mem = Con_Memory_realloc(thread, str_mem, new_str_mem_size_allocated);
			str_mem_size_allocated = new_str_mem_size_allocated;
		}
		
		if (i > 0) {
			memmove(str_mem + str_mem_size, LIST_TO_STR_SEPERATOR, LIST_TO_STR_SEPERATOR_SIZE);
			str_mem_size += LIST_TO_STR_SEPERATOR_SIZE;
		}

		memmove(str_mem + str_mem_size, str_atom->str, str_atom->size);
		str_mem_size += str_atom->size;
		CON_MUTEX_LOCK(&self->mutex);
	}
	CON_MUTEX_UNLOCK(&self->mutex);
	
	if (str_mem_size + LIST_TO_STR_END_SIZE != str_mem_size_allocated) {
		// Since keeping round unused memory for strings is incredibly wasteful, we now realloc the
		// allocated memory to exactly the right size.
	
		str_mem = Con_Memory_realloc(thread, str_mem, str_mem_size + LIST_TO_STR_END_SIZE);
	}
	
	memmove(str_mem + str_mem_size, LIST_TO_STR_END, LIST_TO_STR_END_SIZE);
	str_mem_size += LIST_TO_STR_END_SIZE;
	
	return Con_Builtins_String_Atom_new_no_copy(thread, str_mem, str_mem_size, CON_STR_UTF_8);
}



//
// '==(o)'.
//

Con_Obj *_Con_Builtins_List_Class_eq_func(Con_Obj *thread)
{
	Con_Obj *o, *self;
	CON_UNPACK_ARGS("LO", &self, &o);

	Con_Builtins_List_Atom *self_list_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT));
	
	bool equal = true;
	CON_MUTEX_LOCK(&o->mutex);
	if (o->virgin && o->first_atom->atom_type == CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT)) {
		Con_Builtins_List_Atom *o_list_atom = (Con_Builtins_List_Atom *) o->first_atom;
		CON_MUTEX_ADD_LOCK(&o->mutex, &self->mutex);
		if (self_list_atom->num_entries != o_list_atom->num_entries) {
			equal = false;
			CON_MUTEXES_UNLOCK(&o->mutex, &self->mutex);
		}
		else {
			Con_Int i = 0;
			while (1) {
				if (i >= self_list_atom->num_entries) {
					CON_MUTEXES_UNLOCK(&o->mutex, &self->mutex);
					break;
				}
				
				Con_Obj *self_entry = self_list_atom->entries[i];
				Con_Obj *o_entry = o_list_atom->entries[i];
				CON_MUTEXES_UNLOCK(&o->mutex, &self->mutex);
				
				if (!Con_Object_eq(thread, self_entry, o_entry)) {
					equal = false;
					break;
				}
				
				CON_MUTEXES_LOCK(&o->mutex, &self->mutex);
				if (self_list_atom->num_entries != o_list_atom->num_entries) {
					// The lists shifted while the mutex was unlocked.
					CON_MUTEXES_UNLOCK(&o->mutex, &self->mutex);
					equal = false;
					break;
				}
				i += 1;
			}
		}
	}
	else
		CON_RAISE_EXCEPTION("Type_Exception", CON_BUILTIN(CON_BUILTIN_LIST_CLASS), o, CON_NEW_STRING("XXX"));
	
	if (equal)
		return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
	else
		return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}



//
// '==(o)'.
//

Con_Obj *_Con_Builtins_List_Class_neq_func(Con_Obj *thread)
{
	Con_Obj *o, *self;
	CON_UNPACK_ARGS("LO", &self, &o);

	Con_Builtins_List_Atom *self_list_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT));
	
	bool not_equal = false;
	CON_MUTEX_LOCK(&o->mutex);
	if (o->virgin && o->first_atom->atom_type == CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT)) {
		Con_Builtins_List_Atom *o_list_atom = (Con_Builtins_List_Atom *) o->first_atom;
		CON_MUTEX_ADD_LOCK(&o->mutex, &self->mutex);
		if (self_list_atom->num_entries != o_list_atom->num_entries) {
			not_equal = true;
			CON_MUTEXES_UNLOCK(&o->mutex, &self->mutex);
		}
		else {
			Con_Int i = 0;
			while (1) {
				if (i >= self_list_atom->num_entries) {
					CON_MUTEXES_UNLOCK(&o->mutex, &self->mutex);
					break;
				}
				
				Con_Obj *self_entry = self_list_atom->entries[i];
				Con_Obj *o_entry = o_list_atom->entries[i];
				CON_MUTEXES_UNLOCK(&o->mutex, &self->mutex);
				
				if (Con_Object_eq(thread, self_entry, o_entry)) {
					not_equal = true;
					break;
				}
				
				CON_MUTEXES_LOCK(&o->mutex, &self->mutex);
				if (self_list_atom->num_entries != o_list_atom->num_entries) {
					// The list's shifted while the mutex was unlocked.
					CON_MUTEXES_UNLOCK(&o->mutex, &self->mutex);
					not_equal = true;
					break;
				}
				i += 1;
			}
		}
	}
	else
		CON_XXX;
	
	if (not_equal)
		return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
	else
		return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}



//
// 'append(o)' appends element 'o' to the end of the list.
//

Con_Obj *_Con_Builtins_List_Class_append_func(Con_Obj *thread)
{
	Con_Obj *obj, *self;
	CON_UNPACK_ARGS("LO", &self, &obj);
	
	Con_Builtins_List_Atom *list_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT));

	CON_MUTEX_LOCK(&self->mutex);
	
	Con_Memory_make_array_room(thread, (void **) &list_atom->entries, &self->mutex, &list_atom->num_entries_allocated, &list_atom->num_entries, 1, sizeof(Con_Obj *));
	list_atom->entries[list_atom->num_entries] = obj;
	list_atom->num_entries += 1;
	
	CON_MUTEX_UNLOCK(&self->mutex);
	
	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'del(i)' deletes the element at position 'i'.
//

Con_Obj *_Con_Builtins_List_Class_del_func(Con_Obj *thread)
{
	Con_Obj *self, *i_obj;
	CON_UNPACK_ARGS("LN", &self, &i_obj);
	
	Con_Builtins_List_Atom *list_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT));

	Con_Int i_val = Con_Numbers_Number_to_Con_Int(thread, i_obj);
	
	CON_MUTEX_LOCK(&self->mutex);
	Con_Int i = Con_Misc_translate_index(thread, &self->mutex, i_val, list_atom->num_entries);
	memmove(list_atom->entries + i, list_atom->entries + i + 1, (list_atom->num_entries - i - 1) *  sizeof(Con_Obj *));
	list_atom->num_entries -= 1;
	CON_MUTEX_UNLOCK(&self->mutex);
	
	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'extend(l)' extends the current list with elements from l. Returns null.
//

Con_Obj *_Con_Builtins_List_Class_extend_func(Con_Obj *thread)
{
	Con_Obj *self, *o;
	CON_UNPACK_ARGS("LO", &self, &o);
	
	Con_Builtins_List_Atom *self_list_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT));
	Con_Builtins_List_Atom *o_list_atom;
	CON_MUTEX_LOCK(&o->mutex);
	if (o->virgin && ((o_list_atom = CON_FIND_ATOM(o, CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT))) != NULL)) {
		// The quick case: extending a list with a virgin list.
		
		Con_Int o_num_entries = o_list_atom->num_entries;
		CON_MUTEX_UNLOCK(&o->mutex);
		
		CON_MUTEX_LOCK(&self->mutex);

		Con_Memory_make_array_room(thread, (void **) &self_list_atom->entries, &self->mutex, &self_list_atom->num_entries_allocated, &self_list_atom->num_entries, o_num_entries, sizeof(Con_Obj *));
		
		CON_MUTEX_ADD_LOCK(&self->mutex, &o->mutex);
		if (o_list_atom->num_entries > self_list_atom->num_entries_allocated - self_list_atom->num_entries)
			// 'o' grew larger...
			CON_XXX;
		
		memmove(self_list_atom->entries + self_list_atom->num_entries, o_list_atom->entries, o_list_atom->num_entries * sizeof(Con_Obj *));
		self_list_atom->num_entries += o_list_atom->num_entries;

		CON_MUTEXES_UNLOCK(&self->mutex, &o->mutex);
	}
	else {
		CON_MUTEX_UNLOCK(&o->mutex);
		
		CON_PRE_GET_SLOT_APPLY_PUMP(o, "iterate");
		while (1) {
			Con_Obj *val = CON_APPLY_PUMP();
			if (val == NULL)
				break;
			CON_MUTEX_LOCK(&self->mutex);

			Con_Memory_make_array_room(thread, (void **) &self_list_atom->entries, &self->mutex, &self_list_atom->num_entries_allocated, &self_list_atom->num_entries, 1, sizeof(Con_Obj *));
			self_list_atom->entries[self_list_atom->num_entries++] = val;
			
			CON_MUTEX_UNLOCK(&self->mutex);
		}
	}

	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'find(o)' yields each object which equals 'o' in the list, failing when no more are found.
//

Con_Obj *_Con_Builtins_List_Class_find_func(Con_Obj *thread)
{
	Con_Obj *o, *self;
	CON_UNPACK_ARGS("LO", &self, &o);
	
	Con_Builtins_List_Atom *list_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT));

	CON_MUTEX_LOCK(&self->mutex);
	for (int i = 0; i < list_atom->num_entries; i += 1) {
		Con_Obj *entry = list_atom->entries[i];
		CON_MUTEX_UNLOCK(&self->mutex);
		if (Con_Object_eq(thread, o, entry))
			CON_YIELD(entry);
		CON_MUTEX_LOCK(&self->mutex);
	}
	CON_MUTEX_UNLOCK(&self->mutex);
	
	return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}



//
// 'find_index(o)' yields the index of each object which equals 'o' in the list, failing when no more
// are found.
//

Con_Obj *_Con_Builtins_List_Class_find_index_func(Con_Obj *thread)
{
	Con_Obj *o, *self;
	CON_UNPACK_ARGS("LO", &self, &o);
	
	Con_Builtins_List_Atom *list_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT));

	CON_MUTEX_LOCK(&self->mutex);
	for (int i = 0; i < list_atom->num_entries; i += 1) {
		Con_Obj *entry = list_atom->entries[i];
		CON_MUTEX_UNLOCK(&self->mutex);
		if (Con_Object_eq(thread, o, entry))
			CON_YIELD(CON_NEW_INT(i));
		CON_MUTEX_LOCK(&self->mutex);
	}
	CON_MUTEX_UNLOCK(&self->mutex);
	
	return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}



//
// 'flattened()' returns a flattened copy of 'self'.
//

Con_Obj *_Con_Builtins_List_Class_flattened_func(Con_Obj *thread)
{
	Con_Obj *self;
	CON_UNPACK_ARGS("L", &self);
	
	Con_Builtins_List_Atom *list_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT));

	CON_MUTEX_LOCK(&self->mutex);
	Con_Int num_entries = list_atom->num_entries;
	CON_MUTEX_UNLOCK(&self->mutex);
	
	Con_Obj *flattened_list = Con_Builtins_List_Atom_new_sized(thread, num_entries);
	Con_Builtins_List_Atom *flattened_list_atom = CON_GET_ATOM(flattened_list, CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT));

	CON_MUTEX_LOCK(&self->mutex);	
	Con_Int prev_start = 0, i;
	for (i = 0; i < list_atom->num_entries; i += 1) {
		Con_Obj *entry = list_atom->entries[i];
		CON_MUTEX_UNLOCK(&self->mutex);
		if (CON_GET_SLOT_APPLY_NO_FAIL(CON_BUILTIN(CON_BUILTIN_LIST_CLASS), "conformed_by", entry)) {
			CON_MUTEXES_LOCK(&self->mutex, &flattened_list->mutex);
			if (i >= list_atom->num_entries) {
				// Lift shifted.
				CON_XXX;
			}
			
			Con_Memory_make_array_room(thread, (void **) &flattened_list_atom->entries, &flattened_list->mutex, &flattened_list_atom->num_entries_allocated, &flattened_list_atom->num_entries, i - prev_start, sizeof(Con_Obj *));
			memmove(flattened_list_atom->entries + flattened_list_atom->num_entries, list_atom->entries + prev_start, (i - prev_start) * sizeof(Con_Obj *));
			flattened_list_atom->num_entries += i - prev_start;
			CON_MUTEXES_UNLOCK(&self->mutex, &flattened_list->mutex);
			prev_start = i + 1;
			CON_GET_SLOT_APPLY(flattened_list, "extend", entry);
		}
		CON_MUTEX_LOCK(&self->mutex);
	}
	if (prev_start < i) {
		CON_MUTEX_ADD_LOCK(&self->mutex, &flattened_list->mutex);
		Con_Memory_make_array_room(thread, (void **) &flattened_list_atom->entries, &flattened_list->mutex, &flattened_list_atom->num_entries_allocated, &flattened_list_atom->num_entries, i - prev_start, sizeof(Con_Obj *));
		memmove(flattened_list_atom->entries + flattened_list_atom->num_entries, list_atom->entries + prev_start, (i - prev_start) * sizeof(Con_Obj *));
		flattened_list_atom->num_entries += i - prev_start;
		CON_MUTEXES_UNLOCK(&self->mutex, &flattened_list->mutex);
	}
	else
		CON_MUTEX_UNLOCK(&self->mutex);
	
	return flattened_list;
}



//
// 'get(i)' returns the element at position 'i'.
//

Con_Obj *_Con_Builtins_List_Class_get_func(Con_Obj *thread)
{
	Con_Obj *self, *i_obj;
	CON_UNPACK_ARGS("LN", &self, &i_obj);
	
	Con_Builtins_List_Atom *list_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT));

	Con_Int i_val = Con_Numbers_Number_to_Con_Int(thread, i_obj);
	
	CON_MUTEX_LOCK(&self->mutex);
	Con_Obj *elem = list_atom->entries[Con_Misc_translate_index(thread, &self->mutex, i_val, list_atom->num_entries)];
	CON_MUTEX_UNLOCK(&self->mutex);
	
	return elem;
}



//
// 'get_slice(lower := null, upper := null)'.
//

Con_Obj *_Con_Builtins_List_Class_get_slice_func(Con_Obj *thread)
{
	Con_Obj *lower_obj, *self, *upper_obj;
	CON_UNPACK_ARGS("L;OO", &self, &lower_obj, &upper_obj);
	
	Con_Builtins_List_Atom *list_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT));

	CON_MUTEX_LOCK(&self->mutex);
	Con_Int lower, upper;
	Con_Misc_translate_slice_indices(thread, &self->mutex, lower_obj, upper_obj, &lower, &upper, list_atom->num_entries);
	CON_MUTEX_UNLOCK(&self->mutex);
	
	Con_Obj *new_list = Con_Builtins_List_Atom_new_sized(thread, upper - lower);
	Con_Builtins_List_Atom *new_list_atom = CON_GET_ATOM(new_list, CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT));

	CON_MUTEX_LOCK(&self->mutex);
	// Recalculate lowoer and upper just in case the list has shifted and the indices are now out of
	// bounds.
	Con_Misc_translate_slice_indices(thread, &self->mutex, lower_obj, upper_obj, &lower, &upper, list_atom->num_entries);	

	CON_MUTEX_ADD_LOCK(&self->mutex, &new_list->mutex);
	if (new_list_atom->num_entries + (upper - lower) > new_list_atom->num_entries_allocated)
		CON_XXX;
	
	for (Con_Int i = lower; i < upper; i += 1) {

		new_list_atom->entries[new_list_atom->num_entries++] = list_atom->entries[i];
	}
	CON_MUTEXES_UNLOCK(&self->mutex, &new_list->mutex);
	
	return new_list;
}



//
// 'insert(o, i)' appends element 'o' at position 'i' in the list.
//

Con_Obj *_Con_Builtins_List_Class_insert_func(Con_Obj *thread)
{
	Con_Obj *i_obj, *o, *self;
	CON_UNPACK_ARGS("LNO", &self, &i_obj, &o);
	
	Con_Builtins_List_Atom *list_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT));
	
	Con_Int i_val = Con_Numbers_Number_to_Con_Int(thread, i_obj);
	
	CON_MUTEX_LOCK(&self->mutex);
	
	Con_Int i = Con_Misc_translate_slice_index(thread, &self->mutex, i_val, list_atom->num_entries);
	Con_Memory_make_array_room(thread, (void **) &list_atom->entries, &self->mutex, &list_atom->num_entries_allocated, &list_atom->num_entries, 1, sizeof(Con_Obj *));
	memmove(list_atom->entries + i + 1, list_atom->entries + i, (list_atom->num_entries - i) * sizeof(Con_Obj *));
	list_atom->entries[i] = o;
	list_atom->num_entries += 1;
	
	CON_MUTEX_UNLOCK(&self->mutex);
	
	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}




//
// 'iterate(lower := 0, upper := -1)' is a generator which returns each element of the list in order.
//

Con_Obj *_Con_Builtins_List_Class_iterate_func(Con_Obj *thread)
{
	Con_Obj *lower_obj, *self, *upper_obj;
	CON_UNPACK_ARGS("L;OO", &self, &lower_obj, &upper_obj);
	
	Con_Builtins_List_Atom *list_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT));

	CON_MUTEX_LOCK(&self->mutex);
	Con_Int lower, upper;
	Con_Misc_translate_slice_indices(thread, &self->mutex, lower_obj, upper_obj, &lower, &upper, list_atom->num_entries);
	
	for (Con_Int i = lower; i < upper; i += 1) {
		if (i >= list_atom->num_entries) {
			// The list has shrunk in size since we last unlocked & locked.
			CON_MUTEX_UNLOCK(&self->mutex);
			break;
		}

		Con_Obj *entry = list_atom->entries[i];
		CON_MUTEX_UNLOCK(&self->mutex);
		CON_YIELD(entry);
		CON_MUTEX_LOCK(&self->mutex);
	}
	CON_MUTEX_UNLOCK(&self->mutex);
	
	return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}



//
// 'len()' returns the length of the list.
//

Con_Obj *_Con_Builtins_List_Class_len_func(Con_Obj *thread)
{
	Con_Obj *self;
	CON_UNPACK_ARGS("L", &self);
	
	Con_Builtins_List_Atom *list_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT));

	CON_MUTEX_LOCK(&self->mutex);
	Con_Int len = list_atom->num_entries;
	CON_MUTEX_UNLOCK(&self->mutex);
	
	return CON_NEW_INT(len);
}



//
// 'pop()' deletes the elements' last list, returning that element as its result.
//

Con_Obj *_Con_Builtins_List_Class_pop_func(Con_Obj *thread)
{
	Con_Obj *self, *i_obj;
	CON_UNPACK_ARGS("L", &self, &i_obj);
	
	Con_Builtins_List_Atom *list_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT));

	CON_MUTEX_LOCK(&self->mutex);
	Con_Misc_translate_slice_index(thread, &self->mutex, -1, list_atom->num_entries);
	Con_Obj *entry = list_atom->entries[--list_atom->num_entries];
	CON_MUTEX_UNLOCK(&self->mutex);
	
	return entry;
}



//
// 'riterate(lower := 0, upper := -1)' is a generator which returns each element of the list in
// reverse order.
//

Con_Obj *_Con_Builtins_List_Class_riterate_func(Con_Obj *thread)
{
	Con_Obj *lower_obj, *self, *upper_obj;
	CON_UNPACK_ARGS("L;OO", &self, &lower_obj, &upper_obj);
	
	Con_Builtins_List_Atom *list_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT));

	CON_MUTEX_LOCK(&self->mutex);
	Con_Int lower, upper;
	Con_Misc_translate_slice_indices(thread, &self->mutex, lower_obj, upper_obj, &lower, &upper, list_atom->num_entries);
	
	for (Con_Int i = upper - 1; i >= lower; i -= 1) {
		if (i >= list_atom->num_entries) {
			// The list has shrunk in size since we last unlocked & locked.
			CON_MUTEX_UNLOCK(&self->mutex);
			break;
		}

		Con_Obj *entry = list_atom->entries[i];
		CON_MUTEX_UNLOCK(&self->mutex);
		CON_YIELD(entry);
		CON_MUTEX_LOCK(&self->mutex);
	}
	CON_MUTEX_UNLOCK(&self->mutex);
	
	return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}




//
// 'set(i, o)' sets the element at position 'i' to 'o'. If 'x' is bigger than the lists current size
// an exception is raised.
//

Con_Obj *_Con_Builtins_List_Class_set_func(Con_Obj *thread)
{
	Con_Obj *i_obj, *o, *self;
	CON_UNPACK_ARGS("LNO", &self, &i_obj, &o);
	
	Con_Builtins_List_Atom *list_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT));

	Con_Int i_val = Con_Numbers_Number_to_Con_Int(thread, i_obj);
	
	CON_MUTEX_LOCK(&self->mutex);
	list_atom->entries[Con_Misc_translate_index(thread, &self->mutex, i_val, list_atom->num_entries)] = o;
	CON_MUTEX_UNLOCK(&self->mutex);
	
	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'set_slice(i := null, j := null, o)' overwrites the elements between i and j with the elements
// from the container o.
//

Con_Obj *_Con_Builtins_List_Class_set_slice_func(Con_Obj *thread)
{
	Con_Obj *lower_obj, *o, *self, *upper_obj;
	CON_UNPACK_ARGS("L;OOO", &self, &lower_obj, &upper_obj, &o);
	
	Con_Builtins_List_Atom *self_list_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT));
	Con_Builtins_List_Atom *o_list_atom;
	CON_MUTEX_LOCK(&o->mutex);
	if (o->virgin && ((o_list_atom = CON_FIND_ATOM(o, CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT))) != NULL)) {
		// The quick case: extending a list with a virgin list.
		
		Con_Int o_num_entries = o_list_atom->num_entries;
		CON_MUTEX_UNLOCK(&o->mutex);

		CON_MUTEX_LOCK(&self->mutex);
		Con_Int lower, upper;
		Con_Misc_translate_slice_indices(thread, &self->mutex, lower_obj, upper_obj, &lower, &upper, self_list_atom->num_entries);
		
		Con_Memory_make_array_room(thread, (void **) &self_list_atom->entries, &self->mutex, &self_list_atom->num_entries_allocated, &self_list_atom->num_entries, (upper - lower) + o_num_entries, sizeof(Con_Obj *));

		CON_MUTEX_ADD_LOCK(&self->mutex, &o->mutex);
		if (o_list_atom->num_entries > self_list_atom->num_entries_allocated - self_list_atom->num_entries)
			// 'o' grew larger...
			CON_XXX;
		
		memmove(self_list_atom->entries + upper + o_num_entries, self_list_atom->entries + upper, (self_list_atom->num_entries - upper) * sizeof(Con_Obj *));
		memmove(self_list_atom->entries + lower, o_list_atom->entries, o_num_entries * sizeof(Con_Obj *));
		self_list_atom->num_entries += (upper - lower) + o_num_entries;
		CON_MUTEXES_UNLOCK(&self->mutex, &o->mutex);
	}
	else
		CON_XXX;
	
	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}
