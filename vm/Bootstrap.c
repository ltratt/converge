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

#include "Bootstrap.h"
#include "Core.h"
#include "Memory.h"
#include "Object.h"
#include "Shortcuts.h"

#include "Builtins/Atom_Def_Atom.h"
#include "Builtins/Class/Atom.h"
#include "Builtins/Class/Class.h"
#include "Builtins/Closure/Atom.h"
#include "Builtins/Closure/Class.h"
#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Con_Stack/Class.h"
#include "Builtins/Dict/Atom.h"
#include "Builtins/Dict/Class.h"
#include "Builtins/Exception/Atom.h"
#include "Builtins/Exception/Class.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Func/Class.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/Int/Class.h"
#include "Builtins/List/Atom.h"
#include "Builtins/List/Class.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/Module/Class.h"
#include "Builtins/Object/Class.h"
#include "Builtins/Partial_Application/Atom.h"
#include "Builtins/Partial_Application/Class.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/Set/Atom.h"
#include "Builtins/Set/Class.h"
#include "Builtins/String/Atom.h"
#include "Builtins/String/Class.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/Thread/Class.h"
#include "Builtins/Unique_Atom_Def.h"
#include "Builtins/VM/Atom.h"
#include "Builtins/VM/Class.h"



//
// Create and initialize a new VM. 'argc' and 'argv' should be the values passed to main(...). This
// function returns NULL if an error occurred during VM initialization; otherwise a Con_Thread object
// is returned.
//

