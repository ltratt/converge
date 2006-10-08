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
#include "Numbers.h"
#include "Object.h"
#include "Shortcuts.h"
#include "Slots.h"

#include "Builtins/Atom_Def_Atom.h"
#include "Builtins/Class/Atom.h"
#include "Builtins/Class/Class.h"
#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



Con_Obj *_Con_Builtins_Class_Class_new_object(Con_Obj *);

Con_Obj *_Con_Builtins_Class_Class_new_func(Con_Obj *);
Con_Obj *_Con_Builtins_Class_Class_get_slot_func(Con_Obj *);
Con_Obj *_Con_Builtins_Class_Class_path_func(Con_Obj *);
Con_Obj *_Con_Builtins_Class_Class_get_field_func(Con_Obj *);
Con_Obj *_Con_Builtins_Class_Class_set_field_func(Con_Obj *);
Con_Obj *_Con_Builtins_Class_Class_conformed_by_func(Con_Obj *);
Con_Obj *_Con_Builtins_Class_Class_instantiated_func(Con_Obj *);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap functions
//

void Con_Builtins_Class_Class_bootstrap(Con_Obj *thread)
{
	Con_Obj *class_class = CON_BUILTIN(CON_BUILTIN_CLASS_CLASS);
	
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) class_class->first_atom;
	Con_Builtins_Class_Atom *class_atom = (Con_Builtins_Class_Atom *) (slots_atom + 1);

	slots_atom->next_atom = (Con_Atom *) class_atom;
	class_atom->next_atom = NULL;	
	
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);
	Con_Builtins_Class_Atom_init_atom(thread, class_atom, CON_NEW_STRING("Class"), CON_NEW_UNBOUND_C_FUNC(_Con_Builtins_Class_Class_new_object, "Class_new", CON_BUILTIN(CON_BUILTIN_NULL_OBJ)), CON_BUILTIN(CON_BUILTIN_OBJECT_CLASS), NULL);
	
	Con_Memory_change_chunk_type(thread, class_class, CON_MEMORY_CHUNK_OBJ);
	
	CON_SET_FIELD(class_class, "new", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Class_Class_new_func, "new", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), class_class));
	
	CON_SET_FIELD(class_class, "get_slot", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Class_Class_get_slot_func, "get_slot", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), class_class));

	CON_SET_FIELD(class_class, "path", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Class_Class_path_func, "path", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), class_class));

	CON_SET_FIELD(class_class, "get_field", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Class_Class_get_field_func, "get_field", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), class_class));
	CON_SET_FIELD(class_class, "set_field", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Class_Class_set_field_func, "set_field", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), class_class));
	CON_SET_FIELD(class_class, "conformed_by", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Class_Class_conformed_by_func, "conformed_by", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), class_class));
	CON_SET_FIELD(class_class, "instantiated", CON_NEW_BOUND_C_FUNC(_Con_Builtins_Class_Class_instantiated_func, "instantiated", CON_BUILTIN(CON_BUILTIN_NULL_OBJ), class_class));
}



