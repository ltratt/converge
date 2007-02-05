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
#include "Memory.h"
#include "Object.h"
#include "Shortcuts.h"
#include "Slots.h"

#include "Builtins/Class/Atom.h"
#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/Partial_Application/Class.h"
#include "Builtins/Set/Atom.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



////////////////////////////////////////////////////////////////////////////////////////////////////
// Object creation functions
//

//
// Create a semi-initialized object of 'size' bytes as an instance of '_class'.
//
// Note that 'size' should include sizeof(Con_Obj) and that the Con_Obj returned from this function
// will be a CON_MEMORY_CHUNK_CONSERVATIVE.
//
// A typical useage therefore looks as follows:
//
//	 Con_Obj *new_dict = Con_Object_new_from_class(thread, sizeof(Con_Obj) +
//     sizeof(Con_Dict_Atom) + sizeof(Con_Builtins_Slots_Atom),
//     CON_BUILTIN(CON_BUILTIN_DICT_CLASS));
//   Con_Builtins_Dict_Atom *dict_atom = (Con_Builtins_Dict_Atom *) new_dict->first_atom;
//   Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (dict_atom + 1);
//   dict_atom->next_atom = (Con_Atom *) slots_atom;
//   slots_atom->next_atom = NULL;
//	
//   Con_Builtins_Dict_Atom_init_atom(thread, dict_atom);
//   Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
//	
//   Con_Memory_change_chunk_type(thread, new_dict, CON_MEMORY_CHUNK_OBJ);
//

Con_Obj *Con_Object_new_from_class(Con_Obj *thread, Con_Int size, Con_Obj *_class)
{
	if (Con_Builtins_VM_Atom_get_state(thread) == CON_VM_INITIALIZING)
		return Con_Object_new_from_proto(thread, size, NULL);
		
	Con_Obj *new_obj = Con_Memory_malloc(thread, size, CON_MEMORY_CHUNK_CONSERVATIVE);
	new_obj->size = size;

	Con_Builtins_Class_Atom *class_atom = CON_GET_ATOM(_class, CON_BUILTIN(CON_BUILTIN_CLASS_ATOM_DEF_OBJECT));

	CON_MUTEX_LOCK(&_class->mutex);
	new_obj->creator_slots = Con_Builtins_Class_Atom_get_creator_slots(thread, _class);
	
	new_obj->virgin = class_atom->virgin_class;
	
	if (class_atom->custom_get_slot_field)
		new_obj->custom_get_slot = 1;
	else
		new_obj->custom_get_slot = 0;
		
	if (class_atom->custom_set_slot_field)
		new_obj->custom_set_slot = 1;
	else
		new_obj->custom_set_slot = 0;

	if (class_atom->custom_find_slot_field)
		new_obj->custom_find_slot = 1;
	else
		new_obj->custom_find_slot = 0;
	CON_MUTEX_UNLOCK(&_class->mutex);
	
#	ifndef CON_THREADS_SINGLE_THREADED
	Con_Memory_mutex_init(thread, &new_obj->mutex);
#	endif

	return new_obj;
}



//
// Create a semi-initialized object of 'size' bytes which is a clone of 'proto_obj'.
//

