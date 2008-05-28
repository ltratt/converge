// Copyright (c) 2008 Laurence Tratt <laurie@tratt.net?
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

#include <curses.h>
#include <string.h>
#include <term.h>
#include <unistd.h>

#include "Core.h"
#include "Misc.h"
#include "Numbers.h"
#include "Object.h"
#include "Shortcuts.h"

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




Con_Obj *Con_Module_Curses_init(Con_Obj *thread, Con_Obj *);
Con_Obj *Con_Module_Curses_import(Con_Obj *thread, Con_Obj *);

Con_Obj *_Con_Module_Curses_setupterm_func(Con_Obj *);
Con_Obj *_Con_Module_Curses_tigetstr_func(Con_Obj *);
Con_Obj *_Con_Module_Curses_addstr_func(Con_Obj *);



Con_Obj *Con_Module_Curses_init(Con_Obj *thread, Con_Obj *identifier)
{
	const char* defn_names[] = {"Curses_Exception", "setupterm", "tigetstr", NULL};

	return Con_Builtins_Module_Atom_new_c(thread, identifier, CON_NEW_STRING("Curses"),
      defn_names, CON_BUILTIN(CON_BUILTIN_NULL_OBJ));
}



Con_Obj *Con_Module_Curses_import(Con_Obj *thread, Con_Obj *curses_mod)
{
	Con_Obj *user_exception = CON_GET_MOD_DEFN(CON_BUILTIN(CON_BUILTIN_EXCEPTIONS_MODULE),
      "User_Exception");
	Con_Obj *curses_exception = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_CLASS_CLASS), "new",
      CON_NEW_STRING("Curses_Exception"),
      Con_Builtins_List_Atom_new_va(thread, user_exception, NULL), curses_mod);
	CON_SET_MOD_DEFN(curses_mod, "Curses_Exception", curses_exception);

	// setupterm
	
    Con_Obj *setupterm_func = CON_NEW_UNBOUND_C_FUNC(_Con_Module_Curses_setupterm_func,
      "setupterm", curses_mod);
	CON_SET_MOD_DEFN(curses_mod, "setupterm", setupterm_func);

    // tigetstr

    Con_Obj *tigetstr_func = CON_NEW_UNBOUND_C_FUNC(_Con_Module_Curses_tigetstr_func,
      "tigetstr", curses_mod);
	CON_SET_MOD_DEFN(curses_mod, "tigetstr", tigetstr_func);

	return curses_mod;
}



//
// setupterm(term := null, stream := sys.stdout)
//

Con_Obj *_Con_Module_Curses_setupterm_func(Con_Obj *thread)
{
    Con_Obj *curses_mod = Con_Builtins_VM_Atom_get_functions_module(thread);

	Con_Obj *term_obj, *file_obj;
	CON_UNPACK_ARGS(";sO", &term_obj, &file_obj);

    char *term = NULL;
    if (!(term_obj == NULL || term_obj == CON_BUILTIN(CON_BUILTIN_NULL_OBJ)))
        CON_XXX;

    int filedes = STDOUT_FILENO;
    if (file_obj != NULL)
        filedes = Con_Numbers_Number_to_c_Int(thread, CON_GET_SLOT_APPLY(file_obj, "fileno"));

    int erret;
    if (setupterm(term, filedes, &erret) != OK) {
        Con_Obj *msg = NULL;
        switch (erret) {
            case 1:
                msg = CON_NEW_STRING("Terminal is hardcopy.");
                break;
            case 0:
                msg = CON_NEW_STRING("Terminal not found or not enough information known about it.");
                break;
            case -1:
                msg = CON_NEW_STRING("Can't find terminfo database.");
                break;
            default:
                CON_XXX;
        }
        
        Con_Obj *curses_exception = CON_GET_MOD_DEFN(curses_mod, "Curses_Exception");
    	Con_Obj *exception = CON_GET_SLOT_APPLY(curses_exception, "new", msg);
    	Con_Builtins_VM_Atom_raise(thread, exception);
    }

    return CON_BUILTIN(CON_BUILTIN_NULL_OBJ);
}



//
// tigetstr
//

Con_Obj *_Con_Module_Curses_tigetstr_func(Con_Obj *thread)
{
    Con_Obj *curses_mod = Con_Builtins_VM_Atom_get_functions_module(thread);

	Con_Obj *capname_obj;
	CON_UNPACK_ARGS("S", &capname_obj);

    char *capname = Con_Builtins_String_Atom_to_c_string(thread, capname_obj);
    
    char *rtn = tigetstr(capname);
    
    if (rtn == (char *) 0 || rtn == (char *) -1) {
        Con_Obj *curses_exception = CON_GET_MOD_DEFN(curses_mod, "Curses_Exception");
        Con_Obj *msg = CON_NEW_STRING("'");
        msg = CON_ADD(msg, capname_obj);
        msg = CON_ADD(msg, CON_NEW_STRING("' not found or absent."));
    	Con_Obj *exception = CON_GET_SLOT_APPLY(curses_exception, "new", msg);
    	Con_Builtins_VM_Atom_raise(thread, exception);
    }

    return Con_Builtins_String_Atom_new_copy(thread, (u_char *) rtn, strlen(rtn), CON_STR_UTF_8);
}