Con_Obj *_Con_Builtins_Class_Class_new_object(Con_Obj *thread)
{
	Con_Obj *class, *container, *name, *new_object, *supers;
	CON_UNPACK_ARGS("COOO;O", &class, &name, &supers, &container, &new_object);

	Con_Obj *new_class = Con_Object_new_from_class(thread, sizeof(Con_Obj) + sizeof(Con_Builtins_Class_Atom) + sizeof(Con_Builtins_Slots_Atom), class);
	Con_Builtins_Class_Atom *class_atom = (Con_Builtins_Class_Atom *) new_class->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (class_atom + 1);
	class_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;

	class_atom->atom_type = CON_BUILTIN(CON_BUILTIN_CLASS_ATOM_DEF_OBJECT);
	class_atom->name = name;
	class_atom->container = container;
	Con_Slots_init(thread, &class_atom->fields);
	class_atom->class_fields_for_children = NULL;
	class_atom->virgin_class = 0;
	class_atom->custom_get_slot_field = 0;
	class_atom->custom_set_slot_field = 0;
	class_atom->custom_has_slot_field = 0;

	class_atom->num_supers = Con_Numbers_Number_to_Con_Int(thread, CON_GET_SLOT_APPLY(supers, "len"));
	class_atom->supers = Con_Memory_malloc(thread, sizeof(Con_Obj *) * class_atom->num_supers, CON_MEMORY_CHUNK_OPAQUE);
	
	if (new_object != NULL) {
		// A new_object func has been supplied so we don't need to bother searching through super
		// classes as we copy them over.
		for (Con_Int i = 0; i < class_atom->num_supers; i += 1) {
			class_atom->supers[i] = CON_GET_SLOT_APPLY(supers, "get", CON_NEW_INT(i));
		}
	}
	else {
		// A new_object func hasn't been supplied so we need to search one while copying over super
		// classes.
		for (Con_Int i = 0; i < class_atom->num_supers; i += 1) {
			Con_Obj *super = CON_GET_SLOT_APPLY(supers, "get", CON_NEW_INT(i));
			Con_Builtins_Class_Atom *super_class_atom = CON_GET_ATOM(super, CON_BUILTIN(CON_BUILTIN_CLASS_ATOM_DEF_OBJECT));
			CON_MUTEX_LOCK(&super->mutex);
			if (super_class_atom->new_object != NULL) {
				if (new_object != NULL) {
					// It's only permissible to find a single new_object function in superclasses
					// unless we already have found a new_object function and we've merely bumped
					// into the new_object function defined by the Object class in which case it
					// doesn't count. This is important as otherwise everything would clash with
					// the new_object function defined in Object.
					if (super != CON_BUILTIN(CON_BUILTIN_OBJECT_CLASS))
						CON_XXX;
				}
				else
					new_object = super_class_atom->new_object;
			}
			CON_MUTEX_UNLOCK(&super->mutex);
			class_atom->supers[i] = super;
		}
		assert(new_object != NULL);
	}
	assert(new_object != NULL);
	class_atom->new_object = new_object;
	
	Con_Builtins_Slots_Atom_Def_init_atom(thread, slots_atom);

	Con_Memory_change_chunk_type(thread, new_class, CON_MEMORY_CHUNK_OBJ);
	
	CON_GET_SLOT_APPLY(CON_GET_SLOT(new_class, "init"), "apply", Con_Builtins_List_Atom_new_va(thread, name, supers, container));
	
	CON_RETURN(new_class);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Class class fields
//

//
// 'new(...)'.
//

Con_Obj *_Con_Builtins_Class_Class_new_func(Con_Obj *thread)
{
	Con_Obj *var_args;
	CON_UNPACK_ARGS("v", &var_args);

	Con_Obj *self = CON_GET_SLOT_APPLY(var_args, "get", CON_NEW_INT(0));
	Con_Builtins_Class_Atom *class_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_CLASS_ATOM_DEF_OBJECT));

	CON_MUTEX_LOCK(&self->mutex);
	Con_Obj *new_object = class_atom->new_object;
	CON_MUTEX_UNLOCK(&self->mutex);
	
	if (new_object == NULL)
		CON_XXX; // temporary?

	CON_RETURN(CON_GET_SLOT_APPLY(new_object, "apply", var_args));
}



//
// 'get_slot(name, caller)'.
//

Con_Obj *_Con_Builtins_Class_Class_get_slot_func(Con_Obj *thread)
{
	Con_Obj *self, *slot_name;
	CON_UNPACK_ARGS("CO", &self, &slot_name);

	Con_Builtins_Class_Atom *class_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_CLASS_ATOM_DEF_OBJECT));

	if (CON_C_STRING_EQ("name", slot_name)) {
		CON_MUTEX_LOCK(&self->mutex);
		Con_Obj *name = class_atom->name;
		CON_MUTEX_UNLOCK(&self->mutex);
		CON_RETURN(name);
	}
	else
		CON_RETURN(CON_APPLY(CON_EXBI(CON_BUILTIN(CON_BUILTIN_OBJECT_CLASS), "get_slot", self), slot_name));
}



//
// 'path(stop_at)' returns the path (including containers up to, but including, stop_at) of this
// function.
//
// XXX incomplete
//

Con_Obj *_Con_Builtins_Class_Class_path_func(Con_Obj *thread)
{
	Con_Obj *self, *stop_at;
	CON_UNPACK_ARGS("CO", &self, &stop_at);

	if (self == stop_at)
		CON_RETURN(CON_NEW_STRING(""));

	Con_Builtins_Class_Atom *self_class_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_CLASS_ATOM_DEF_OBJECT));
	
	CON_MUTEX_LOCK(&self->mutex);
	Con_Obj *name = self_class_atom->name;
	Con_Obj *container = self_class_atom->container;
	CON_MUTEX_UNLOCK(&self->mutex);
	
	if ((container == CON_BUILTIN(CON_BUILTIN_NULL_OBJ)) || (container == stop_at))
		CON_RETURN(name);
	else {
		Con_Obj *rtn = CON_GET_SLOT_APPLY(container, "path", stop_at);
		rtn = CON_ADD(rtn, CON_NEW_STRING("."));
		rtn = CON_ADD(rtn, name);
		
		CON_RETURN(rtn);
	}
}



//
// 'get_field(name)'.
//

