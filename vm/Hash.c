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
#include "Hash.h"
#include "Numbers.h"
#include "Shortcuts.h"

#include "Builtins/Int/Atom.h"
#include "Builtins/String/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



Con_Int Con_Hash_get(Con_Obj *thread, Con_Obj *obj)
{
	CON_MUTEX_LOCK(&obj->mutex);
	if (obj->virgin && obj->first_atom->atom_type == CON_BUILTIN(CON_BUILTIN_STRING_ATOM_DEF_OBJECT)) {
		CON_MUTEX_UNLOCK(&obj->mutex);
		return ((Con_Builtins_String_Atom *) obj->first_atom)->hash;
	}
	else if (obj->virgin && obj->first_atom->atom_type == CON_BUILTIN(CON_BUILTIN_INT_ATOM_DEF_OBJECT)) {
		CON_MUTEX_UNLOCK(&obj->mutex);
		return Con_Hash_calc_con_int_hash(thread, ((Con_Builtins_Int_Atom *) obj->first_atom)->val);
	}
	else {
		CON_MUTEX_UNLOCK(&obj->mutex);
		return Con_Numbers_Number_to_Con_Int(thread, CON_GET_SLOT_APPLY(obj, "hash"));
	}
}



Con_Int Con_Hash_calc_string_hash(Con_Obj *thread, const char *str, int str_size)
{
	Con_Hash hash = 5381;
	int i;

	for (i = 0; i < str_size; i += 1)
		hash = ((hash << 5) + hash) + str[i]; /* hash * 33 + c */

	return hash;
}



Con_Int Con_Hash_calc_con_int_hash(Con_Obj *thread, Con_Int val)
{
#	if SIZEOF_CON_INT == 4
	// This is based on Robert Jenkin's and Thomas Wang's 32-bit hash function, reported at:
	//   http://www.concentric.net/~Ttwang/tech/inthash.htm

	val += val << 12;
	val ^= val >> 22;
	val += val << 4;
	val ^= val >> 9;
	val += val << 10;
	val ^= val >> 2;
	val += val << 7;
	val ^= val >> 12;

	return val;
#	elif SIZEOF_CON_INT == 8
	// This is based on Thomas Wang's 64-bit hash function, reported at:
	//   http://www.concentric.net/~Ttwang/tech/inthash.htm

	uint64_t key = val;

	key = (~key) + (key << 21);
	key = key ^ (key >> 24);
	key = (key + (key << 3)) + (key << 8);
	key = key ^ (key >> 14);
	key = (key + (key << 2)) + (key << 4);
	key = key ^ (key >> 28);
	key = key + (key << 31);

	return key;
#	else
	CON_XXX;
#	endif
}
