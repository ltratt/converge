// Copyright (c) 2006 Laurence Tratt
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

#include <sys/param.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef CON_HAVE_SRANDOMDEV
#include <sys/time.h>
#endif

#include <libxml/parser.h>

#include "Core.h"
#include "Memory.h"
#include "Numbers.h"
#include "Object.h"
#include "Shortcuts.h"
#include "Slots.h"

#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Dict/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



Con_Obj *Con_Module_Random_init(Con_Obj *, Con_Obj *);
Con_Obj *Con_Module_Random_import(Con_Obj *, Con_Obj *);

Con_Obj *_Con_Module_Random_pluck_func(Con_Obj *);
Con_Obj *_Con_Module_Random_random_func(Con_Obj *);
Con_Obj *_Con_Module_Random_shuffle_func(Con_Obj *);



Con_Obj *Con_Module_Random_init(Con_Obj *thread, Con_Obj *identifier)
{
	const char* defn_names[] = {"pluck", "random", "shuffle", NULL};

	return Con_Builtins_Module_Atom_new_c(thread, identifier, CON_NEW_STRING("Random"), defn_names, CON_BUILTIN(CON_BUILTIN_NULL_OBJ));
}



Con_Obj *Con_Module_Random_import(Con_Obj *thread, Con_Obj *random_mod)
{
	CON_SET_MOD_DEF(random_mod, "pluck", CON_NEW_UNBOUND_C_FUNC(_Con_Module_Random_pluck_func, "pluck", random_mod));
	CON_SET_MOD_DEF(random_mod, "random", CON_NEW_UNBOUND_C_FUNC(_Con_Module_Random_random_func, "random", random_mod));
	CON_SET_MOD_DEF(random_mod, "shuffle", CON_NEW_UNBOUND_C_FUNC(_Con_Module_Random_shuffle_func, "shuffle", random_mod));

#	ifdef CON_HAVE_SRANDOMDEV
	srandomdev();
#	else
    struct timeval tv;

    gettimeofday(&tv, NULL);
    srandom(tv.tv_sec ^ tv.tv_usec);
#	endif

	return random_mod;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions in Random module
//

//
// 'pluck(c)' returns a random element from 'c'.
//

Con_Obj *_Con_Module_Random_pluck_func(Con_Obj *thread)
{
	Con_Obj *collection;

	CON_UNPACK_ARGS("O", &collection);

	// This algorithm is based on that found in Python's random.sort function.

	Con_Int num_elems = Con_Numbers_Number_to_Con_Int(thread, CON_GET_SLOT_APPLY(collection, "len"));

	if (num_elems == 0)
		CON_RAISE_EXCEPTION("Bounds_Exception", CON_NEW_INT(0), CON_NEW_INT(0));

	Con_Int i = random() % num_elems;

	return CON_GET_SLOT_APPLY(collection, "get", CON_NEW_INT(i));
}



//
// Return a random number between 0 and the maximum size of an integer.
//

Con_Obj *_Con_Module_Random_random_func(Con_Obj *thread)
{
	CON_UNPACK_ARGS("");

	return CON_NEW_INT(random());
}



//
// 'shuffle(c)' sorts 'c' into a semi-random order.
//

Con_Obj *_Con_Module_Random_shuffle_func(Con_Obj *thread)
{
	Con_Obj *collection;
	
	CON_UNPACK_ARGS("O", &collection);

	// This algorithm is based on that found in Python's random.sort function.

	Con_Int num_elems = Con_Numbers_Number_to_Con_Int(thread, CON_GET_SLOT_APPLY(collection, "len"));
	for (Con_Int i = num_elems - 1; i > 0; i -= 1) {
		Con_Int j = random() % (i + 1);
		Con_Obj *ith = CON_GET_SLOT_APPLY(collection, "get", CON_NEW_INT(i));
		Con_Obj *jth = CON_GET_SLOT_APPLY(collection, "get", CON_NEW_INT(j));
		CON_GET_SLOT_APPLY(collection, "set", CON_NEW_INT(i), jth);
		CON_GET_SLOT_APPLY(collection, "set", CON_NEW_INT(j), ith);
	}
	
	return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}
