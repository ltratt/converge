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
#include "Numbers.h"
#include "Object.h"
#include "Shortcuts.h"

#include "Builtins/Class/Atom.h"
#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/Set/Atom.h"
#include "Builtins/Set/Class.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"


Con_Obj *_Con_Builtins_Set_Class_new_object(Con_Obj *);

Con_Obj *_Con_Builtins_Set_Class_init_func(Con_Obj *);
Con_Obj *_Con_Builtins_Set_Class_add_plus_func(Con_Obj *);
Con_Obj *_Con_Builtins_Set_Class_eq_func(Con_Obj *);
Con_Obj *_Con_Builtins_Set_Class_to_str_func(Con_Obj *);
Con_Obj *_Con_Builtins_Set_Class_add_func(Con_Obj *);
Con_Obj *_Con_Builtins_Set_Class_complement_func(Con_Obj *);
Con_Obj *_Con_Builtins_Set_Class_extend_func(Con_Obj *);
Con_Obj *_Con_Builtins_Set_Class_find_func(Con_Obj *);
Con_Obj *_Con_Builtins_Set_Class_intersect_func(Con_Obj *);
Con_Obj *_Con_Builtins_Set_Class_iter_func(Con_Obj *);
Con_Obj *_Con_Builtins_Set_Class_len_func(Con_Obj *);
Con_Obj *_Con_Builtins_Set_Class_scopy_func(Con_Obj *);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_Set_Class_bootstrap(Con_Obj *thread)
{
	Con_Obj *set_class = CON_BUILTIN(CON_BUILTIN_SET_CLASS);

	Con_Builtins_Class_Atom *class_atom = (Con_Builtins_Class_Atom *) set_class->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (class_atom + 1);

	class_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;
	
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	Con_Builtins_Class_Atom_init_atom(thread, class_atom, CON_NEW_STRING("Set"), CON_NEW_UNBOUND_C_FUNC(_Con_Builtins_Set_Class_new_object, "Set_new", CON_BUILTIN(CON_BUILTIN_NULL_OBJ)), CON_BUILTIN(CON_BUILTIN_OBJECT_CLASS), NULL);
	
	Con_Memory_change_chunk_type(thread, set_class, CON_MEMORY_CHUNK_OBJ);

	CON_SET_FIELD(set_class, "init", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Set_Class_init_func, "init", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), set_class));
	CON_SET_FIELD(set_class, "+", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Set_Class_add_plus_func, "+", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), set_class));
	CON_SET_FIELD(set_class, "==", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Set_Class_eq_func, "==", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), set_class));
	CON_SET_FIELD(set_class, "to_str", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Set_Class_to_str_func, "to_str", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), set_class));
	CON_SET_FIELD(set_class, "add", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Set_Class_add_func, "add", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), set_class));
	CON_SET_FIELD(set_class, "complement", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Set_Class_complement_func, "complement", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), set_class));
	CON_SET_FIELD(set_class, "extend", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Set_Class_extend_func, "extend", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), set_class));
	CON_SET_FIELD(set_class, "find", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Set_Class_find_func, "find", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), set_class));
	CON_SET_FIELD(set_class, "intersect", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Set_Class_intersect_func, "intersect", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), set_class));
	CON_SET_FIELD(set_class, "iter", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Set_Class_iter_func, "iter", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), set_class));
	CON_SET_FIELD(set_class, "len", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Set_Class_len_func, "len", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), set_class));
	CON_SET_FIELD(set_class, "scopy", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Set_Class_scopy_func, "scopy", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), set_class));
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// func Set_new
//

Con_Obj *_Con_Builtins_Set_Class_new_object(Con_Obj *thread)
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
	
	Con_Obj *new_set = Con_Builtins_Set_Atom_new_sized(thread, num_entries_to_allocate);\

	CON_GET_SLOT_APPLY(CON_GET_SLOT(new_set, "init"), "apply", var_args);

	return new_set;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Set class fields
//

//
// 'init(o)'.
//

