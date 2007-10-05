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
#include "Object.h"
#include "Shortcuts.h"
#include "Slots.h"

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



void _Con_Builtins_Class_Atom_gc_scan_func(Con_Obj *, Con_Obj *, Con_Atom *);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_Class_Atom_bootstrap(Con_Obj *thread)
{
	Con_Obj *class_atom_def = CON_BUILTIN(CON_BUILTIN_CLASS_ATOM_DEF_OBJECT);
	Con_Builtins_Atom_Def_Atom *atom_def_atom = (Con_Builtins_Atom_Def_Atom *) class_atom_def->first_atom;
	atom_def_atom->next_atom = NULL;
	
	Con_Builtins_Atom_Def_Atom_init_atom(thread, atom_def_atom, _Con_Builtins_Class_Atom_gc_scan_func, NULL);
	
	Con_Memory_change_chunk_type(thread, class_atom_def, CON_MEMORY_CHUNK_OBJ);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Class creation functions
//

//
// Create a new class.
//

Con_Obj *Con_Builtins_Class_Atom_new(Con_Obj *thread, Con_Obj *name, Con_Obj *new_object, Con_Obj *container, ...)
{
	Con_Obj *new_class = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(Con_Builtins_Class_Atom) + sizeof(Con_Builtins_Slots_Atom), CON_BUILTIN(CON_BUILTIN_CLASS_CLASS));
	Con_Builtins_Class_Atom *class_atom = (Con_Builtins_Class_Atom *) new_class->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (class_atom + 1);
	class_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;

	class_atom->atom_type = CON_BUILTIN(CON_BUILTIN_CLASS_ATOM_DEF_OBJECT);
	class_atom->name = name;
	class_atom->container = container;
	Con_Slots_init(thread, &class_atom->fields);
	class_atom->virgin_class = 0;
	class_atom->class_fields_for_children = NULL;
	class_atom->custom_get_slot_field = 0;
	class_atom->custom_set_slot_field = 0;
	class_atom->custom_find_slot_field = 0;
	class_atom->new_object = new_object;
	
	va_list ap;
	va_start(ap, container);
	class_atom->num_supers = 0;
	while (va_arg(ap, Con_Obj *) != NULL)
		class_atom->num_supers += 1;
	va_end(ap);

	class_atom->supers = Con_Memory_malloc(thread, sizeof(Con_Obj *) * class_atom->num_supers, CON_MEMORY_CHUNK_OPAQUE);
	
	va_start(ap, container);
	Con_Obj *super;
	int i = 0;
	while ((super = va_arg(ap, Con_Obj *)) != NULL) {
		class_atom->supers[i] = super;
		i += 1;
	}
	va_end(ap);
	
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);

	Con_Memory_change_chunk_type(thread, new_class, CON_MEMORY_CHUNK_OBJ);
	
	return new_class;
}



//
// Initialize the atom of a built-in class. This *MUST NOT* be used to initialize a non-built-in
// class's atom.
//

