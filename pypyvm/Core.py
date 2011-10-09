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




class Con_Thingy(object):
    __slots__ = ()


class PC(object):
    __slots__ = ("mod")
    _immutable_fields_ = ("mod",)


class BC_PC(PC):
    __slots__ = ("off")
    _immutable_fields_ = ("mod", "off")
    
    def __init__(self, mod, off):
        assert isinstance(mod, Builtins.Con_Module)
        self.mod = mod
        self.off = off


class Py_PC(PC):
    __slots__ = ("f")
    _immutable_fields_ = ("mod", "f")

    def __init__(self, mod, f):
        assert isinstance(mod, Builtins.Con_Module)
        self.mod = mod
        self.f = f