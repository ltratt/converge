// Copyright (c) 2007 King's College London, created by Laurence Tratt
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

#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Func/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/List/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



Con_Obj *Con_Module_C_Platform_Exec_init(Con_Obj *, Con_Obj *);
Con_Obj *Con_Module_C_Platform_Exec_import(Con_Obj *, Con_Obj *);

Con_Obj *_Con_Module_C_Platform_Exec_command(Con_Obj *);



Con_Obj *Con_Module_C_Platform_Exec_init(Con_Obj *thread, Con_Obj *identifier)
{
	const char* defn_names[] = {"command", NULL};

	return Con_Builtins_Module_Atom_new_c(thread, identifier, CON_NEW_STRING("C_Platform_Exec"), defn_names, CON_BUILTIN(CON_BUILTIN_NULL_OBJ));
}



Con_Obj *Con_Module_C_Platform_Exec_import(Con_Obj *thread, Con_Obj *env_mod)
{
	CON_SET_MOD_DEF(env_mod, "command", CON_NEW_UNBOUND_C_FUNC(_Con_Module_C_Platform_Exec_command, "command", env_mod));

	return env_mod;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions in Env module
//

//
// 'command(cmd)' passes 'cmd' to the shell for execution, returning the exit code.
//

Con_Obj *_Con_Module_C_Platform_Exec_command(Con_Obj *thread)
{
	Con_Obj *cmd;
	CON_UNPACK_ARGS("S", &cmd);
	
	return CON_NEW_INT(system(Con_Builtins_String_Atom_to_c_string(thread, cmd)));
}