Con_Obj *_Con_Builtins_Set_Class_init_func(Con_Obj *thread)
{
	Con_Obj *o, *self;
	CON_UNPACK_ARGS("W;O", &self, &o);
	
	if (o != NULL)
		CON_GET_SLOT_APPLY(self, "extend", o);

	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// '+(o)'.
//

Con_Obj *_Con_Builtins_Set_Class_add_plus_func(Con_Obj *thread)
{
	Con_Obj *o, *self;
	CON_UNPACK_ARGS("Wo", &self, &o);

	Con_Obj *new_set = CON_GET_SLOT_APPLY(self, "scopy");
	CON_GET_SLOT_APPLY(new_set, "extend", o);
	
	return new_set;
}



//
// '==(o)'.
//

Con_Obj *_Con_Builtins_Set_Class_eq_func(Con_Obj *thread)
{
	Con_Obj *o, *self;
	CON_UNPACK_ARGS("Wo", &self, &o);

	Con_Builtins_Set_Atom *self_set_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_SET_ATOM_DEF_OBJECT));	
	Con_Builtins_Set_Atom *o_set_atom = CON_FIND_ATOM(o, CON_BUILTIN(CON_BUILTIN_SET_ATOM_DEF_OBJECT));
	
	if (o_set_atom == NULL)
		CON_XXX;
	
	CON_MUTEXES_LOCK(&self->mutex, &o->mutex);
	if (self_set_atom->num_entries != o_set_atom->num_entries) {
		CON_MUTEXES_UNLOCK(&self->mutex, &o->mutex);
		return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
	}
	
	CON_MUTEXES_UNLOCK(&self->mutex, &o->mutex);
	
	CON_MUTEX_LOCK(&self->mutex);
	for (Con_Int i = 0; i < self_set_atom->num_entries_allocated; i += 1) {
		if (self_set_atom->entries[i].obj == NULL)
			continue;
		
		Con_Hash hash = self_set_atom->entries[i].hash;
		Con_Obj *obj = self_set_atom->entries[i].obj;
		CON_MUTEX_UNLOCK(&self->mutex);

		CON_MUTEX_LOCK(&o->mutex);
		Con_Int offset = Con_Builtins_Set_Atom_find_entry(thread, &o->mutex, o_set_atom->entries, o_set_atom->num_entries_allocated, obj, hash);
		if (o_set_atom->entries[offset].obj == NULL) {
			CON_MUTEX_UNLOCK(&o->mutex);
			return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
		}
		CON_MUTEX_UNLOCK(&o->mutex);

		CON_MUTEX_LOCK(&self->mutex);
	}
	CON_MUTEX_UNLOCK(&self->mutex);
	
	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'to_str().
//

Con_Obj *_Con_Builtins_Set_Class_to_str_func(Con_Obj *thread)
{
	Con_Obj *self;
	CON_UNPACK_ARGS("W", &self);

#	define SET_TO_STR_START "Set{"
#	define SET_TO_STR_START_SIZE ((Con_Int) (sizeof(SET_TO_STR_START) - 1))
#	define SET_TO_STR_SEPERATOR ", "
#	define SET_TO_STR_SEPERATOR_SIZE ((Con_Int) (sizeof(SET_TO_STR_SEPERATOR) - 1))
#	define SET_TO_STR_END "}"
#	define SET_TO_STR_END_SIZE ((Con_Int) (sizeof(SET_TO_STR_END) - 1))
	
	Con_Builtins_Set_Atom *set_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_SET_ATOM_DEF_OBJECT));

	CON_MUTEX_LOCK(&self->mutex);

	// We have to make some sort of guess about how much memory we're going to need. Unfortunately
	// the penalty for guessing wrong either way is likely to be quite high.

	Con_Int str_mem_size_allocated;
	if (set_atom->num_entries == 0)
		// This is the easy case; if there are no entries we only need to allocate constant size
		// memory to hold "[" and "]".
		str_mem_size_allocated = SET_TO_STR_START_SIZE + SET_TO_STR_END_SIZE;
	else
		str_mem_size_allocated = set_atom->num_entries * 8;
	
	u_char *str_mem = Con_Memory_malloc_no_gc(thread, str_mem_size_allocated, CON_MEMORY_CHUNK_OPAQUE);
	if (str_mem == NULL)
		CON_XXX;

	assert(str_mem_size_allocated > SET_TO_STR_START_SIZE);
	memmove(str_mem, SET_TO_STR_START, SET_TO_STR_START_SIZE);
	Con_Int str_mem_size = SET_TO_STR_START_SIZE;

	Con_Int num_processed = 0;
	for (int i = 0; i < set_atom->num_entries_allocated; i += 1) {
		Con_Obj *entry = set_atom->entries[i].obj;
		if (entry == NULL)
			continue;
		CON_MUTEX_UNLOCK(&self->mutex);
		
		Con_Obj *to_str = CON_GET_SLOT_APPLY(entry, "to_str");
		Con_Builtins_String_Atom *str_atom = CON_GET_ATOM(to_str, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
		assert(str_atom->encoding == CON_STR_UTF_8);
		
		Con_Int extra_size_needed;
		if (num_processed == 0)
			extra_size_needed = str_atom->size;
		else
			extra_size_needed = str_atom->size + SET_TO_STR_SEPERATOR_SIZE;

		if (str_mem_size + extra_size_needed > str_mem_size_allocated) {
			Con_Int new_str_mem_size_allocated = str_mem_size_allocated + str_mem_size_allocated / 2;
			if (new_str_mem_size_allocated < str_mem_size + extra_size_needed)
				new_str_mem_size_allocated = str_mem_size + extra_size_needed;
			str_mem = Con_Memory_realloc(thread, str_mem, new_str_mem_size_allocated);
			str_mem_size_allocated = new_str_mem_size_allocated;
		}
		
		if (num_processed > 0) {
			memmove(str_mem + str_mem_size, SET_TO_STR_SEPERATOR, SET_TO_STR_SEPERATOR_SIZE);
			str_mem_size += SET_TO_STR_SEPERATOR_SIZE;
		}
		num_processed += 1;

		memmove(str_mem + str_mem_size, str_atom->str, str_atom->size);
		str_mem_size += str_atom->size;
		CON_MUTEX_LOCK(&self->mutex);
	}
	CON_MUTEX_UNLOCK(&self->mutex);
	
	if (str_mem_size + SET_TO_STR_END_SIZE != str_mem_size_allocated) {
		// Since keeping round unused memory for strings is incredibly wasteful, we now realloc the
		// allocated memory to exactly the right size.
	
		str_mem = Con_Memory_realloc(thread, str_mem, str_mem_size + SET_TO_STR_END_SIZE);
	}
	
	memmove(str_mem + str_mem_size, SET_TO_STR_END, SET_TO_STR_END_SIZE);
	str_mem_size += SET_TO_STR_END_SIZE;
	
	return Con_Builtins_String_Atom_new_no_copy(thread, str_mem, str_mem_size, CON_STR_UTF_8);
}



