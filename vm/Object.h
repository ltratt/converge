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


#ifndef _CON_OBJECT_H
#define _CON_OBJECT_H

#include "Core.h"

Con_Obj *Con_Object_new_blank(Con_Obj *, Con_Int, Con_Obj *);

void *Con_Object_find_atom(Con_Obj *, Con_Obj *, Con_Obj *);
void *Con_Object_get_atom(Con_Obj *, Con_Obj *, Con_Obj *);

Con_Obj *Con_Object_new_from_class(Con_Obj *, Con_Int, Con_Obj *);
Con_Obj *Con_Object_new_from_proto(Con_Obj *, Con_Int, Con_Obj *);

Con_Obj *Con_Object_get_slot(Con_Obj *, Con_Obj *, Con_Obj *, const u_char *, Con_Int);
Con_Obj *Con_Object_get_slot_no_binding(Con_Obj *, Con_Obj *, Con_Obj *, const u_char *, Con_Int, bool *);
Con_Obj *Con_Object_get_slot_no_custom(Con_Obj *, Con_Obj *, const u_char *, Con_Int);
Con_Obj *Con_Object_get_slots(Con_Obj *, Con_Obj *);
Con_Obj *Con_Object_find_slot_no_custom(Con_Obj *, Con_Obj *, const u_char *, Con_Int);
void Con_Object_set_slot(Con_Obj *, Con_Obj *, Con_Obj *, const u_char *, Con_Int, Con_Obj *);

Con_Obj *Con_Object_add(Con_Obj *, Con_Obj *, Con_Obj *);
Con_Obj *Con_Object_subtract(Con_Obj *, Con_Obj *, Con_Obj *);
bool Con_Object_eq(Con_Obj *, Con_Obj *, Con_Obj *);
bool Con_Object_neq(Con_Obj *, Con_Obj *, Con_Obj *);
bool Con_Object_gt(Con_Obj *, Con_Obj *, Con_Obj *);
bool Con_Object_le(Con_Obj *, Con_Obj *, Con_Obj *);
bool Con_Object_le_eq(Con_Obj *, Con_Obj *, Con_Obj *);
bool Con_Object_gr_eq(Con_Obj *, Con_Obj *, Con_Obj *);

#endif
