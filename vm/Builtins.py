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

from rpython.rlib import debug, jit, objectmodel, rarithmetic, rweakref
from rpython.rtyper.lltypesystem import lltype, rffi

NUM_BUILTINS = 41

from Core import *
import Bytecode, Target, VM




BUILTIN_NULL_OBJ = 0
BUILTIN_FAIL_OBJ = 1

# Core atom defs

BUILTIN_ATOM_DEF_OBJECT = 2
BUILTIN_SLOTS_ATOM_DEF_OBJECT = 3
BUILTIN_CLASS_ATOM_DEF_OBJECT = 4
BUILTIN_VM_ATOM_DEF_OBJECT = 5
BUILTIN_THREAD_ATOM_DEF_OBJECT = 6
BUILTIN_FUNC_ATOM_DEF_OBJECT = 7
BUILTIN_STRING_ATOM_DEF_OBJECT = 8
BUILTIN_CON_STACK_ATOM_DEF_OBJECT = 9
BUILTIN_LIST_ATOM_DEF_OBJECT = 10
BUILTIN_DICT_ATOM_DEF_OBJECT = 11
BUILTIN_MODULE_ATOM_DEF_OBJECT = 12
BUILTIN_INT_ATOM_DEF_OBJECT = 13
BUILTIN_UNIQUE_ATOM_DEF_OBJECT = 14
BUILTIN_CLOSURE_ATOM_DEF_OBJECT = 15
BUILTIN_PARTIAL_APPLICATION_ATOM_DEF_OBJECT = 16
BUILTIN_EXCEPTION_ATOM_DEF_OBJECT = 17
BUILTIN_SET_ATOM_DEF_OBJECT = 18

# Core classes

BUILTIN_OBJECT_CLASS = 19
BUILTIN_CLASS_CLASS = 20
BUILTIN_VM_CLASS = 21
BUILTIN_THREAD_CLASS = 22
BUILTIN_FUNC_CLASS = 23
BUILTIN_STRING_CLASS = 24
BUILTIN_CON_STACK_CLASS = 25
BUILTIN_LIST_CLASS = 26
BUILTIN_DICT_CLASS = 27
BUILTIN_MODULE_CLASS = 28
BUILTIN_INT_CLASS = 29
BUILTIN_CLOSURE_CLASS = 30
BUILTIN_PARTIAL_APPLICATION_CLASS = 31
BUILTIN_EXCEPTION_CLASS = 32
BUILTIN_SET_CLASS = 33
BUILTIN_NUMBER_CLASS = 34

BUILTIN_BUILTINS_MODULE = 35
BUILTIN_C_FILE_MODULE = 36
BUILTIN_EXCEPTIONS_MODULE = 37
BUILTIN_SYS_MODULE = 38

# Floats

BUILTIN_FLOAT_ATOM_DEF_OBJECT = 39
BUILTIN_FLOAT_CLASS = 40




################################################################################
# Con_Object
#

class Con_Object(Con_Thingy):
    __slots__ = ()


class Version(object):
    pass



# This map class is inspired by:
#   http://morepypy.blogspot.com/2011/03/controlling-tracing-of-interpreter-with_21.html

class _Con_Map(object):
    __slots__ = ("index_map", "other_maps")
    _immutable_fields_ = ("index_map", "other_maps")


    def __init__(self):
        self.index_map = {}
        self.other_maps = {}


    @jit.elidable
    def find(self, n):
        return self.index_map.get(n, -1)


    @jit.elidable
    def extend(self, n):
        if n not in self.other_maps:
            nm = _Con_Map()
            nm.index_map.update(self.index_map)
            nm.index_map[n] = len(self.index_map)
            self.other_maps[n] = nm
        return self.other_maps[n]



_EMPTY_MAP = _Con_Map()



class Con_Boxed_Object(Con_Object):
    __slots__ = ("instance_of", "slots_map", "slots")


    def __init__(self, vm, instance_of=None):
        if instance_of is None:
            self.instance_of = vm.get_builtin(BUILTIN_OBJECT_CLASS)
        else:
            self.instance_of = instance_of
        self.slots_map = _EMPTY_MAP
        self.slots = None


    def has_slot(self, vm, n):
        if self.slots is not None:
            m = jit.promote(self.slots_map)
            i = m.find(n)
            if i != -1:
                return True

        return False


    def find_slot(self, vm, n):
        o = None
        if self.slots is not None:
            m = jit.promote(self.slots_map)
            i = m.find(n)
            if i != -1:
                o = self.slots[i]
    
        if o is None:
            o = self.instance_of.find_field(vm, n)
            if o is None:
                if n == "instance_of":
                    o = self.instance_of
                if o is None:
                    return o

        if isinstance(o, Con_Func) and o.is_bound:
            return Con_Partial_Application(vm, o, [self])
        
        return o


    def get_slot(self, vm, n):
        o = None
        if self.slots is not None:
            m = jit.promote(self.slots_map)
            i = m.find(n)
            if i != -1:
                o = self.slots[i]
    
        if o is None:
            o = self.instance_of.find_field(vm, n)
            if o is None:
                if n == "instance_of":
                    o = self.instance_of
                if o is None:
                    vm.raise_helper("Slot_Exception", [Con_String(vm, n), self])

        if isinstance(o, Con_Func) and o.is_bound:
            return Con_Partial_Application(vm, o, [self])
        
        return o



    def set_slot(self, vm, n, o):
        assert o is not None
        m = jit.promote(self.slots_map)
        if self.slots is not None:
            i = m.find(n)
            if i == -1:
                self.slots_map = m.extend(n)
                self.slots.append(o)
            else:
                self.slots[i] = o
        else:
            self.slots_map = m.extend(n)
            self.slots = [o]


    def is_(self, o):
        return self is o


    def add(self, vm, o):
        return vm.get_slot_apply(self, "+", [o])


    def subtract(self, vm, o):
        return vm.get_slot_apply(self, "-", [o])


    def eq(self, vm, o):
        if vm.get_slot_apply(self, "==", [o], allow_fail=True):
            return True
        else:
            return False


    def neq(self, vm, o):
        if vm.get_slot_apply(self, "!=", [o], allow_fail=True):
            return True
        else:
            return False


    def le(self, vm, o):
        if vm.get_slot_apply(self, "<", [o], allow_fail=True):
            return True
        else:
            return False


    def le_eq(self, vm, o):
        if vm.get_slot_apply(self, "<=", [o], allow_fail=True):
            return True
        else:
            return False


    def gr_eq(self, vm, o):
        if vm.get_slot_apply(self, ">=", [o], allow_fail=True):
            return True
        else:
            return False


    def gt(self, vm, o):
        if vm.get_slot_apply(self, ">", [o], allow_fail=True):
            return True
        else:
            return False


@con_object_proc
def _new_func_Con_Object(vm):
    (c,), vargs = vm.decode_args("O", vargs=True)
    o = Con_Boxed_Object(vm, c)
    vm.apply(o.get_slot(vm, "init"), vargs)
    return o


@con_object_proc
def _Con_Object_find_slot(vm):
    (self, sn_o),_ = vm.decode_args("OS")
    assert isinstance(sn_o, Con_String)

    v = self.find_slot(vm, sn_o.v)
    if not v:
        v = vm.get_builtin(BUILTIN_FAIL_OBJ)
    return v


@con_object_proc
def _Con_Object_get_slot(vm):
    (self, sn_o),_ = vm.decode_args("OS")
    assert isinstance(sn_o, Con_String)

    return self.get_slot(vm, sn_o.v)


@con_object_proc
def _Con_Object_init(vm):
    (self,), vargs = vm.decode_args("O", vargs=True)
    return vm.get_builtin(BUILTIN_NULL_OBJ)


@con_object_proc
def _Con_Object_is(vm):
    (self, o),_ = vm.decode_args("OO")
    if self.is_(o):
        return vm.get_builtin(BUILTIN_NULL_OBJ)
    else:
        return vm.get_builtin(BUILTIN_FAIL_OBJ)


@con_object_proc
def _Con_Object_to_str(vm):
    (self,),_ = vm.decode_args("O")
    return Con_String(vm, "<Object@%x>" % objectmodel.current_object_addr_as_int(self))


def bootstrap_con_object(vm):
    # This is where the hardcore bootstrapping stuff happens. Many things here are done in a very
    # specific order - changing something can easily cause lots of later things to break.
    
    object_class_nm = Con_String(vm, "Object")
    object_class = Con_Class(vm, object_class_nm, [], None)
    vm.set_builtin(BUILTIN_OBJECT_CLASS, object_class)
    class_class_nm = Con_String(vm, "Class")
    class_class = Con_Class(vm, class_class_nm, [object_class], None)
    vm.set_builtin(BUILTIN_CLASS_CLASS, class_class)
    object_class.instance_of = class_class
    class_class.instance_of = class_class

    string_class_nm = Con_String(vm, "String")
    string_class = Con_Class(vm, string_class_nm, [object_class], None)
    vm.set_builtin(BUILTIN_STRING_CLASS, string_class)
    object_class_nm.instance_of = string_class
    class_class_nm.instance_of = string_class
    string_class_nm.instance_of = string_class
    
    vm.set_builtin(BUILTIN_NULL_OBJ, Con_Boxed_Object(vm))
    vm.set_builtin(BUILTIN_FAIL_OBJ, Con_Boxed_Object(vm))

    module_class = Con_Class(vm, Con_String(vm, "Module"), [object_class], None)
    vm.set_builtin(BUILTIN_MODULE_CLASS, module_class)
    
    # In order that later objects can refer to the Builtins module, we have to create it now.
    builtins_module = new_c_con_module(vm, "Builtins", "Builtins", __file__, None, \
      ["Object", "Class", "Func", "Partial_Application", "String", "Module", "Number", "Int",
       "Float", "List", "Set", "Dict", "Exception"])
    # We effectively initialize the Builtins module through the bootstrapping process, so it doesn't
    # need a separate initialization function.
    builtins_module.initialized = True
    vm.set_builtin(BUILTIN_BUILTINS_MODULE, builtins_module)
    vm.set_mod(builtins_module)
    
    object_class.set_slot(vm, "container", builtins_module)
    class_class.set_slot(vm, "container", builtins_module)
    string_class.set_slot(vm, "container", builtins_module)
    module_class.set_slot(vm, "container", builtins_module)
    builtins_module.set_defn(vm, "Object", object_class)
    builtins_module.set_defn(vm, "Class", class_class)
    builtins_module.set_defn(vm, "String", string_class)
    builtins_module.set_defn(vm, "Module", module_class)

    func_class = Con_Class(vm, Con_String(vm, "Func"), [object_class], builtins_module)
    vm.set_builtin(BUILTIN_FUNC_CLASS, func_class)
    builtins_module.set_defn(vm, "Func", func_class)
    partial_application_class = Con_Class(vm, Con_String(vm, "Partial_Application"), \
      [object_class], builtins_module)
    vm.set_builtin(BUILTIN_PARTIAL_APPLICATION_CLASS, partial_application_class)
    builtins_module.set_defn(vm, "Partial_Application", partial_application_class)
    number_class = Con_Class(vm, Con_String(vm, "Number"), [object_class], builtins_module)
    vm.set_builtin(BUILTIN_NUMBER_CLASS, number_class)
    builtins_module.set_defn(vm, "Number", number_class)
    int_class = Con_Class(vm, Con_String(vm, "Int"), [object_class], builtins_module)
    vm.set_builtin(BUILTIN_INT_CLASS, int_class)
    builtins_module.set_defn(vm, "Int", int_class)
    float_class = Con_Class(vm, Con_String(vm, "Float"), [object_class], builtins_module)
    vm.set_builtin(BUILTIN_FLOAT_CLASS, float_class)
    builtins_module.set_defn(vm, "Float", float_class)
    list_class = Con_Class(vm, Con_String(vm, "List"), [object_class], builtins_module)
    vm.set_builtin(BUILTIN_LIST_CLASS, list_class)
    builtins_module.set_defn(vm, "List", list_class)
    set_class = Con_Class(vm, Con_String(vm, "Set"), [object_class], builtins_module)
    vm.set_builtin(BUILTIN_SET_CLASS, set_class)
    builtins_module.set_defn(vm, "Set", set_class)
    dict_class = Con_Class(vm, Con_String(vm, "Dict"), [object_class], builtins_module)
    vm.set_builtin(BUILTIN_DICT_CLASS, dict_class)
    builtins_module.set_defn(vm, "Dict", dict_class)
    exception_class = Con_Class(vm, Con_String(vm, "Exception"), [object_class], builtins_module)
    vm.set_builtin(BUILTIN_EXCEPTION_CLASS, exception_class)
    builtins_module.set_defn(vm, "Exception", exception_class)

    object_class.new_func = \
      new_c_con_func(vm, Con_String(vm, "new_Object"), False, _new_func_Con_Object, \
        builtins_module)

    new_c_con_func_for_class(vm, "find_slot", _Con_Object_find_slot, object_class)
    new_c_con_func_for_class(vm, "get_slot", _Con_Object_get_slot, object_class)
    new_c_con_func_for_class(vm, "init", _Con_Object_init, object_class)
    new_c_con_func_for_class(vm, "is", _Con_Object_is, object_class)
    new_c_con_func_for_class(vm, "to_str", _Con_Object_to_str, object_class)