//
// 'add(o)' adds 'o' to the set, overwriting a previous object equal to 'o' if it exists. Returns
// null.
//

Con_Obj *_Con_Builtins_Set_Class_add_func(Con_Obj *thread)
{
	Con_Obj *o, *self;
	CON_UNPACK_ARGS("Wo", &self, &o);

	Con_Hash hash = Con_Hash_get(thread, o);
	CON_MUTEX_LOCK(&self->mutex);
	Con_Builtins_Set_Atom_add_entry(thread, self, o, hash);
	CON_MUTEX_UNLOCK(&self->mutex);
	
	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'complement(o)' returns a set which contains every element of 'self' which is not in 'o'.
//

Con_Obj *_Con_Builtins_Set_Class_complement_func(Con_Obj *thread)
{
	Con_Obj *o_obj, *self_obj;
	CON_UNPACK_ARGS("WO", &self_obj, &o_obj);

	Con_Builtins_Set_Atom *self_set_atom = CON_FIND_ATOM(self_obj, CON_BUILTIN(CON_BUILTIN_SET_ATOM_DEF_OBJECT));	
	Con_Builtins_Set_Atom *o_set_atom = CON_FIND_ATOM(o_obj, CON_BUILTIN(CON_BUILTIN_SET_ATOM_DEF_OBJECT));

	if (o_set_atom != NULL) {
		CON_MUTEX_LOCK(&self_obj->mutex);
		Con_Int max_num_entries = self_set_atom->num_entries;
		CON_MUTEX_UNLOCK(&self_obj->mutex);
		// Guessing how many entries the new set is going to have is a stab in the dark at best.
		Con_Obj *new_set = Con_Builtins_Set_Atom_new_sized(thread, max_num_entries);
		
		CON_MUTEX_LOCK(&self_obj->mutex);
		for (Con_Int i = 0; i < self_set_atom->num_entries_allocated; i += 1) {
			if (self_set_atom->entries[i].obj == NULL)
				continue;
			
			Con_Hash hash = self_set_atom->entries[i].hash;
			Con_Obj *obj = self_set_atom->entries[i].obj;
			CON_MUTEX_UNLOCK(&self_obj->mutex);
			
			CON_MUTEX_LOCK(&o_obj->mutex);
			Con_Int offset = Con_Builtins_Set_Atom_find_entry(thread, &o_obj->mutex, o_set_atom->entries, o_set_atom->num_entries_allocated, obj, hash);
			if (o_set_atom->entries[offset].obj == NULL) {
				CON_MUTEX_UNLOCK(&o_obj->mutex);
				CON_MUTEX_LOCK(&new_set->mutex);
				Con_Builtins_Set_Atom_add_entry(thread, new_set, obj, hash);
				CON_MUTEX_UNLOCK(&new_set->mutex);
				CON_MUTEX_LOCK(&self_obj->mutex);
			}
			else {
				CON_MUTEX_UNLOCK(&o_obj->mutex);
				CON_MUTEX_LOCK(&self_obj->mutex);
			}
			
			CON_ASSERT_MUTEX_LOCKED(&self_obj->mutex);
		}
		CON_MUTEX_UNLOCK(&self_obj->mutex);
		
		return new_set;
	}

	CON_RAISE_EXCEPTION("Type_Exception", CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_SET_CLASS), "path"), o_obj);
}