Con_Obj *_Con_Builtins_Class_Class_get_field_func(Con_Obj *thread)
{
	Con_Obj *name, *self;
	CON_UNPACK_ARGS("CS", &self, &name);

	Con_Builtins_String_Atom *name_string_atom = CON_GET_ATOM(name, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	
	assert(name_string_atom->encoding = CON_STR_UTF_8);
	
	CON_RETURN(Con_Builtins_Class_Atom_get_field(thread, self, name_string_atom->str, name_string_atom->size));
}



//
// 'set_field(name, o)'.
//

Con_Obj *_Con_Builtins_Class_Class_set_field_func(Con_Obj *thread)
{
	Con_Obj *name, *self, *o;
	CON_UNPACK_ARGS("CSO", &self, &name, &o);

	Con_Builtins_String_Atom *name_string_atom = CON_GET_ATOM(name, CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT));
	
	assert(name_string_atom->encoding = CON_STR_UTF_8);
	
	Con_Builtins_Class_Atom_set_field(thread, self, name_string_atom->str, name_string_atom->size, o);

	CON_RETURN(CON_BUILTIN(CON_BUILTIN_NULL_OBJ));
}



//
// 'conformed_by(obj)'.
//

Con_Obj *_Con_Builtins_Class_Class_conformed_by_func(Con_Obj *thread)
{
	Con_Obj *self, *o;
	CON_UNPACK_ARGS("CO", &self, &o);

	Con_Builtins_Class_Atom *class_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_CLASS_ATOM_DEF_OBJECT));

	CON_MUTEXES_LOCK(&self->mutex, &o->mutex);
	bool match = true;
	// First of all check the low-cost route: if o's 'creator_slots' is the same as this classes
	// class' 'class_fields_for_children' then by definition 'o' is conformant and we can return
	// true.
	if (!((class_atom->class_fields_for_children != NULL) && (o->creator_slots == class_atom->class_fields_for_children))) {
		// Bugger. We have to do it the long way by checking that 'o' has a slot for every field
		// in 'self'.
		if (o->custom_has_slot)
			CON_XXX;
		else {
			Con_Builtins_Slots_Atom *o_slots_atom = CON_FIND_ATOM(o, CON_BUILTIN(CON_BUILTIN_SLOTS_ATOM_DEF_OBJECT));
			Con_Int i = 0;
			while (1) {
				const char *slot_name;
				Con_Int slot_name_size;
				Con_Obj *val;
				if (!Con_Slots_read_slot(thread, &class_atom->fields, &i, &slot_name, &slot_name_size, &val))
					break;

				if (o_slots_atom != NULL) {
					Con_Obj *val = Con_Slots_get_slot(thread, &o_slots_atom->slots, slot_name, slot_name_size);
					if (val != NULL)
						continue;
				}

				if (o->creator_slots != NULL) {
					Con_Obj *val = Con_Slots_get_slot(thread, o->creator_slots, slot_name, slot_name_size);
					if (val != NULL)
						continue;
				}

				match = false;
				break;
			}
		}
	}
	
	CON_MUTEXES_UNLOCK(&self->mutex, &o->mutex);

	if (match)
		CON_RETURN(CON_BUILTIN(CON_BUILTIN_NULL_OBJ));
	else
		CON_RETURN(CON_BUILTIN(CON_BUILTIN_FAIL_OBJ));
}



//
// 'instantiated(obj)' returns null if this class (or one of its superclasses) instantiated 'obj', or
// fails otherwise.
//

Con_Obj *_Con_Builtins_Class_Class_instantiated_func(Con_Obj *thread)
{
	Con_Obj *self, *o;
	CON_UNPACK_ARGS("CO", &self, &o);

	Con_Builtins_Class_Atom *class_atom = CON_GET_ATOM(self, CON_BUILTIN(CON_BUILTIN_CLASS_ATOM_DEF_OBJECT));

	Con_Obj *instance_of = CON_GET_SLOT(o, "instance_of");
	if (instance_of == self)
		CON_RETURN(CON_BUILTIN(CON_BUILTIN_NULL_OBJ));
	else {
		Con_Obj *stack = Con_Builtins_List_Atom_new(thread);
		CON_GET_SLOT_APPLY(stack, "append", self);
		Con_Int i = 0;
		while (i < Con_Numbers_Number_to_Con_Int(thread, CON_GET_SLOT_APPLY(stack, "len"))) {
			Con_Obj *cnd = CON_GET_SLOT_APPLY(stack, "get", CON_NEW_INT(i));
			if (instance_of == self)
				CON_RETURN(CON_BUILTIN(CON_BUILTIN_NULL_OBJ));
			
			Con_Builtins_Class_Atom *cnd_class_atom = CON_GET_ATOM(cnd, CON_BUILTIN(CON_BUILTIN_CLASS_ATOM_DEF_OBJECT));
			
			CON_MUTEX_LOCK(&cnd->mutex);
			for (Con_Int j = 0; j < cnd_class_atom->num_supers; j += 1) {
				Con_Obj *super = cnd_class_atom->supers[j];
				CON_MUTEX_UNLOCK(&cnd->mutex);
				CON_GET_SLOT_APPLY(stack, "append", super);
				CON_MUTEX_LOCK(&cnd->mutex);
			}
			CON_MUTEX_LOCK(&cnd->mutex);
			
			i += 1;
		}
		
		CON_RETURN(CON_BUILTIN(CON_BUILTIN_FAIL_OBJ));
	}
}
