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

#include "Core.h"
#include "Shortcuts.h"

#include "Builtins/Atom_Def_Atom.h"
#include "Builtins/Slots_Atom_Def.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



void _Con_Builtins_Slots_Atom_Def_gc_scan_func(Con_Obj *, Con_Obj *, Con_Atom *);



////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap
//

void Con_Builtins_Slots_Atom_Def_bootstrap(Con_Obj *thread)
{
	Con_Obj *slots_atom_def_object = CON_BUILTIN(CON_BUILTIN_SLOTS_ATOM_DEF_OBJECT);

#	ifndef CON_THREADS_SINGLE_THREADED
	Con_Memory_mutex_init(thread, &slots_atom_def_object->mutex);
#	endif
	slots_atom_def_object->size = sizeof(Con_Obj) + sizeof(Con_Builtins_Atom_Def_Atom) + sizeof(Con_Builtins_Slots_Atom);
	slots_atom_def_object->creator_slots = NULL;
	slots_atom_def_object->custom_get_slot = 0;
	slots_atom_def_object->custom_set_slot = 0;
	slots_atom_def_object->custom_has_slot = 0;

	Con_Builtins_Atom_Def_Atom *atom_def_atom = (Con_Builtins_Atom_Def_Atom *) slots_atom_def_object->first_atom;
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) (atom_def_atom + 1);
	atom_def_atom->next_atom = (Con_Atom *) slots_atom;
	slots_atom->next_atom = NULL;

	Con_Builtins_Atom_Def_Atom_init_atom(thread, atom_def_atom, _Con_Builtins_Slots_Atom_Def_gc_scan_func, NULL);

	slots_atom->atom_type = slots_atom_def_object;
	slots_atom->proto_slots_for_clones = NULL;
	slots_atom->slots.num_entries = 0;
	slots_atom->slots.num_hash_entries_allocated = 0;
	slots_atom->slots.hash_entries = NULL;
	slots_atom->slots.full_entries_size = slots_atom->slots.full_entries_size_allocated = 0;
	slots_atom->slots.full_entries = NULL;
	
	Con_Memory_change_chunk_type(thread, slots_atom_def_object, CON_MEMORY_CHUNK_OBJ);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Slots atom functions
//

void Con_Builtins_Slots_Atom_Def_init_atom(Con_Obj *thread, Con_Builtins_Slots_Atom *slots_atom)
{
	slots_atom->atom_type = CON_BUILTIN(CON_BUILTIN_SLOTS_ATOM_DEF_OBJECT);
	slots_atom->proto_slots_for_clones = NULL;
	Con_Slots_init(thread, &slots_atom->slots);
}



Con_Slots *Con_Builtins_Slots_Atom_Def_get_proto_slots(Con_Obj *thread, Con_Obj *proto_obj)
{
	CON_XXX;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Garbage collection
//

void _Con_Builtins_Slots_Atom_Def_gc_scan_func(Con_Obj *thread, Con_Obj *obj, Con_Atom *atom)
{
	Con_Builtins_Slots_Atom *slots_atom = (Con_Builtins_Slots_Atom *) atom;
	
	Con_Slots_gc_scan_slots(thread, &slots_atom->slots);
	if (slots_atom->proto_slots_for_clones != NULL)
		Con_Memory_gc_push(thread, slots_atom->proto_slots_for_clones);
}
