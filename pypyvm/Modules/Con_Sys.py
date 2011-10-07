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


import Builtins




def init(vm):
    mod = Builtins.new_c_con_module(vm, "Sys", "Sys", __file__, ["print", "println"])
    init_func = Builtins.new_c_con_func(vm, Builtins.new_con_string(vm, "init"), False, import_, mod)
    mod.init_func = init_func
    
    return mod



def import_(vm):
    (mod,),_ = vm.decode_args("O")
    
    println_func = Builtins.new_c_con_func(vm, Builtins.new_con_string(vm, "println"), False, println, mod)
    mod.set_defn("println", println_func)
    
    vm.return_(vm.get_builtin(Builtins.BUILTIN_NULL_OBJ))



def println(vm):
    _,vargs = vm.decode_args("v")
    for o in vargs:
        if isinstance(o, Builtins.Con_String):
            print o.v
        else:
            s = vm.apply(o.get_slot("to_str"))
            print o, s
            raise Exception("XXX")

    vm.return_(vm.get_builtin(Builtins.BUILTIN_NULL_OBJ))