void Con_Builtins_Class_Atom_init_atom(Con_Obj *thread, Con_Builtins_Class_Atom *class_atom, Con_Obj *name, Con_Obj *new_object, ...)
{
	class_atom->atom_type = CON_BUILTIN(CON_BUILTIN_CLASS_ATOM_DEF_OBJECT);
	class_atom->name = name;
	class_atom->container = CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
	Con_Slots_init(thread, &class_atom->fields);
	class_atom->virgin_class = 1;
	class_atom->class_fields_for_children = NULL;
	class_atom->custom_get_slot_field = 0;
	class_atom->custom_set_slot_field = 0;
	class_atom->custom_find_slot_field = 0;
	class_atom->new_object = new_object;

	va_list ap;
	va_start(ap, new_object);
	class_atom->num_supers = 0;
	while (va_arg(ap, Con_Obj *) != NULL)
		class_atom->num_supers += 1;
	va_end(ap);

	class_atom->supers = Con_Memory_malloc(thread, sizeof(Con_Obj *) * class_atom->num_supers, CON_MEMORY_CHUNK_OPAQUE);
	
	va_start(ap, new_object);
	Con_Obj *super;
	int i = 0;
	while ((super = va_arg(ap, Con_Obj *)) != NULL) {
		class_atom->supers[i] = super;
		i += 1;
	}
	va_end(ap);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Class atom functions
//

Con_Obj *Con_Builtins_Class_Atom_get_field(Con_Obj *thread, Con_Obj *class_, const u_char *slot_name, Con_Int slot_name_size)
{
	Con_Builtins_Class_Atom *class_atom = CON_GET_ATOM(class_, CON_BUILTIN(CON_BUILTIN_CLASS_ATOM_DEF_OBJECT));

	CON_MUTEX_LOCK(&class_->mutex);
	Con_Obj *val;
	bool got = Con_Slots_get_slot(thread, &class_atom->fields, slot_name, slot_name_size, &val);
	CON_MUTEX_UNLOCK(&class_->mutex);

	if (!got)
		CON_RAISE_EXCEPTION("Field_Exception", Con_Builtins_String_Atom_new_copy(thread, slot_name, slot_name_size, CON_STR_UTF_8), class_);
	
	return val;
}



void Con_Builtins_Class_Atom_set_field(Con_Obj *thread, Con_Obj *class_, const u_char *slot_name, Con_Int slot_name_size, Con_Obj *val)
{
	Con_Builtins_Class_Atom *class_atom = CON_GET_ATOM(class_, CON_BUILTIN(CON_BUILTIN_CLASS_ATOM_DEF_OBJECT));

	CON_MUTEX_LOCK(&class_->mutex);
	Con_Slots_set_slot(thread, &class_->mutex, &class_atom->fields, slot_name, slot_name_size, val);

#	define NAME_COMPARISON(name) ((slot_name_size == sizeof(name) - 1) && (memcmp(slot_name, name, sizeof(name) - 1) == 0))

	// If the class we're setting the field on is the Object class, we don't set the custom_* slots,
	// as the default behaviour is implemented in the VM and is far more efficient than continually
	// using the MOP.
	
	if (class_ != CON_BUILTIN(CON_BUILTIN_OBJECT_CLASS)) {
		if (NAME_COMPARISON("get_slot"))
			class_atom->custom_get_slot_field = 1;
		else if (NAME_COMPARISON("set_slot")) 
			class_atom->custom_set_slot_field = 1;
		else if (NAME_COMPARISON("find_slot")) 
			class_atom->custom_find_slot_field = 1;
	}

	// Since we've altered the class's fields, we need to reset class_fields_for_children to ensure
	// that it is recalculated the next time an object is created.

	class_atom->class_fields_for_children = NULL;

	CON_MUTEX_UNLOCK(&class_->mutex);
}



Con_Slots *Con_Builtins_Class_Atom_get_creator_slots(Con_Obj *thread, Con_Obj *class_)
{
	CON_ASSERT_MUTEX_LOCKED(&class_->mutex);

	Con_Builtins_Class_Atom *class_atom = CON_GET_ATOM(class_, CON_BUILTIN(CON_BUILTIN_CLASS_ATOM_DEF_OBJECT));

	// If there's already a valid class_fields_for_children, return that.
	
	Con_Slots *class_fields_for_children = class_atom->class_fields_for_children;
	if (class_fields_for_children != NULL) {
		return class_fields_for_children;
	}
	
	// We're going to have to create a new class_fields_for_children.
	
	bool custom_get_slot_field = false, custom_set_slot_field = false, custom_find_slot_field = false;
	class_fields_for_children = Con_Memory_malloc_no_gc(thread, sizeof(Con_Slots), CON_MEMORY_CHUNK_CONSERVATIVE);
	if (class_fields_for_children == NULL)
		CON_XXX;
	Con_Slots_init(thread, class_fields_for_children);
	
	// First of all copy all the required fields from this classes' supers.
	
	for (Con_Int i = 0; i < class_atom->num_supers; i += 1) {
		Con_Obj *super_obj = class_atom->supers[i];
		Con_Builtins_Class_Atom *super_class_atom = CON_GET_ATOM(super_obj, CON_BUILTIN(CON_BUILTIN_CLASS_ATOM_DEF_OBJECT));
		Con_Slots *supers_class_fields_for_children;
		CON_MUTEX_UNLOCK(&class_->mutex);
		
		CON_MUTEX_LOCK(&super_obj->mutex);
		if (super_class_atom->class_fields_for_children == NULL) {
			supers_class_fields_for_children = super_class_atom->class_fields_for_children = Con_Builtins_Class_Atom_get_creator_slots(thread, super_obj);
		}
		else {
			supers_class_fields_for_children = super_class_atom->class_fields_for_children;
		}
		
		if (super_class_atom->custom_get_slot_field)
			custom_get_slot_field = 1;
		if (super_class_atom->custom_set_slot_field)
			custom_set_slot_field = 1;
		if (super_class_atom->custom_find_slot_field)
			custom_find_slot_field = 1;
		CON_MUTEX_UNLOCK(&super_obj->mutex);
		
		CON_MUTEX_LOCK(&class_->mutex);
		
		Con_Int j = 0;
		while (1) {
			const u_char *slot_name;
			Con_Int slot_name_size;
			Con_Obj *val;
			if (!Con_Slots_read_slot(thread, supers_class_fields_for_children, &j, &slot_name, &slot_name_size, &val))
				break;
			Con_Slots_set_slot(thread, &class_->mutex, class_fields_for_children, slot_name, slot_name_size, val);
		}
	}

	// Second, copy all the fields from this class.

	Con_Int j = 0;
	while (1) {
		const u_char *slot_name;
		Con_Int slot_name_size;
		Con_Obj *val;
		if (!Con_Slots_read_slot(thread, &class_atom->fields, &j, &slot_name, &slot_name_size, &val))
			break;
		Con_Slots_set_slot(thread, &class_->mutex, class_fields_for_children, slot_name, slot_name_size, val);
	}
	
	Con_Slots_set_slot(thread, &class_->mutex, class_fields_for_children, (u_char *) "instance_of", sizeof("instance_of") - 1, class_);

	if (custom_get_slot_field)
		class_atom->custom_get_slot_field = 1;
	if (custom_set_slot_field)
		class_atom->custom_set_slot_field = 1;
	if (custom_find_slot_field)
		class_atom->custom_find_slot_field = 1;

	// Update class_fields_for_children so it won't be unnecessarily calculated again.

	class_atom->class_fields_for_children = class_fields_for_children;

	CON_MUTEX_UNLOCK(&class_->mutex);
	
	Con_Memory_change_chunk_type(thread, class_fields_for_children, CON_MEMORY_CHUNK_SLOTS);

	CON_MUTEX_LOCK(&class_->mutex);

	return class_fields_for_children;
}



//
// This will attempt to mark 'class' as being virgin. This function should be called iff the class
// has been created by calling Builtins.Class.new and only the CON_SET_FIELD macro has been used on
// it.
//
// Note that this function may not succeed in marking the class as virgin if Builtins.Class is not
// itself a virgin. Whether this function succeeds or not should *NOT* alter the observable behaviour
// of a Converge system; it may simply limit opportunities for optimization etc.
//

void Con_Builtins_Class_Atom_mark_as_virgin(Con_Obj *thread, Con_Obj *new_class)
{
	Con_Builtins_Class_Atom *new_class_class_atom = CON_FIND_ATOM(new_class, CON_BUILTIN(CON_BUILTIN_CLASS_ATOM_DEF_OBJECT));
	
	if (new_class_class_atom == NULL) {
		// If new_class doesn't contain a class atom, there's no point going further.
		return;
	}
	
	CON_MUTEX_LOCK(&CON_BUILTIN(CON_BUILTIN_CLASS_CLASS)->mutex);
	Con_Slots *class_creator_slots = Con_Builtins_Class_Atom_get_creator_slots(thread, CON_BUILTIN(CON_BUILTIN_CLASS_CLASS));
	
	CON_MUTEX_ADD_LOCK(&CON_BUILTIN(CON_BUILTIN_CLASS_CLASS)->mutex, &new_class->mutex);
	if (new_class->creator_slots == class_creator_slots) {
		assert(new_class_class_atom->virgin_class == 0);
		new_class_class_atom->virgin_class = 1;
	}
	CON_MUTEXES_UNLOCK(&CON_BUILTIN(CON_BUILTIN_CLASS_CLASS)->mutex, &new_class->mutex);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Garbage collection
//

void _Con_Builtins_Class_Atom_gc_scan_func(Con_Obj *thread, Con_Obj *obj, Con_Atom *atom)
{
	Con_Builtins_Class_Atom *class_atom = (Con_Builtins_Class_Atom *) atom;

	Con_Memory_gc_push(thread, class_atom->name);
	Con_Memory_gc_push(thread, class_atom->container);
	if (class_atom->new_object != NULL)
		Con_Memory_gc_push(thread, class_atom->new_object);
	Con_Slots_gc_scan_slots(thread, &class_atom->fields);
	if (class_atom->class_fields_for_children != NULL)
		Con_Memory_gc_push(thread, class_atom->class_fields_for_children);
	Con_Memory_gc_push(thread, class_atom->supers);
	for (Con_Int i = 0; i < class_atom->num_supers; i += 1)
		Con_Memory_gc_push(thread, class_atom->supers[i]);
}
