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

#include <errno.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "Core.h"
#include "Memory.h"
#include "Object.h"
#include "Shortcuts.h"
#include "Slots.h"

#include "Builtins/Atom_Def_Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



bool _Con_Memory_make_array_room_no_lock(Con_Obj *, void **, Con_Int *, Con_Int *, Con_Int, size_t);

int _Con_Memory_gc_chunks_sorter(const void*, const void*);
void _Con_Memory_gc_push_chunk(Con_Obj *, Con_Memory_Chunk *);
void _Con_Memory_gc_scan_obj(Con_Obj *, Con_Obj *);



Con_Memory_Store *Con_Memory_init()
{
	// Initialize the memory store.

	Con_Memory_Store* mem_store = malloc(sizeof(Con_Memory_Store));
	if (mem_store == NULL)
		CON_XXX;
	
	mem_store->num_chunks_allocated = CON_DEFAULT_NUM_OBJECTS_TO_TRACK;
	mem_store->num_chunks = 0;
	mem_store->chunks = malloc(sizeof(Con_Memory_Chunk *) * mem_store->num_chunks_allocated);
	if (mem_store->chunks == NULL)
		return NULL;


	mem_store->gc_stack_num_chunks_allocated = CON_DEFAULT_GC_STACK_NUM_ENTRIES;
	mem_store->gc_stack_num_chunks = 0;
	mem_store->gc_stack = malloc(sizeof(Con_Memory_Counting_Mutex *) * mem_store->gc_stack_num_chunks_allocated);
	if (mem_store->gc_stack == NULL) {
		free(mem_store->chunks);
		free(mem_store);
		return NULL;
	}
	
	mem_store->num_allocations_since_last_gc = 0;
	if ((mem_store->time_of_last_gc = clock()) == (clock_t) -1)
		CON_XXX;
	
#ifndef CON_THREADS_SINGLE_THREADED
	// The mem_store has a separate mutex from everything else to avoid deadlock situations.
	
	Con_Builtins_Thread_Atom_mutex_init(NULL, &mem_store->mem_mutex);

	Con_Builtins_Thread_Atom_condition_init(NULL, &mem_store->gc_condition);
#endif

	mem_store->waiting_to_gc = false;
	
#ifndef CON_THREADS_SINGLE_THREADED
	mem_store->num_counting_mutexes = CON_DEFAULT_NUM_COUNTING_MUTEXES;
	mem_store->counting_mutexes = malloc(sizeof(Con_Memory_Counting_Mutex) * mem_store->num_counting_mutexes);
	if (mem_store->counting_mutexes == NULL) {
		free(mem_store->gc_stack);
		free(mem_store->chunks);
		free(mem_store);
		return NULL;
	}

	// Initialize counting mutexes.
	
	for (Con_Int i = 0; i < mem_store->num_counting_mutexes; i += 1) {
		Con_Builtins_Thread_Atom_mutex_init(NULL, &mem_store->counting_mutexes[i].mutex);
		mem_store->counting_mutexes[i].num_users = 0;
	}
#endif
		
	return mem_store;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// malloc type functions
//

void *Con_Memory_malloc(Con_Obj *thread, size_t size, Con_Memory_Chunk_Type type)
{
	assert(type != CON_MEMORY_CHUNK_OBJ);

	Con_Memory_gc_poll(thread);

	Con_Memory_Store *mem_store = Con_Builtins_VM_Atom_get_mem_store(thread);

	Con_Memory_Chunk *chunk = malloc(sizeof(Con_Memory_Chunk) + size);
	
	if (chunk == NULL) {
		Con_Memory_gc_force(thread);
		if ((chunk = malloc(sizeof(Con_Memory_Chunk) + size)) == NULL)
			CON_FATAL_ERROR("Unable to allocate memory.");
	}
	
	if (type == CON_MEMORY_CHUNK_CONSERVATIVE)
		bzero(chunk + 1, size);

	chunk->size = size;
	chunk->type = type;
	chunk->gc_to_be_deleted = 1;
	chunk->gc_incomplete_visit = 0;

	CON_MUTEX_LOCK(&mem_store->mem_mutex);
	if (!_Con_Memory_make_array_room_no_lock(thread, (void **) &mem_store->chunks, &mem_store->num_chunks_allocated, &mem_store->num_chunks, 1, sizeof(Con_Memory_Chunk *)))
		CON_FATAL_ERROR("Unable to allocate memory.");
	mem_store->chunks[mem_store->num_chunks++] = chunk;
	
	mem_store->num_allocations_since_last_gc += 1;
	CON_MUTEX_UNLOCK(&mem_store->mem_mutex);
	
	return chunk + 1;
}



void *Con_Memory_malloc_no_gc(Con_Obj *thread, size_t size, Con_Memory_Chunk_Type type)
{
	assert(type != CON_MEMORY_CHUNK_OBJ);

	Con_Memory_Store *mem_store = Con_Builtins_VM_Atom_get_mem_store(thread);

	Con_Memory_Chunk *chunk = malloc(sizeof(Con_Memory_Chunk) + size);
	
	if (chunk == NULL)
		return NULL;
	
	if (type == CON_MEMORY_CHUNK_CONSERVATIVE)
		bzero(chunk + 1, size);

	chunk->size = size;
	chunk->type = type;
	chunk->gc_to_be_deleted = 1;
	chunk->gc_incomplete_visit = 0;
	
	CON_MUTEX_LOCK(&mem_store->mem_mutex);
	if (!_Con_Memory_make_array_room_no_lock(thread, (void **) &mem_store->chunks, &mem_store->num_chunks_allocated, &mem_store->num_chunks, 1, sizeof(Con_Memory_Chunk *)))
		CON_FATAL_ERROR("Unable to allocate memory.");
	mem_store->chunks[mem_store->num_chunks++] = chunk;
	
	mem_store->num_allocations_since_last_gc += 1;
	CON_MUTEX_UNLOCK(&mem_store->mem_mutex);

	
	return chunk + 1;
}



void *Con_Memory_realloc(Con_Obj *thread, void *ptr, size_t size)
{
	Con_Memory_Chunk *chunk = ((Con_Memory_Chunk*) ptr) - 1, *new_chunk;

	Con_Memory_gc_poll(thread);

	Con_Memory_Store *mem_store = Con_Builtins_VM_Atom_get_mem_store(thread);
	CON_MUTEX_LOCK(&mem_store->mem_mutex);

	if ((new_chunk = realloc(chunk, sizeof(Con_Memory_Chunk) + size)) == NULL) {
		CON_MUTEX_UNLOCK(&mem_store->mem_mutex);
		Con_Memory_gc_force(thread);
		if ((new_chunk = realloc(chunk, sizeof(Con_Memory_Chunk) + size)) == NULL)
			CON_FATAL_ERROR("Unable to allocate memory.");
		CON_MUTEX_LOCK(&mem_store->mem_mutex);
	}

	if (new_chunk->type == CON_MEMORY_CHUNK_CONSERVATIVE)
		bzero(((u_char *) (new_chunk + 1)) + new_chunk->size, size - new_chunk->size);

	new_chunk->size = size;

	// We only need to update the entry for this chunk if its address has been changed by the
	// reallocation.

	if (new_chunk != chunk) {
		Con_Int i;
		for (i = 0; i < mem_store->num_chunks; i += 1) {
			if (mem_store->chunks[i] == chunk) {
				mem_store->chunks[i] = new_chunk;
				break;
			}
		}
		assert(i != mem_store->num_chunks);
	}
	
	mem_store->num_allocations_since_last_gc += 1;
	CON_MUTEX_UNLOCK(&mem_store->mem_mutex);
	
	return new_chunk + 1;
}



void *Con_Memory_realloc_no_gc(Con_Obj *thread, void *ptr, size_t size)
{
	Con_Memory_Chunk *chunk = ((Con_Memory_Chunk*) ptr) - 1, *new_chunk;

	Con_Memory_Store *mem_store = Con_Builtins_VM_Atom_get_mem_store(thread);
	CON_MUTEX_LOCK(&mem_store->mem_mutex);

	if ((new_chunk = realloc(chunk, sizeof(Con_Memory_Chunk) + size)) == NULL) {
		CON_MUTEX_UNLOCK(&mem_store->mem_mutex);
		return NULL;
	}

	if (new_chunk->type == CON_MEMORY_CHUNK_CONSERVATIVE)
		bzero(((u_char *) (new_chunk + 1)) + new_chunk->size, size - new_chunk->size);

	new_chunk->size = size;
	
	// We only need to update the entry for this chunk if its address has been changed by the
	// reallocation.

	if (new_chunk != chunk) {
		Con_Int i;
		for (i = 0; i < mem_store->num_chunks; i += 1) {
			if (mem_store->chunks[i] == chunk) {
				mem_store->chunks[i] = new_chunk;
				break;
			}
		}
		assert(i != mem_store->num_chunks);
	}
	
	mem_store->num_allocations_since_last_gc += 1;
	CON_MUTEX_UNLOCK(&mem_store->mem_mutex);
	
	return new_chunk + 1;
}



void Con_Memory_change_chunk_type(Con_Obj *thread, void *ptr, Con_Memory_Chunk_Type new_type)
{
	Con_Memory_Chunk *chunk = ((Con_Memory_Chunk*) ptr) - 1;	

	Con_Memory_Store *mem_store = Con_Builtins_VM_Atom_get_mem_store(thread);

	CON_MUTEX_LOCK(&mem_store->mem_mutex);
	assert(chunk->type != new_type);
	chunk->type = new_type;
	CON_MUTEX_UNLOCK(&mem_store->mem_mutex);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Misc functions
//

#ifndef CON_THREADS_SINGLE_THREADED

//
// Initialize 'mutex' from the counting mutex store.
//

void Con_Memory_mutex_init(Con_Obj *thread, Con_Mutex *mutex)
{
	Con_Memory_Store *mem_store = Con_Builtins_VM_Atom_get_mem_store(thread);

	CON_MUTEX_LOCK(&mem_store->mem_mutex);
	
	// Find the counting mutex with the current lowest number of users.
	
	int min_num_users = CON_DEFAULT_MAX_USERS_PER_COUNTING_MUTEX;
	int min_num_users_counting_mutex = 0;
	for (Con_Int i = 0; i < mem_store->num_counting_mutexes; i += 1) {
		int num_users = mem_store->counting_mutexes[i].num_users;
		if (num_users < min_num_users) {
			min_num_users = num_users;
			min_num_users_counting_mutex = i;
		}
	}
	assert((min_num_users_counting_mutex >= 0 ) && (min_num_users_counting_mutex < mem_store->num_counting_mutexes));

	// Use the counting mutex with the lowest number of users.
	
	*mutex = mem_store->counting_mutexes[min_num_users_counting_mutex].mutex;
	mem_store->counting_mutexes[min_num_users_counting_mutex].num_users += 1;
	
	CON_MUTEX_UNLOCK(&mem_store->mem_mutex);
}

#endif



//
// Ensure that at least *'num_allocated' + 'num_free_needed' slots of size 'size' are available in
// the array *'array'. *'array' will be resized if this is not the case.
//
// mutex should be locked when this routine is called; it may be unlocked during execution but will
// be locked before this functions returns.
//

void Con_Memory_make_array_room(Con_Obj *thread, void **array, Con_Mutex *mutex, Con_Int *num_allocated, Con_Int *num_used, Con_Int num_free_needed, size_t size)
{
#	ifndef CON_THREADS_SINGLE_THREADED
	if (mutex != NULL)
		CON_ASSERT_MUTEX_LOCKED(mutex);
#	endif
	
	if (*num_used + num_free_needed > *num_allocated) {
		void *new_array;
		Con_Int num_to_resize_to;
		while (1) {
			num_to_resize_to = *num_allocated + (*num_allocated / 2);
			if (num_to_resize_to < *num_used + num_free_needed)
				num_to_resize_to = *num_used + num_free_needed;
				
#			ifdef CON_THREADS_SINGLE_THREADED
			new_array = Con_Memory_realloc(thread, *array, sizeof(Con_Memory_Chunk) + num_to_resize_to * size);
			break;
#			else
			new_array = Con_Memory_realloc_no_gc(thread, *array, sizeof(Con_Memory_Chunk) + num_to_resize_to * size);
			if (new_array != NULL)
				break;
			CON_MUTEX_UNLOCK(mutex);
			CON_XXX;
			CON_MUTEX_LOCK(mutex);
#			endif
		}
		*array = new_array;
		*num_allocated = num_to_resize_to;
	}
}



//
// This function is similar to Con_Memory_make_array_room; however it doesn't require a mutex to be
// held on array. It returns true if sufficient space has been made available, and false otherwise.
//
// This function is intended for internal use for manipulating mem_store->chunks.
//

bool _Con_Memory_make_array_room_no_lock(Con_Obj *thread, void **array, Con_Int *num_allocated, Con_Int *num_used, Con_Int num_free_needed, size_t size)
{
	if (*num_used + num_free_needed > *num_allocated) {
		void *new_array;
		Con_Int num_to_resize_to;
		while (1) {
			num_to_resize_to = *num_allocated * 2;
			if (num_to_resize_to < *num_used + num_free_needed)
				num_to_resize_to = *num_used + num_free_needed;
			new_array = realloc(*array, sizeof(Con_Memory_Chunk) + num_to_resize_to * size);
			if (new_array != NULL)
				break;
			CON_XXX;
		}
		*array = new_array;
		*num_allocated = num_to_resize_to;
	}
	
	return true;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Garbage collection
//

//
// Notice that although it should be possible to extend Converge with a parallel garbage collector,
// the current garbage collector API is fundamentally designed to run when all other threads are
// suspended.
//



void Con_Memory_gc_poll(Con_Obj *thread)
{
	Con_Memory_Store *mem_store = Con_Builtins_VM_Atom_get_mem_store(thread);

#ifdef CON_THREADS_SINGLE_THREADED
	// We only consider whether we might need to check garbage collection every N allocations.
	// This is mostly to avoid the call to clock which although relatively cheap gets called often
	// enough to be a surprising resource drain.
	//
	// Note that this does mean it is possible to go through a "drout" where garbage collection
	// simply does not occur. However this is highly unlikely, and given that this simple check leads
	// to a 25-30% speedup we ignore this possibility.

	if (mem_store->num_allocations_since_last_gc % 1000 != 0)
		return;

	clock_t t = clock();
	if (t == (clock_t) -1)
		CON_XXX;

	// We force garbage collection if:
	//   1) A certain number of memory allocations / reallocations have occurred.
	//   2) The clock has wrapped. On a 32-bit machine this happens every 36 minutes, so this is
	//      a fairly marginal case.
	//   3) A certain number of seconds have passed since the last garbage collection.

	if ((mem_store->num_allocations_since_last_gc >= 100000) || (t < mem_store->time_of_last_gc) || ((t - mem_store->time_of_last_gc) / CLOCKS_PER_SEC > 3))
		Con_Memory_gc_force(thread);
	else
		return;
#else
	CON_MUTEX_LOCK(&mem_store->mem_mutex);
	if (mem_store->waiting_to_gc) {
		CON_MUTEX_UNLOCK(&mem_store->mem_mutex);

		Con_Builtins_Thread_Atom_suspend_for_gc(thread, &mem_store->gc_condition, &mem_store->mem_mutex);
	}
	else {
		// We only consider whether we might need to check garbage collection every N allocations.
		// This is mostly to avoid the call to clock which although relatively cheap gets called
		// often enough to be a surprising resource drain.
		//
		// Note that this does mean it is possible to go through a "drout" where garbage collection
		// simply does not occur. However this is highly unlikely, and given that this simple check
		// leads to a 25-30% speedup we ignore this possibility.

		if (mem_store->num_allocations_since_last_gc % 1000 != 0) {
			CON_MUTEX_UNLOCK(&mem_store->mem_mutex);
			return;
		}

		clock_t t = clock();
		if (t == (clock_t) -1)
			CON_XXX;

		// We force garbage collection if:
		//   1) A certain number of memory allocations / reallocations have occurred.
		//   2) The clock has wrapped. On a 32-bit machine this happens every 36 minutes, so this is
		//      a fairly marginal case.
		//   3) A certain number of seconds have passed since the last garbage collection.

		if ((mem_store->num_allocations_since_last_gc >= 100000) || (t < mem_store->time_of_last_gc) || ((t - mem_store->time_of_last_gc) / CLOCKS_PER_SEC > 3)) {
			CON_MUTEX_UNLOCK(&mem_store->mem_mutex);
			Con_Memory_gc_force(thread);
		}
		else {
			CON_MUTEX_UNLOCK(&mem_store->mem_mutex);
			return;
		}
	}
#endif
}

//
// Force garbage collection.
//

void Con_Memory_gc_force(Con_Obj *thread)
{
	if (Con_Builtins_VM_Atom_get_state(thread) != CON_VM_RUNNING)
		return;

	Con_Memory_Store *mem_store = Con_Builtins_VM_Atom_get_mem_store(thread);

#ifndef CON_THREADS_SINGLE_THREADED

	CON_MUTEX_LOCK(&mem_store->mem_mutex);
	
	if (mem_store->waiting_to_gc == true) {
		// The running thread has tried to force garbage collection, but in between it thinking that
		// it needed to force GC another thread and getting here another thread has already got ahead
		// of it, and is waiting to perform GC. So we simply poll again, knowing that this time this
		// thread is most likely to join the mem_store->gc_condition condition. [Other outcomes are
		// possible, but practically speaking it's unlikely that Con_Memory_gc_poll and
		// Con_Memory_gc_force will recurse more than once.]
		
		CON_MUTEX_UNLOCK(&mem_store->mem_mutex);
		Con_Memory_gc_poll(thread);
		return;
	}
	mem_store->waiting_to_gc = true;
	
	CON_MUTEX_UNLOCK(&mem_store->mem_mutex);

	// Put all other threads into a state such that this thread can perform garbage collection
	// safely.

	Con_Builtins_Thread_Atom_wait_until_safe_to_gc(thread);

	CON_MUTEX_LOCK(&mem_store->mem_mutex);
#endif

#	if CON_FULL_DEBUG
	printf("gc [thread %p] %d\n", thread, mem_store->num_chunks);
	fflush(NULL);
#	endif

#ifdef CON_HAVE_MERGESORT
	// mergesort is normally a touch quicker for this (because the entries are often largely
	// sorted), but requires extra memory, so we try mergesort first and only fall back on qsort
	// (which doesn't require extra memory) if we have to.

	if (mergesort(mem_store->chunks, mem_store->num_chunks, sizeof(Con_Memory_Chunk *), _Con_Memory_gc_chunks_sorter) == -1)
		qsort(mem_store->chunks, mem_store->num_chunks, sizeof(Con_Memory_Chunk *), _Con_Memory_gc_chunks_sorter);
#else
	qsort(mem_store->chunks, mem_store->num_chunks, sizeof(Con_Memory_Chunk *), _Con_Memory_gc_chunks_sorter);
#endif

	// Update the values of known_low_addr and known_high_addr; these are used to significantly speed
	// up conservative garbage collection. Since mem_store->chunks is sorted, calculating these two
	// values is trivial.

	mem_store->known_low_addr = (u_char *) mem_store->chunks[0];
	mem_store->known_high_addr = ((u_char *) (mem_store->chunks[mem_store->num_chunks - 1] + 1)) + mem_store->chunks[mem_store->num_chunks - 1]->size;

	// Reset the garbage collection stack.
	//
	// gc_stack_overflow will be set to true iff at least one chunk has gc_incomplete_visit set to 1.
	
	mem_store->gc_stack_overflow = false;
	mem_store->gc_stack_num_chunks = 0;

	// Scan the root set
	
	// First of all, use setjmp to conservatively scan processor registers etc.

	JMP_BUF buf;
#	ifdef CON_HAVE_SIGSETJMP
	sigsetjmp(buf, 0);
#	else
	setjmp(buf);
#	endif
	Con_Memory_gc_scan_conservative(thread, buf, sizeof(JMP_BUF));

	Con_Memory_gc_push(thread, thread);
	while (1) {	
		while (mem_store->gc_stack_num_chunks > 0) {
			Con_Memory_Chunk *current_chunk = mem_store->gc_stack[--mem_store->gc_stack_num_chunks];
			current_chunk->gc_to_be_deleted = 0;

			if (current_chunk->type == CON_MEMORY_CHUNK_OPAQUE)
				continue;
			else if (current_chunk->type == CON_MEMORY_CHUNK_CONSERVATIVE)
				Con_Memory_gc_scan_conservative(thread, current_chunk + 1, current_chunk->size);
			else if (current_chunk->type == CON_MEMORY_CHUNK_OBJ)
				_Con_Memory_gc_scan_obj(thread, (Con_Obj *) (current_chunk + 1));
			else if (current_chunk->type == CON_MEMORY_CHUNK_SLOTS) {
				Con_Slots_gc_scan_slots(thread, (Con_Slots *) (current_chunk + 1));
			}
			else
				CON_FATAL_ERROR("Unknown memory type (probable memory corruption).");
		}
		
		if (!mem_store->gc_stack_overflow)
			// If the gc stack hasn't overflowed, we can break early.
			break;
	
		mem_store->gc_stack_overflow = false;
	
		// The gc stack overflowed, so we need to traverse the stack pushing chunks with
		// gc_incomplete_visit set onto the stack.
		//
		// Note that it is possible that the stack will overflow again; this code copes with that
		// correctly.
		
		mem_store->gc_stack_overflow = false;
		for (Con_Int i = 0; i < mem_store->num_chunks; i += 1) {
			if (mem_store->chunks[i]->gc_incomplete_visit == 1) {
				mem_store->chunks[i]->gc_incomplete_visit = 0;
				_Con_Memory_gc_push_chunk(thread, mem_store->chunks[i]);
				if (mem_store->gc_stack_overflow)
					break;
			}
		}
	}

	// Free unused chunks.

#	if CON_FULL_DEBUG
	printf("Freeing:");
#	endif
	int last_i = 0;
	int num_chunks = mem_store->num_chunks;
	for (int i = 0; i < num_chunks; i += 1) {
		Con_Memory_Chunk *chunk = mem_store->chunks[i];

		if (chunk->gc_to_be_deleted == 0) {
			// Since this chunk is not going to be deleted, we need to reset gc_to_be_deleted.
			chunk->gc_to_be_deleted = 1;
			assert(chunk->gc_incomplete_visit == 0);
			mem_store->chunks[last_i++] = chunk;
			continue;
		}

		if (chunk->type == CON_MEMORY_CHUNK_OBJ) {
			Con_Obj *obj = (Con_Obj *) (chunk + 1);
			Con_Atom *atom = obj->first_atom;
			while (atom != NULL) {
				Con_Builtins_Atom_Def_Atom *atom_type_atom_def = CON_GET_ATOM(atom->atom_type, CON_BUILTIN(CON_BUILTIN_ATOM_DEF_OBJECT));
				if (atom_type_atom_def->gc_clean_up_func != NULL) {
					atom_type_atom_def->gc_clean_up_func(thread, obj, atom);
				}
				atom = atom->next_atom;
			}
		}

#		if CON_FULL_DEBUG
		if (chunk->type == CON_MEMORY_CHUNK_OBJ) {
			Con_Obj *obj = (Con_Obj *) (chunk + 1);
			Con_Atom *atom = obj->first_atom;
			while ((atom != NULL) && ((atom->atom_type == CON_BUILTIN(CON_BUILTIN_SLOTS_ATOM_DEF_OBJECT)) || (atom->atom_type == CON_BUILTIN(CON_BUILTIN_ATOM_DEF_OBJECT)) || (atom->atom_type == CON_BUILTIN(CON_BUILTIN_UNIQUE_ATOM_DEF_OBJECT)))) {
				atom = atom->next_atom;
			}
			if (atom == NULL) {
				printf(" .");
			}
			else {
				Con_Int j;
				for (j = 0; j < CON_NUMBER_OF_BUILTINS; j += 1) {
					if (atom->atom_type == CON_BUILTIN(j)) {
						printf(" %d", j);
						break;
					}
				}
				if (j == CON_NUMBER_OF_BUILTINS)
					printf(" unknown");
			}
		}
#		endif

		free(chunk);
	}
	mem_store->num_chunks = last_i;
#	if CON_FULL_DEBUG
	printf("\n");
	printf(" -> %d\n", mem_store->num_chunks);
	fflush(NULL);
#	endif
	
	// Reset the counters.
	
	mem_store->num_allocations_since_last_gc = 0;
	if ((mem_store->time_of_last_gc = clock()) == (clock_t) -1)
		CON_XXX;

#ifndef CON_THREADS_SINGLE_THREADED
	mem_store->waiting_to_gc = false;

	CON_MUTEX_UNLOCK(&mem_store->mem_mutex);

	Con_Builtins_Thread_Atom_condition_unblock_all(thread, &mem_store->gc_condition);
#endif
}



int _Con_Memory_gc_chunks_sorter(const void *chunk1, const void *chunk2)
{
	size_t val1 = *((size_t *) chunk1);
	size_t val2 = *((size_t *) chunk2);

	if (val1 < val2)
		return -1;
	else if (val1 > val2)
		return 1;
	return 0;
}



//
// Push 'chunk' onto the garbage collection stack.
//
// This function assumes that the VM is in the garbage collection state.
//

void _Con_Memory_gc_push_chunk(Con_Obj *thread, Con_Memory_Chunk *chunk)
{
	if (chunk->gc_to_be_deleted == 0)
		return;

	Con_Memory_Store *mem_store = Con_Builtins_VM_Atom_get_mem_store(thread);
	
	if (mem_store->gc_stack_num_chunks == mem_store->gc_stack_num_chunks_allocated) {
		Con_Int new_num_chunks_allocated = mem_store->gc_stack_num_chunks_allocated + (mem_store->gc_stack_num_chunks_allocated / 2);
		assert(new_num_chunks_allocated > mem_store->gc_stack_num_chunks_allocated);
		Con_Memory_Chunk **new_gc_stack = realloc(mem_store->gc_stack, new_num_chunks_allocated * sizeof(Con_Memory_Chunk *));
		if (new_gc_stack == NULL) {
			chunk->gc_incomplete_visit = 1;
			mem_store->gc_stack_overflow = true;
			return;
		}
		mem_store->gc_stack = new_gc_stack;
		mem_store->gc_stack_num_chunks_allocated = new_num_chunks_allocated;
	}
	
	mem_store->gc_stack[mem_store->gc_stack_num_chunks++] = chunk;
}



//
// Scan 'obj'.
//
// This function assumes that the VM is in the garbage collection state.
//

void _Con_Memory_gc_scan_obj(Con_Obj *thread, Con_Obj *obj)
{
	if (obj->creator_slots != NULL)
		Con_Memory_gc_push(thread, obj->creator_slots);
		
	Con_Atom *atom = obj->first_atom;
	while (atom != NULL) {
		Con_Memory_gc_push(thread, atom->atom_type);
		
		Con_Builtins_Atom_Def_Atom *atom_type_atom_def = CON_GET_ATOM(atom->atom_type, CON_BUILTIN(CON_BUILTIN_ATOM_DEF_OBJECT));
		if (atom_type_atom_def->gc_scan_func != NULL) {
			// This atoms atom type defines a custom gc scanning function.
			atom_type_atom_def->gc_scan_func(thread, obj, atom);
		}
		atom = atom->next_atom;
	}
}



//
// Conservatively scan 'size' bytes from 'start_addr'.
//
// Note that conservative scanning is potentially quite slow and should be used only when accurate
// scanning is impractical or impossible.
//
// This function assumes that the VM is in the garbage collection state.
//

void Con_Memory_gc_scan_conservative(Con_Obj *thread, void *start_addr, size_t size)
{
	Con_Memory_Store *mem_store = Con_Builtins_VM_Atom_get_mem_store(thread);

    if (size % sizeof(Con_Int) != 0)
    	size = (size / sizeof(Con_Int)) * sizeof(Con_Int);

	Con_Int *current_addr = start_addr;
	void *end_addr = ((u_char *) start_addr) + size;
	while ((void *) current_addr < end_addr) {
		void *ptr = (void *) (*current_addr);
		if (((u_char *) ptr >= mem_store->known_low_addr) && ((u_char *) ptr <= mem_store->known_high_addr))
			Con_Memory_gc_push_ptr(thread, ptr);
		current_addr += 1;
	}
}



//
// Scan 'pc'.
//
// This function assumes that the VM is in the garbage collection state.
//

void Con_Memory_gc_scan_pc(Con_Obj *thread, Con_PC pc)
{
	if (pc.type == PC_TYPE_C_FUNCTION)
		// Nothing to scan if the pc refers to a C function.
		return;
	
	Con_Memory_gc_push(thread, pc.module);
}




//
// Push the memory block starting at 'ptr' onto the garbage collection stack.
//
// This function assumes that the VM is in the garbage collection state.
//

void Con_Memory_gc_push(Con_Obj *thread, void *ptr)
{
	_Con_Memory_gc_push_chunk(thread, ((Con_Memory_Chunk *) ptr) - 1);
}



//
// Attempt to push the arbitrary address 'ptr' onto the garbage collection stack. 'ptr' is checked
// against the current known chunks and if it refers to an address within a particular chunks
// memory range, then that chunk is pushed onto the stack.
//
// In general, this function should only be called by Con_Memory_gc_scan_conservative.
//
// This function assumes that the VM is in the garbage collection state.
//

void Con_Memory_gc_push_ptr(Con_Obj *thread, const void *ptr)
{
	Con_Memory_Store *mem_store = Con_Builtins_VM_Atom_get_mem_store(thread);
	
	if (((u_char *) ptr < mem_store->known_low_addr) || ((u_char *) ptr > mem_store->known_high_addr))
		return;

	Con_Int high = mem_store->num_chunks, low = -1, probe;
	while (high - low > 1) {
		probe = (high + low) / 2;
		if (((void *) (mem_store->chunks[probe])) > ptr)
			high = probe;
		else
			low = probe;
	}
	if (low == -1)
		return;
	
	Con_Memory_Chunk *chunk = mem_store->chunks[low];
	void *chunk_mem = (void *) (chunk + 1);

	if ((ptr >= chunk_mem) && (ptr < (void *) (((u_char *) chunk_mem) + chunk->size)))
		Con_Memory_gc_push(thread, chunk_mem);
}
