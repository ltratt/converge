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
#include "Builtins/Module/Atom.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"




typedef enum {_CON_MODULES_ARRAY_TYPE_INT32, _CON_MODULES_ARRAY_TYPE_INT64} _Con_Modules_Array_Type;

typedef struct {
	CON_ATOM_HEAD

	_Con_Modules_Array_Type type; // Immutable.
	size_t entry_size; // Immutable.
	u_char *entries;
	Con_Int num_entries, num_entries_allocated;
} Con_Modules_Array_Array_Atom;

void _Con_Modules_Array_Module_Array_Atom_gc_scan(Con_Obj *, Con_Obj *, Con_Atom *);


Con_Obj *Con_Modules_Array_init(Con_Obj *thread, Con_Obj *);

Con_Obj *_Con_Modules_Array_Array_new(Con_Obj *);

Con_Obj *_Con_Modules_Array_Array_append_func(Con_Obj *);
Con_Obj *_Con_Modules_Array_Array_extend_func(Con_Obj *);
Con_Obj *_Con_Modules_Array_Array_extend_from_string_func(Con_Obj *);
Con_Obj *_Con_Modules_Array_Array_get_func(Con_Obj *);
Con_Obj *_Con_Modules_Array_Array_get_slice_func(Con_Obj *);
Con_Obj *_Con_Modules_Array_Array_iterate_func(Con_Obj *);
Con_Obj *_Con_Modules_Array_Array_len_func(Con_Obj *);
Con_Obj *_Con_Modules_Array_Array_len_bytes_func(Con_Obj *);
Con_Obj *_Con_Modules_Array_Array_serialize_func(Con_Obj *);
Con_Obj *_Con_Modules_Array_Array_set_func(Con_Obj *);
Con_Obj *_Con_Modules_Array_Array_to_str_func(Con_Obj *);



Con_Obj *Con_Modules_Array_init(Con_Obj *thread, Con_Obj *identifier)
{
	Con_Obj *array_mod = Con_Builtins_Module_Atom_new_c(thread, identifier, CON_NEW_STRING("Array"), CON_BUILTIN(CON_BUILTIN_NULL_OBJ));

	// Array_Atom_Def
	
	CON_SET_SLOT(array_mod, "Array_Atom_Def", Con_Builtins_Atom_Def_Atom_new(thread, _Con_Modules_Array_Module_Array_Atom_gc_scan, NULL));

	// Array.Array
	
	Con_Obj *array_class = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_CLASS_CLASS), "new", CON_NEW_STRING("Array"), Con_Builtins_List_Atom_new_va(thread, CON_BUILTIN(CON_BUILTIN_OBJECT_CLASS), NULL), array_mod, CON_NEW_UNBOUND_C_FUNC(_Con_Modules_Array_Array_new, "Array_new", array_mod));
	CON_SET_SLOT(array_mod, "Array", array_class);
	
	CON_SET_FIELD(array_class, "append", CON_NEW_BOUND_C_FUNC(_Con_Modules_Array_Array_append_func, "append", array_mod, array_class));
	CON_SET_FIELD(array_class, "extend", CON_NEW_BOUND_C_FUNC(_Con_Modules_Array_Array_extend_func, "extend", array_mod, array_class));
	CON_SET_FIELD(array_class, "extend_from_string", CON_NEW_BOUND_C_FUNC(_Con_Modules_Array_Array_extend_from_string_func, "extend_from_string", array_mod, array_class));
	CON_SET_FIELD(array_class, "get", CON_NEW_BOUND_C_FUNC(_Con_Modules_Array_Array_get_func, "get", array_mod, array_class));
	CON_SET_FIELD(array_class, "get_slice", CON_NEW_BOUND_C_FUNC(_Con_Modules_Array_Array_get_slice_func, "get_slice", array_mod, array_class));
	CON_SET_FIELD(array_class, "iterate", CON_NEW_BOUND_C_FUNC(_Con_Modules_Array_Array_iterate_func, "iterate", array_mod, array_class));
	CON_SET_FIELD(array_class, "len", CON_NEW_BOUND_C_FUNC(_Con_Modules_Array_Array_len_func, "len", array_mod, array_class));
	CON_SET_FIELD(array_class, "len_bytes", CON_NEW_BOUND_C_FUNC(_Con_Modules_Array_Array_len_bytes_func, "len_bytes", array_mod, array_class));
	CON_SET_FIELD(array_class, "serialize", CON_NEW_BOUND_C_FUNC(_Con_Modules_Array_Array_serialize_func, "serialize", array_mod, array_class));
	CON_SET_FIELD(array_class, "set", CON_NEW_BOUND_C_FUNC(_Con_Modules_Array_Array_set_func, "set", array_mod, array_class));
	CON_SET_FIELD(array_class, "to_str", CON_NEW_BOUND_C_FUNC(_Con_Modules_Array_Array_to_str_func, "to_str", array_mod, array_class));
	
	Con_Builtins_Class_Atom_mark_as_virgin(thread, array_class);

	return array_mod;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Garbage collection