Con_Obj *Con_Object_new_from_proto(Con_Obj *thread, Con_Int size, Con_Obj *proto_obj)
{
	Con_Obj *new_obj = Con_Memory_malloc(thread, size, CON_MEMORY_CHUNK_CONSERVATIVE);
	new_obj->size = size;

	if (proto_obj == NULL) {
		new_obj->creator_slots = NULL;
		new_obj->custom_get_slot = 0;
		new_obj->custom_set_slot = 0;
		new_obj->custom_find_slot = 0;
		new_obj->virgin = 0;
	}
	else {
		CON_MUTEX_LOCK(&proto_obj->mutex);
		new_obj->creator_slots = Con_Builtins_Slots_Atom_Def_get_proto_slots(thread, proto_obj);
		new_obj->virgin = proto_obj->virgin;
		new_obj->custom_get_slot = proto_obj->custom_get_slot;
		new_obj->custom_set_slot = proto_obj->custom_set_slot;
		new_obj->custom_set_slot = proto_obj->custom_find_slot;
		CON_MUTEX_UNLOCK(&proto_obj->mutex);
	}
	
#	ifndef CON_THREADS_SINGLE_THREADED
	Con_Memory_mutex_init(thread, &new_obj->mutex);
#	endif

	return new_obj;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions
//

//
// Return the atom of type 'atom_type' in the object 'obj'. If no atom of that type is found, return
// NULL.
//
// See: Con_Object_get_atom
//

void *Con_Object_find_atom(Con_Obj *thread, Con_Obj *obj, Con_Obj *atom_type)
{
	Con_Atom *atom = obj->first_atom;

	do {
		if (atom->atom_type == atom_type)
			return atom;
	}
	while ((atom = atom->next_atom) != NULL);
	
	return NULL;
}



//
// Return the atom of type 'atom_type' in the object 'obj'. If no atom of that type is found, an
// exception is raised.
//
// See: Con_Object_find_atom
//

void *Con_Object_get_atom(Con_Obj *thread, Con_Obj *obj, Con_Obj *atom_type)
{
	Con_Atom *atom = obj->first_atom;

	do {
		if (atom->atom_type == atom_type)
			return atom;
	}
	while ((atom = atom->next_atom) != NULL);
	
	CON_XXX;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Object functions
//

//
// Returns the value of a slot in an object.
//
// One or other of 'slot_name_obj' and 'slot_name' must be non-NULL. 'slot_name_obj' should be an
// object containing a string atom; 'slot_name' should point to a UTF-8 string.
//
// If 'slot_name' is non-NULL, 'slot_name_size' must be the length of the 'slot_name' string;
// 'slot_name_size' must not include a zero-terminator and 'slot_name' need not be zero-terminated.
//
// If both 'slot_name_obj' and 'slot_name' are non-NULL but refer to different slot names, undefined
// behaviour will occur.
//

Con_Obj *Con_Object_get_slot(Con_Obj *thread, Con_Obj *obj, Con_Obj *slot_name_obj, const u_char *slot_name, Con_Int slot_name_size)
{
	bool custom_get_slot;

	Con_Obj *val = Con_Object_get_slot_no_binding(thread, obj, slot_name_obj, slot_name, slot_name_size, &custom_get_slot);

	if (custom_get_slot)
		return val;

	if ((CON_FIND_ATOM(val, CON_BUILTIN(CON_BUILTIN_FUNC_ATOM_DEF_OBJECT)) != NULL) && (Con_Builtins_Func_Atom_is_bound(thread, val)))
		return Con_Builtins_Partial_Application_Class_new(thread, val, obj, NULL);
	
	return val;
}



//
// Returns the value of a slot in an object attempting not to create a Function_Binding if possible.
//
// One or other of 'slot_name_obj' and 'slot_name' must be non-NULL. 'slot_name_obj' should be an
// object containing a string atom; 'slot_name' should point to a UTF-8 string.
//
// If 'slot_name' is non-NULL, 'slot_name_size' must be the length of the 'slot_name' string;
// 'slot_name_size' must not include a zero-terminator and 'slot_name' need not be zero-terminated.
//
// If both 'slot_name_obj' and 'slot_name' are non-NULL but refer to different slot names, undefined
// behaviour will occur.
//
// 'custom_get_slot' is a pointer to a boolean which will be set to 'true' on return if 'obj' defined
// a custom get_slot function. If set to true, the user should accept the return object from this
// function 'as is'. If it is set to false, the user may assume that if the return object conforms to
// a function then a Function_Binding would have been created.
//
// This function is largely intended for internal use; calling it incorrectly can lead to Converge's
// function calling semantics being subverted.
//

Con_Obj *Con_Object_get_slot_no_binding(Con_Obj *thread, Con_Obj *obj, Con_Obj *slot_name_obj, const u_char *slot_name, Con_Int slot_name_size, bool *custom_get_slot)
{
	assert(((slot_name_obj != NULL) || (slot_name != NULL)) && !((slot_name_obj != NULL) && (slot_name != NULL)));

	CON_MUTEX_LOCK(&obj->mutex);

	*custom_get_slot = obj->custom_get_slot;
	if (obj->custom_get_slot) {
		// The object defines a custom get slot. We now need to call 'obj.get_slot(slot_name,
		// caller)'.
	
		Con_Obj *get_slot_func = Con_Object_get_slot_no_custom(thread, obj, (u_char *) "get_slot", sizeof("get_slot") - 1);
		CON_MUTEX_UNLOCK(&obj->mutex);
		
		if (slot_name_obj != NULL)
			return CON_APPLY(get_slot_func, obj, slot_name_obj);
		else
			return CON_APPLY(get_slot_func, obj, Con_Builtins_String_Atom_new_copy(thread, slot_name, slot_name_size, CON_STR_UTF_8));
	}

	if (slot_name == NULL) {
		assert(slot_name_size == 0);
		Con_Builtins_String_Atom *slot_name_obj_string_atom = CON_GET_ATOM(slot_name_obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
		slot_name = slot_name_obj_string_atom->str;
		slot_name_size = slot_name_obj_string_atom->size;
	}

	Con_Obj *val = Con_Object_get_slot_no_custom(thread, obj, slot_name, slot_name_size);
	CON_MUTEX_UNLOCK(&obj->mutex);
	
	return val;
}



//
// Do a "raw" lookup of 'slot_name' in 'obj'. The mutex on 'obj' must be held when calling this
// function; it will be locked upon return.
//
// If 'slot_name' is not found, 'obj's mutex is unlocked and an exception raised.
//
// This function is intended for internal use and by Object.get_slot.
//

Con_Obj *Con_Object_get_slot_no_custom(Con_Obj *thread, Con_Obj *obj, const u_char *slot_name, Con_Int slot_name_size)
{
	Con_Builtins_Slots_Atom *slots_atom = CON_FIND_ATOM(obj, CON_BUILTIN(CON_BUILTIN_SLOTS_ATOM_DEF_OBJECT));
	if (slots_atom != NULL) {
		Con_Obj *val = Con_Slots_get_slot(thread, &slots_atom->slots, slot_name, slot_name_size);
		if (val != NULL) {
			return val;
		}
	}
	
	if (obj->creator_slots != NULL) {
		Con_Obj *val = Con_Slots_get_slot(thread, obj->creator_slots, slot_name, slot_name_size);
		if (val != NULL) {
			return val;
		}
	}

	if (memcmp(slot_name, "id", 2) == 0) {
		CON_MUTEX_UNLOCK(&obj->mutex);
		Con_Obj *id = CON_NEW_INT((Con_Int) obj);
		CON_MUTEX_LOCK(&obj->mutex);
		return id;
	}
	
	CON_MUTEX_UNLOCK(&obj->mutex);
	CON_RAISE_EXCEPTION("Slot_Exception", Con_Builtins_String_Atom_new_copy(thread, slot_name, slot_name_size, CON_STR_UTF_8), obj);
}



Con_Obj *Con_Object_get_slots(Con_Obj *thread, Con_Obj *obj)
{
	Con_Obj *slots = Con_Builtins_Set_Atom_new(thread);

	if (obj->creator_slots != NULL) {
		Con_Int j = 0;
		while (1) {
			Con_Obj *val;
			const u_char *slot_name;
			Con_Int slot_name_size;
			if (!Con_Slots_read_slot(thread, obj->creator_slots, &j, &slot_name, &slot_name_size, &val))
				break;
			
			CON_GET_SLOT_APPLY(slots, "add", Con_Builtins_String_Atom_new_copy(thread, slot_name, slot_name_size, CON_STR_UTF_8));
		}
	}

	Con_Builtins_Slots_Atom *slots_atom = CON_FIND_ATOM(obj, CON_BUILTIN(CON_BUILTIN_SLOTS_ATOM_DEF_OBJECT));
	if (slots_atom != NULL) {
		Con_Int slot_name_buffer_size = 16;
		u_char *slot_name_buffer = Con_Memory_malloc(thread, slot_name_buffer_size, CON_MEMORY_CHUNK_OPAQUE);
		Con_Int j = 0;
		while (1) {
			Con_Obj *val;
			Con_Int old_j = j;
			const u_char *slot_name;
			Con_Int slot_name_size;
			CON_MUTEX_LOCK(&obj->mutex);
			if (!Con_Slots_read_slot(thread, &slots_atom->slots, &j, &slot_name, &slot_name_size, &val))
				break;
			if (slot_name_size > slot_name_buffer_size) {
				CON_MUTEX_UNLOCK(&obj->mutex);
				slot_name_buffer_size = slot_name_size;
				slot_name = Con_Memory_malloc(thread, slot_name_buffer_size, CON_MEMORY_CHUNK_OPAQUE);
				j = old_j;
				CON_MUTEX_LOCK(&obj->mutex);
				continue;
			}
			
			memmove(slot_name_buffer, slot_name, slot_name_size);
			
			CON_MUTEX_UNLOCK(&obj->mutex);
			CON_GET_SLOT_APPLY(slots, "add", Con_Builtins_String_Atom_new_copy(thread, slot_name, slot_name_size, CON_STR_UTF_8));
		}
	}
	
	return slots;
}



//
// Do a "raw" lookup of 'slot_name' in 'obj'. The mutex on 'obj' must be held when calling this
// function; it will be locked upon return.
//
// If 'slot_name' is not found, 'obj's mutex is unlocked and NULL is returned.
//
// This function is intended for internal use and by Object.find_slot.
//

Con_Obj *Con_Object_find_slot_no_custom(Con_Obj *thread, Con_Obj *obj, const u_char *slot_name, Con_Int slot_name_size)
{
	Con_Builtins_Slots_Atom *slots_atom = CON_FIND_ATOM(obj, CON_BUILTIN(CON_BUILTIN_SLOTS_ATOM_DEF_OBJECT));
	if (slots_atom != NULL) {
		Con_Obj *val = Con_Slots_get_slot(thread, &slots_atom->slots, slot_name, slot_name_size);
		if (val != NULL) {
			return val;
		}
	}
	
	if (obj->creator_slots != NULL) {
		Con_Obj *val = Con_Slots_get_slot(thread, obj->creator_slots, slot_name, slot_name_size);
		if (val != NULL) {
			return val;
		}
	}
	
	return NULL;
}



//
// Sets slot 'slot_name' in 'obj' to 'val'.
//

void Con_Object_set_slot(Con_Obj *thread, Con_Obj *obj, Con_Obj *slot_name_obj, const u_char *slot_name, Con_Int slot_name_size, Con_Obj *val)
{
	assert((slot_name_obj != NULL) || (slot_name != NULL));

	CON_MUTEX_LOCK(&obj->mutex);
	
	if (obj->custom_set_slot)
		CON_XXX;
	
	if (slot_name == NULL) {
		assert(slot_name_size == 0);
		Con_Builtins_String_Atom *slot_name_obj_string_atom = CON_GET_ATOM(slot_name_obj, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
		slot_name = slot_name_obj_string_atom->str;
		slot_name_size = slot_name_obj_string_atom->size;
	}

	Con_Builtins_Slots_Atom *slots_atom = CON_FIND_ATOM(obj, CON_BUILTIN(CON_BUILTIN_SLOTS_ATOM_DEF_OBJECT));
	if (slots_atom == NULL)
		CON_XXX; // immutable object
	
	Con_Slots_set_slot(thread, &obj->mutex, &slots_atom->slots, slot_name, slot_name_size, val);
	if (memcmp(slot_name, "get_slot", sizeof("get_slot") - 1) == 0)
		obj->custom_get_slot = 1;
	else if (memcmp(slot_name, "set_slot", sizeof("set_slot") - 1) == 0)
		obj->custom_set_slot = 1;

	CON_MUTEX_UNLOCK(&obj->mutex);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Shortcut equality tests and arithmetic
//

//
// Returns 'lhs' + 'rhs' or NULL if addition between the two objects failed.
//

Con_Obj *Con_Object_add(Con_Obj *thread, Con_Obj *lhs, Con_Obj *rhs)
{
	CON_MUTEXES_LOCK(&lhs->mutex, &rhs->mutex);
	
	if (lhs->virgin && rhs->virgin && lhs->first_atom->atom_type == CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT) && rhs->first_atom->atom_type == CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT)) {
		CON_MUTEXES_UNLOCK(&lhs->mutex, &rhs->mutex);
		Con_Int lhs_val = ((Con_Builtins_Int_Atom *) lhs->first_atom)->val;
		Con_Int rhs_val = ((Con_Builtins_Int_Atom *) rhs->first_atom)->val;
		
		// We optimise the simple cases:
		//
		//   x + 0 == l
		//   0 + x == x
		//
		// to avoid creating new objects when possible.
		
		if (lhs_val == 0)
			return rhs;
			
		if (rhs_val == 0)
			return lhs;
			
		return Con_Builtins_Int_Atom_new(thread, lhs_val + rhs_val);
	}

	CON_MUTEXES_UNLOCK(&lhs->mutex, &rhs->mutex);
	
	return CON_GET_SLOT_APPLY_NO_FAIL(lhs, "+", rhs);
}



//
// Returns 'lhs' + 'rhs' or NULL if addition between the two objects failed.
//

Con_Obj *Con_Object_subtract(Con_Obj *thread, Con_Obj *lhs, Con_Obj *rhs)
{
	CON_MUTEXES_LOCK(&lhs->mutex, &rhs->mutex);
	
	if (lhs->virgin && rhs->virgin && lhs->first_atom->atom_type == CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT) && rhs->first_atom->atom_type == CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT)) {
		CON_MUTEXES_UNLOCK(&lhs->mutex, &rhs->mutex);

		// Since x - 0 == x, return x without creating a new object.

		if (((Con_Builtins_Int_Atom *) rhs->first_atom)->val == 0)
			return lhs;

		return Con_Builtins_Int_Atom_new(thread, ((Con_Builtins_Int_Atom *) lhs->first_atom)->val - ((Con_Builtins_Int_Atom *) rhs->first_atom)->val);
	}

	CON_MUTEXES_UNLOCK(&lhs->mutex, &rhs->mutex);
	
	return CON_GET_SLOT_APPLY_NO_FAIL(lhs, "-", rhs);
}



//
// Return true if 'lhs' == 'rhs'.
//

bool Con_Object_eq(Con_Obj *thread, Con_Obj *lhs, Con_Obj *rhs)
{
	CON_MUTEXES_LOCK(&lhs->mutex, &rhs->mutex);
	
	if (lhs->virgin && rhs->virgin && lhs->first_atom->atom_type == CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT) && rhs->first_atom->atom_type == CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT)) {
		CON_MUTEXES_UNLOCK(&lhs->mutex, &rhs->mutex);
		if (((Con_Builtins_Int_Atom *) lhs->first_atom)->val == ((Con_Builtins_Int_Atom *) rhs->first_atom)->val)
			return true;
		else
			return false;
	}
	else if (lhs->virgin && rhs->virgin && lhs->first_atom->atom_type == CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT) && rhs->first_atom->atom_type == CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT)) {
		CON_MUTEXES_UNLOCK(&lhs->mutex, &rhs->mutex);
		Con_Builtins_String_Atom *lhs_string_atom = (Con_Builtins_String_Atom *) lhs->first_atom;
		Con_Builtins_String_Atom *rhs_string_atom = (Con_Builtins_String_Atom *) rhs->first_atom;

		if (lhs_string_atom->encoding == rhs_string_atom->encoding) {
			if ((lhs_string_atom->hash == rhs_string_atom->hash) && (lhs_string_atom->size == rhs_string_atom->size) && (memcmp(lhs_string_atom->str, rhs_string_atom->str, lhs_string_atom->size) == 0))
				return true;
			else
				return false;
		}
	}
	
	CON_MUTEXES_UNLOCK(&lhs->mutex, &rhs->mutex);
	
	if (CON_GET_SLOT_APPLY_NO_FAIL(lhs, "==", rhs) == NULL)
		return false;
	else
		return true;
}