################################################################################
# Con_Class
#

class Con_Class(Con_Boxed_Object):
    __slots__ = ("supers", "fields_map", "fields", "new_func", "version", "dependents")
    _immutable_fields_ = ("supers", "fields", "dependents")


    def __init__(self, vm, name, supers, container, instance_of=None, new_func=None):
        if instance_of is None:
            instance_of = vm.get_builtin(BUILTIN_CLASS_CLASS)
        Con_Boxed_Object.__init__(self, vm, instance_of)
        
        if new_func is None:
            # A new object function hasn't been supplied so we need to search for one.
            # See http://tratt.net/laurie/tech_articles/articles/more_meta_matters for
            # more details about this algorithm.
            for sc in supers:
                assert isinstance(sc, Con_Class)
                if new_func is None:
                    new_func = sc.new_func
                elif new_func is not sc.new_func:
                    new_func = sc.new_func
                    object_class = vm.get_builtin(BUILTIN_OBJECT_CLASS)
                    assert isinstance(object_class, Con_Class)
                    object_class.new_func
                    if new_func is object_class.new_func:
                        new_func = sc.new_func
                    else:
                        # There's a clash between superclass's metaclasses.
                        raise Exception("XXX")
        self.new_func = new_func
        
        self.supers = supers
        self.fields_map = _EMPTY_MAP
        self.fields = []
        
        # To optimise slot lookups, we need to be a little more cunning. We make a (reasonable)
        # assumption that classes rarely change their fields (most classes never change their fields
        # after their initial creation at all), so that in general we can simply elide field
        # lookups entirely. When a field is changed, all subsequent field lookups on that class and
        # its subclasses need to be regenerated. To force this, every class a "version" which is
        # incremented whenever its fields are changed. As well as changing the class itself, all
        # subclasses must be changed too. We maintain a list of all subclasses (even indirect ones!)
        # to do this.
        
        self.version = Version()
        self.dependents = []
        sc_stack = supers[:]
        while len(sc_stack) > 0:
            sc = type_check_class(vm, sc_stack.pop())
            sc.dependents.append(rweakref.ref(sc))
            sc_stack.extend(sc.supers)

        self.set_slot(vm, "name", name)
        if container:
            self.set_slot(vm, "container", container)


    @jit.elidable_promote("0")
    def _get_field_i(self, vm, n, version):
        m = jit.promote(self.fields_map)
        i = m.find(n)
        if i != -1:
            return self.fields[i]

        for s in self.supers:
            assert isinstance(s, Con_Class)
            o = s.find_field(vm, n)
            if o is not None:
                return o

        return None


    def find_field(self, vm, n):
        return self._get_field_i(vm, n, jit.promote(self.version))


    def get_field(self, vm, n):
        o = self._get_field_i(vm, n, jit.promote(self.version))
        if o is None:
            vm.raise_helper("Field_Exception", [Con_String(vm, n), self])
        return o


    def set_field(self, vm, n, o):
        assert o is not None
        m = jit.promote(self.fields_map)
        i = m.find(n)
        if i == -1:
            self.fields_map = m.extend(n)
            self.fields.append(o)
        else:
            self.fields[i] = o
        self.version = Version()
        
        j = 0
        while j < len(self.dependents):
            dep = self.dependents[j]()
            if dep is None:
                del self.dependents[j]
                continue
            dep.version = Version()
            j += 1


@con_object_proc
def _new_func_Con_Class(vm):
    (c, name, supers, container), vargs = vm.decode_args("CSLO", vargs=True)
    assert isinstance(c, Con_Class)
    assert isinstance(name, Con_String)
    assert isinstance(supers, Con_List)
    o = Con_Class(vm, name, supers.l[:], container, c)
    vm.apply(o.get_slot(vm, "init"), vargs)
    return o


@con_object_proc
def _Con_Class_new(vm):
    _, v = vm.decode_args(vargs=True)
    c = type_check_class(vm, v[0])
    if c.new_func is None:
        p = type_check_string(vm, vm.get_slot_apply(c, "path")).v
        msg = "Instance of %s has no new_func." % p
        vm.raise_helper("VM_Exception", [Con_String(vm, msg)])
    return vm.apply(c.new_func, v)


@con_object_proc
def _Con_Class_get_field(vm):
    (self, n),_ = vm.decode_args("CS")
    assert isinstance(self, Con_Class)
    assert isinstance(n, Con_String)

    o = self.find_field(vm, n.v)
    if o is None:
        vm.raise_helper("Field_Exception", [n, self])

    return o


@con_object_proc
def _Con_Class_set_field(vm):
    (self, n, o),_ = vm.decode_args("CSO")
    assert isinstance(self, Con_Class)
    assert isinstance(n, Con_String)
    self.set_field(vm, n.v, o)

    return vm.get_builtin(BUILTIN_NULL_OBJ)


@con_object_proc
def _Con_Class_path(vm):
    (self, stop_at),_ = vm.decode_args("C", opt="o")
    assert isinstance(self, Con_Class)
    
    name = type_check_string(vm, self.get_slot(vm, "name"))
    if name.v == "":
        name = Con_String(vm, "<anon>")

    container = self.get_slot(vm, "container")
    if container is vm.get_builtin(BUILTIN_NULL_OBJ) or container is stop_at:
        return name
    else:
        if stop_at is None:
            stop_at = vm.get_builtin(BUILTIN_NULL_OBJ)
        rtn = type_check_string(vm, vm.get_slot_apply(container, "path", [stop_at]))
        if isinstance(container, Con_Module):
            sep = "::"
        else:
            sep = "."
        return Con_String(vm, "%s%s%s" % (rtn.v, sep, name.v))


@con_object_proc
def _Con_Class_to_str(vm):
    (self,),_ = vm.decode_args("C")
    assert isinstance(self, Con_Class)

    nm = type_check_string(vm, self.get_slot(vm, "name"))
    return Con_String(vm, "<Class %s>" % nm.v)


@con_object_proc
def _Con_Class_conformed_by(vm):
    (self, o),_ = vm.decode_args("CO")
    assert isinstance(self, Con_Class)
    assert isinstance(o, Con_Boxed_Object)

    if o.instance_of is self:
        # We optimise the easy case.
        return vm.get_builtin(BUILTIN_NULL_OBJ)
    else:
        stack = [self]
        while len(stack) > 0:
            cnd = stack.pop()
            assert isinstance(cnd, Con_Class)
            for f in cnd.fields_map.index_map.keys():
                if not o.has_slot(vm, f):
                    return vm.get_builtin(BUILTIN_FAIL_OBJ)
            stack.extend(cnd.supers)  

    return vm.get_builtin(BUILTIN_NULL_OBJ)


@con_object_proc
def _Con_Class_instantiated(vm):
    (self, o),_ = vm.decode_args("CO")
    assert isinstance(self, Con_Class)
    assert isinstance(o, Con_Boxed_Object)

    if o.instance_of is self:
        # We optimise the easy case.
        return vm.get_builtin(BUILTIN_NULL_OBJ)
    else:
		# What we do now is to put 'instance_of' onto a stack; if the current class on the stack
		# does not match 'self', we push all the class's superclasses onto the stack.
		#
		# If we run off the end of the stack then there is no match.
        stack = [o.instance_of]
        while len(stack) > 0:
            cnd = stack.pop()
            assert isinstance(cnd, Con_Class)
            if cnd is self:
                return vm.get_builtin(BUILTIN_NULL_OBJ)
            stack.extend(cnd.supers)

    return vm.get_builtin(BUILTIN_FAIL_OBJ)


def bootstrap_con_class(vm):
    class_class = vm.get_builtin(BUILTIN_CLASS_CLASS)
    assert isinstance(class_class, Con_Class)
    class_class.new_func = \
      new_c_con_func(vm, Con_String(vm, "new_Class"), False, _new_func_Con_Class, \
        vm.get_builtin(BUILTIN_BUILTINS_MODULE))

    new_c_con_func_for_class(vm, "conformed_by", _Con_Class_conformed_by, class_class)
    new_c_con_func_for_class(vm, "instantiated", _Con_Class_instantiated, class_class)
    new_c_con_func_for_class(vm, "new", _Con_Class_new, class_class)
    new_c_con_func_for_class(vm, "get_field", _Con_Class_get_field, class_class)
    new_c_con_func_for_class(vm, "set_field", _Con_Class_set_field, class_class)
    new_c_con_func_for_class(vm, "path", _Con_Class_path, class_class)
    new_c_con_func_for_class(vm, "to_str", _Con_Class_to_str, class_class)



################################################################################
# Con_Module
#