//
// 'extend(o)' adds every element from 'o' into 'self'.
//

Con_Obj *_Con_Builtins_Set_Class_extend_func(Con_Obj *thread)
{
	Con_Obj *o_obj, *self_obj;
	CON_UNPACK_ARGS("WO", &self_obj, &o_obj);
	
	Con_Builtins_Set_Atom *o_set_atom = CON_FIND_ATOM(o_obj, CON_BUILTIN(CON_BUILTIN_SET_ATOM_DEF_OBJECT));

	if (o_set_atom != NULL) {
		CON_MUTEX_LOCK(&o_obj->mutex);
		for (Con_Int i = 0; i < o_set_atom->num_entries_allocated; i += 1) {
			if (o_set_atom->entries[i].obj == NULL)
				continue;
			
			Con_Hash hash = o_set_atom->entries[i].hash;
			Con_Obj *obj = o_set_atom->entries[i].obj;
			CON_MUTEX_UNLOCK(&o_obj->mutex);
			
			CON_MUTEX_LOCK(&self_obj->mutex);
			Con_Builtins_Set_Atom_add_entry(thread, self_obj, obj, hash);
			CON_MUTEX_UNLOCK(&self_obj->mutex);			
			
			CON_MUTEX_LOCK(&o_obj->mutex);
		}
		CON_MUTEX_UNLOCK(&o_obj->mutex);
	}
	else {
		CON_PRE_GET_SLOT_APPLY_PUMP(o_obj, "iter");
		while (1) {
			Con_Obj *e = CON_APPLY_PUMP();
			if (e == NULL)
				break;

			CON_GET_SLOT_APPLY(self_obj, "add", e);
		}
	}
			
	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'find(o)' returns the object in the set which is == 'key'. This function only returns a single
// value.
//

Con_Obj *_Con_Builtins_Set_Class_find_func(Con_Obj *thread)
{
	Con_Obj *o, *self;
	CON_UNPACK_ARGS("WO", &self, &o);
	
	Con_Builtins_Set_Atom *set_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_SET_ATOM_DEF_OBJECT));

	Con_Hash hash = Con_Hash_get(thread, o);
	CON_MUTEX_LOCK(&self->mutex);
	Con_Int i = Con_Builtins_Set_Atom_find_entry(thread, &self->mutex, set_atom->entries, set_atom->num_entries_allocated, o, hash);
	Con_Obj *rtn = NULL;
	if (set_atom->entries[i].obj != NULL)
		rtn = set_atom->entries[i].obj;
	else {
		rtn = CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
	}
	CON_MUTEX_UNLOCK(&self->mutex);
	
	return rtn;
}



//
// 'intersect(o)' returns a set which contains every element of 'self' which is also in 'o'.
//

