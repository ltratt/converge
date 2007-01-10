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


#ifndef _CON_BUILTINS_CLASS_ATOM_H
#define _CON_BUILTINS_CLASS_ATOM_H

#include "Core.h"
#include "Slots.h"

typedef struct {
	CON_ATOM_HEAD
	
	Con_Obj *name, *container, *new_object;
	Con_Slots fields;
	// class_fields_for_children starts as NULL; if this class is used to create an object, then
	// it points to a Con_Slots chunk of memory so that it can be reused by multiple objects,
	// saving on memory and the time necessary to setup that memory. If subsequently the class's
	// fields change, then class_fields_for_children is set to NULL, forcing recalculation the next
	// time the class creates an object.
	//
	// After its initial population, a 'class_fields_for_children' is immutable.
	Con_Slots *class_fields_for_children;

	// virgin_class 		 : whether this object is a virgin builtin class i.e. no methods have
	//  					   been altered since it was created. Only virgin classes can create
	//  					   virgin objects.
	// custom_get_slot_field : whether this object defines a non-default get_slot function
	// custom_set_slot_field : whether this object defines a non-default get_slot function
	// custom_find_slot_field : whether this object defines a non-default get_slot function
	unsigned int
		virgin_class : 1, custom_get_slot_field : 1, custom_set_slot_field : 1, custom_find_slot_field : 1;
	
	Con_Int num_supers;
	Con_Obj **supers;
} Con_Builtins_Class_Atom;


void Con_Builtins_Class_Atom_bootstrap(Con_Obj *);

Con_Obj *Con_Builtins_Class_Atom_new(Con_Obj *, Con_Obj *, Con_Obj *, Con_Obj *, ...);
void Con_Builtins_Class_Atom_init_atom(Con_Obj *, Con_Builtins_Class_Atom *, Con_Obj *, Con_Obj *, ...);

Con_Obj *Con_Builtins_Class_Atom_get_field(Con_Obj *, Con_Obj *, const u_char *, Con_Int);
void Con_Builtins_Class_Atom_set_field(Con_Obj *thread, Con_Obj *, const u_char *, Con_Int, Con_Obj *);
Con_Slots *Con_Builtins_Class_Atom_get_creator_slots(Con_Obj *, Con_Obj *);
void Con_Builtins_Class_Atom_mark_as_virgin(Con_Obj *, Con_Obj *);

#endif