class Con_Module(Con_Boxed_Object):
    __slots__ = ("is_bc", "bc", "id_", "src_path", "imps", "tlvars_map", "consts",
      "init_func", "values", "closure", "initialized")
    _immutable_fields_ = ("is_bc", "bc", "name", "id_", "src_path", "imps", "tlvars_map",
      "init_func", "consts")


    def __init__(self, vm, is_bc, bc, name, id_, src_path, imps, tlvars_map, num_consts, init_func, \
      instance_of=None):
        if instance_of is None:
            instance_of = vm.get_builtin(BUILTIN_MODULE_CLASS)
        Con_Boxed_Object.__init__(self, vm, instance_of)

        self.is_bc = is_bc # True for bytecode modules; False for RPython modules
        self.bc = bc
        self.id_ = id_
        self.src_path = src_path
        self.imps = imps
        self.tlvars_map = tlvars_map
        self.consts = [None] * num_consts
        debug.make_sure_not_resized(self.consts)
        self.init_func = init_func
        
        self.values = []
        if is_bc:
            self.closure = None
        else:
            self.closure = VM.Closure(None, len(tlvars_map))

        self.set_slot(vm, "name", name)
        self.set_slot(vm, "src_path", Con_String(vm, src_path))
        self.set_slot(vm, "mod_id", Con_String(vm, id_))
        self.set_slot(vm, "container", vm.get_builtin(BUILTIN_NULL_OBJ))

        self.initialized = False


    def import_(self, vm):
        if self.initialized:
            return
        
        if self.is_bc:
            # Bytecode modules use the old "push a Con_Int onto the stack to signify how many
            # parameters are being passed" hack. To add insult injury, they simply pop this object
            # off without using it. So we pass null as a 'magic' parameter (since it will be popped
            # first, it's actually the second parameter), knowing that it won't actually be used for
            # anything.
            v, self.closure = vm.apply_closure(self.init_func, \
              [self, vm.get_builtin(BUILTIN_NULL_OBJ)])
        else:
            vm.apply(self.init_func, [self])
        self.initialized = True
        return


    @jit.elidable_promote("0")
    def get_closure_i(self, vm, n):
        return self.tlvars_map.get(n, -1)


    def get_defn(self, vm, n):
        i = self.get_closure_i(vm, n)
        if i == -1:
            name = type_check_string(vm, self.get_slot(vm, "name")).v
            vm.raise_helper("Mod_Defn_Exception", \
              [Builtins.Con_String(vm, "No such definition '%s' in '%s'." % (n, name))])
        o = self.closure.vars[i]
        if o is None:
            name = type_check_string(vm, self.get_slot(vm, "name")).v
            vm.raise_helper("Mod_Defn_Exception", \
              [Builtins.Con_String(vm, "Definition '%s' unassigned in '%s'." % (n, name))])

        return o


    @jit.elidable_promote("0")
    def has_defn(self, vm, n):
        if self.get_closure_i(vm, n) == -1:
            return False
        return True


    def set_defn(self, vm, n, o):
        i = self.get_closure_i(vm, n)
        if i == -1:
            name = type_check_string(vm, self.get_slot(vm, "name")).v
            vm.raise_helper("Mod_Defn_Exception", \
              [Builtins.Con_String(vm, "No such definition '%s' in '%s'." % (n, name))])
        self.closure.vars[i] = o


    @jit.elidable_promote("0")
    def get_const(self, vm, i):
        v = self.consts[i]
        if v is not None:
            return v
        consts_offs = Target.read_word(self.bc, Target.BC_MOD_CONSTANTS_OFFSETS)
        const_off = Target.read_word(self.bc, Target.BC_MOD_CONSTANTS) \
          + Target.read_word(self.bc, consts_offs + i * Target.INTSIZE)
        type = Target.read_word(self.bc, const_off)
        if type == Target.CONST_INT:
            v = Con_Int(vm, Target.read_word(self.bc, const_off + Target.INTSIZE))
        elif type == Target.CONST_FLOAT:
            v = Con_Float(vm, Target.read_float(self.bc, const_off + Target.INTSIZE))
        else:
            assert type == Target.CONST_STRING
            s = Target.extract_str(self.bc,
              const_off + Target.INTSIZE + Target.INTSIZE,
              Target.read_word(self.bc, const_off + Target.INTSIZE))
            v = Con_String(vm, s)
        self.consts[i] = v
        return v


    def bc_off_to_src_infos(self, vm, bc_off):
        bc = self.bc
        cur_bc_off = Target.read_word(bc, Target.BC_MOD_INSTRUCTIONS)
        instr_i = 0
        while cur_bc_off < bc_off:
            instr = Target.read_word(bc, cur_bc_off)
            it = Target.get_instr(instr)
            if it == Target.CON_INSTR_EXBI:
                start, size = Target.unpack_exbi(instr)
                cur_bc_off += Target.align(start + size)
            elif it == Target.CON_INSTR_IS_ASSIGNED:
                cur_bc_off += Target.INTSIZE + Target.INTSIZE
            elif it == Target.CON_INSTR_SLOT_LOOKUP or it == Target.CON_INSTR_PRE_SLOT_LOOKUP_APPLY:
                start, size = Target.unpack_slot_lookup(instr)
                cur_bc_off += Target.align(start + size)
            elif it == Target.CON_INSTR_ASSIGN_SLOT:
                start, size = Target.unpack_assign_slot(instr)
                cur_bc_off += Target.align(start + size)
            elif it == Target.CON_INSTR_UNPACK_ARGS:
                num_args, has_vargs = Target.unpack_unpack_args(instr)
                cur_bc_off += Target.INTSIZE + num_args * Target.INTSIZE
                if has_vargs:
                    cur_bc_off += Target.INTSIZE
            elif it == Target.CON_INSTR_MODULE_LOOKUP:
                start, size = Target.unpack_mod_lookup(instr)
                cur_bc_off += Target.align(start + size)
            elif it == Target.CON_INSTR_VAR_LOOKUP \
              or it == Target.CON_INSTR_VAR_ASSIGN \
              or it == Target.CON_INSTR_ADD_FAILURE_FRAME \
              or it == Target.CON_INSTR_ADD_FAIL_UP_FRAME \
              or it == Target.CON_INSTR_REMOVE_FAILURE_FRAME \
              or it == Target.CON_INSTR_IS \
              or it == Target.CON_INSTR_FAIL_NOW \
              or it == Target.CON_INSTR_POP \
              or it == Target.CON_INSTR_IMPORT \
              or it == Target.CON_INSTR_LIST \
              or it == Target.CON_INSTR_APPLY \
              or it == Target.CON_INSTR_FUNC_DEFN \
              or it == Target.CON_INSTR_RETURN \
              or it == Target.CON_INSTR_BRANCH \
              or it == Target.CON_INSTR_YIELD \
              or it == Target.CON_INSTR_DICT \
              or it == Target.CON_INSTR_DUP \
              or it == Target.CON_INSTR_PULL \
              or it == Target.CON_INSTR_BUILTIN_LOOKUP \
              or it == Target.CON_INSTR_EYIELD \
              or it == Target.CON_INSTR_ADD_EXCEPTION_FRAME \
              or it == Target.CON_INSTR_REMOVE_EXCEPTION_FRAME \
              or it == Target.CON_INSTR_RAISE \
              or it == Target.CON_INSTR_SET \
              or it == Target.CON_INSTR_BRANCH_IF_NOT_FAIL \
              or it == Target.CON_INSTR_BRANCH_IF_FAIL \
              or it == Target.CON_INSTR_CONST_GET \
              or it == Target.CON_INSTR_UNPACK_ASSIGN \
              or it == Target.CON_INSTR_EQ \
              or it == Target.CON_INSTR_NEQ \
              or it == Target.CON_INSTR_GT \
              or it == Target.CON_INSTR_LE \
              or it == Target.CON_INSTR_LE_EQ \
              or it == Target.CON_INSTR_GR_EQ \
              or it == Target.CON_INSTR_ADD \
              or it == Target.CON_INSTR_SUBTRACT:
                cur_bc_off += Target.INTSIZE
            else:
                print it
                raise Exception("XXX")
        
            instr_i += 1

        assert cur_bc_off == bc_off
        
        src_info_pos = src_info_num = 0
        src_infos_off = Target.read_word(bc, Target.BC_MOD_SRC_POSITIONS)
        while 1:
            src_info1 = Target.read_uint32_word(bc, src_infos_off + src_info_pos * 4)
            if src_info_num + (src_info1 & ((1 << 4) - 1)) > instr_i:
                break
            src_info_num += src_info1 & ((1 << 4) - 1)
            while src_info1 & (1 << 4):
                src_info_pos += 2
                src_info1 = Target.read_uint32_word(bc, src_infos_off + src_info_pos * 4)
            src_info_pos += 2

        src_infos = []
        while 1:
            src_info1 = Target.read_uint32_word(bc, src_infos_off + src_info_pos * 4)
            src_info2 = Target.read_uint32_word(bc, src_infos_off + (src_info_pos + 1) * 4)

            if src_info2 & ((1 << 12) - 1) == ((1 << 12) - 1):
                mod_id = self.id_
            else:
                mod_id = self.imps[src_info2 & ((1 << 12) - 1)]

            src_off = (src_info1 >> 5) & ((1 << (31 - 5)) - 1)
            src_len = src_info2 >> 12
            src_info = Con_List(vm, \
              [Con_String(vm, mod_id), Con_Int(vm, src_off), Con_Int(vm, src_len)])
            src_infos.append(src_info)

            if not (src_info1 & (1 << 4)):
                break

            src_info_pos += 2
        
        return Con_List(vm, src_infos)


@con_object_proc
def _new_func_Con_Module(vm):
    (class_, bc_o), vargs = vm.decode_args("CS", vargs=True)
    assert isinstance(bc_o, Con_String)
    
    bc = rffi.str2charp(bc_o.v)
    mod = Bytecode.mk_mod(vm, bc, 0)
    return mod


@con_object_proc
def _Con_Module_get_defn(vm):
    (self, n),_ = vm.decode_args("MS")
    assert isinstance(self, Con_Module)
    assert isinstance(n, Con_String)

    return self.get_defn(vm, n.v)


@con_object_proc
def _Con_Module_has_defn(vm):
    (self, n),_ = vm.decode_args("MS")
    assert isinstance(self, Con_Module)
    assert isinstance(n, Con_String)

    if self.has_defn(vm, n.v):
        r_o = vm.get_builtin(BUILTIN_NULL_OBJ)
    else:
        r_o = vm.get_builtin(BUILTIN_FAIL_OBJ)

    return r_o


@con_object_proc
def _Con_Module_set_defn(vm):
    (self, n, o),_ = vm.decode_args("MSO")
    assert isinstance(self, Con_Module)
    assert isinstance(n, Con_String)

    self.set_defn(vm, n.v, o)
    return vm.get_builtin(BUILTIN_NULL_OBJ)


@con_object_gen
def _Con_Module_iter_defns(vm):
    (self,),_ = vm.decode_args("M")
    assert isinstance(self, Con_Module)

    for d in self.tlvars_map.keys():
        yield Con_List(vm, [Con_String(vm, d), self.get_defn(vm, d)])


@con_object_gen
def _Con_Module_iter_newlines(vm):
    (self,),_ = vm.decode_args("M")
    assert isinstance(self, Con_Module)

    bc = self.bc
    newlines_off = Target.read_word(bc, Target.BC_MOD_NEWLINES)
    for i in range(Target.read_word(bc, Target.BC_MOD_NUM_NEWLINES)):
        yield Con_Int(vm, Target.read_word(bc, newlines_off + i * Target.INTSIZE))


@con_object_proc
def _Con_Module_path(vm):
    (self, stop_at),_ = vm.decode_args("M", opt="o")
    assert isinstance(self, Con_Module)
    
    if self is stop_at:
        return Con_String(vm, "")
    
    name = type_check_string(vm, self.get_slot(vm, "name"))
    container = self.get_slot(vm, "container")
    if container is vm.get_builtin(BUILTIN_NULL_OBJ) or container is stop_at:
        return name
    else:
        if stop_at is None:
            stop_at = vm.get_builtin(BUILTIN_NULL_OBJ)
        rtn = type_check_string(vm, vm.get_slot_apply(container, "path", [stop_at]))
        if isinstance(container, Con_Module):
            sep = "::"
        else:
            sep = "."
        return Con_String(vm, "%s%s%s" % (rtn.v, sep, name.v))


@con_object_proc
def _Con_Module_src_offset_to_line_column(vm):
    (self, off_o),_ = vm.decode_args("MI")
    assert isinstance(self, Con_Module)
    assert isinstance(off_o, Con_Int)

    off = off_o.v
    if off < 0:
        raise Exception("XXX")

    bc = self.bc
    newlines_off = Target.read_word(bc, Target.BC_MOD_NEWLINES)
    for i in range(Target.read_word(bc, Target.BC_MOD_NUM_NEWLINES)):
        if off < Target.read_word(bc, newlines_off + i * Target.INTSIZE):
            return Con_List(vm, [Con_Int(vm, i), Con_Int(vm, off - \
              Target.read_word(bc, newlines_off + (i - 1) * Target.INTSIZE))])
    
    raise Exception("XXX")
    


def bootstrap_con_module(vm):
    module_class = vm.get_builtin(BUILTIN_MODULE_CLASS)
    assert isinstance(module_class, Con_Class)
    module_class.new_func = \
      new_c_con_func(vm, Con_String(vm, "new_Module"), False, \
        _new_func_Con_Module, vm.get_builtin(BUILTIN_BUILTINS_MODULE))


    new_c_con_func_for_class(vm, "get_defn", _Con_Module_get_defn, module_class)
    new_c_con_func_for_class(vm, "has_defn", _Con_Module_has_defn, module_class)
    new_c_con_func_for_class(vm, "iter_defns", _Con_Module_iter_defns, module_class)
    new_c_con_func_for_class(vm, "iter_newlines", _Con_Module_iter_newlines, module_class)
    new_c_con_func_for_class(vm, "path", _Con_Module_path, module_class)
    new_c_con_func_for_class(vm, "set_defn", _Con_Module_set_defn, module_class)
    new_c_con_func_for_class(vm, "src_offset_to_line_column", _Con_Module_src_offset_to_line_column, module_class)


