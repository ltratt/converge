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


#ifndef _CON_MEMORY_H
#define _CON_MEMORY_H

#include <stdlib.h>
#include <time.h>

#include "Core.h"



/////////////////////////////////////////////////////////////////////////////////////////////////////
// 
//

typedef enum {CON_MEMORY_CHUNK_OPAQUE, CON_MEMORY_CHUNK_CONSERVATIVE, CON_MEMORY_CHUNK_OBJ, CON_MEMORY_CHUNK_SLOTS} Con_Memory_Chunk_Type;

typedef struct {
	// The way the mark and sweep works is to mark each chunk as being deleted and, as that chunk
	// is visited, to unmark it for deletion.
	//
	// gc_to_be_deleted therefore is therefore initialized to 1, and is set back to that value at
	// the end of each GC round. If it is set to 0 during the mark and sweep, the chunk is not
	// deleted.
	//
	// The gc_incomplete_visit boolean is set to true on a chunk when the garbage collection stack is
	// full and cannot be resized. It means that the garbage collector can, when necessary, run in
	// constant space (albeit much less efficiently) since one can continually iterate over every
	// chunk in the system, scanning those which have gc_incomplete_visit set to 1.
	//
	// Both of these attributes have to be set to 1 and 0 respectively when a chunk is created,
	// and must be reset to these values at the end of each GC round.
	unsigned int
		gc_to_be_deleted : 1, gc_incomplete_visit : 1;
	Con_Memory_Chunk_Type type;
	size_t size;
} Con_Memory_Chunk;

typedef struct {
	Con_Mutex mutex;
	Con_Int num_users;
} Con_Memory_Counting_Mutex;

struct Con_Memory_Store {
	Con_Mutex mem_mutex;
	
	u_char *known_low_addr;
	u_char *known_high_addr;
	
	Con_Memory_Chunk **chunks;
	Con_Int num_chunks_allocated, num_chunks;
	
	Con_Memory_Counting_Mutex *counting_mutexes;
	Con_Int num_counting_mutexes;
	
	Con_Int num_allocations_since_last_gc;
	clock_t time_of_last_gc;
	
	// Garbage collection stack
	
	Con_Memory_Chunk **gc_stack;
	Con_Int gc_stack_num_chunks_allocated, gc_stack_num_chunks;
	bool gc_stack_overflow;
	
	// waiting_to_gc: signifies to threads that they should put themselves in a suitable state to
	// be garbage collected. They should wait on gc_condition.
	
	bool waiting_to_gc;
	Con_Thread_Condition gc_condition;
};



/////////////////////////////////////////////////////////////////////////////////////////////////////
// Memory subsystem
//

Con_Memory_Store *Con_Memory_init(void);



/////////////////////////////////////////////////////////////////////////////////////////////////////
// Blocking malloc interface
//

void *Con_Memory_malloc(Con_Obj *, size_t, Con_Memory_Chunk_Type);
void *Con_Memory_realloc(Con_Obj *, void *, size_t);
void Con_Memory_free(Con_Obj *, void *);



/////////////////////////////////////////////////////////////////////////////////////////////////////
// Non-blocking malloc interface
//

void *Con_Memory_malloc_no_gc(Con_Obj *, size_t, Con_Memory_Chunk_Type);
void *Con_Memory_realloc_no_gc(Con_Obj *, void *, size_t);



/////////////////////////////////////////////////////////////////////////////////////////////////////
// Misc
//

void Con_Memory_mutex_init(Con_Obj *, Con_Mutex *);
void Con_Memory_change_chunk_type(Con_Obj *, void *, Con_Memory_Chunk_Type);
void Con_Memory_make_array_room(Con_Obj *, void **, Con_Mutex *, Con_Int *, Con_Int *, Con_Int, size_t);



/////////////////////////////////////////////////////////////////////////////////////////////////////
// Garbage collection
//

void Con_Memory_gc_poll(Con_Obj *);
void Con_Memory_gc_force(Con_Obj *);
void Con_Memory_gc_scan_conservative(Con_Obj *, void *, size_t);
void Con_Memory_gc_scan_pc(Con_Obj *, Con_PC);
void Con_Memory_gc_push(Con_Obj *, void *);
void Con_Memory_gc_push_ptr(Con_Obj *, const void *);

#endif

