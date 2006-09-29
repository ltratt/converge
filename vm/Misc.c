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
#include "Misc.h"
#include "Numbers.h"
#include "Object.h"
#include "Shortcuts.h"
#include "Slots.h"

#include "Builtins/Exception/Atom.h"
#include "Builtins/Int/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



//
// Cleanup (and possibly) convert 'index' as an index into a sized object of 'upper_bound' elements.
// If 'index' is a -ve value, it will be converted into a +ve value. If it equals or exceeds
// 'upper_bound' an exception is raised.
//
// This function may be called with 'mutex' held which will only be unlocked if an exception is
// raised. If this function returns a value, it guarantees not to have unlocked 'mutex' at any point.
// 'mutex' may be NULL.
//

Con_Int Con_Misc_translate_index(Con_Obj *thread, Con_Mutex *mutex, Con_Int index, Con_Int upper_bound)
{
	Con_Int new_index;
	if (index < 0)
		new_index = upper_bound + index;
	else
		new_index = index;
	
	if ((new_index < 0) || (new_index >= upper_bound)) {
		if (mutex != NULL)
			CON_MUTEX_UNLOCK(mutex);
		CON_RAISE_EXCEPTION("Bounds_Exception", CON_NEW_INT(index), CON_NEW_INT(upper_bound));
	}
	
	return new_index;
}



//
// Cleanup (and possibly) convert 'index' as a slice index into a sized object of 'upper_bound'
// elements. If 'index' is a -ve value, it will be converted into a +ve value. If it exceeds
// 'upper_bound' an exception is raised. This function differs from Con_Misc_translate_index in that
// it is acceptable for a slice index to equal (but note not to exceed) 'upper_bound'.
//
// This function may be called with 'mutex' held which will only be unlocked if an exception is
// raised. If this function returns a value, it guarantees not to have unlocked 'mutex' at any point.
// 'mutex' may be NULL.
//

Con_Int Con_Misc_translate_slice_index(Con_Obj *thread, Con_Mutex *mutex, Con_Int index, Con_Int upper_bound)
{
	Con_Int new_index;
	if (index < 0)
		new_index = upper_bound + index;
	else
		new_index = index;
	
	if ((new_index < 0) || (new_index > upper_bound)) {
		if (mutex != NULL)
			CON_MUTEX_UNLOCK(mutex);
		CON_RAISE_EXCEPTION("Bounds_Exception", CON_NEW_INT(index), CON_NEW_INT(upper_bound));
	}
	
	return new_index;
}



//
// Cleanup (and possibly) convert slice indexes. This function is intended to be used by 'get_slice'
// methods in the following way:
//
//  Con_Obj *lower_obj, *self, *upper_obj;
//  CON_UNPACK_ARGS("S;OO", &self, &lower_obj, &upper_obj);
//  
//  Con_Whatever_Atom *whatever_atom = CON_GET_ATOM(self, ...);
//  
//  Con_Int lower, upper;
//  Con_Misc_translate_slice_indices(thread, NULL, lower_obj, upper_obj, &lower, &upper,
//    whatever_atom->size);
//
// Note that lower_obj and upper_obj should be declared as being optional arguments of type object;
// this function handles dealing with the expected NULL (C) / null (Converge) values, checking
// bounds, and whether the lower value is <= upper value.
//
// This function may be called with 'mutex' held which will only be unlocked if an exception is
// raised. If this function returns, it guarantees not to have unlocked 'mutex' at any point.
// 'mutex' may be NULL.
//

void Con_Misc_translate_slice_indices(Con_Obj *thread, Con_Mutex *mutex, Con_Obj *lower_obj, Con_Obj *upper_obj, Con_Int *lower_index, Con_Int *upper_index, Con_Int upper_bound)
{
	if ((lower_obj == NULL) || (lower_obj == CON_BUILTIN(CON_BUILTIN_NULL_OBJ)))
		*lower_index = Con_Misc_translate_slice_index(thread, mutex, 0, upper_bound);
	else
		*lower_index = Con_Misc_translate_slice_index(thread, mutex, Con_Numbers_Number_to_Con_Int(thread, lower_obj), upper_bound);

	if ((upper_obj == NULL) || (upper_obj == CON_BUILTIN(CON_BUILTIN_NULL_OBJ)))
		*upper_index = Con_Misc_translate_slice_index(thread, mutex, upper_bound, upper_bound);
	else
		*upper_index = Con_Misc_translate_slice_index(thread, mutex, Con_Numbers_Number_to_Con_Int(thread, upper_obj), upper_bound);
	
	if (*upper_index < *lower_index) {
		if (mutex != NULL)
			CON_MUTEX_UNLOCK(mutex);
		CON_RAISE_EXCEPTION("Indices_Exception", CON_NEW_INT(*lower_index), CON_NEW_INT(*upper_index));
	}
}
