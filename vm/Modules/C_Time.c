// Copyright (c) 2007 Laurence Tratt
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

#include <sys/time.h>

#include "Core.h"
#include "Numbers.h"
#include "Object.h"
#include "Shortcuts.h"
#include "Slots.h"

#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



Con_Obj *Con_Module_C_Time_init(Con_Obj *, Con_Obj *);
Con_Obj *Con_Module_C_Time_import(Con_Obj *, Con_Obj *);

Con_Obj *_Con_Module_C_Time_current_func(Con_Obj *);



Con_Obj *Con_Module_C_Time_init(Con_Obj *thread, Con_Obj *identifier)
{
	const char* defn_names[] = {"current", "current_mono", NULL};

	return Con_Builtins_Module_Atom_new_c(thread, identifier, CON_NEW_STRING("C_Time"), defn_names, CON_BUILTIN(CON_BUILTIN_NULL_OBJ));
}



Con_Obj *Con_Module_C_Time_import(Con_Obj *thread, Con_Obj *c_time_mod)
{
	CON_SET_MOD_DEFN(c_time_mod, "current", CON_NEW_UNBOUND_C_FUNC(_Con_Module_C_Time_current_func, "current", c_time_mod));
	CON_SET_MOD_DEFN(c_time_mod, "current_mono", CON_GET_MOD_DEFN(c_time_mod, "current"));
	
	return c_time_mod;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions in C_Time module
//

//
// Returns the current time of day as a pair (sec, nsec).
//

Con_Obj *_Con_Module_C_Time_current_func(Con_Obj *thread)
{
	CON_UNPACK_ARGS("");

	struct timeval tp;
	
	gettimeofday(&tp, NULL);
	
	Con_Obj *sec = CON_NEW_INT(tp.tv_sec);
	Con_Obj *usec = CON_NEW_INT(tp.tv_usec);
	
	return Con_Builtins_List_Atom_new_va(thread, sec, usec, NULL);
}
