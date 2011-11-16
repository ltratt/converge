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


import os, sys
from Builtins import *
from Core import *



if sys.byteorder == "big":
    ENDIANNESS = "BIG_ENDIAN"
else:
    ENDIANNESS = "LITTLE_ENDIAN"

if sys.platform.startswith("openbsd"):
    PLATFORM = os.uname()[0]



def init(vm):
    return new_c_con_module(vm, "C_Platform_Properties", "C_Platform_Properties", __file__, \
      import_, \
      ["word_bits", "LITTLE_ENDIAN", "BIG_ENDIAN", "endianness", "osname", \
        "case_sensitive_filenames"])


def import_(vm):
    (mod,),_ = vm.decode_args("O")

    mod.set_defn(vm, "word_bits", Con_Int(vm, Target.INTSIZE * 8))
    mod.set_defn(vm, "LITTLE_ENDIAN", Con_String(vm, "LITTLE_ENDIAN"))
    mod.set_defn(vm, "BIG_ENDIAN", Con_String(vm, "BIG_ENDIAN"))
    mod.set_defn(vm, "endianness", Con_String(vm, ENDIANNESS))
    mod.set_defn(vm, "osname", Con_String(vm, PLATFORM))
    mod.set_defn(vm, "case_sensitive_filenames", Con_Int(vm, CASE_SENSITIVE_FILENAMES))
    
    vm.return_(vm.get_builtin(BUILTIN_NULL_OBJ))
