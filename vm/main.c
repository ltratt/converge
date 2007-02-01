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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <err.h>
#include <libgen.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Bootstrap.h"
#include "Bytecode.h"
#include "Core.h"
#include "Numbers.h"
#include "Object.h"
#include "Shortcuts.h"
#include "Version.h"

#include "Builtins/Con_Stack/Atom.h"
#include "Builtins/Module/Atom.h"
#include "Builtins/Thread/Atom.h"
#include "Builtins/VM/Atom.h"



extern char* __progname;



int main(int argc, char** argv) {
#ifdef __GNUC__
	u_char *root_stack_start = __builtin_frame_address(0);
#else
	CON_FATAL_ERROR("Unable to determine starting stack address.");
#endif

#if CON_FULL_DEBUG
		printf("%s %s (%s) %s\n", CON_NAME, CON_VERSION, CON_DATE, CON_COPYRIGHT);
		printf("stack root %p\n", root_stack_start);
#endif

	Con_Obj *thread = Con_Bootstrap_do(root_stack_start, argc, argv);
	if (thread == NULL) {
		fprintf(stderr, "%s: error when trying to initialize virtual machine.\n", __progname);
		exit(1);
	}

	if (argc == 1) {
		fprintf(stderr, "%s: too few options.\nusage: %s <file> [args]\n", __progname, __progname);
		exit(1);
	}
	
	struct stat con_binary_file_stat;
	if (stat(argv[1], &con_binary_file_stat) == -1) {
		err(1, ": trying to stat '%s'", argv[1]);
	}
	
	FILE *con_binary_file = fopen(argv[1], "rb");
	if (con_binary_file == NULL) {
		err(1, ": unable to open '%s", argv[1]);
		exit(1);
	}
	
	size_t bytecode_size = con_binary_file_stat.st_size;
	u_char *bytecode = malloc(bytecode_size);
	fread(bytecode, 1, bytecode_size, con_binary_file);
	
	// We now go through the file and look for where CONVEXEC starts. This means that files can have
	// arbitrary text at their beginning allowing e.g. "#!" UNIX commands to be inserted there.
	
	size_t bytecode_start = 0;
	while (bytecode_size - bytecode_start >= 8 && strncmp(bytecode + bytecode_start, "CONVEXEC", 8) != 0) {
		bytecode_start += 1;
	}
	if (bytecode_size - bytecode_start < 8) {
		fprintf(stderr, "%s: '%s' does not appear to be a valid Converge executeable.\n", __progname, argv[1]);
		exit(1);
	}
	Con_Obj *main_module_identifier = Con_Bytecode_add_executable(thread, bytecode + bytecode_start);
	free(bytecode);

	if (fclose(con_binary_file) != 0) {
		err(1, ": error closing '%s'", argv[1]);
		exit(1);
	}

	Con_Obj *exception;
	CON_TRY {
		Con_Obj *main_module = Con_Builtins_Module_Atom_import(thread, main_module_identifier);
		CON_APPLY(CON_GET_MODULE_DEF(main_module, "main"));
	}
	CON_CATCH(exception) {
		Con_Int exit_code;
		
		// We check to see if this exception is an instance of System_Exit_Exception; if so, we
		// don't print a backtrace.
		
		if (CON_GET_SLOT_APPLY_NO_FAIL(CON_GET_SLOT(CON_BUILTIN(CON_BUILTIN_EXCEPTIONS_MODULE), "System_Exit_Exception"), "instantiated", exception) != NULL)
			exit_code = Con_Numbers_Number_to_Con_Int(thread, CON_GET_SLOT(exception, "code"));
		else {
			Con_Obj *backtrace = CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_EXCEPTIONS_MODULE), "backtrace", exception);
			CON_GET_SLOT_APPLY(CON_BUILTIN(CON_BUILTIN_SYS_MODULE), "println", backtrace);
			backtrace = NULL;
			exit_code = 1;
		}
		
#ifndef CON_THREADS_SINGLE_THREADED
		// At this point the main thread has finished. We can't let the application finish until all
		// other threads have finished. We therefore can't have it removed from the thread pool as
		// otherwise bits of this thread (the Con_Stack etc) might start being freed by the garbage
		// collector.

		Con_Builtins_Thread_Atom_mark_as_finished(thread);

		Con_Builtins_Thread_Atom_join_all(thread);
#endif

		// Clear up as much memory as we can. There will still be some live objects after this has
		// executed, but they should mostly (if not all) be objects created during the bootstrap
		// process so hopefully aren't too important.

		main_module_identifier = exception = NULL;
		Con_Memory_gc_force(thread);
		
		exit(exit_code);
	}
	CON_TRY_END

#ifndef CON_THREADS_SINGLE_THREADED
	// At this point the main thread has finished. We can't let the application finish until all
	// other threads have finished. We therefore can't have it removed from the thread pool as
	// otherwise bits of this thread (the Con_Stack etc) might start being freed by the garbage
	// collector.

	Con_Builtins_Thread_Atom_mark_as_finished(thread);

	Con_Builtins_Thread_Atom_join_all(thread);
#endif

	// Clear up as much memory as we can. There will still be some live objects after this has
	// executed, but they should mostly (if not all) be objects created during the bootstrap
	// process so hopefully aren't too important.

	main_module_identifier = NULL;
	Con_Memory_gc_force(thread);
	
	return 0;
}