//
// Return true if 'lhs' != 'rhs'.
//

bool Con_Object_neq(Con_Obj *thread, Con_Obj *lhs, Con_Obj *rhs)
{
	CON_MUTEXES_LOCK(&lhs->mutex, &rhs->mutex);
	
	if (lhs->virgin && rhs->virgin && lhs->first_atom->atom_type == CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT) && rhs->first_atom->atom_type == CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT)) {
		CON_MUTEXES_UNLOCK(&lhs->mutex, &rhs->mutex);
		if (((Con_Builtins_Int_Atom *) lhs->first_atom)->val != ((Con_Builtins_Int_Atom *) rhs->first_atom)->val)
			return true;
		else
			return false;
	}
	else if (lhs->virgin && rhs->virgin && lhs->first_atom->atom_type == CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT) && rhs->first_atom->atom_type == CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT)) {
		CON_MUTEXES_UNLOCK(&lhs->mutex, &rhs->mutex);
		Con_Builtins_String_Atom *lhs_string_atom = (Con_Builtins_String_Atom *) lhs->first_atom;
		Con_Builtins_String_Atom *rhs_string_atom = (Con_Builtins_String_Atom *) rhs->first_atom;

		if (lhs_string_atom->encoding == rhs_string_atom->encoding) {
			if (!((lhs_string_atom->hash == rhs_string_atom->hash) && (lhs_string_atom->size == rhs_string_atom->size) && (memcmp(lhs_string_atom->str, rhs_string_atom->str, lhs_string_atom->size) == 0)))
				return true;
			else
				return false;
		}
	}
	
	CON_MUTEXES_UNLOCK(&lhs->mutex, &rhs->mutex);
	
	if (CON_GET_SLOT_APPLY_NO_FAIL(lhs, "!=", rhs) == NULL)
		return false;
	else
		return true;
}



