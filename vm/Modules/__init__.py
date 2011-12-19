# Copyright (c) 2011 King's College London, created by Laurence Tratt
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.


__all__ = ["Con_Array", "Con_C_Earley_Parser", "Con_C_Platform_Env", "Con_C_Platform_Exec", \
  "Con_C_Platform_Host", "Con_C_Platform_Properties", "Con_C_Strings", "Con_C_Time", "Con_Curses", \
  "Con_Exceptions", "Con_PCRE", "Con_POSIX_File", "Con_Sys", "Con_Thread", "Con_VM", "libXML2"]

import Con_Array, Con_C_Earley_Parser, Con_C_Platform_Env, Con_C_Platform_Exec, \
  Con_C_Platform_Host, Con_C_Platform_Properties, Con_C_Strings, Con_C_Time, Con_Curses, \
  Con_Exceptions, Con_PCRE, Con_POSIX_File, Con_Sys, Con_Thread, Con_VM, libXML2

BUILTIN_MODULES = \
  [Con_Array.init, Con_C_Earley_Parser.init, Con_C_Platform_Env.init, Con_C_Platform_Exec.init, \
   Con_C_Platform_Host.init, Con_C_Platform_Properties.init, Con_C_Strings.init, Con_C_Time.init, \
   Con_Curses.init, Con_Exceptions.init, Con_PCRE.init, Con_POSIX_File.init, Con_Sys.init, \
   Con_Thread.init, Con_VM.init, libXML2.init]