//

void _Con_Modules_Array_Module_Array_Atom_gc_scan(Con_Obj *thread, Con_Obj *obj, Con_Atom *atom)
{
	Con_Modules_Array_Array_Atom *array_atom = (Con_Modules_Array_Array_Atom *) atom;

	Con_Memory_gc_push(thread, array_atom->entries);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// func Array_new
//

//
// 'Array_new(class_, type, initial_data := null)' initializes a new array of type 'type'.
//
// type must be one of the following:
//   'i' : array of integers (native size, as determined by host platform)
//   'i32' : array of 32-bit integers
//

Con_Obj *_Con_Modules_Array_Array_new(Con_Obj *thread)
{
	Con_Obj *array_mod = Con_Builtins_VM_Atom_get_functions_module(thread);

	Con_Obj *class, *initial_data, *type;
	CON_UNPACK_ARGS("CO;O", &class, &type, &initial_data);

	Con_Obj *new_array = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(Con_Modules_Array_Array_Atom) + sizeof(Con_Builtins_Slots_Atom), class);
	Con_Modules_Array_Array_Atom *array_atom = (Con_Modules_Array_Array_Atom *) new_array->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (array_atom + 1);
	array_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;

	array_atom->atom_type = CON_GET_MODULE_DEF(array_mod, "Array_Atom_Def");

	if (CON_C_STRING_EQ("i", type)) {
		// Native integer size this architecture / OS.
#		if SIZEOF_CON_INT == 4
		array_atom->type = _CON_MODULES_ARRAY_TYPE_INT32;
		array_atom->entry_size = sizeof(int32_t);
#		else
		CON_XXX;
#		endif
	}
	else if (CON_C_STRING_EQ("i32", type)) {
		array_atom->type = _CON_MODULES_ARRAY_TYPE_INT32;
		array_atom->entry_size = sizeof(int32_t);
	}
	else
		CON_XXX;
	

	if (initial_data != NULL) {
		CON_MUTEX_LOCK(&initial_data->mutex);
		if (initial_data->virgin && initial_data->first_atom->atom_type == CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT)) {
			// Being initialised from a string is a special case.
			CON_MUTEX_UNLOCK(&initial_data->mutex);
			Con_Builtins_String_Atom *string_atom = (Con_Builtins_String_Atom *) initial_data->first_atom;
			if (string_atom->encoding != CON_STR_UTF_8)
				CON_XXX;

			if (string_atom->size % array_atom->entry_size != 0)
				CON_XXX;
			
			Con_Int num_entries_allocated = string_atom->size / array_atom->entry_size;
			array_atom->entries = (u_char *) Con_Memory_malloc(thread, string_atom->size, CON_MEMORY_CHUNK_OPAQUE);
			memmove(array_atom->entries, string_atom->str, string_atom->size);
			array_atom->num_entries = num_entries_allocated;
			array_atom->num_entries_allocated = num_entries_allocated;

			Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);

			Con_Memory_change_chunk_type(thread, new_array, CON_MEMORY_CHUNK_OBJ);
		}
		else {
			Con_Int num_entries_allocated = Con_Numbers_Number_to_Con_Int(thread, CON_GET_SLOT_APPLY(initial_data, "len"));
			array_atom->entries = (u_char *) Con_Memory_malloc(thread, array_atom->entry_size * num_entries_allocated, CON_MEMORY_CHUNK_OPAQUE);
			array_atom->num_entries = 0;
			array_atom->num_entries_allocated = num_entries_allocated;

			Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);

			Con_Memory_change_chunk_type(thread, new_array, CON_MEMORY_CHUNK_OBJ);

			CON_GET_SLOT_APPLY(new_array, "extend", initial_data);
		}
	}
	else {
		Con_Int num_entries_allocated = CON_DEFAULT_ARRAY_NUM_ENTRIES_ALLOCATED;
		array_atom->entries = (u_char *) Con_Memory_malloc(thread, array_atom->entry_size * num_entries_allocated, CON_MEMORY_CHUNK_OPAQUE);
		array_atom->num_entries = 0;
		array_atom->num_entries_allocated = num_entries_allocated;

		Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);

		Con_Memory_change_chunk_type(thread, new_array, CON_MEMORY_CHUNK_OBJ);

		if (initial_data != NULL)
			CON_GET_SLOT_APPLY(new_array, "extend", initial_data);
	}
	
	return new_array;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// class Array
