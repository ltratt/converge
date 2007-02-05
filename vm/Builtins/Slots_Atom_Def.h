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


#ifndef _CON_BUILTINS_SLOTS_ATOM_DEF_H
#define _CON_BUILTINS_SLOTS_ATOM_DEF_H

#include "Core.h"
#include "Slots.h"

typedef struct {
	CON_ATOM_HEAD
	
	Con_Slots slots;
	// proto_slots_for_clones starts as NULL; if the object this atom is part of is cloned, then
	// it points to a Con_Slots chunk of memory so that it can be reused by multiple objects,
	// saving on memory and the time necessary to setup the memory. If subsequently the slots of
	// the object this atom is a part of change, then proto_slots_for_clones is set to NULL,
	// which will force it to be recalculated the next time the object is cloned. This means that
	// the slots pointed to by proto_slots_for_clones are immutable, and no mutex needs to be held
	// to access them.
	Con_Slots *proto_slots_for_clones;
} Con_Builtins_Slots_Atom;


void Con_Builtins_Slots_Atom_Def_bootstrap(Con_Obj *);

void Con_Builtins_Slots_Atom_Def_init_atom(Con_Obj *, Con_Builtins_Slots_Atom *);

Con_Slots *Con_Builtins_Slots_Atom_Def_get_proto_slots(Con_Obj *, Con_Obj *);

#endif
