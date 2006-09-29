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
#include "Object.h"
#include "Shortcuts.h"

#include "Builtins/Module/Atom.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



Con_Obj *Con_Modules_Builtins_init(Con_Obj *thread, Con_Obj *);



Con_Obj *Con_Modules_Builtins_init(Con_Obj *thread, Con_Obj *identifier)
{
	Con_Obj *builtins_mod = Con_Builtins_Module_Atom_new_c(thread, identifier, CON_NEW_STRING("Builtins"), CON_BUILTIN(CON_BUILTIN_NULL_OBJ));

	CON_SET_SLOT(builtins_mod, "Object", CON_BUILTIN(CON_BUILTIN_OBJECT_CLASS));
	CON_SET_SLOT(builtins_mod, "Class", CON_BUILTIN(CON_BUILTIN_CLASS_CLASS));
	CON_SET_SLOT(builtins_mod, "VM", CON_BUILTIN(CON_BUILTIN_VM_CLASS));
	CON_SET_SLOT(builtins_mod, "Thread", CON_BUILTIN(CON_BUILTIN_THREAD_CLASS));
	CON_SET_SLOT(builtins_mod, "Func", CON_BUILTIN(CON_BUILTIN_FUNC_CLASS));
	CON_SET_SLOT(builtins_mod, "String", CON_BUILTIN(CON_BUILTIN_STRING_CLASS));
	CON_SET_SLOT(builtins_mod, "Con_Stack", CON_BUILTIN(CON_BUILTIN_CON_STACK_CLASS));
	CON_SET_SLOT(builtins_mod, "List", CON_BUILTIN(CON_BUILTIN_LIST_CLASS));
	CON_SET_SLOT(builtins_mod, "Dict", CON_BUILTIN(CON_BUILTIN_DICT_CLASS));
	CON_SET_SLOT(builtins_mod, "Module", CON_BUILTIN(CON_BUILTIN_MODULE_CLASS));
	CON_SET_SLOT(builtins_mod, "Int", CON_BUILTIN(CON_BUILTIN_INT_CLASS));
	CON_SET_SLOT(builtins_mod, "Closure", CON_BUILTIN(CON_BUILTIN_CLOSURE_CLASS));
	CON_SET_SLOT(builtins_mod, "Partial_Application", CON_BUILTIN(CON_BUILTIN_PARTIAL_APPLICATION_CLASS));
	CON_SET_SLOT(builtins_mod, "Exception", CON_BUILTIN(CON_BUILTIN_EXCEPTION_CLASS));
	
	return builtins_mod;
}
