// Copyright (c) 2007 Laurence Tratt <laurie@tratt.net>
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
#include <string.h>
#include <unistd.h>

#ifdef CON_PLATFORM_MINGW
#	include <winsock2.h>
#endif

#include "Core.h"
#include "Memory.h"
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



Con_Obj *Con_Module_C_Platform_Host_init(Con_Obj *, Con_Obj *);
Con_Obj *Con_Module_C_Platform_Host_import(Con_Obj *, Con_Obj *);

Con_Obj *_Con_Module_C_Platform_Host_get_hostname(Con_Obj *);



Con_Obj *Con_Module_C_Platform_Host_init(Con_Obj *thread, Con_Obj *identifier)
{
	const char* defn_names[] = {"get_hostname", NULL};

	return Con_Builtins_Module_Atom_new_c(thread, identifier, CON_NEW_STRING("C_Platform_Host"), defn_names, CON_BUILTIN(CON_BUILTIN_NULL_OBJ));
}



Con_Obj *Con_Module_C_Platform_Host_import(Con_Obj *thread, Con_Obj *mod)
{
	CON_SET_MOD_DEFN(mod, "get_hostname", CON_NEW_UNBOUND_C_FUNC(_Con_Module_C_Platform_Host_get_hostname, "get_hostname", mod));

	return mod;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions in Host module
//

//
// 'get_hostname(name)' returns the machines hostname.
//

Con_Obj *_Con_Module_C_Platform_Host_get_hostname(Con_Obj *thread)
{
	CON_UNPACK_ARGS("");

#	ifndef MAXHOSTNAMELEN
#	define MAXHOSTNAMELEN 256
#	endif

	char hostname[MAXHOSTNAMELEN];
	if (gethostname(hostname, MAXHOSTNAMELEN) != 0)
		CON_XXX;
	
	return Con_Builtins_String_Atom_new_copy(thread, (u_char *) hostname, strlen(hostname), CON_STR_UTF_8);
}