//
// Return true if 'lhs' > 'rhs'.
//

bool Con_Object_gt(Con_Obj *thread, Con_Obj *lhs, Con_Obj *rhs)
{
	CON_MUTEXES_LOCK(&lhs->mutex, &rhs->mutex);
	
	if (lhs->virgin && rhs->virgin && lhs->first_atom->atom_type == CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT) && rhs->first_atom->atom_type == CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT)) {
		CON_MUTEXES_UNLOCK(&lhs->mutex, &rhs->mutex);
		if (((Con_Builtins_Int_Atom *) lhs->first_atom)->val > ((Con_Builtins_Int_Atom *) rhs->first_atom)->val)
			return true;
		else
			return false;
	}

	CON_MUTEXES_UNLOCK(&lhs->mutex, &rhs->mutex);
	
	if (CON_GET_SLOT_APPLY_NO_FAIL(lhs, ">", rhs) == NULL)
		return false;
	else
		return true;
}



//
// Return true if 'lhs' < 'rhs'.
//

bool Con_Object_le(Con_Obj *thread, Con_Obj *lhs, Con_Obj *rhs)
{
	CON_MUTEXES_LOCK(&lhs->mutex, &rhs->mutex);
	
	if (lhs->virgin && rhs->virgin && lhs->first_atom->atom_type == CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT) && rhs->first_atom->atom_type == CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT)) {
		CON_MUTEXES_UNLOCK(&lhs->mutex, &rhs->mutex);
		if (((Con_Builtins_Int_Atom *) lhs->first_atom)->val < ((Con_Builtins_Int_Atom *) rhs->first_atom)->val)
			return true;
		else
			return false;
	}

	CON_MUTEXES_UNLOCK(&lhs->mutex, &rhs->mutex);
	
	if (CON_GET_SLOT_APPLY_NO_FAIL(lhs, "<", rhs) == NULL)
		return false;
	else
		return true;
}