def new_c_con_module(vm, name, id_, src_path, import_func, names):
    tlvars_map = {}
    i = 0
    for j in names:
        assert j not in tlvars_map
        tlvars_map[j] = i
        i += 1
    mod = Con_Module(vm, False, lltype.nullptr(rffi.CCHARP.TO), Con_String(vm, name), id_, \
      src_path, [], tlvars_map, 0, None)
    mod.init_func = new_c_con_func(vm, Con_String(vm, "$$init$$"), False, import_func, mod)
    
    return mod


def new_bc_con_module(vm, bc, name, id_, src_path, imps, tlvars_map, num_consts):
    return Con_Module(vm, True, bc, Con_String(vm, name), id_, src_path, imps, tlvars_map, \
      num_consts, None)



################################################################################
# Con_Func
#

class Con_Func(Con_Boxed_Object):
    __slots__ = ("name", "is_bound", "pc", "max_stack_size", "num_vars", 
                 "container_closure", "stack_count")
    _immutable_fields_ = ("name", "is_bound", "pc", "max_stack_size",
                 "num_vars", "container_closure")


    def __init__(self, vm, name, is_bound, pc, max_stack_size, num_params, num_vars, container, \
      container_closure, instance_of=None):
        if instance_of is None:
            instance_of = vm.get_builtin(BUILTIN_FUNC_CLASS)
        Con_Boxed_Object.__init__(self, vm, instance_of)
    
        self.name = name
        self.is_bound = is_bound
        self.pc = pc
        self.max_stack_size = max_stack_size
        self.num_vars = num_vars
        self.container_closure = container_closure
        self.stack_count = 0
        
        self.set_slot(vm, "container", container)
        self.set_slot(vm, "name", name)
        self.set_slot(vm, "num_params", Con_Int(vm, num_params))


    def __repr__(self):
        return "<Func %s>" % self.name.v


@con_object_proc
def _Con_Func_path(vm):
    (self, stop_at),_ = vm.decode_args("Fo")
    assert isinstance(self, Con_Func)
    
    if self is stop_at:
        return Con_String(vm, "")
    
    container = self.get_slot(vm, "container")
    if container is vm.get_builtin(BUILTIN_NULL_OBJ) or container is stop_at:
        return self.name
    else:
        rtn = type_check_string(vm, vm.get_slot_apply(container, "path", [stop_at]))
        if isinstance(container, Con_Module):
            sep = "::"
        else:
            sep = "."
        name = self.name
        assert isinstance(name, Con_String)
        return Con_String(vm, "%s%s%s" % (rtn.v, sep, name.v))


def bootstrap_con_func(vm):
    func_class = vm.get_builtin(BUILTIN_FUNC_CLASS)
    assert isinstance(func_class, Con_Class)
    
    builtins_module = vm.get_builtin(BUILTIN_BUILTINS_MODULE)
    builtins_module.set_defn(vm, "Func", func_class)
    
    new_c_con_func_for_class(vm, "path", _Con_Func_path, func_class)


def new_c_con_func(vm, name, is_bound, func, container):
    cnd = container
    while not (isinstance(cnd, Con_Module)):
        cnd = cnd.get_slot(vm, "container")
    return Con_Func(vm, name, is_bound, VM.Py_PC(cnd, func), 0, -1, 0, container, None)


def new_c_con_func_for_class(vm, name, func, class_):
    f = new_c_con_func(vm, Con_String(vm, name), True, func, class_)
    class_.set_field(vm, name, f)


def new_c_con_func_for_mod(vm, name, func, mod):
    f = new_c_con_func(vm, Con_String(vm, name), False, func, mod)
    mod.set_defn(vm, name, f)



################################################################################
# Con_Partial_Application
#

class Con_Partial_Application(Con_Boxed_Object):
    __slots__ = ("f", "args")
    _immutable_fields_ = ("f", "args")


    def __init__(self, vm, f, args, instance_of=None):
        if instance_of is None:
            instance_of = vm.get_builtin(BUILTIN_PARTIAL_APPLICATION_CLASS)
        Con_Boxed_Object.__init__(self, vm, instance_of)
        self.f = f
        self.args = args


    def __repr__(self):
        return "<Partial_Application %s>" % self.f.name.v



@con_object_proc
def _new_func_Con_Partial_Application(vm):
    (class_, func_o, args_o),_ = vm.decode_args("CFL")
    assert isinstance(args_o, Con_List)

    o = Con_Partial_Application(vm, func_o, args_o.l)
    vm.apply(o.get_slot(vm, "init"), [func_o] + args_o.l)
    return o


@con_object_gen
def _Con_Partial_Application_apply(vm):
    (self, args_o),_ = vm.decode_args("!L", self_of=Con_Partial_Application)
    assert isinstance(self, Con_Partial_Application)
    assert isinstance(args_o, Con_List)
    
    vm.pre_apply_pump(self.f, self.args + args_o.l)
    while 1:
        e_o = vm.apply_pump()
        if not e_o:
            break
        yield e_o


def bootstrap_con_partial_application(vm):
    partial_application_class = vm.get_builtin(BUILTIN_PARTIAL_APPLICATION_CLASS)
    assert isinstance(partial_application_class, Con_Class)
    partial_application_class.new_func = \
      new_c_con_func(vm, Con_String(vm, "new_Partial_Application"), False, \
        _new_func_Con_Partial_Application, vm.get_builtin(BUILTIN_BUILTINS_MODULE))

    new_c_con_func_for_class(vm, "apply", _Con_Partial_Application_apply, partial_application_class)



################################################################################
# Con_Number
#

class Con_Number(Con_Boxed_Object):
    __slots__ = ()

    def as_int(self):
        raise Exception("XXX")


    def as_float(self):
        raise Exception("XXX")



################################################################################
# Con_Int
#