//

//
// 'append(o)'.
//

Con_Obj *_Con_Modules_Array_Array_append_func(Con_Obj *thread)
{
	Con_Obj *array_atom_def = CON_GET_MODULE_DEF(Con_Builtins_VM_Atom_get_functions_module(thread), "Array_Atom_Def");

	Con_Obj *o, *self;
	CON_UNPACK_ARGS("UO", array_atom_def, &self, &o);
	
	Con_Modules_Array_Array_Atom *array_atom = CON_GET_ATOM(self, array_atom_def);
	
	if (array_atom->type == _CON_MODULES_ARRAY_TYPE_INT32) {
#		if SIZEOF_CON_INT == 4
		int32_t val = Con_Numbers_Number_to_Con_Int(thread, o);
#		else
		CON_XXX;
#		endif
		CON_MUTEX_LOCK(&self->mutex);
		Con_Memory_make_array_room(thread, (void **) &array_atom->entries, &self->mutex, &array_atom->num_entries_allocated, &array_atom->num_entries, 1, array_atom->entry_size);
		((int32_t *) array_atom->entries)[array_atom->num_entries++] = val;
		CON_MUTEX_UNLOCK(&self->mutex);
	}
	else
		CON_XXX;
	
	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'extend(container)'.
//

Con_Obj *_Con_Modules_Array_Array_extend_func(Con_Obj *thread)
{
	Con_Obj *array_atom_def = CON_GET_MODULE_DEF(Con_Builtins_VM_Atom_get_functions_module(thread), "Array_Atom_Def");

	Con_Obj *container, *self;
	CON_UNPACK_ARGS("UO", array_atom_def, &self, &container);
	
	CON_MUTEX_LOCK(&container->mutex);
	if (container->virgin && container->first_atom->atom_type == array_atom_def) {
		Con_Modules_Array_Array_Atom *self_array_atom = CON_GET_ATOM(self, array_atom_def);
		Con_Modules_Array_Array_Atom *container_array_atom = (Con_Modules_Array_Array_Atom *) container->first_atom;
		
		if (self_array_atom->type == container_array_atom->type) {
			Con_Int container_num_entries = container_array_atom->num_entries;
			CON_MUTEX_UNLOCK(&container->mutex);
			
			CON_MUTEX_LOCK(&self->mutex);
			Con_Memory_make_array_room(thread, (void **) &self_array_atom->entries, &self->mutex, &self_array_atom->num_entries_allocated, &self_array_atom->num_entries, container_num_entries, self_array_atom->entry_size);
			CON_MUTEX_ADD_LOCK(&self->mutex, &container->mutex);
			if (container_array_atom->num_entries > container_num_entries)
				CON_XXX;
			memmove(self_array_atom->entries + self_array_atom->num_entries * self_array_atom->entry_size, container_array_atom->entries, container_array_atom->num_entries * container_array_atom->entry_size);
			self_array_atom->num_entries += container_array_atom->num_entries;
			CON_MUTEXES_UNLOCK(&self->mutex, &container->mutex);
			
			return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
		}
	}
	CON_MUTEX_UNLOCK(&container->mutex);

	// The default - and slow - behaviour...

	CON_PRE_GET_SLOT_APPLY_PUMP(container, "iterate");
	while (1) {
		Con_Obj *val = CON_APPLY_PUMP();
		if (val == NULL)
			break;
		CON_GET_SLOT_APPLY(self, "append", val);
	}
	
	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'extend_from_string(container)'.
//

Con_Obj *_Con_Modules_Array_Array_extend_from_string_func(Con_Obj *thread)
{
	Con_Obj *array_atom_def = CON_GET_MODULE_DEF(Con_Builtins_VM_Atom_get_functions_module(thread), "Array_Atom_Def");

	Con_Obj *str_obj, *self;
	CON_UNPACK_ARGS("US", array_atom_def, &self, &str_obj);
	
	CON_MUTEX_LOCK(&str_obj->mutex);
	if (str_obj->virgin && str_obj->first_atom->atom_type == CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT)) {
		Con_Modules_Array_Array_Atom *self_array_atom = CON_GET_ATOM(self, array_atom_def);
		Con_Builtins_String_Atom *str_atom = (Con_Builtins_String_Atom *) str_obj->first_atom;
		
		CON_MUTEX_UNLOCK(&str_obj->mutex);
		
		if (str_atom->size % self_array_atom->entry_size != 0)
			CON_XXX;
		
		CON_MUTEX_LOCK(&self->mutex);
		Con_Int new_num_entries = str_atom->size / self_array_atom->entry_size;
		Con_Memory_make_array_room(thread, (void **) &self_array_atom->entries, &self->mutex, &self_array_atom->num_entries_allocated, &self_array_atom->num_entries, new_num_entries, self_array_atom->entry_size);
		memmove(self_array_atom->entries + self_array_atom->num_entries * self_array_atom->entry_size, str_atom->str, str_atom->size);
		self_array_atom->num_entries += new_num_entries;
		CON_MUTEX_UNLOCK(&self->mutex);
		
		return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
	}
	CON_XXX;
}




//
// 'get(i)'.
//

Con_Obj *_Con_Modules_Array_Array_get_func(Con_Obj *thread)
{
	Con_Obj *array_atom_def = CON_GET_MODULE_DEF(Con_Builtins_VM_Atom_get_functions_module(thread), "Array_Atom_Def");

	Con_Obj *i_obj, *self;
	CON_UNPACK_ARGS("UN", array_atom_def, &self, &i_obj);
	
	Con_Modules_Array_Array_Atom *array_atom = CON_GET_ATOM(self, array_atom_def);
	
	Con_Int i_val = Con_Numbers_Number_to_Con_Int(thread, i_obj);
	
	Con_Obj *rtn;
	if (array_atom->type == _CON_MODULES_ARRAY_TYPE_INT32) {
		CON_MUTEX_LOCK(&self->mutex);
#		if SIZEOF_CON_INT == 4
		Con_Int val = ((int32_t *) array_atom->entries)[Con_Misc_translate_index(thread, &self->mutex, i_val, array_atom->num_entries)];
#		else
		CON_XXX;
#		endif
		CON_MUTEX_UNLOCK(&self->mutex);
		rtn = CON_NEW_INT(val);
	}
	else
		CON_XXX;
	
	return rtn;
}




//
// 'get_slice(i)'.
//

Con_Obj *_Con_Modules_Array_Array_get_slice_func(Con_Obj *thread)
{
	Con_Obj *array_mod = Con_Builtins_VM_Atom_get_functions_module(thread);
	Con_Obj *array_atom_def = CON_GET_MODULE_DEF(array_mod, "Array_Atom_Def");

	Con_Obj *lower_obj, *self, *upper_obj;
	CON_UNPACK_ARGS("UOO", array_atom_def, &self, &lower_obj, &upper_obj);
	
	Con_Modules_Array_Array_Atom *self_array_atom = CON_GET_ATOM(self, array_atom_def);

	CON_MUTEX_LOCK(&self->mutex);
	Con_Int lower, upper;
	Con_Misc_translate_slice_indices(thread, &self->mutex, lower_obj, upper_obj, &lower, &upper, self_array_atom->num_entries);
	CON_MUTEX_UNLOCK(&self->mutex);

	Con_Obj *new_array = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(Con_Modules_Array_Array_Atom) + sizeof(Con_Builtins_Slots_Atom), CON_GET_MODULE_DEF(array_mod, "Array"));
	Con_Modules_Array_Array_Atom *new_array_atom = (Con_Modules_Array_Array_Atom *) new_array->first_atom;
	Con_Builtins_Slots_Atom *new_slots_atom = (Con_Builtins_Slots_Atom *) (new_array_atom + 1);
	new_array_atom->next_atom = (Con_Atom *) new_slots_atom;
	new_slots_atom->next_atom = NULL;

	new_array_atom->type = self_array_atom->type;
	new_array_atom->entry_size = self_array_atom->entry_size;

	Con_Int num_entries_allocated = upper - lower;
	new_array_atom->entries = (u_char *) Con_Memory_malloc(thread, num_entries_allocated * self_array_atom->entry_size, CON_MEMORY_CHUNK_OPAQUE);
	CON_MUTEX_LOCK(&self->mutex);
	if (upper > self_array_atom->num_entries)
		CON_XXX;
	memmove(new_array_atom->entries, self_array_atom->entries + lower * self_array_atom->entry_size, num_entries_allocated * self_array_atom->entry_size);
	CON_MUTEX_UNLOCK(&self->mutex);
	new_array_atom->num_entries = num_entries_allocated;
	new_array_atom->num_entries_allocated = num_entries_allocated;

	new_array_atom->atom_type = array_atom_def;
	
	Con_Builtins_Slots_Atom_Def_init_atom(thread, new_slots_atom);
	
	Con_Memory_change_chunk_type(thread, new_array, CON_MEMORY_CHUNK_OBJ);
	
	return new_array;
}