//
// Return true if 'lhs' <= 'rhs'.
//

bool Con_Object_le_eq(Con_Obj *thread, Con_Obj *lhs, Con_Obj *rhs)
{
	CON_MUTEXES_LOCK(&lhs->mutex, &rhs->mutex);
	
	if (lhs->virgin && rhs->virgin && lhs->first_atom->atom_type == CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT) && rhs->first_atom->atom_type == CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT)) {
		CON_MUTEXES_UNLOCK(&lhs->mutex, &rhs->mutex);
		if (((Con_Builtins_Int_Atom *) lhs->first_atom)->val <= ((Con_Builtins_Int_Atom *) rhs->first_atom)->val)
			return true;
		else
			return false;
	}

	CON_MUTEXES_UNLOCK(&lhs->mutex, &rhs->mutex);
	
	if (CON_GET_SLOT_APPLY_NO_FAIL(lhs, "<=", rhs) == NULL)
		return false;
	else
		return true;
}



//
// Return true if 'lhs' >= 'rhs'.
//

bool Con_Object_gr_eq(Con_Obj *thread, Con_Obj *lhs, Con_Obj *rhs)
{
	CON_MUTEXES_LOCK(&lhs->mutex, &rhs->mutex);
	
	if (lhs->virgin && rhs->virgin && lhs->first_atom->atom_type == CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT) && rhs->first_atom->atom_type == CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT)) {
		CON_MUTEXES_UNLOCK(&lhs->mutex, &rhs->mutex);
		if (((Con_Builtins_Int_Atom *) lhs->first_atom)->val >= ((Con_Builtins_Int_Atom *) rhs->first_atom)->val)
			return true;
		else
			return false;
	}

	CON_MUTEXES_UNLOCK(&lhs->mutex, &rhs->mutex);
	
	if (CON_GET_SLOT_APPLY_NO_FAIL(lhs, ">=", rhs) == NULL)
		return false;
	else
		return true;
}
