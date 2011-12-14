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
    return new_c_con_module(vm, "libXML2", "libXML2", __file__, import_, \
      ["XML_Exception", "parse"])


def import_(vm):
    (mod,),_ = vm.decode_args("O")

    class_class = vm.get_builtin(BUILTIN_CLASS_CLASS)
    user_exception_class = vm.get_builtin(BUILTIN_EXCEPTIONS_MODULE). \
      get_defn(vm, "User_Exception")
    xml_exception = vm.get_slot_apply(class_class, "new", \
      [Con_String(vm, "XML_Exception"), Con_List(vm, [user_exception_class]), mod])
    mod.set_defn(vm, "XML_Exception", xml_exception)

    vm.return_(vm.get_builtin(BUILTIN_NULL_OBJ))