class Con_Int(Con_Number):
    __slots__ = ("v",)
    _immutable_fields_ = ("v",)


    def __init__(self, vm, v, instance_of=None):
        if instance_of is None:
            instance_of = vm.get_builtin(BUILTIN_INT_CLASS)
        Con_Number.__init__(self, vm, instance_of)
        assert v is not None
        self.v = rffi.cast(lltype.Signed, v)


    def as_int(self):
        return self.v


    def as_float(self):
        return float(self.v)


    def is_(self, o):
        if isinstance(o, Con_Int):
            return self.v == o.v
        else:
            return self is o


    def add(self, vm, o):
        o = type_check_number(vm, o)
        if isinstance(o, Con_Int):
            return Con_Int(vm, self.v + o.v)
        else:
            assert isinstance(o, Con_Float)
            return Con_Float(vm, self.as_float() + o.v)


    def subtract(self, vm, o):
        o = type_check_number(vm, o)
        if isinstance(o, Con_Int):
            return Con_Int(vm, self.v - o.v)
        else:
            assert isinstance(o, Con_Float)
            return Con_Float(vm, self.as_float() - o.v)


    def div(self, vm, o):
        o = type_check_number(vm, o)
        if isinstance(o, Con_Int):
            if self.v % o.v == 0:
                return Con_Int(vm, self.v / o.v)
            return Con_Float(vm, self.as_float() / o.v)
        else:
            assert isinstance(o, Con_Float)
            return Con_Float(vm, self.as_float() / o.v)


    def idiv(self, vm, o):
        o = type_check_number(vm, o)
        return Con_Int(vm, self.v // o.as_int())


    def mod(self, vm, o):
        o = type_check_number(vm, o)
        return Con_Int(vm, self.v % o.as_int())


    def mul(self, vm, o):
        o = type_check_number(vm, o)
        if isinstance(o, Con_Int):
            return Con_Int(vm, self.v * o.v)
        else:
            assert isinstance(o, Con_Float)
            return Con_Float(vm, self.as_float() * o.v)


    def pow(self, vm, o):
        o = type_check_int(vm, o)
        p = 1
        x = self.v
        y = o.v
        while y > 0:
            p *= x
            y -= 1
        return Con_Int(vm, p)


    def eq(self, vm, o):
        if isinstance(o, Con_Int):
            return self.v == o.v
        elif isinstance(o, Con_Float):
            return self.as_float() == o.v
        return False


    def neq(self, vm, o):
        if isinstance(o, Con_Int):
            return self.v != o.v
        elif isinstance(o, Con_Float):
            return self.as_float() != o.v
        return True


    def le(self, vm, o):
        o = type_check_number(vm, o)
        if isinstance(o, Con_Int):
            return self.v < o.v
        else:
            assert isinstance(o, Con_Float)
            return self.as_float() < o.v


    def le_eq(self, vm, o):
        o = type_check_number(vm, o)
        if isinstance(o, Con_Int):
            return self.v <= o.v
        else:
            assert isinstance(o, Con_Float)
            return self.as_float() <= o.v


    def gr_eq(self, vm, o):
        o = type_check_number(vm, o)
        if isinstance(o, Con_Int):
            return self.v >= o.v
        else:
            assert isinstance(o, Con_Float)
            return self.as_float() >= o.v


    def gt(self, vm, o):
        o = type_check_number(vm, o)
        if isinstance(o, Con_Int):
            return self.v > o.v
        else:
            assert isinstance(o, Con_Float)
            return self.as_float() > o.v


@con_object_proc
def _new_func_Con_Int(vm):
    (class_, o_o), vargs = vm.decode_args("CO", vargs=True)
    if isinstance(o_o, Con_Int):
        return o_o
    elif isinstance(o_o, Con_String):
        v = None
        try:
            if o_o.v.startswith("0x") or o_o.v.startswith("0X"):
                v = int(o_o.v[2:], 16)
            else:
                v = int(o_o.v)
        except ValueError:
            vm.raise_helper("Number_Exception", [o_o])
        return Con_Int(vm, v)
    elif isinstance(o_o, Con_Float):
        return Con_Int(vm, o_o.v)
    else:
        vm.raise_helper("Type_Exception", [Con_String(vm, "Number | String"), o_o])


@con_object_proc
def _Con_Int_add(vm):
    (self, o_o),_ = vm.decode_args("IN")
    assert isinstance(self, Con_Int)
    assert isinstance(o_o, Con_Int)

    return self.add(vm, o_o)


@con_object_proc
def _Con_Int_and(vm):
    (self, o_o),_ = vm.decode_args("II")
    assert isinstance(self, Con_Int)
    assert isinstance(o_o, Con_Int)

    return Con_Int(vm, self.v & o_o.v)


@con_object_proc
def _Con_Int_div(vm):
    (self, o_o),_ = vm.decode_args("IN")
    assert isinstance(self, Con_Int)
    assert isinstance(o_o, Con_Number)

    return self.div(vm, o_o)


@con_object_proc
def _Con_Int_eq(vm):
    (self, o_o),_ = vm.decode_args("IO")
    assert isinstance(self, Con_Int)
    assert isinstance(o_o, Con_Object)
    
    if self.eq(vm, o_o):
        return vm.get_builtin(BUILTIN_NULL_OBJ)
    else:
        return vm.get_builtin(BUILTIN_FAIL_OBJ)


@con_object_proc
def _Con_Int_gt(vm):
    (self, o_o),_ = vm.decode_args("IN")
    assert isinstance(self, Con_Int)
    assert isinstance(o_o, Con_Number)

    if self.gt(vm, o_o):
        return vm.get_builtin(BUILTIN_NULL_OBJ)
    else:
        return vm.get_builtin(BUILTIN_FAIL_OBJ)


@con_object_proc
def _Con_Int_gtq(vm):
    (self, o_o),_ = vm.decode_args("IN")
    assert isinstance(self, Con_Int)
    assert isinstance(o_o, Con_Number)

    if self.gr_eq(vm, o_o):
        return vm.get_builtin(BUILTIN_NULL_OBJ)
    else:
        return vm.get_builtin(BUILTIN_FAIL_OBJ)


@con_object_proc
def _Con_Int_hash(vm):
    (self,),_ = vm.decode_args("I")
    assert isinstance(self, Con_Int)

    return Con_Int(vm, objectmodel.compute_hash(self.v))


@con_object_proc
def _Con_Int_idiv(vm):
    (self, o_o),_ = vm.decode_args("IN")
    assert isinstance(self, Con_Int)
    assert isinstance(o_o, Con_Number)

    return self.idiv(vm, o_o)


@con_object_proc
def _Con_Int_inv(vm):
    (self,),_ = vm.decode_args("I")
    assert isinstance(self, Con_Int)

    return Con_Int(vm, ~self.v)


@con_object_gen
def _Con_Int_iter_to(vm):
    (self, to_o, step_o),_ = vm.decode_args("II", opt="I")
    assert isinstance(self, Con_Int)
    assert isinstance(to_o, Con_Int)
    
    if step_o is None:
        step = 1
    else:
        assert isinstance(step_o, Con_Int)
        step = step_o.v

    for i in range(self.v, to_o.v, step):
        yield Con_Int(vm, i)


@con_object_proc
def _Con_Int_le(vm):
    (self, o_o),_ = vm.decode_args("IN")
    assert isinstance(self, Con_Int)
    assert isinstance(o_o, Con_Number)

    if self.le(vm, o_o):
        return vm.get_builtin(BUILTIN_NULL_OBJ)
    else:
        return vm.get_builtin(BUILTIN_FAIL_OBJ)


@con_object_proc
def _Con_Int_leq(vm):
    (self, o_o),_ = vm.decode_args("IN")
    assert isinstance(self, Con_Int)
    assert isinstance(o_o, Con_Number)

    if self.le_eq(vm, o_o):
        return vm.get_builtin(BUILTIN_NULL_OBJ)
    else:
        return vm.get_builtin(BUILTIN_FAIL_OBJ)


@con_object_proc
def _Con_Int_lsl(vm):
    (self, o),_ = vm.decode_args("II")
    assert isinstance(self, Con_Int)
    assert isinstance(o, Con_Int)

    return Con_Int(vm, self.v << o.v)


@con_object_proc
def _Con_Int_lsr(vm):
    (self, o),_ = vm.decode_args("II")
    assert isinstance(self, Con_Int)
    assert isinstance(o, Con_Int)

    return Con_Int(vm, self.v >> o.v)


@con_object_proc
def _Con_Int_mod(vm):
    (self, o_o),_ = vm.decode_args("IN")
    assert isinstance(self, Con_Int)

    return self.mod(vm, o_o)


@con_object_proc
def _Con_Int_mul(vm):
    (self, o_o),_ = vm.decode_args("IN")
    assert isinstance(self, Con_Int)

    return self.mul(vm, o_o)


@con_object_proc
def _Con_Int_or(vm):
    (self, o_o),_ = vm.decode_args("II")
    assert isinstance(self, Con_Int)
    assert isinstance(o_o, Con_Int)

    return Con_Int(vm, self.v | o_o.v)


@con_object_proc
def _Con_Int_pow(vm):
    (self, o_o),_ = vm.decode_args("IN")
    assert isinstance(self, Con_Int)

    return self.pow(vm, o_o)



@con_object_proc
def _Con_Int_sub(vm):
    (self, o_o),_ = vm.decode_args("IN")
    assert isinstance(self, Con_Int)
    assert isinstance(o_o, Con_Number)

    return self.subtract(vm, o_o)


@con_object_proc
def _Con_Int_str_val(vm):
    (self,),_ = vm.decode_args("I")
    assert isinstance(self, Con_Int)

    v = self.v
    if v < 0 or v > 255:
        vm.raise_helper("Number_Exception", [Con_String(vm, "'%d' out of ASCII range." % v)])

    return Con_String(vm, chr(v))


@con_object_proc
def _Con_Int_to_str(vm):
    (self,),_ = vm.decode_args("I")
    assert isinstance(self, Con_Int)

    return Con_String(vm, str(self.v))


@con_object_proc
def _Con_Int_xor(vm):
    (self, o_o),_ = vm.decode_args("II")
    assert isinstance(self, Con_Int)
    assert isinstance(o_o, Con_Int)

    return Con_Int(vm, self.v ^ o_o.v)


def bootstrap_con_int(vm):
    int_class = vm.get_builtin(BUILTIN_INT_CLASS)
    assert isinstance(int_class, Con_Class)
    int_class.new_func = \
      new_c_con_func(vm, Con_String(vm, "new_Int"), False, _new_func_Con_Int, \
        vm.get_builtin(BUILTIN_BUILTINS_MODULE))

    new_c_con_func_for_class(vm, "+", _Con_Int_add, int_class)
    new_c_con_func_for_class(vm, "and", _Con_Int_and, int_class)
    new_c_con_func_for_class(vm, "/", _Con_Int_div, int_class)
    new_c_con_func_for_class(vm, "==", _Con_Int_eq, int_class)
    new_c_con_func_for_class(vm, ">", _Con_Int_gt, int_class)
    new_c_con_func_for_class(vm, ">=", _Con_Int_gtq, int_class)
    new_c_con_func_for_class(vm, "hash", _Con_Int_hash, int_class)
    new_c_con_func_for_class(vm, "idiv", _Con_Int_idiv, int_class)
    new_c_con_func_for_class(vm, "inv", _Con_Int_inv, int_class)
    new_c_con_func_for_class(vm, "iter_to", _Con_Int_iter_to, int_class)
    new_c_con_func_for_class(vm, "<", _Con_Int_le, int_class)
    new_c_con_func_for_class(vm, "<=", _Con_Int_leq, int_class)
    new_c_con_func_for_class(vm, "lsl", _Con_Int_lsl, int_class)
    new_c_con_func_for_class(vm, "lsr", _Con_Int_lsr, int_class)
    new_c_con_func_for_class(vm, "%", _Con_Int_mod, int_class)
    new_c_con_func_for_class(vm, "*", _Con_Int_mul, int_class)
    new_c_con_func_for_class(vm, "or", _Con_Int_or, int_class)
    new_c_con_func_for_class(vm, "pow", _Con_Int_pow, int_class)
    new_c_con_func_for_class(vm, "str_val", _Con_Int_str_val, int_class)
    new_c_con_func_for_class(vm, "-", _Con_Int_sub, int_class)
    new_c_con_func_for_class(vm, "to_str", _Con_Int_to_str, int_class)
    new_c_con_func_for_class(vm, "xor", _Con_Int_xor, int_class)



################################################################################
# Con_Float
#

class Con_Float(Con_Number):
    __slots__ = ("v",)
    _immutable_fields_ = ("v",)


    def __init__(self, vm, v, instance_of=None):
        if instance_of is None:
            instance_of = vm.get_builtin(BUILTIN_FLOAT_CLASS)
        Con_Number.__init__(self, vm, instance_of)
        assert v is not None
        self.v = v


    def as_int(self):
        return int(self.v)


    def as_float(self):
        return self.v


    def add(self, vm, o):
        o = type_check_number(vm, o)
        if isinstance(o, Con_Int):
            return Con_Float(vm, self.v + o.v)
        else:
            assert isinstance(o, Con_Float)
            return Con_Float(vm, self.v + o.v)


    def subtract(self, vm, o):
        o = type_check_number(vm, o)
        if isinstance(o, Con_Int):
            return Con_Float(vm, self.v - o.v)
        else:
            assert isinstance(o, Con_Float)
            return Con_Float(vm, self.v - o.v)


    def div(self, vm, o):
        o = type_check_number(vm, o)
        if isinstance(o, Con_Int):
            return Con_Float(vm, self.v / o.v)
        else:
            assert isinstance(o, Con_Float)
            return Con_Float(vm, self.v / o.v)


    def idiv(self, vm, o):
        o = type_check_number(vm, o)
        return Con_Int(vm, self.v // o.as_int())


    def mod(self, vm, o):
        o = type_check_number(vm, o)
        return Con_Int(vm, self.v % o.as_int())


    def mul(self, vm, o):
        o = type_check_number(vm, o)
        if isinstance(o, Con_Int):
            return Con_Int(vm, self.v * o.v)
        else:
            assert isinstance(o, Con_Float)
            return Con_Float(vm, self.v * o.v)


    def eq(self, vm, o):
        if isinstance(o, Con_Int):
            return self.v == o.v
        elif isinstance(o, Con_Float):
            return self.v == o.v
        return False


    def neq(self, vm, o):
        if isinstance(o, Con_Int):
            return self.v != o.v
        elif isinstance(o, Con_Float):
            return self.v != o.v
        return True


    def le(self, vm, o):
        o = type_check_number(vm, o)
        if isinstance(o, Con_Int):
            return self.as_int() < o.v
        else:
            assert isinstance(o, Con_Float)
            return self.v < o.v


    def le_eq(self, vm, o):
        o = type_check_number(vm, o)
        if isinstance(o, Con_Int):
            return self.as_int() <= o.v
        else:
            assert isinstance(o, Con_Float)
            return self.v <= o.v


    def gr_eq(self, vm, o):
        o = type_check_number(vm, o)
        if isinstance(o, Con_Int):
            return self.as_int() >= o.v
        else:
            assert isinstance(o, Con_Float)
            return self.v >= o.v


    def gt(self, vm, o):
        o = type_check_number(vm, o)
        if isinstance(o, Con_Int):
            return self.as_int() > o.v
        else:
            assert isinstance(o, Con_Float)
            return self.v > o.v


@con_object_proc
def _new_func_Con_Float(vm):
    (class_, o_o), vargs = vm.decode_args("CO", vargs=True)
    if isinstance(o_o, Con_Int):
        return Con_Float(vm, float(o_o.v))
    elif isinstance(o_o, Con_Float):
        return o_o
    elif isinstance(o_o, Con_String):
        v = None
        try:
            v = float(o_o.v)
        except ValueError:
            vm.raise_helper("Number_Exception", [o_o])
        return Con_Float(vm, v)


@con_object_proc
def _Con_Float_div(vm):
    (self, o_o),_ = vm.decode_args("!N", self_of=Con_Float)
    assert isinstance(self, Con_Float)
    assert isinstance(o_o, Con_Number)
    
    return Con_Float(vm, self.v / o_o.as_float())


@con_object_proc
def _Con_Float_mul(vm):
    (self, o_o),_ = vm.decode_args("!N", self_of=Con_Float)
    assert isinstance(self, Con_Float)
    assert isinstance(o_o, Con_Number)
    
    return Con_Float(vm, self.v * o_o.as_float())


@con_object_proc
def _Con_Float_to_str(vm):
    (self,),_ = vm.decode_args("!", self_of=Con_Float)
    assert isinstance(self, Con_Float)

    return Con_String(vm, str(self.v))


def bootstrap_con_float(vm):
    float_class = vm.get_builtin(BUILTIN_FLOAT_CLASS)
    assert isinstance(float_class, Con_Class)
    float_class.new_func = \
      new_c_con_func(vm, Con_String(vm, "new_Float"), False, _new_func_Con_Float, \
        vm.get_builtin(BUILTIN_BUILTINS_MODULE))

    new_c_con_func_for_class(vm, "/", _Con_Float_div, float_class)
    new_c_con_func_for_class(vm, "*", _Con_Float_mul, float_class)
    new_c_con_func_for_class(vm, "to_str", _Con_Float_to_str, float_class)




################################################################################
# Con_String
#

class Con_String(Con_Boxed_Object):
    __slots__ = ("v",)
    _immutable_fields_ = ("v",)


    def __init__(self, vm, v):
        Con_Boxed_Object.__init__(self, vm, vm.get_builtin(BUILTIN_STRING_CLASS))
        assert v is not None
        self.v = v


    def add(self, vm, o):
        o = type_check_string(vm, o)
        return Con_String(vm, self.v + o.v)


    def eq(self, vm, o):
        if isinstance(o, Con_String):
            return self.v == o.v
        return False


    def neq(self, vm, o):
        if isinstance(o, Con_String):
            return self.v != o.v
        return True


    def le(self, vm, o):
        o = type_check_string(vm, o)
        return self.v < o.v


    def le_eq(self, vm, o):
        o = type_check_string(vm, o)
        return self.v <= o.v


    def gr_eq(self, vm, o):
        o = type_check_string(vm, o)
        return self.v >= o.v


    def gt(self, vm, o):
        o = type_check_string(vm, o)
        return self.v > o.v


    def get_slice(self, vm, i, j):
        i, j = translate_slice_idxs(vm, i, j, len(self.v))
        return Con_String(vm, self.v[i:j])


@con_object_proc
def _Con_String_add(vm):
    (self, o_o),_ = vm.decode_args("SS")
    assert isinstance(self, Con_String)
    assert isinstance(o_o, Con_String)
    
    return Con_String(vm, self.v + o_o.v)


@con_object_proc
def _Con_String_eq(vm):
    (self, o_o),_ = vm.decode_args("SO")
    assert isinstance(self, Con_String)
    
    if isinstance(o_o, Con_String):
        if self.v == o_o.v:
            return vm.get_builtin(BUILTIN_NULL_OBJ)
    return vm.get_builtin(BUILTIN_FAIL_OBJ)


@con_object_gen
def _Con_String_find(vm):
    (self, o_o),_ = vm.decode_args("SS")
    assert isinstance(self, Con_String)
    assert isinstance(o_o, Con_String)

    v = self.v
    o = o_o.v
    o_len = len(o)
    for i in range(0, len(v) - o_len + 1):
        if v[i:i+o_len] == o:
            yield o_o


@con_object_gen
def _Con_String_find_index(vm):
    (self, o_o),_ = vm.decode_args("SS")
    assert isinstance(self, Con_String)
    assert isinstance(o_o, Con_String)

    v = self.v
    o = o_o.v
    o_len = len(o)
    for i in range(0, len(v) - o_len + 1):
        if v[i:i+o_len] == o:
            yield Con_Int(vm, i)


@con_object_proc
def _Con_String_get(vm):
    (self, i_o),_ = vm.decode_args("SI")
    assert isinstance(self, Con_String)
    assert isinstance(i_o, Con_Int)

    return Con_String(vm, self.v[i_o.v])


@con_object_proc
def _Con_String_get_slice(vm):
    (self, i_o, j_o),_ = vm.decode_args("S", opt="ii")
    assert isinstance(self, Con_String)

    i, j = translate_slice_idx_objs(vm, i_o, j_o, len(self.v))

    return Con_String(vm, self.v[i:j])


@con_object_proc
def _Con_String_hash(vm):
    (self,),_ = vm.decode_args("S")
    assert isinstance(self, Con_String)

    return Con_Int(vm, objectmodel.compute_hash(self.v))


@con_object_proc
def _Con_String_int_val(vm):
    (self, i_o),_ = vm.decode_args("S", opt="I")
    assert isinstance(self, Con_String)

    if i_o is not None:
        assert isinstance(i_o, Con_Int)
        i = translate_idx(vm, i_o.v, len(self.v))
    else:
        i = translate_idx(vm, 0, len(self.v))

    return Con_Int(vm, ord(self.v[i]))


@con_object_gen
def _Con_String_iter(vm):
    (self, i_o, j_o),_ = vm.decode_args("S", opt="ii")
    assert isinstance(self, Con_String)
    
    i, j = translate_slice_idx_objs(vm, i_o, j_o, len(self.v))
    while i < j:
        yield Con_String(vm, self.v[i])
        i += 1


@con_object_proc
def _Con_String_len(vm):
    (self,),_ = vm.decode_args("S")
    assert isinstance(self, Con_String)

    return Con_Int(vm, len(self.v))


@con_object_proc
def _Con_String_lower_cased(vm):
    (self,),_ = vm.decode_args("S")
    assert isinstance(self, Con_String)
    
    return Con_String(vm, self.v.lower())


@con_object_proc
def _Con_String_lstripped(vm):
    (self,),_ = vm.decode_args("S")
    assert isinstance(self, Con_String)
    
    v = self.v
    v_len = len(v)
    i = 0
    while i < v_len:
        if v[i] not in " \t\n\r":
            break
        i += 1

    return Con_String(vm, self.v[i:])


@con_object_proc
def _Con_String_mul(vm):
    (self, i_o),_ = vm.decode_args("SI")
    assert isinstance(self, Con_String)
    assert isinstance(i_o, Con_Int)
    
    return Con_String(vm, self.v * i_o.v)


@con_object_proc
def _Con_String_neq(vm):
    (self, o_o),_ = vm.decode_args("SO")
    assert isinstance(self, Con_String)

    if isinstance(o_o, Con_String):
        if self.v != o_o.v:
            return vm.get_builtin(BUILTIN_NULL_OBJ)
        else:
            return vm.get_builtin(BUILTIN_FAIL_OBJ)
    return vm.get_builtin(BUILTIN_NULL_OBJ)


@con_object_proc
def _Con_String_prefixed_by(vm):
    (self, o_o, i_o),_ = vm.decode_args("SS", opt="I")
    assert isinstance(self, Con_String)
    assert isinstance(o_o, Con_String)

    i = translate_slice_idx_obj(vm, i_o, len(self.v))

    if self.v[i:].startswith(o_o.v):
        return vm.get_builtin(BUILTIN_NULL_OBJ)
    else:
        return vm.get_builtin(BUILTIN_FAIL_OBJ)


@con_object_gen
def _Con_String_rfind_index(vm):
    (self, o_o),_ = vm.decode_args("SS")
    assert isinstance(self, Con_String)
    assert isinstance(o_o, Con_String)

    v = self.v
    o = o_o.v
    o_len = len(o)
    for i in range(len(v) - o_len, -1, -1):
        if v[i:i+o_len] == o:
            yield Con_Int(vm, i)


@con_object_proc
def _Con_String_replaced(vm):
    (self, old_o, new_o),_ = vm.decode_args("SSS")
    assert isinstance(self, Con_String)
    assert isinstance(old_o, Con_String)
    assert isinstance(new_o, Con_String)

    v = self.v
    v_len = len(v)
    old = old_o.v
    old_len = len(old)
    new = new_o.v
    out = []
    i = 0
    while i < v_len:
        j = v.find(old, i)
        if j == -1:
            break
        assert j >= i
        out.append(v[i:j])
        out.append(new)
        i = j + old_len
    if i < v_len:
        out.append(v[i:])

    return Con_String(vm, "".join(out))


@con_object_proc
def _Con_String_split(vm):
    (self, o_o),_ = vm.decode_args("SS")
    assert isinstance(self, Con_String)
    assert isinstance(o_o, Con_String)

    return Con_List(vm, [Con_String(vm, x) for x in self.v.split(o_o.v)])


@con_object_proc
def _Con_String_stripped(vm):
    (self,),_ = vm.decode_args("S")
    assert isinstance(self, Con_String)

    v = self.v
    v_len = len(v)
    i = 0
    while i < v_len:
        if v[i] not in " \t\n\r":
            break
        i += 1
    j = v_len - 1
    while j >= i:
        if v[j] not in " \t\n\r":
            break
        j -= 1
    j += 1

    assert j >= i
    return Con_String(vm, self.v[i:j])


@con_object_proc
def _Con_String_suffixed_by(vm):
    (self, o_o, i_o),_ = vm.decode_args("SS", opt="I")
    assert isinstance(self, Con_String)
    assert isinstance(o_o, Con_String)

    if i_o is None:
        i = len(self.v)
    else:
        i = translate_slice_idx_obj(vm, i_o, len(self.v))

    if self.v[:i].endswith(o_o.v):
        return vm.get_builtin(BUILTIN_NULL_OBJ)
    else:
        return vm.get_builtin(BUILTIN_FAIL_OBJ)


@con_object_proc
def _Con_String_to_str(vm):
    (self,),_ = vm.decode_args("S")
    assert isinstance(self, Con_String)

    return Con_String(vm, '"%s"' % self.v)


@con_object_proc
def _Con_String_upper_cased(vm):
    (self,),_ = vm.decode_args("S")
    assert isinstance(self, Con_String)
    
    return Con_String(vm, self.v.upper())


def bootstrap_con_string(vm):
    string_class = vm.get_builtin(BUILTIN_STRING_CLASS)
    assert isinstance(string_class, Con_Class)

    new_c_con_func_for_class(vm, "+", _Con_String_add, string_class)
    new_c_con_func_for_class(vm, "==", _Con_String_eq, string_class)
    new_c_con_func_for_class(vm, "find", _Con_String_find, string_class)
    new_c_con_func_for_class(vm, "find_index", _Con_String_find_index, string_class)
    new_c_con_func_for_class(vm, "get", _Con_String_get, string_class)
    new_c_con_func_for_class(vm, "get_slice", _Con_String_get_slice, string_class)
    new_c_con_func_for_class(vm, "hash", _Con_String_hash, string_class)
    new_c_con_func_for_class(vm, "int_val", _Con_String_int_val, string_class)
    new_c_con_func_for_class(vm, "iter", _Con_String_iter, string_class)
    new_c_con_func_for_class(vm, "len", _Con_String_len, string_class)
    new_c_con_func_for_class(vm, "lower_cased", _Con_String_lower_cased, string_class)
    new_c_con_func_for_class(vm, "lstripped", _Con_String_lstripped, string_class)
    new_c_con_func_for_class(vm, "*", _Con_String_mul, string_class)
    new_c_con_func_for_class(vm, "!=", _Con_String_neq, string_class)
    new_c_con_func_for_class(vm, "prefixed_by", _Con_String_prefixed_by, string_class)
    new_c_con_func_for_class(vm, "replaced", _Con_String_replaced, string_class)
    new_c_con_func_for_class(vm, "rfind_index", _Con_String_rfind_index, string_class)
    new_c_con_func_for_class(vm, "split", _Con_String_split, string_class)
    new_c_con_func_for_class(vm, "stripped", _Con_String_stripped, string_class)
    new_c_con_func_for_class(vm, "suffixed_by", _Con_String_suffixed_by, string_class)
    new_c_con_func_for_class(vm, "to_str", _Con_String_to_str, string_class)
    new_c_con_func_for_class(vm, "upper_cased", _Con_String_upper_cased, string_class)



################################################################################
# Con_List
#

class Con_List(Con_Boxed_Object):
    __slots__ = ("l",)
    _immutable_fields_ = ("l",)

    def __init__(self, vm, l, instance_of=None):
        assert None not in l
        if instance_of is None:
            instance_of = vm.get_builtin(BUILTIN_LIST_CLASS)
        Con_Boxed_Object.__init__(self, vm, instance_of)
        self.l = l


@con_object_proc
def _new_func_Con_List(vm):
    (class_,), vargs = vm.decode_args("C", vargs=True)
    
    o = Con_List(vm, [], class_)
    vm.apply(o.get_slot(vm, "init"), vargs)
    return o


@con_object_proc
def _Con_List_init(vm):
    (self, o_o,), _ = vm.decode_args("L", opt="O")
    assert isinstance(self, Con_List)
    
    if isinstance(o_o, Con_List):
        self.l = o_o.l[:]
    elif o_o:
        vm.pre_get_slot_apply_pump(o_o, "iter")
        while 1:
            e_o = vm.apply_pump()
            if not e_o:
                break
            self.l.append(e_o)

    return vm.get_builtin(BUILTIN_NULL_OBJ)


@con_object_proc
def _Con_List_add(vm):
    (self, o_o),_ = vm.decode_args("LO")
    assert isinstance(self, Con_List)
    
    if isinstance(o_o, Con_List):
        new_l = self.l + o_o.l
    else:
        new_l = self.l[:]
        vm.pre_get_slot_apply_pump(o_o, "iter")
        while 1:
            e_o = vm.apply_pump()
            if not e_o:
                break
            new_l.append(e_o)
    return Con_List(vm, new_l)


@con_object_proc
def _Con_List_append(vm):
    (self, o),_ = vm.decode_args("LO")
    assert isinstance(self, Con_List)
    
    self.l.append(o)
    return vm.get_builtin(Builtins.BUILTIN_NULL_OBJ)


@con_object_proc
def _Con_List_del(vm):
    (self, i_o),_ = vm.decode_args("LI")
    assert isinstance(self, Con_List)
    assert isinstance(i_o, Con_Int)

    del self.l[translate_idx(vm, i_o.v, len(self.l))]

    return vm.get_builtin(Builtins.BUILTIN_NULL_OBJ)


@con_object_proc
def _Con_List_del_slice(vm):
    (self, i_o, j_o),_ = vm.decode_args("L", opt="ii")
    assert isinstance(self, Con_List)
    assert isinstance(i_o, Con_Int)

    i, j = translate_slice_idx_objs(vm, i_o, j_o, len(self.l))
    del self.l[i:j]

    return vm.get_builtin(Builtins.BUILTIN_NULL_OBJ)


@con_object_proc
def _Con_List_extend(vm):
    (self, o_o),_ = vm.decode_args("LO")
    assert isinstance(self, Con_List)
    
    if isinstance(o_o, Con_List):
        self.l.extend(o_o.l)
    else:
        vm.pre_get_slot_apply_pump(o_o, "iter")
        while 1:
            e_o = vm.apply_pump()
            if not e_o:
                break
            self.l.append(e_o)
    return vm.get_builtin(Builtins.BUILTIN_NULL_OBJ)


@con_object_proc
def _Con_List_eq(vm):
    (self, o_o),_ = vm.decode_args("LO")
    assert isinstance(self, Con_List)
    
    if isinstance(o_o, Con_List):
        self_len = len(self.l)
        if self_len != len(o_o.l):
            return vm.get_builtin(Builtins.BUILTIN_FAIL_OBJ)

        self_l = self.l
        o_l = o_o.l
        for i in range(0, self_len):
            if not self_l[i].eq(vm, o_l[i]):
                return vm.get_builtin(Builtins.BUILTIN_FAIL_OBJ)
        return vm.get_builtin(Builtins.BUILTIN_NULL_OBJ)

    return vm.get_builtin(Builtins.BUILTIN_FAIL_OBJ)


@con_object_gen
def _Con_List_find(vm):
    (self, o),_ = vm.decode_args("LO")
    assert isinstance(self, Con_List)
    
    for e in self.l:
        if o.eq(vm, e):
            yield vm.get_builtin(BUILTIN_NULL_OBJ)


@con_object_gen
def _Con_List_find_index(vm):
    (self, o),_ = vm.decode_args("LO")
    assert isinstance(self, Con_List)
    
    i = 0
    for e in self.l:
        if e.eq(vm, o):
            yield Con_Int(vm, i)
        i += 1


@con_object_proc
def _Con_List_flattened(vm):
    (self,),_ = vm.decode_args("L")
    assert isinstance(self, Con_List)
    
    f = []
    for e in self.l:
        if isinstance(e, Con_List):
            f.extend(type_check_list(vm, vm.get_slot_apply(e, "flattened")).l)
        else:
            f.append(e)
    
    return Con_List(vm, f)


@con_object_proc
def _Con_List_get(vm):
    (self, i_o),_ = vm.decode_args("LI")
    assert isinstance(self, Con_List)
    assert isinstance(i_o, Con_Int)

    i = translate_idx(vm, i_o.v, len(self.l))
    
    return self.l[i]


@con_object_proc
def _Con_List_get_slice(vm):
    (self, i_o, j_o),_ = vm.decode_args("L", opt="ii")
    assert isinstance(self, Con_List)

    i, j = translate_slice_idx_objs(vm, i_o, j_o, len(self.l))

    return Con_List(vm, self.l[i:j])


@con_object_proc
def _Con_List_insert(vm):
    (self, i_o, o_o),_ = vm.decode_args("LIO")
    assert isinstance(self, Con_List)
    assert isinstance(i_o, Con_Int)
    
    self.l.insert(translate_slice_idx(vm, i_o.v, len(self.l)), o_o)

    return vm.get_builtin(BUILTIN_FAIL_OBJ)


@con_object_gen
def _Con_List_iter(vm):
    (self, i_o, j_o),_ = vm.decode_args("L", opt="ii")
    assert isinstance(self, Con_List)
    
    i, j = translate_slice_idx_objs(vm, i_o, j_o, len(self.l))
    while i < j:
        yield self.l[i]
        i += 1


@con_object_proc
def _Con_List_len(vm):
    (self,),_ = vm.decode_args("L")
    assert isinstance(self, Con_List)
    
    return Con_Int(vm, len(self.l))


@con_object_proc
def _Con_List_mult(vm):
    (self, i_o),_ = vm.decode_args("LI")
    assert isinstance(self, Con_List)
    assert isinstance(i_o, Con_Int)

    return Con_List(vm, self.l * i_o.v)


@con_object_proc
def _Con_List_neq(vm):
    (self, o_o),_ = vm.decode_args("LL")
    assert isinstance(self, Con_List)
    
    if isinstance(o_o, Con_List):
        self_len = len(self.l)
        if self_len != len(o_o.l):
            return vm.get_builtin(Builtins.BUILTIN_NULL_OBJ)

        self_l = self.l
        o_l = o_o.l
        for i in range(0, self_len):
            if not self_l[i].neq(vm, o_l[i]):
                return vm.get_builtin(Builtins.BUILTIN_FAIL_OBJ)
        return vm.get_builtin(Builtins.BUILTIN_NULL_OBJ)
    else:
        return vm.get_builtin(Builtins.BUILTIN_NULL_OBJ)


@con_object_proc
def _Con_List_pop(vm):
    (self,),_ = vm.decode_args("L")
    assert isinstance(self, Con_List)
    
    translate_slice_idx(vm, -1, len(self.l))

    return self.l.pop()


@con_object_gen
def _Con_List_remove(vm):
    (self, o_o),_ = vm.decode_args("LO")
    assert isinstance(self, Con_List)

    i = 0
    l = self.l
    while i < len(l):
        e = l[i]
        if o_o.eq(vm, e):
            del l[i]
            yield e
        else:
            i += 1


@con_object_gen
def _Con_List_riter(vm):
    (self, i_o, j_o),_ = vm.decode_args("L", opt="ii")
    assert isinstance(self, Con_List)
    
    i, j = translate_slice_idx_objs(vm, i_o, j_o, len(self.l))
    j -= 1
    while j >= i:
        yield self.l[j]
        j -= 1


@con_object_proc
def _Con_List_set(vm):
    (self, i, o),_ = vm.decode_args("LIO")
    assert isinstance(self, Con_List)
    assert isinstance(i, Con_Int)
    self.l[i.v] = o
    return vm.get_builtin(Builtins.BUILTIN_NULL_OBJ)


@con_object_proc
def _Con_List_set_slice(vm):
    (self, i_o, j_o, o_o),_ = vm.decode_args("LiiL")
    assert isinstance(self, Con_List)
    assert isinstance(o_o, Con_List)

    i, j = translate_slice_idx_objs(vm, i_o, j_o, len(self.l))
    # Setting slices in RPython is currently broken.
    # self.l[i:j] = o_o.l
    # For the time, use a slow but simple work around.
    del self.l[i:j]
    for e in o_o.l:
        self.l.insert(i, e)
        i += 1

    return vm.get_builtin(Builtins.BUILTIN_NULL_OBJ)


@con_object_proc
def _Con_List_to_str(vm):
    (self,),_ = vm.decode_args("L")
    assert isinstance(self, Con_List)
    
    es = []
    for e in self.l:
        s = type_check_string(vm, vm.get_slot_apply(e, "to_str"))
        es.append(s.v)

    return Con_String(vm, "[%s]" % ", ".join(es))


def bootstrap_con_list(vm):
    list_class = vm.get_builtin(BUILTIN_LIST_CLASS)
    assert isinstance(list_class, Con_Class)
    list_class.new_func = \
      new_c_con_func(vm, Con_String(vm, "new_List"), False, _new_func_Con_List, \
        vm.get_builtin(BUILTIN_BUILTINS_MODULE))

    new_c_con_func_for_class(vm, "init", _Con_List_init, list_class)
    new_c_con_func_for_class(vm, "+", _Con_List_add, list_class)
    new_c_con_func_for_class(vm, "append", _Con_List_append, list_class)
    new_c_con_func_for_class(vm, "del", _Con_List_del, list_class)
    new_c_con_func_for_class(vm, "del_slice", _Con_List_del_slice, list_class)
    new_c_con_func_for_class(vm, "extend", _Con_List_extend, list_class)
    new_c_con_func_for_class(vm, "==", _Con_List_eq, list_class)
    new_c_con_func_for_class(vm, "find", _Con_List_find, list_class)
    new_c_con_func_for_class(vm, "find_index", _Con_List_find_index, list_class)
    new_c_con_func_for_class(vm, "flattened", _Con_List_flattened, list_class)
    new_c_con_func_for_class(vm, "get", _Con_List_get, list_class)
    new_c_con_func_for_class(vm, "get_slice", _Con_List_get_slice, list_class)
    new_c_con_func_for_class(vm, "insert", _Con_List_insert, list_class)
    new_c_con_func_for_class(vm, "iter", _Con_List_iter, list_class)
    new_c_con_func_for_class(vm, "len", _Con_List_len, list_class)
    new_c_con_func_for_class(vm, "*", _Con_List_mult, list_class)
    new_c_con_func_for_class(vm, "!=", _Con_List_neq, list_class)
    new_c_con_func_for_class(vm, "pop", _Con_List_pop, list_class)
    new_c_con_func_for_class(vm, "remove", _Con_List_remove, list_class)
    new_c_con_func_for_class(vm, "riter", _Con_List_riter, list_class)
    new_c_con_func_for_class(vm, "set", _Con_List_set, list_class)
    new_c_con_func_for_class(vm, "set_slice", _Con_List_set_slice, list_class)
    new_c_con_func_for_class(vm, "to_str", _Con_List_to_str, list_class)



################################################################################
# Con_Set
#

class Con_Set(Con_Boxed_Object):
    __slots__ = ("s", "vm")
    _immutable_fields_ = ("s",)


    def __init__(self, vm, l, instance_of=None):
        if instance_of is None:
            instance_of = vm.get_builtin(BUILTIN_SET_CLASS)
        Con_Boxed_Object.__init__(self, vm, instance_of)
        # RPython doesn't have sets, so we use dictionaries for the time being
        self.s = objectmodel.r_dict(_dict_key_eq, _dict_key_hash)
        for e in l:
            self.s[e] = None


@con_object_proc
def _Con_Set_add(vm):
    (self, o),_ = vm.decode_args("WO")
    assert isinstance(self, Con_Set)
    
    self.s[o] = None

    return vm.get_builtin(Builtins.BUILTIN_NULL_OBJ)


@con_object_proc
def _Con_Set_add_plus(vm):
    (self, o_o),_ = vm.decode_args("WO")
    assert isinstance(self, Con_Set)
    
    n_o = Con_Set(vm, self.s.keys())
    vm.get_slot_apply(n_o, "extend", [o_o])

    return n_o


@con_object_proc
def _Con_Set_complement(vm):
    (self, o_o),_ = vm.decode_args("WO")
    assert isinstance(self, Con_Set)

    n_s = []
    for k in self.s.keys():
        if isinstance(o_o, Con_Set):
            if k not in o_o.s:
                n_s.append(k)
        else:
            raise Exception("XXX")
    
    return Con_Set(vm, n_s)


@con_object_proc
def _Con_Set_extend(vm):
    (self, o_o),_ = vm.decode_args("WO")
    assert isinstance(self, Con_Set)

    if isinstance(o_o, Con_Set):
        for k in o_o.s.keys():
            self.s[k] = None
    else:
        vm.pre_get_slot_apply_pump(o_o, "iter")
        while 1:
            e_o = vm.apply_pump()
            if not e_o:
                break
            self.s[e_o] = None

    return vm.get_builtin(Builtins.BUILTIN_NULL_OBJ)


@con_object_gen
def _Con_Set_find(vm):
    (self, o),_ = vm.decode_args("WO")
    assert isinstance(self, Con_Set)
    
    if o in self.s:
        yield o


@con_object_gen
def _Con_Set_iter(vm):
    (self,),_ = vm.decode_args("W")
    assert isinstance(self, Con_Set)
    
    for k in self.s.keys():
        yield k


@con_object_proc
def _Con_Set_len(vm):
    (self,),_ = vm.decode_args("W")
    assert isinstance(self, Con_Set)
    
    return Con_Int(vm, len(self.s))


@con_object_proc
def _Con_Set_scopy(vm):
    (self,),_ = vm.decode_args("W")
    assert isinstance(self, Con_Set)
    
    return Con_Set(vm, self.s.keys())


@con_object_proc
def _Con_Set_to_str(vm):
    (self,),_ = vm.decode_args("W")
    assert isinstance(self, Con_Set)
    
    es = []
    for e in self.s.keys():
        s = type_check_string(vm, vm.get_slot_apply(e, "to_str"))
        es.append(s.v)

    return Con_String(vm, "Set{%s}" % ", ".join(es))


def bootstrap_con_set(vm):
    set_class = vm.get_builtin(BUILTIN_SET_CLASS)
    assert isinstance(set_class, Con_Class)

    new_c_con_func_for_class(vm, "add", _Con_Set_add, set_class)
    new_c_con_func_for_class(vm, "+", _Con_Set_add_plus, set_class)
    new_c_con_func_for_class(vm, "complement", _Con_Set_complement, set_class)
    new_c_con_func_for_class(vm, "extend", _Con_Set_extend, set_class)
    new_c_con_func_for_class(vm, "find", _Con_Set_find, set_class)
    new_c_con_func_for_class(vm, "iter", _Con_Set_iter, set_class)
    new_c_con_func_for_class(vm, "len", _Con_Set_len, set_class)
    new_c_con_func_for_class(vm, "scopy", _Con_Set_scopy, set_class)
    new_c_con_func_for_class(vm, "to_str", _Con_Set_to_str, set_class)



################################################################################
# Con_Dict
#

class Con_Dict(Con_Boxed_Object):
    __slots__ = ("d",)
    _immutable_fields_ = ("d",)


    def __init__(self, vm, l, instance_of=None):
        if instance_of is None:
            instance_of = vm.get_builtin(BUILTIN_DICT_CLASS)
        Con_Boxed_Object.__init__(self, vm, instance_of)
        self.d = objectmodel.r_dict(_dict_key_eq, _dict_key_hash)
        i = 0
        while i < len(l):
            self.d[l[i]] = l[i + 1]
            i += 2


def _dict_key_hash(k):
    vm = VM.global_vm # XXX Offensively gross hack!
    return int(Builtins.type_check_int(vm, vm.get_slot_apply(k, "hash")).v)


def _dict_key_eq(k1, k2):
    vm = VM.global_vm # XXX Offensively gross hack!
    if vm.get_slot_apply(k1, "==", [k2], allow_fail=True):
        return True
    else:
        return False


@con_object_proc
def _Con_Dict_find(vm):
    (self, k),_ = vm.decode_args("DO")
    assert isinstance(self, Con_Dict)
    
    r = self.d.get(k, None)
    if r is None:
        return vm.get_builtin(BUILTIN_FAIL_OBJ)
    
    return r


@con_object_proc
def _Con_Dict_extend(vm):
    (self, o),_ = vm.decode_args("DD")
    assert isinstance(self, Con_Dict)
    assert isinstance(o, Con_Dict)
    
    for k, v in o.d.items():
        self.d[k] = v
    
    return vm.get_builtin(BUILTIN_FAIL_OBJ)


@con_object_proc
def _Con_Dict_get(vm):
    (self, k),_ = vm.decode_args("DO")
    assert isinstance(self, Con_Dict)
    
    r = self.d.get(k, None)
    if r is None:
        vm.raise_helper("Key_Exception", [k])
    
    return r


@con_object_gen
def _Con_Dict_iter(vm):
    (self,),_ = vm.decode_args("D")
    assert isinstance(self, Con_Dict)

    for k, v in self.d.items():
        yield Con_List(vm, [k, v])


@con_object_gen
def _Con_Dict_iter_keys(vm):
    (self,),_ = vm.decode_args("D")
    assert isinstance(self, Con_Dict)

    for v in self.d.keys():
        yield v


@con_object_gen
def _Con_Dict_iter_vals(vm):
    (self,),_ = vm.decode_args("D")
    assert isinstance(self, Con_Dict)

    for v in self.d.values():
        yield v


@con_object_proc
def _Con_Dict_len(vm):
    (self,),_ = vm.decode_args("D")
    assert isinstance(self, Con_Dict)
    
    return Con_Int(vm, len(self.d))


@con_object_proc
def _Con_Dict_set(vm):
    (self, k, v),_ = vm.decode_args("DOO")
    assert isinstance(self, Con_Dict)
    
    self.d[k] = v
    
    return vm.get_builtin(BUILTIN_NULL_OBJ)


@con_object_proc
def _Con_Dict_scopy(vm):
    (self,),_ = vm.decode_args("D")
    assert isinstance(self, Con_Dict)

    n_o = Con_Dict(vm, [])
    for k, v in self.d.items():
        n_o.d[k] = v

    return n_o


@con_object_proc
def _Con_Dict_to_str(vm):
    (self,),_ = vm.decode_args("D")
    assert isinstance(self, Con_Dict)
    
    es = []
    for k, v in self.d.items():
        ks = type_check_string(vm, vm.get_slot_apply(k, "to_str"))
        vs = type_check_string(vm, vm.get_slot_apply(v, "to_str"))
        es.append("%s : %s" % (ks.v, vs.v))

    return Con_String(vm, "Dict{%s}" % ", ".join(es))


def bootstrap_con_dict(vm):
    dict_class = vm.get_builtin(BUILTIN_DICT_CLASS)
    assert isinstance(dict_class, Con_Class)

    new_c_con_func_for_class(vm, "extend", _Con_Dict_extend, dict_class)
    new_c_con_func_for_class(vm, "find", _Con_Dict_find, dict_class)
    new_c_con_func_for_class(vm, "get", _Con_Dict_get, dict_class)
    new_c_con_func_for_class(vm, "iter", _Con_Dict_iter, dict_class)
    new_c_con_func_for_class(vm, "iter_keys", _Con_Dict_iter_keys, dict_class)
    new_c_con_func_for_class(vm, "iter_vals", _Con_Dict_iter_vals, dict_class)
    new_c_con_func_for_class(vm, "len", _Con_Dict_len, dict_class)
    new_c_con_func_for_class(vm, "scopy", _Con_Dict_scopy, dict_class)
    new_c_con_func_for_class(vm, "set", _Con_Dict_set, dict_class)
    new_c_con_func_for_class(vm, "to_str", _Con_Dict_to_str, dict_class)



################################################################################
# Con_Exception
#

class Con_Exception(Con_Boxed_Object):
    __slots__ = ("call_chain",)
    _immutable_fields_ = ()


    def __init__(self, vm, instance_of=None):
        if instance_of is None:
            instance_of = vm.get_builtin(BUILTIN_EXCEPTION_CLASS)
        Con_Boxed_Object.__init__(self, vm, instance_of)
        self.call_chain = None


@con_object_proc
def _new_func_Con_Exception(vm):
    (class_, ), vargs = vm.decode_args("C", vargs=True)
    o = Con_Exception(vm, class_)
    vm.apply(o.get_slot(vm, "init"), vargs)
    return o


@con_object_proc
def _Con_Exception_init(vm):
    (self, msg),_ = vm.decode_args("O", opt="O")
    if msg is None:
        self.set_slot(vm, "msg", Con_String(vm, ""))
    else:
        self.set_slot(vm, "msg", msg)
    return vm.get_builtin(Builtins.BUILTIN_NULL_OBJ)


@con_object_gen
def _Con_Exception_iter_call_chain(vm):
    (self,),_ = vm.decode_args("E")
    assert isinstance(self, Con_Exception)

    for pc, func, bc_off in self.call_chain:
        if isinstance(pc, BC_PC):
            src_infos = pc.mod.bc_off_to_src_infos(vm, bc_off)
        else:
            assert isinstance(pc, Py_PC)
            src_infos = vm.get_builtin(BUILTIN_NULL_OBJ)
        yield Con_List(vm, [func, src_infos])


@con_object_proc
def _Con_Exception_to_str(vm):
    (self,),_ = vm.decode_args("E")
    ex_name = type_check_string(vm, self.get_slot(vm, "instance_of").get_slot(vm, "name"))
    msg = type_check_string(vm, self.get_slot(vm, "msg"))
    return Con_String(vm, "%s: %s" % (ex_name.v, msg.v))


def bootstrap_con_exception(vm):
    exception_class = vm.get_builtin(BUILTIN_EXCEPTION_CLASS)
    assert isinstance(exception_class, Con_Class)
    exception_class.new_func = \
      new_c_con_func(vm, Con_String(vm, "new_Exception"), False, _new_func_Con_Exception, \
        vm.get_builtin(BUILTIN_BUILTINS_MODULE))

    new_c_con_func_for_class(vm, "init", _Con_Exception_init, exception_class)
    new_c_con_func_for_class(vm, "iter_call_chain", _Con_Exception_iter_call_chain, exception_class)
    new_c_con_func_for_class(vm, "to_str", _Con_Exception_to_str, exception_class)



################################################################################
# Convenience type checking functions
#

# Note that the returning of the object passed for type-checking is just about convenience -
# calling functions can safely ignore the return value if that's easier for them. However,
# if ignored, RPython doesn't infer that the object pointed to by o is of the correct type,
# so generally one wants to write:
#
#   o = type_check_X(vm, z)


def type_check_class(vm, o):
    if not isinstance(o, Con_Class):
        vm.raise_helper("Type_Exception", [vm.get_builtin(BUILTIN_CLASS_CLASS), o])
    return o


def type_check_dict(vm, o):
    if not isinstance(o, Con_Dict):
        vm.raise_helper("Type_Exception", [Con_String(vm, "Dict"), o])
    return o


def type_check_exception(vm, o):
    if not isinstance(o, Con_Exception):
        vm.raise_helper("Type_Exception", [Con_String(vm, "Exception"), o])
    return o


def type_check_number(vm, o):
    if not (isinstance(o, Con_Int) or isinstance(o, Con_Float)):
        vm.raise_helper("Type_Exception", [Con_String(vm, "Number"), o])
    return o


def type_check_int(vm, o):
    if not isinstance(o, Con_Int):
        vm.raise_helper("Type_Exception", [Con_String(vm, "Int"), o])
    return o


def type_check_func(vm, o):
    if not isinstance(o, Con_Func):
        vm.raise_helper("Type_Exception", [Con_String(vm, "Func"), o])
    return o


def type_check_list(vm, o):
    if not isinstance(o, Con_List):
        vm.raise_helper("Type_Exception", [Con_String(vm, "List"), o])
    return o


def type_check_module(vm, o):
    if not isinstance(o, Con_Module):
        vm.raise_helper("Type_Exception", [Con_String(vm, "Module"), o])
    return o


def type_check_set(vm, o):
    if not isinstance(o, Con_Set):
        vm.raise_helper("Type_Exception", [Con_String(vm, "Set"), o])
    return o


def type_check_string(vm, o):
    if not isinstance(o, Con_String):
        vm.raise_helper("Type_Exception", [Con_String(vm, "String"), o])
    return o