Con_Obj *_Con_Builtins_Set_Class_intersect_func(Con_Obj *thread)
{
	Con_Obj *o_obj, *self_obj;
	CON_UNPACK_ARGS("WO", &self_obj, &o_obj);

	Con_Builtins_Set_Atom *self_set_atom = CON_FIND_ATOM(self_obj, CON_BUILTIN(CON_BUILTIN_SET_ATOM_DEF_OBJECT));	
	Con_Builtins_Set_Atom *o_set_atom = CON_FIND_ATOM(o_obj, CON_BUILTIN(CON_BUILTIN_SET_ATOM_DEF_OBJECT));

	if (o_set_atom != NULL) {
		CON_MUTEX_LOCK(&self_obj->mutex);
		Con_Int max_num_entries = self_set_atom->num_entries;
		CON_MUTEX_UNLOCK(&self_obj->mutex);
		// Guessing how many entries the new set is going to have is a stab in the dark at best.
		Con_Obj *new_set = Con_Builtins_Set_Atom_new_sized(thread, max_num_entries);

		CON_MUTEX_LOCK(&self_obj->mutex);
		for (Con_Int i = 0; i < self_set_atom->num_entries_allocated; i += 1) {
			if (self_set_atom->entries[i].obj == NULL)
				continue;

			Con_Hash hash = self_set_atom->entries[i].hash;
			Con_Obj *obj = self_set_atom->entries[i].obj;
			CON_MUTEX_UNLOCK(&self_obj->mutex);

			CON_MUTEX_LOCK(&o_obj->mutex);
			Con_Int offset = Con_Builtins_Set_Atom_find_entry(thread, &o_obj->mutex, o_set_atom->entries, o_set_atom->num_entries_allocated, obj, hash);
			if (o_set_atom->entries[offset].obj != NULL) {
				CON_MUTEX_UNLOCK(&o_obj->mutex);
				CON_MUTEX_LOCK(&new_set->mutex);
				Con_Builtins_Set_Atom_add_entry(thread, new_set, obj, hash);
				CON_MUTEX_UNLOCK(&new_set->mutex);
				CON_MUTEX_LOCK(&self_obj->mutex);
			}
			else {
				CON_MUTEX_UNLOCK(&o_obj->mutex);
				CON_MUTEX_LOCK(&self_obj->mutex);
			}

			CON_ASSERT_MUTEX_LOCKED(&self_obj->mutex);
		}
		CON_MUTEX_UNLOCK(&self_obj->mutex);

		return new_set;
	}

	CON_RAISE_EXCEPTION("Type_Exception", CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_SET_CLASS), "path"), o_obj);
}



//
// 'iter()'. Notice that with a set one can't specify lower and upper indices to iter over.
//

Con_Obj *_Con_Builtins_Set_Class_iter_func(Con_Obj *thread)
{
	Con_Obj *self;
	CON_UNPACK_ARGS("W", &self);
	
	Con_Builtins_Set_Atom *set_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_SET_ATOM_DEF_OBJECT));

	CON_MUTEX_LOCK(&self->mutex);
	for (Con_Int i = 0; i < set_atom->num_entries_allocated; i += 1) {
		Con_Obj *obj = set_atom->entries[i].obj;
		if (obj == NULL)
			continue;
		
		CON_MUTEX_UNLOCK(&self->mutex);
		CON_YIELD(obj);
		CON_MUTEX_LOCK(&self->mutex);
	}
	CON_MUTEX_UNLOCK(&self->mutex);
	
	return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}



//
// 'len()'.
//

Con_Obj *_Con_Builtins_Set_Class_len_func(Con_Obj *thread)
{
	Con_Obj *self;
	CON_UNPACK_ARGS("W", &self);
	
	Con_Builtins_Set_Atom *set_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_SET_ATOM_DEF_OBJECT));

	CON_MUTEX_LOCK(&self->mutex);
	Con_Int len = set_atom->num_entries;
	CON_MUTEX_UNLOCK(&self->mutex);
	
	return CON_NEW_INT(len);
}



//
// 'scopy()'.
//

Con_Obj *_Con_Builtins_Set_Class_scopy_func(Con_Obj *thread)
{
	Con_Obj *self;
	CON_UNPACK_ARGS("W", &self);
	
	Con_Builtins_Set_Atom *self_set_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_SET_ATOM_DEF_OBJECT));

	Con_Obj *new_set = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(Con_Builtins_Set_Atom) + sizeof(Con_Builtins_Slots_Atom), CON_BUILTIN(CON_BUILTIN_SET_CLASS));
	Con_Builtins_Set_Atom *new_set_atom = (Con_Builtins_Set_Atom *) new_set->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (new_set_atom + 1);
	new_set_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;

	new_set_atom->atom_type = CON_BUILTIN(CON_BUILTIN_SET_ATOM_DEF_OBJECT);
	
	CON_MUTEX_LOCK(&self->mutex);
	
	new_set_atom->entries = Con_Memory_malloc_no_gc(thread, self_set_atom->num_entries_allocated * sizeof(Con_Builtins_Set_Atom_Entry), CON_MEMORY_CHUNK_CONSERVATIVE);
	if (new_set_atom->entries == NULL)
		CON_XXX;
	new_set_atom->num_entries = self_set_atom->num_entries;
	new_set_atom->num_entries_allocated = self_set_atom->num_entries_allocated;
	
	memmove(new_set_atom->entries, self_set_atom->entries, self_set_atom->num_entries_allocated * sizeof(Con_Builtins_Set_Atom_Entry));
	CON_MUTEX_UNLOCK(&self->mutex);

	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	Con_Memory_change_chunk_type(thread, new_set, CON_MEMORY_CHUNK_OBJ);

	return new_set;
}
