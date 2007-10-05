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


#ifndef _CON_MODULES_H
#define _CON_MODULES_H

typedef Con_Obj* _Con_Modules_get_init_t(Con_Obj *, Con_Obj *);
typedef Con_Obj* _Con_Modules_get_import_t(Con_Obj *, Con_Obj *);

typedef struct {
	const u_char *mod_name;
	_Con_Modules_get_init_t *init_func;
	_Con_Modules_get_import_t *import_func;
} Con_Modules_Spec;

Con_Obj *Con_Modules_find(Con_Obj *, Con_Obj *);
Con_Obj *Con_Modules_get(Con_Obj *, Con_Obj *);
Con_Obj *Con_Modules_import_mod_from_bytecode(Con_Obj *, Con_Obj *, Con_Int);
Con_Obj *Con_Modules_import(Con_Obj *, Con_Obj *);

#endif