//
// 'iterate(lower := 0, upper := -1)'.
//

Con_Obj *_Con_Modules_Array_Array_iterate_func(Con_Obj *thread)
{
	Con_Obj *array_atom_def = CON_GET_MODULE_DEF(Con_Builtins_VM_Atom_get_functions_module(thread), "Array_Atom_Def");

	Con_Obj *lower_obj, *self, *upper_obj;
	CON_UNPACK_ARGS("U;OO", array_atom_def, &self, &lower_obj, &upper_obj);
	
	Con_Modules_Array_Array_Atom *array_atom = CON_GET_ATOM(self, array_atom_def);
	
	CON_MUTEX_LOCK(&self->mutex);
	Con_Int lower, upper;
	Con_Misc_translate_slice_indices(thread, &self->mutex, lower_obj, upper_obj, &lower, &upper, array_atom->num_entries);
	
	for (int i = lower; i < upper; i += 1) {
		if (i >= array_atom->num_entries) {
			// The array has shrunk in size since we last unlocked & locked.
			CON_MUTEX_UNLOCK(&self->mutex);
			break;
		}

		Con_Obj *entry;
		if (array_atom->type == _CON_MODULES_ARRAY_TYPE_INT32) {
#			if SIZEOF_CON_INT == 4
			Con_Int val = ((int32_t *) array_atom->entries)[i];
#			else
			CON_XXX
#			endif
			CON_MUTEX_UNLOCK(&self->mutex);
			entry = CON_NEW_INT(val);
		}
		else
			CON_XXX;
		CON_YIELD(entry);
		CON_MUTEX_LOCK(&self->mutex);
	}
	CON_MUTEX_UNLOCK(&self->mutex);
	
	return CON_BUILTIN(CON_BUILTIN_FAIL_OBJ);
}



