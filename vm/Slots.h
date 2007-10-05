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


#ifndef _CON_SLOTS_H
#define _CON_SLOTS_H

// Slot names are always stored as null-terminated UTF-8 strings.

typedef struct {
	Con_Int hash;
	// A full_entry_offset of -1 signifies that this entry is empty.
	Con_Int full_entry_offset;
} Con_Slots_Hash_Entry;

typedef struct {
	Con_Obj *value;
	Con_Int slot_name_size;
	u_char slot_name[0];
} Con_Slots_Full_Entry;

struct Con_Slots {
	Con_Int num_entries;
	Con_Slots_Hash_Entry *hash_entries;
	Con_Int num_hash_entries_allocated;
	u_char *full_entries;
	Con_Int full_entries_size_allocated, full_entries_size;
};

void Con_Slots_init(Con_Obj *, Con_Slots *);
bool Con_Slots_get_slot(Con_Obj *, Con_Slots *, const u_char *, Con_Int, Con_Obj **);
void Con_Slots_set_slot(Con_Obj *, Con_Mutex *, Con_Slots *, const u_char *, Con_Int, Con_Obj *);
bool Con_Slots_read_slot(Con_Obj *, Con_Slots *, Con_Int *, const u_char **, Con_Int *, Con_Obj **);

void Con_Slots_gc_scan_slots(Con_Obj *, Con_Slots *);

#endif
