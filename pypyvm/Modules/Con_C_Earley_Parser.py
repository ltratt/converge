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


from Builtins import *




def init(vm):
    return new_c_con_module(vm, "C_Earley_Parser", "C_Earley_Parser", __file__, import_, \
      ["Parser", "parse"])


def import_(vm):
    (mod,),_ = vm.decode_args("O")
    
    mod.set_defn(vm, "Parser", \
      Con_Class(vm, Con_String(vm, "Parser"), [vm.get_builtin(BUILTIN_OBJECT_CLASS)], mod))
    new_c_con_func_for_mod(vm, "parse", parse, mod)
    
    vm.return_(vm.get_builtin(BUILTIN_NULL_OBJ))


def parse(vm):
    raise Exception("XXX")