//
// 'len()' returns the number of entries in this array.
//

Con_Obj *_Con_Modules_Array_Array_len_func(Con_Obj *thread)
{
	Con_Obj *array_atom_def = CON_GET_MODULE_DEF(Con_Builtins_VM_Atom_get_functions_module(thread), "Array_Atom_Def");

	Con_Obj *self;
	CON_UNPACK_ARGS("U", array_atom_def, &self);
	
	Con_Modules_Array_Array_Atom *array_atom = CON_GET_ATOM(self, array_atom_def);
	
	CON_MUTEX_LOCK(&self->mutex);
	Con_Int len = array_atom->num_entries;
	CON_MUTEX_UNLOCK(&self->mutex);
	
	return CON_NEW_INT(len);
}



//
// 'len_bytes()' returns the length of this array in bytes.
//

Con_Obj *_Con_Modules_Array_Array_len_bytes_func(Con_Obj *thread)
{
	Con_Obj *array_atom_def = CON_GET_MODULE_DEF(Con_Builtins_VM_Atom_get_functions_module(thread), "Array_Atom_Def");

	Con_Obj *self;
	CON_UNPACK_ARGS("U", array_atom_def, &self);
	
	Con_Modules_Array_Array_Atom *array_atom = CON_GET_ATOM(self, array_atom_def);
	
	CON_MUTEX_LOCK(&self->mutex);
	Con_Int len_bytes = array_atom->entry_size * array_atom->num_entries;
	CON_MUTEX_UNLOCK(&self->mutex);
	
	return CON_NEW_INT(len_bytes);
}