Con_Obj *Con_Bootstrap_do(u_char *c_stack_start, int argc, char **argv)
{
	// During the bootstrapping process, only the thread setting up the system may run, and garbage
	// collection can not occur. These assumptions are implicit in most of the following.

	// First initialize the memory system.

	Con_Memory_Store *mem_store = Con_Memory_init();
	if (mem_store == NULL)
		return NULL;

	Con_Obj *bootstrap_vm = malloc(sizeof(Con_Obj) + sizeof(Con_Builtins_VM_Atom));
	if (bootstrap_vm == NULL)
		return NULL;
	bzero(bootstrap_vm, sizeof(Con_Obj) + sizeof(Con_Builtins_VM_Atom));

	Con_Obj *bootstrap_thread = malloc(sizeof(Con_Obj) + sizeof(Con_Builtins_Thread_Atom));
	if (bootstrap_thread == NULL)
		return NULL;
	bzero(bootstrap_thread, sizeof(Con_Obj) + sizeof(Con_Builtins_Thread_Atom));	
	
	Con_Builtins_Thread_Atom *bootstrap_thread_atom = (Con_Builtins_Thread_Atom *) bootstrap_thread->first_atom;
	bootstrap_thread_atom->vm = bootstrap_vm;
	
	Con_Builtins_VM_Atom *bootstrap_vm_atom = (Con_Builtins_VM_Atom *) bootstrap_vm->first_atom;
	bootstrap_vm_atom->mem_store = mem_store;
	bootstrap_vm_atom->state = CON_VM_INITIALIZING;

	for (int i = 0; i < CON_NUMBER_OF_BUILTINS; i += 1)
		bootstrap_vm_atom->builtins[i] = NULL;

	// Create and bootstrap the slots atom def object and the atom def object before anything else.

	Con_Obj *slots_atom_def_object = Con_Memory_malloc(bootstrap_thread, sizeof(Con_Obj) + sizeof(Con_Builtins_Atom_Def_Atom) + sizeof(Con_Builtins_Slots_Atom), CON_MEMORY_CHUNK_CONSERVATIVE);
	Con_Obj *atom_def_object = Con_Memory_malloc(bootstrap_thread, sizeof(Con_Obj) + sizeof(Con_Builtins_Atom_Def_Atom) + sizeof(Con_Builtins_Slots_Atom), CON_MEMORY_CHUNK_CONSERVATIVE);

	bootstrap_vm_atom->builtins[CON_BUILTIN_SLOTS_ATOM_DEF_OBJECT] = slots_atom_def_object;
	bootstrap_vm_atom->builtins[CON_BUILTIN_ATOM_DEF_OBJECT] = atom_def_object;

	Con_Builtins_Atom_Def_Atom_bootstrap(bootstrap_thread);
	Con_Builtins_Slots_Atom_Def_bootstrap(bootstrap_thread);

#	define NEW_ATOM_DEF_OBJECT(extra_size) Con_Object_new_from_proto(bootstrap_thread, sizeof(Con_Obj) + sizeof(Con_Builtins_Atom_Def_Atom) + sizeof(Con_Builtins_Slots_Atom) + extra_size, NULL);

	bootstrap_vm_atom->builtins[CON_BUILTIN_CLASS_ATOM_DEF_OBJECT] = NEW_ATOM_DEF_OBJECT(0);
	bootstrap_vm_atom->builtins[CON_BUILTIN_VM_ATOM_DEF_OBJECT] = NEW_ATOM_DEF_OBJECT(0);
#	ifdef CON_THREADS_SINGLE_THREADED
	bootstrap_vm_atom->builtins[CON_BUILTIN_THREAD_ATOM_DEF_OBJECT] = NEW_ATOM_DEF_OBJECT(0);
#	else
	bootstrap_vm_atom->builtins[CON_BUILTIN_THREAD_ATOM_DEF_OBJECT] = NEW_ATOM_DEF_OBJECT(sizeof(Con_Builtins_Thread_Class_Unique_Atom));
#	endif
	bootstrap_vm_atom->builtins[CON_BUILTIN_FUNC_ATOM_DEF_OBJECT] = NEW_ATOM_DEF_OBJECT(0);
	bootstrap_vm_atom->builtins[CON_BUILTIN_STRING_ATOM_DEF_OBJECT] = NEW_ATOM_DEF_OBJECT(0);
	bootstrap_vm_atom->builtins[CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT] = NEW_ATOM_DEF_OBJECT(0);
	bootstrap_vm_atom->builtins[CON_BUILTIN_LIST_ATOM_DEF_OBJECT] = NEW_ATOM_DEF_OBJECT(0);
	bootstrap_vm_atom->builtins[CON_BUILTIN_DICT_ATOM_DEF_OBJECT] = NEW_ATOM_DEF_OBJECT(0);
	bootstrap_vm_atom->builtins[CON_BUILTIN_MODULE_ATOM_DEF_OBJECT] = NEW_ATOM_DEF_OBJECT(0);
	bootstrap_vm_atom->builtins[CON_BUILTIN_INT_ATOM_DEF_OBJECT] = NEW_ATOM_DEF_OBJECT(sizeof(Con_Builtins_Int_Class_Unique_Atom));
	bootstrap_vm_atom->builtins[CON_BUILTIN_CLOSURE_ATOM_DEF_OBJECT] = NEW_ATOM_DEF_OBJECT(0);
	bootstrap_vm_atom->builtins[CON_BUILTIN_PARTIAL_APPLICATION_ATOM_DEF_OBJECT] = NEW_ATOM_DEF_OBJECT(0);
	bootstrap_vm_atom->builtins[CON_BUILTIN_EXCEPTION_ATOM_DEF_OBJECT] = NEW_ATOM_DEF_OBJECT(0);
	bootstrap_vm_atom->builtins[CON_BUILTIN_SET_ATOM_DEF_OBJECT] = NEW_ATOM_DEF_OBJECT(0);

#	define NEW_ATOM_DEF_SLOT_CLASS_OBJECT(extra_size) Con_Object_new_from_proto(bootstrap_thread, sizeof(Con_Obj) + sizeof(Con_Builtins_Atom_Def_Atom) + sizeof(Con_Builtins_Slots_Atom) + sizeof(Con_Builtins_Class_Atom) + extra_size, NULL);

#	define NEW_SLOT_CLASS_OBJECT(extra_size) Con_Object_new_from_proto(bootstrap_thread, sizeof(Con_Obj) + sizeof(Con_Builtins_Slots_Atom) + sizeof(Con_Builtins_Class_Atom) + extra_size, NULL);

	// The order that classes are created is important: classes must be created before their
	// subclasses.

	bootstrap_vm_atom->builtins[CON_BUILTIN_OBJECT_CLASS] = NEW_SLOT_CLASS_OBJECT(0);
	bootstrap_vm_atom->builtins[CON_BUILTIN_CLASS_CLASS] = NEW_ATOM_DEF_SLOT_CLASS_OBJECT(0);
	bootstrap_vm_atom->builtins[CON_BUILTIN_VM_CLASS] = NEW_ATOM_DEF_SLOT_CLASS_OBJECT(0);
	bootstrap_vm_atom->builtins[CON_BUILTIN_THREAD_CLASS] = NEW_ATOM_DEF_SLOT_CLASS_OBJECT(0);
	bootstrap_vm_atom->builtins[CON_BUILTIN_FUNC_CLASS] = NEW_ATOM_DEF_SLOT_CLASS_OBJECT(0);
	bootstrap_vm_atom->builtins[CON_BUILTIN_STRING_CLASS] = NEW_ATOM_DEF_SLOT_CLASS_OBJECT(0);
	bootstrap_vm_atom->builtins[CON_BUILTIN_CON_STACK_CLASS] = NEW_ATOM_DEF_SLOT_CLASS_OBJECT(0);
	bootstrap_vm_atom->builtins[CON_BUILTIN_LIST_CLASS] = NEW_ATOM_DEF_SLOT_CLASS_OBJECT(0);
	bootstrap_vm_atom->builtins[CON_BUILTIN_DICT_CLASS] = NEW_ATOM_DEF_SLOT_CLASS_OBJECT(0);
	bootstrap_vm_atom->builtins[CON_BUILTIN_INT_CLASS] = NEW_ATOM_DEF_SLOT_CLASS_OBJECT(0);
	bootstrap_vm_atom->builtins[CON_BUILTIN_UNIQUE_ATOM_DEF_OBJECT] = NEW_ATOM_DEF_OBJECT(0);
	bootstrap_vm_atom->builtins[CON_BUILTIN_CLOSURE_CLASS] = NEW_ATOM_DEF_SLOT_CLASS_OBJECT(0);
	bootstrap_vm_atom->builtins[CON_BUILTIN_MODULE_CLASS] = NEW_ATOM_DEF_SLOT_CLASS_OBJECT(0);
	bootstrap_vm_atom->builtins[CON_BUILTIN_PARTIAL_APPLICATION_CLASS] = NEW_ATOM_DEF_SLOT_CLASS_OBJECT(0);
	bootstrap_vm_atom->builtins[CON_BUILTIN_EXCEPTION_CLASS] = NEW_ATOM_DEF_SLOT_CLASS_OBJECT(0);
	bootstrap_vm_atom->builtins[CON_BUILTIN_SET_CLASS] = NEW_ATOM_DEF_SLOT_CLASS_OBJECT(0);

#	define NEW_BLANK_OBJECT(b) bootstrap_vm_atom->builtins[b] = Con_Object_new_from_proto(bootstrap_thread, sizeof(Con_Obj) + sizeof(Con_Builtins_Slots_Atom), NULL); \
		Con_Builtins_Slots_Atom_Def_init_atom(bootstrap_thread, (Con_Builtins_Slots_Atom *) bootstrap_vm_atom->builtins[b]->first_atom); \
		bootstrap_vm_atom->builtins[b]->first_atom->next_atom = NULL;

	NEW_BLANK_OBJECT(CON_BUILTIN_NULL_OBJ);
	NEW_BLANK_OBJECT(CON_BUILTIN_FAIL_OBJ);
	Con_Memory_change_chunk_type(bootstrap_thread, bootstrap_vm_atom->builtins[CON_BUILTIN_NULL_OBJ], CON_MEMORY_CHUNK_OBJ);
	Con_Memory_change_chunk_type(bootstrap_thread, bootstrap_vm_atom->builtins[CON_BUILTIN_FAIL_OBJ], CON_MEMORY_CHUNK_OBJ);
	
	Con_Builtins_Class_Atom_bootstrap(bootstrap_thread);
	Con_Builtins_VM_Atom_bootstrap(bootstrap_thread);
	Con_Builtins_Thread_Atom_bootstrap(bootstrap_thread);
	Con_Builtins_Func_Atom_bootstrap(bootstrap_thread);
	Con_Builtins_String_Atom_bootstrap(bootstrap_thread);
	Con_Builtins_Closure_Atom_bootstrap(bootstrap_thread);
	Con_Builtins_Con_Stack_Atom_bootstrap(bootstrap_thread);
	Con_Builtins_List_Atom_bootstrap(bootstrap_thread);
	Con_Builtins_Dict_Atom_bootstrap(bootstrap_thread);
	Con_Builtins_Module_Atom_bootstrap(bootstrap_thread);
	Con_Builtins_Int_Atom_bootstrap(bootstrap_thread);
	Con_Builtins_Partial_Application_Atom_bootstrap(bootstrap_thread);
	Con_Builtins_Exception_Atom_bootstrap(bootstrap_thread);
	Con_Builtins_Set_Atom_bootstrap(bootstrap_thread);

	Con_Builtins_Object_Class_bootstrap(bootstrap_thread);
	Con_Builtins_Class_Class_bootstrap(bootstrap_thread);	
	Con_Builtins_VM_Class_bootstrap(bootstrap_thread);
	Con_Builtins_Thread_Class_bootstrap(bootstrap_thread);
	Con_Builtins_Func_Class_bootstrap(bootstrap_thread);
	Con_Builtins_String_Class_bootstrap(bootstrap_thread);
	Con_Builtins_Con_Stack_Class_bootstrap(bootstrap_thread);
	Con_Builtins_List_Class_bootstrap(bootstrap_thread);
	Con_Builtins_Dict_Class_bootstrap(bootstrap_thread);
	Con_Builtins_Int_Class_bootstrap(bootstrap_thread);
	Con_Builtins_Unique_Atom_Def_bootstrap(bootstrap_thread);
	Con_Builtins_Closure_Class_bootstrap(bootstrap_thread);
	Con_Builtins_Module_Class_bootstrap(bootstrap_thread);
	Con_Builtins_Partial_Application_Class_bootstrap(bootstrap_thread);
	Con_Builtins_Exception_Class_bootstrap(bootstrap_thread);
	Con_Builtins_Set_Class_bootstrap(bootstrap_thread);

	// Create the real VM and root thread objects.

	Con_Obj *vm = Con_Builtins_VM_Atom_new(bootstrap_thread, mem_store, argc, argv);
	Con_Obj *thread = Con_Builtins_Thread_Atom_new_from_self(bootstrap_thread, c_stack_start, vm);

	// The real VM object isn't quite properly filled in at this point: we need to copy over the
	// known builtins to complete the filling in of the VM.
	
	Con_Builtins_VM_Atom *vm_atom = Con_Object_get_atom(bootstrap_thread, vm, bootstrap_vm_atom->builtins[CON_BUILTIN_VM_ATOM_DEF_OBJECT]);
	for (int i = 0; i < CON_NUMBER_OF_BUILTINS; i += 1)
		vm_atom->builtins[i] = bootstrap_vm_atom->builtins[i];

	// Now comes the hairy bit. We have a number of objects churning around in the system at this
	// point. However the majority of them will not have the correct slots associated with them; some
	// will have no slots at all, whilst most will have an incomplete set. What we do here is go
	// through each object and attempt to associate it with a class.
	//
	// In order to do this, objects have to be traversed in a certain order; specifically, classes
	// must have their slots correctly associated before subclasses receive the same treatment.

	for (Con_Int i = 0; i < mem_store->num_chunks; i += 1) {
		Con_Memory_Chunk *chunk = mem_store->chunks[i];
		if (chunk->type == CON_MEMORY_CHUNK_OBJ) {
			Con_Obj *obj = (Con_Obj *) (chunk + 1);

			Con_Builtins_Slots_Atom *slots_atom = NULL;
			if ((slots_atom = Con_Object_find_atom(bootstrap_thread, obj, bootstrap_vm_atom->builtins[CON_BUILTIN_SLOTS_ATOM_DEF_OBJECT])) != NULL)
				slots_atom->proto_slots_for_clones = NULL;
			
			Con_Builtins_Class_Atom *class_atom = NULL;
			if ((class_atom = Con_Object_find_atom(bootstrap_thread, obj, bootstrap_vm_atom->builtins[CON_BUILTIN_CLASS_ATOM_DEF_OBJECT])) != NULL)
				class_atom->class_fields_for_children = NULL;
		}
	}

	for (Con_Int i = 0; i < mem_store->num_chunks; i += 1) {
		Con_Memory_Chunk *chunk = mem_store->chunks[i];
		if (chunk->type == CON_MEMORY_CHUNK_OBJ) {
			Con_Obj *obj = (Con_Obj *) (chunk + 1);
			if ((obj == slots_atom_def_object) || (obj == atom_def_object))
				continue;
			Con_Obj *creator;
			Con_Atom *atom = obj->first_atom;
			while ((atom != NULL) && ((atom->atom_type == slots_atom_def_object) || (atom->atom_type == atom_def_object) || (atom->atom_type == vm_atom->builtins[CON_BUILTIN_UNIQUE_ATOM_DEF_OBJECT]))) {
				atom = atom->next_atom;
			}

			// Now comes a semi-fudge. Because atoms and classes are separated from one
			// another, it is necessary to manually map atom def objects to classes.

			if (atom == NULL)
				creator = vm_atom->builtins[CON_BUILTIN_OBJECT_CLASS];
			else if (atom->atom_type == CON_BUILTIN(CON_BUILTIN_CLASS_ATOM_DEF_OBJECT))
				creator = CON_BUILTIN(CON_BUILTIN_CLASS_CLASS);
			else if (atom->atom_type == CON_BUILTIN(CON_BUILTIN_VM_ATOM_DEF_OBJECT))
				creator = CON_BUILTIN(CON_BUILTIN_VM_CLASS);
			else if (atom->atom_type == CON_BUILTIN(CON_BUILTIN_THREAD_ATOM_DEF_OBJECT))
				creator = CON_BUILTIN(CON_BUILTIN_THREAD_CLASS);
			else if (atom->atom_type == CON_BUILTIN(CON_BUILTIN_FUNC_ATOM_DEF_OBJECT))
				creator = CON_BUILTIN(CON_BUILTIN_FUNC_CLASS);
			else if (atom->atom_type == CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT))
				creator = CON_BUILTIN(CON_BUILTIN_STRING_CLASS);
			else if (atom->atom_type == CON_BUILTIN(CON_BUILTIN_CON_STACK_ATOM_DEF_OBJECT))
				creator = CON_BUILTIN(CON_BUILTIN_CON_STACK_CLASS);
			else if (atom->atom_type == CON_BUILTIN(CON_BUILTIN_LIST_ATOM_DEF_OBJECT))
				creator = CON_BUILTIN(CON_BUILTIN_LIST_CLASS);
			else if (atom->atom_type == CON_BUILTIN(CON_BUILTIN_DICT_ATOM_DEF_OBJECT))
				creator = CON_BUILTIN(CON_BUILTIN_DICT_CLASS);
			else if (atom->atom_type == CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT))
				creator = CON_BUILTIN(CON_BUILTIN_INT_CLASS);
			else if (atom->atom_type == CON_BUILTIN(CON_BUILTIN_EXCEPTION_ATOM_DEF_OBJECT))
				creator = CON_BUILTIN(CON_BUILTIN_EXCEPTION_CLASS);
			else if (atom->atom_type == CON_BUILTIN(CON_BUILTIN_PARTIAL_APPLICATION_ATOM_DEF_OBJECT))
				creator = CON_BUILTIN(CON_BUILTIN_PARTIAL_APPLICATION_CLASS);
			else if (atom->atom_type == CON_BUILTIN(CON_BUILTIN_SET_ATOM_DEF_OBJECT))
				creator = CON_BUILTIN(CON_BUILTIN_SET_CLASS);
			else
				creator = atom->atom_type;
			
			Con_Builtins_Class_Atom *class_atom = Con_Object_get_atom(bootstrap_thread, creator, CON_BUILTIN(CON_BUILTIN_CLASS_ATOM_DEF_OBJECT));

			CON_MUTEX_LOCK(&creator->mutex);
			obj->creator_slots = Con_Builtins_Class_Atom_get_creator_slots(bootstrap_thread, creator);
			CON_MUTEX_UNLOCK(&creator->mutex);
			obj->custom_get_slot = class_atom->custom_get_slot_field;
			obj->custom_set_slot = class_atom->custom_set_slot_field;
			obj->custom_has_slot = class_atom->custom_has_slot_field;
		}
	}
	
	free(bootstrap_vm);
	free(bootstrap_thread);
	
	// Import the "core" modules.

	// As such, none of the File modules are "core", but we need to import them so stderr etc. can
	// be created properly.

#	ifdef CON_PLATFORM_POSIX
	vm_atom->builtins[CON_BUILTIN_C_FILE_MODULE] = Con_Builtins_Module_Atom_import(thread, CON_NEW_STRING("POSIX_File"));
#	else
	XXX;
#	endif

	vm_atom->builtins[CON_BUILTIN_SYS_MODULE] = Con_Builtins_Module_Atom_import(thread, CON_NEW_STRING("Sys"));
	vm_atom->builtins[CON_BUILTIN_EXCEPTIONS_MODULE] = Con_Builtins_Module_Atom_import(thread, CON_NEW_STRING("Exceptions"));
	
	return thread;
}