//
// 'serialize()' returns a string representing this array.
//

Con_Obj *_Con_Modules_Array_Array_serialize_func(Con_Obj *thread)
{
	Con_Obj *array_atom_def = CON_GET_MODULE_DEF(Con_Builtins_VM_Atom_get_functions_module(thread), "Array_Atom_Def");

	Con_Obj *self;
	CON_UNPACK_ARGS("U", array_atom_def, &self);
	
	Con_Modules_Array_Array_Atom *array_atom = CON_GET_ATOM(self, array_atom_def);
	
	CON_MUTEX_LOCK(&self->mutex);
	Con_Int str_size = array_atom->num_entries * array_atom->entry_size;
	u_char *str_mem = Con_Memory_malloc_no_gc(thread, str_size, CON_MEMORY_CHUNK_OPAQUE);
	if (str_mem == NULL)
		CON_XXX;
	memmove(str_mem, array_atom->entries, str_size);
	CON_MUTEX_UNLOCK(&self->mutex);
	
	return Con_Builtins_String_Atom_new_no_copy(thread, str_mem, str_size, CON_STR_UTF_8);
}



//
// 'set(i, o)'.
//

Con_Obj *_Con_Modules_Array_Array_set_func(Con_Obj *thread)
{
	Con_Obj *array_atom_def = CON_GET_MODULE_DEF(Con_Builtins_VM_Atom_get_functions_module(thread), "Array_Atom_Def");

	Con_Obj *i_obj, *o, *self;
	CON_UNPACK_ARGS("UNO", array_atom_def, &self, &i_obj, &o);
	
	Con_Modules_Array_Array_Atom *array_atom = CON_GET_ATOM(self, array_atom_def);
	
	Con_Int i_val = Con_Numbers_Number_to_Con_Int(thread, i_obj);
	
	if (array_atom->type == _CON_MODULES_ARRAY_TYPE_INT32) {
#		if SIZEOF_CON_INT == 4
		int32_t val = Con_Numbers_Number_to_Con_Int(thread, o);
#		else
		CON_XXX
#		endif
		CON_MUTEX_LOCK(&self->mutex);
		((int32_t *) array_atom->entries)[Con_Misc_translate_index(thread, &self->mutex, i_val, array_atom->num_entries)] = val;
		CON_MUTEX_UNLOCK(&self->mutex);
	}
	else
		CON_XXX;
	
	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// 'to_str()'.
//

Con_Obj *_Con_Modules_Array_Array_to_str_func(Con_Obj *thread)
{
	Con_Obj *array_atom_def = CON_GET_MODULE_DEF(Con_Builtins_VM_Atom_get_functions_module(thread), "Array_Atom_Def");

	Con_Obj *self;
	CON_UNPACK_ARGS("U", array_atom_def, &self);
	
	Con_Modules_Array_Array_Atom *array_atom = CON_GET_ATOM(self, array_atom_def);
	
	CON_MUTEX_LOCK(&self->mutex);
	u_char *str_mem = Con_Memory_malloc_no_gc(thread, array_atom->num_entries * array_atom->entry_size, CON_MEMORY_CHUNK_OPAQUE);
	if (str_mem == NULL)
		CON_XXX;
	memmove(str_mem, array_atom->entries, array_atom->num_entries * array_atom->entry_size);
	CON_MUTEX_UNLOCK(&self->mutex);
	
	return Con_Builtins_String_Atom_new_no_copy(thread, str_mem, array_atom->num_entries * array_atom->entry_size, CON_STR_UTF_8);
}
