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

from pypy.config.pypyoption import get_pypy_config
from pypy.rlib import debug, jit, objectmodel
from pypy.rpython.lltypesystem import lltype, llmemory, rffi

from Core import *
import Builtins, Target




DEBUG = False



def get_printable_location(bc_off, mod_bc, pc, self):
    assert isinstance(pc, BC_PC)
    instr = Target.read_word(mod_bc, bc_off)
    it = Target.get_instr(instr)
    return "%s:%s at offset %s. bytecode: %s" % (pc.mod, pc.off, bc_off, Target.INSTR_NAMES[it])

jitdriver = jit.JitDriver(greens=["bc_off", "mod_bc", "pc", "self"],
                          reds=["prev_bc_off", "cf"],
                          virtualizables=["cf"],
                          get_printable_location=get_printable_location)


class VM(object):
    __slots__ = ("argv", "builtins", "cur_cf", "mods", "pypy_config", "vm_path")
    _immutable_fields = ("argv", "builtins", "cur_cf", "mods", "vm_path")

    def __init__(self): 
        self.builtins = [None] * Builtins.NUM_BUILTINS
        self.mods = {}
        self.cur_cf = None # Current continuation frame
        self.pypy_config = None


    def init(self, vm_path,argv):
        self.vm_path = vm_path
        self.argv = argv
        
        Builtins.bootstrap_con_object(self)
        Builtins.bootstrap_con_class(self)
        Builtins.bootstrap_con_dict(self)
        Builtins.bootstrap_con_func(self)
        Builtins.bootstrap_con_int(self)
        Builtins.bootstrap_con_float(self)
        Builtins.bootstrap_con_list(self)
        Builtins.bootstrap_con_module(self)
        Builtins.bootstrap_con_partial_application(self)
        Builtins.bootstrap_con_set(self)
        Builtins.bootstrap_con_string(self)
        Builtins.bootstrap_con_exception(self)

        import Modules
        for init_func in Modules.BUILTIN_MODULES:
            self.set_mod(init_func(self))

        self.get_mod("Exceptions").import_(self)
        self.get_mod("POSIX_File").import_(self)
        self.get_mod("Sys").import_(self)


    ################################################################################################
    # Generic helper functions
    #


    @jit.elidable
    def get_builtin(self, i):
        if DEBUG:
            assert self.builtins[i] is not None # Builtins can not be read before they are set
        return self.builtins[i]


    def set_builtin(self, i, o):
        if DEBUG:
            assert self.builtins[i] is None # Once set, a builtin can never change
        self.builtins[i] = o


    def set_mod(self, mod):
        self.mods[mod.id_] = mod


    def find_mod(self, mod_id):
        return self.mods.get(mod_id, None)


    def get_mod(self, mod_id):
        m = self.find_mod(mod_id)
        if m is None:
            self.raise_helper("Import_Exception", [Builtins.Con_String(self, mod_id)])
        return m


    def import_stdlib_mod(self, ptl_mod_id):
        if not ptl_mod_id.startswith(os.sep):
            return self.get_mod(ptl_mod_id)
        
        for cnd_mod_id in self.mods.keys():
            bt_cnd_mod_id = cnd_mod_id

            # XXX. The next two operations are pure evil and are basically a poor-man's
            # bootstrapping. They're intended to convert between module IDs across different
            # platforms. The potential problems with the below are fairly obvious, although
            # unlikely to actually manifest themselves in real life.
            if CASE_SENSITIVE_FILENAMES == 0:
                bt_cnd_mod_id = bt_cnd_mod_id.lower()

            if os.sep == "/":
                bt_cnd_mod_id = bt_cnd_mod_id.replace("\\", "/")
            elif os.sep == "\\":
                bt_cnd_mod_id = bt_cnd_mod_id.replace("/", "\\")
            else:
                self.raise_helper("VM_Exception", [Con_String(vm, "Unknown separator %s." % os.sep)])

            if bt_cnd_mod_id.endswith(ptl_mod_id):
                mod = self.get_mod(cnd_mod_id)
                mod.import_(self)
                return mod

        self.raise_helper("Import_Exception", [Builtins.Con_String(self, ptl_mod_id)])


    def has_mod(self, mod_id):
        return mod_id in self.mods



    ################################################################################################
    # Core VM functions
    #

    def apply(self, func, args=None, allow_fail=False):
        o, _ = self.apply_closure(func, args, allow_fail=allow_fail)
        return o


    def get_slot_apply(self, o, n, args=None, allow_fail=False):
        return self.apply(o.get_slot(self, n), args, allow_fail=allow_fail)


    def apply_closure(self, func, args=None, allow_fail=False):
        if not args:
            nargs = 0
        else:
            nargs = len(args)
        
        if isinstance(func, Builtins.Con_Partial_Application):
            cf = self._add_continuation_frame(func.f, nargs + 1)
            cf.stack_extend(func.args)
        else: 
            cf = self._add_continuation_frame(func, nargs)

        if args:
            cf.stack_extend(list(args))

        o = self.execute_proc(cf)
        self._remove_continuation_frame()
        
        if o is self.get_builtin(Builtins.BUILTIN_FAIL_OBJ):
            o = None
        if not allow_fail and o is None:
            self.raise_helper("VM_Exception", [Builtins.Con_String(self, \
              "Function attempting to return fail, but caller can not handle failure.")])

        return o, cf.closure


    def pre_get_slot_apply_pump(self, o, n, args=None):
        return self.pre_apply_pump(o.get_slot(self, n), args)


    def pre_apply_pump(self, func, args=None):
        if not args:
            nargs = 0
        else:
            nargs = len(args)

        cf = self.cur_cf
        gf = Stack_Generator_Frame(cf.gfp, -1)
        cf.gfp = cf.stackpe
        cf.stack_push(gf)
        if isinstance(func, Builtins.Con_Partial_Application):
            new_cf = self._add_continuation_frame(func.f, nargs + 1)
            new_cf.stack_extend(func.args)
        else: 
            new_cf = self._add_continuation_frame(func, nargs)

        if args:
            new_cf.stack_extend(list(args))


    def apply_pump(self, remove_generator_frames=False):
        # At this point, we're in one of two situations:
        #
        #   1) A generator frame has been added but not yet used. So the con stacks look like:
        #        calling function: [..., <generator frame>]
        #        callee function:  [<argument objects>]
        #
        #   2) We need to resume a previous generator. The con stack looks like:
        #        [..., <generator frame>, <random objects>]
        #
        # Fortunately there's an easy way to distinguish the two: if the current continuation on the
        # stack has a generator frame we're in situation #2, otherwise we're in situation #1.

        cf = self.cur_cf
        if cf.gfp == -1:
            # We're in case 1) from above.
            gen = self.execute_gen(cf)
            cf = self.cur_cf
        else:
            # We're in case 2) from above.
            cf.stack_del_from(cf.gfp + 1)
            gf = cf.stack_get(cf.gfp)
            assert isinstance(gf, Stack_Generator_Frame)
            if gf.returned:
                return None
            cf = gf.saved_cf
            assert cf.parent is self.cur_cf
            self.cur_cf = cf
            gen = gf.gen

        try:
            o = gen.next()
            assert o is not None
        except StopIteration:
            cf.returned = True
            o = None

        if cf.returned or o is None:
            self._remove_continuation_frame()
            cf = self.cur_cf
            gf = cf.stack_get(cf.gfp)
            assert isinstance(gf, Stack_Generator_Frame)
            self._remove_generator_frame(cf)
        else:
            saved_cf = self.cur_cf
            self.cur_cf = cf = saved_cf.parent

            # At this point cf.stack looks like:
            #   [..., <gen obj 1>, ..., <gen obj n>, <generator frame>]

            gf = cf.stack_get(cf.gfp)
            assert isinstance(gf, Stack_Generator_Frame)
            gf.gen = gen
            gf.saved_cf = saved_cf
            if gf.prev_gfp > cf.ffp:
                i = gf.prev_gfp + 1
            else:
                i = cf.ffp + 1
            j = cf.gfp
            assert i >= 0 and j >= i
            cf.stack_extend(cf.stack_get_slice(i, j))

            # At this point cf.stack looks like:
            #   [..., <gen obj 1>, ..., <gen obj n>, <generator frame>, <gen obj 1>, ...,
            #     <gen obj n>]
        
        if o is self.get_builtin(Builtins.BUILTIN_FAIL_OBJ):
            # Currently the failure of a function is signalled in the bytecode by returning the
            # FAIL object.
            o = None

        return o


    @jit.unroll_safe
    def decode_args(self, mand="", opt="", vargs=False, self_of=None):
        cf = self.cur_cf
        nargs = cf.nargs # Number of arguments passed
        
        mand = jit.promote_string(mand)
        opt = jit.promote_string(opt)
        self_of = jit.promote(self_of)

        if nargs < len(mand):
            if vargs:
                self.raise_helper("Parameters_Exception", [Builtins.Con_String(self, \
                  "Too few parameters (%d passed, but at least %d needed)." % (nargs, len(mand)))])
            else:
                self.raise_helper("Parameters_Exception", [Builtins.Con_String(self, \
                  "Too few parameters (%d passed, but %d needed)." % (nargs, len(mand)))])
        elif nargs > (len(mand) + len(opt)) and not vargs:
            raise Exception("XXX")

        if nargs == 0:
            if vargs:
                return (None, [])
            else:
                return (None, None)
        
        nrmp = [None] * (len(mand) + len(opt)) # Normal params
        i = 0
        while i < (len(mand) + len(opt)):
            if i >= nargs:
                for j in range(i, nargs):
                    nrmp[j] = None
                break

            if i < len(mand):
                t = mand[i]
            else:
                t = opt[i - len(mand)]
        
            o = cf.stack_get(cf.stackpe - nargs + i)
            
            if t == "!":
                assert self_of is not None
                if not isinstance(o, self_of):
                    raise Exception("XXX")
                nrmp[i] = o
                i += 1
                continue
            
            if t >= "a":
                if o is self.get_builtin(Builtins.BUILTIN_NULL_OBJ):
                    nrmp[i] = None
                    i += 1
                    continue
                t = chr(ord("A") + ord(t) - ord("a"))
        
            if t == "O":
                nrmp[i] = o
            else:
                if t == "C":
                    Builtins.type_check_class(self, o)
                elif t == "D":
                    Builtins.type_check_dict(self, o)
                elif t == "E":
                    Builtins.type_check_exception(self, o)
                elif t == "F":
                    Builtins.type_check_func(self, o)
                elif t == "I":
                    Builtins.type_check_int(self, o)
                elif t == "L":
                    Builtins.type_check_list(self, o)
                elif t == "M":
                    Builtins.type_check_module(self, o)
                elif t == "N":
                    Builtins.type_check_number(self, o)
                elif t == "S":
                    Builtins.type_check_string(self, o)
                elif t == "W":
                    Builtins.type_check_set(self, o)
                else:
                    print t
                    raise Exception("XXX")
                nrmp[i] = o
            
            i += 1

        if vargs:
            vap = [None] * (nargs - i)
            for j in range(i, nargs):
                vap[j - i] = cf.stack_get(cf.stackpe - nargs + j)
        else:
            vap = None

        cf.stack_del_from(cf.stackpe - nargs)
        
        return (nrmp, vap)


    def get_funcs_mod(self):
        cf = self.cur_cf
        return cf.pc.mod


    def get_mod_and_bc_off(self, i):
        cf = self.cur_cf
        while i >= 0:
            cf = cf.parent
            i -= 1
        mod = cf.pc.mod
        if mod.is_bc:
            return mod, cf.bc_off
        return mod, -1


    def raise_(self, ex):
        ex = Builtins.type_check_exception(self, ex)
        if ex.call_chain is None:
            cc = [] # Call chain
            cf = self.cur_cf
            while cf is not None:
                cc.append((cf.pc, cf.func, cf.bc_off))
                cf = cf.parent
            ex.call_chain = cc
        raise Con_Raise_Exception(ex)


    def raise_helper(self, ex_name, args=None):
        if args is None:
            args = []

        ex_mod = self.get_builtin(Builtins.BUILTIN_EXCEPTIONS_MODULE)
        assert isinstance(ex_mod, Builtins.Con_Module)
        ex = self.get_slot_apply(ex_mod.get_defn(self, ex_name), "new", args)
        self.raise_(ex)


    ################################################################################################
    # The interepreter
    #

    def execute_proc(self, cf):
        pc = cf.pc
        if isinstance(pc, Py_PC):
            f = pc.f(self)
            if isinstance(f, Class_Con_Proc):
                try:
                    o = f.next()
                    assert o is not None
                    cf.returned = True
                    return o
                except Con_Raise_Exception, e:
                    if cf.xfp == -1:
                        # There is no exception handler, so kill this continuation frame and propagate
                        # the exception
                        self._remove_continuation_frame()
                        raise
                    raise Exception("XXX")
            assert isinstance(f, Class_Con_Gen)
            try:
                o = f.next()
                assert o is not None
            except StopIteration:
                cf.returned = True
                return
            except Con_Raise_Exception, e:
                if cf.xfp == -1:
                    # There is no exception handler, so kill this continuation frame and propagate
                    # the exception
                    self._remove_continuation_frame()
                    raise
                raise Exception("XXX")
            assert not cf.returned or o is not None
            return o
        else:
            assert isinstance(pc, BC_PC)
            cf.bc_off = pc.off
            return self.bc_loop(cf)


    def execute_gen(self, cf):
        pc = cf.pc
        if isinstance(pc, Py_PC):
            f = pc.f(self)
            if isinstance(f, Class_Con_Proc):
                try:
                    o = f.next()
                    assert o is not None
                    cf.returned = True
                    yield o
                except Con_Raise_Exception, e:
                    if cf.xfp == -1:
                        # There is no exception handler, so kill this continuation frame and propagate
                        # the exception
                        self._remove_continuation_frame()
                        raise
                    raise Exception("XXX")
            assert isinstance(f, Class_Con_Gen)
            while 1:
                assert not cf.returned
                try:
                    o = f.next()
                    assert o is not None
                except StopIteration:
                    cf.returned = True
                    return
                except Con_Raise_Exception, e:
                    if cf.xfp == -1:
                        # There is no exception handler, so kill this continuation frame and propagate
                        # the exception
                        self._remove_continuation_frame()
                        raise
                    raise Exception("XXX")
                assert not cf.returned or o is not None
                yield o
        else:
            assert isinstance(pc, BC_PC)
            cf.bc_off = pc.off
            while 1:
                assert not cf.returned
                yield self.bc_loop(cf)


    def bc_loop(self, cf):
        pc = cf.pc
        mod_bc = pc.mod.bc
        prev_bc_off = -1
        while 1:
            bc_off = cf.bc_off
            if prev_bc_off != -1 and prev_bc_off > bc_off:
                jitdriver.can_enter_jit(bc_off=bc_off, mod_bc=mod_bc, cf=cf, prev_bc_off=prev_bc_off, pc=pc, self=self)
            jitdriver.jit_merge_point(bc_off=bc_off, mod_bc=mod_bc, cf=cf, prev_bc_off=prev_bc_off, pc=pc, self=self)
            assert cf is self.cur_cf
            prev_bc_off = bc_off
            instr = Target.read_word(mod_bc, bc_off)
            it = Target.get_instr(instr)

            try:
                #x = cf.stackpe; assert x >= 0; print "%s %s %d [stackpe:%d ffp:%d gfp:%d xfp:%d]" % (Target.INSTR_NAMES[instr & 0xFF], str(cf.stack[:x]), bc_off, cf.stackpe, cf.ffp, cf.gfp, cf.xfp)
                if it == Target.CON_INSTR_EXBI:
                    self._instr_exbi(instr, cf)
                elif it == Target.CON_INSTR_VAR_LOOKUP:
                    self._instr_var_lookup(instr, cf)
                elif it == Target.CON_INSTR_VAR_ASSIGN:
                    self._instr_var_assign(instr, cf)
                elif it == Target.CON_INSTR_ADD_FAILURE_FRAME:
                    self._instr_add_failure_frame(instr, cf)
                elif it == Target.CON_INSTR_ADD_FAIL_UP_FRAME:
                    self._instr_add_fail_up_frame(instr, cf)
                elif it == Target.CON_INSTR_REMOVE_FAILURE_FRAME:
                    self._instr_remove_failure_frame(instr, cf)
                elif it == Target.CON_INSTR_IS_ASSIGNED:
                    self._instr_is_assigned(instr, cf)
                elif it == Target.CON_INSTR_IS:
                    self._instr_is(instr, cf)
                elif it == Target.CON_INSTR_FAIL_NOW:
                    self._instr_fail_now(instr, cf)
                elif it == Target.CON_INSTR_POP:
                    self._instr_pop(instr, cf)
                elif it == Target.CON_INSTR_LIST:
                    self._instr_list(instr, cf)
                elif it == Target.CON_INSTR_SLOT_LOOKUP:
                    self._instr_slot_lookup(instr, cf)
                elif it == Target.CON_INSTR_APPLY:
                    self._instr_apply(instr, cf)
                elif it == Target.CON_INSTR_FUNC_DEFN:
                    self._instr_func_defn(instr, cf)
                elif it == Target.CON_INSTR_RETURN:
                    cf.returned = True
                    return cf.stack_pop()
                elif it == Target.CON_INSTR_BRANCH:
                    self._instr_branch(instr, cf)
                elif it == Target.CON_INSTR_YIELD:
                    cf.bc_off += Target.INTSIZE
                    return cf.stack_get(cf.stackpe - 1)
                elif it == Target.CON_INSTR_IMPORT:
                    self._instr_import(instr, cf)
                elif it == Target.CON_INSTR_DICT:
                    self._instr_dict(instr, cf)
                elif it == Target.CON_INSTR_DUP:
                    self._instr_dup(instr, cf)
                elif it == Target.CON_INSTR_PULL:
                    self._instr_pull(instr, cf)
                elif it == Target.CON_INSTR_BUILTIN_LOOKUP:
                    self._instr_builtin_lookup(instr, cf)
                elif it == Target.CON_INSTR_ASSIGN_SLOT:
                    self._instr_assign_slot(instr, cf)
                elif it == Target.CON_INSTR_EYIELD:
                    self._instr_eyield(instr, cf)
                elif it == Target.CON_INSTR_ADD_EXCEPTION_FRAME:
                    self._instr_add_exception_frame(instr, cf)
                elif it == Target.CON_INSTR_REMOVE_EXCEPTION_FRAME:
                    self._instr_remove_exception_frame(instr, cf)
                elif it == Target.CON_INSTR_RAISE:
                    self._instr_raise(instr, cf)
                elif it == Target.CON_INSTR_UNPACK_ARGS:
                    self._instr_unpack_args(instr, cf)
                elif it == Target.CON_INSTR_SET:
                    self._instr_set(instr, cf)
                elif it == Target.CON_INSTR_CONST_GET:
                    self._instr_const_get(instr, cf)
                elif it == Target.CON_INSTR_PRE_SLOT_LOOKUP_APPLY:
                    # In the C Converge VM, this instruction is used to avoid a very expensive path
                    # through the VM; it's currently unclear whether this VM will suffer from the
                    # same problem. Until we are more sure, we simply use the normal slot lookup
                    # function, which has the correct semantics, but may perhaps not be fully
                    # optimised.
                    self._instr_slot_lookup(instr, cf)
                elif it == Target.CON_INSTR_UNPACK_ASSIGN:
                    self._instr_unpack_assign(instr, cf)
                elif it == Target.CON_INSTR_BRANCH_IF_NOT_FAIL:
                    self._instr_branch_if_not_fail(instr, cf)
                elif it == Target.CON_INSTR_BRANCH_IF_FAIL:
                    self._instr_branch_if_fail(instr, cf)
                elif it == Target.CON_INSTR_EQ or it == Target.CON_INSTR_LE \
                  or it == Target.CON_INSTR_NEQ or it == Target.CON_INSTR_LE_EQ \
                  or it == Target.CON_INSTR_GR_EQ or it == Target.CON_INSTR_GT:
                    self._instr_cmp(instr, cf)
                elif it == Target.CON_INSTR_ADD or it == Target.CON_INSTR_SUBTRACT:
                    self._instr_calc(instr, cf)
                elif it == Target.CON_INSTR_MODULE_LOOKUP:
                    self._instr_module_lookup(instr, cf)
                else:
                    #print it, cf.stack
                    raise Exception("XXX")
            except Con_Raise_Exception, e:
                # An exception has been raised and is working its way up the call chain. Each bc_loop
                # catches the Con_Raise_Exception and either a) kills its continuation and passes the
                # exception up a level b) deals with it.
                if cf.xfp == -1:
                    # There is no exception handler, so kill this continuation frame and propagate
                    # the exception
                    self._remove_continuation_frame()
                    raise
                # We have an exception handler, so deal with it.
                ef = cf.stack_get(cf.xfp)
                assert isinstance(ef, Stack_Exception_Frame)
                self._remove_exception_frame(cf)
                cf.stack_push(e.ex_obj)
                cf.bc_off = ef.bc_off


    def _instr_exbi(self, instr, cf):
        class_ = Builtins.type_check_class(self, cf.stack_pop())
        bind_o = cf.stack_pop()
        nm_start, nm_size = Target.unpack_exbi(instr)
        nm = Target.extract_str(cf.pc.mod.bc, nm_start + cf.bc_off, nm_size)
        pa = Builtins.Con_Partial_Application(self, class_.get_field(self, nm), [bind_o])
        cf.stack_push(pa)
        cf.bc_off += Target.align(nm_start + nm_size)


    @jit.unroll_safe
    def _instr_var_lookup(self, instr, cf):
        closure_off, var_num = Target.unpack_var_lookup(instr)
        closure = cf.closure
        while closure_off > 0:
            closure = closure.parent
            closure_off -= 1
        v = closure.vars[var_num]
        if not v:
            self.raise_helper("Unassigned_Var_Exception")
        cf.stack_push(v)
        cf.bc_off += Target.INTSIZE


    @jit.unroll_safe
    def _instr_var_assign(self, instr, cf):
        closure_off, var_num = Target.unpack_var_assign(instr)
        closure = cf.closure
        while closure_off > 0:
            closure = closure.parent
            closure_off -= 1
        closure.vars[var_num] = cf.stack_get(cf.stackpe - 1)
        cf.bc_off += Target.INTSIZE


    def _instr_add_failure_frame(self, instr, cf):
        off = Target.unpack_add_failure_frame(instr)
        self._add_failure_frame(cf, False, cf.bc_off + off)
        cf.bc_off += Target.INTSIZE


    def _instr_add_fail_up_frame(self, instr, cf):
        self._add_failure_frame(cf, True)
        cf.bc_off += Target.INTSIZE


    def _instr_remove_failure_frame(self, instr, cf):
        self._remove_failure_frame(cf)
        cf.bc_off += Target.INTSIZE


    @jit.unroll_safe
    def _instr_is_assigned(self, instr, cf):
        closure_off, var_num = Target.unpack_var_lookup(instr)
        closure = cf.closure
        while closure_off > 0:
            closure = closure.parent
            closure_off -= 1
        v = closure.vars[var_num]
        if closure.vars[var_num] is not None:
            pc = cf.pc
            assert isinstance(pc, BC_PC)
            mod_bc = pc.mod.bc
            instr2 = Target.read_word(mod_bc, cf.bc_off + Target.INTSIZE)
            cf.bc_off += Target.unpack_is_assigned(instr2)
        else:
            cf.bc_off += Target.INTSIZE + Target.INTSIZE


    def _instr_is(self, instr, cf):
        o1 = cf.stack_pop()
        o2 = cf.stack_pop()
        if not o1.is_(o2):
            self._fail_now(cf)
            return
        cf.stack_push(o2)
        cf.bc_off += Target.INTSIZE


    def _instr_pop(self, instr, cf):
        cf.stack_pop()
        cf.bc_off += Target.INTSIZE


    def _instr_list(self, instr, cf):
        ne = Target.unpack_list(instr)
        l = cf.stack_get_slice_del(cf.stackpe - ne)
        cf.stack_push(Builtins.Con_List(self, l))
        cf.bc_off += Target.INTSIZE


    def _instr_slot_lookup(self, instr, cf):
        o = cf.stack_pop()
        nm_start, nm_size = Target.unpack_slot_lookup(instr)
        nm = Target.extract_str(cf.pc.mod.bc, nm_start + cf.bc_off, nm_size)
        cf.stack_push(o.get_slot(self, nm))
        cf.bc_off += Target.align(nm_start + nm_size)


    @jit.unroll_safe
    def _instr_apply(self, instr, cf):
        ff = cf.stack_get(cf.ffp)
        assert isinstance(ff, Stack_Failure_Frame)
        num_args = Target.unpack_apply(instr)
        fp = cf.stackpe - num_args - 1
        func = cf.stack_get(fp)

        if isinstance(func, Builtins.Con_Partial_Application):
            new_cf = self._add_continuation_frame(func.f, num_args + 1)
            new_cf.stack_extend(func.args)
            i = 1
        else:
            new_cf = self._add_continuation_frame(func, num_args)
            i = 0

        for j in range(0, num_args):
            k = i + num_args - j - 1
            assert k >= 0
            jit.promote(k)
            new_cf.stack[k] = cf.stack_pop()
        new_cf.stackpe = i + num_args

        if ff.is_fail_up:
            gf = Stack_Generator_Frame(cf.gfp, cf.bc_off + Target.INTSIZE)
            cf.stack_set(fp, gf)
            cf.gfp = fp
            o = self.apply_pump()
        else:
            cf.stack_pop() # Function pointer
            o = self.execute_proc(new_cf)
            self._remove_continuation_frame()
            
            if o is self.get_builtin(Builtins.BUILTIN_FAIL_OBJ):
                o = None

        if o is None:
            self._fail_now(cf)
            return
        cf.stack_push(o)
        cf.bc_off += Target.INTSIZE


    def _instr_fail_now(self, instr, cf):
        self._fail_now(cf)


    def _instr_func_defn(self, instr, cf):
        is_bound, max_stack_size = Target.unpack_func_defn(instr)
        np_o = cf.stack_pop()
        assert isinstance(np_o, Builtins.Con_Int)
        nv_o = cf.stack_pop()
        assert isinstance(nv_o, Builtins.Con_Int)
        name = cf.stack_pop()
        new_pc = BC_PC(cf.pc.mod, cf.bc_off + 2 * Target.INTSIZE)
        container = cf.func.get_slot(self, "container")
        f = Builtins.Con_Func(self, name, is_bound, new_pc, max_stack_size, np_o.v, nv_o.v, \
          container, cf.closure)
        cf.stack_push(f)
        cf.bc_off += Target.INTSIZE


    def _instr_branch(self, instr, cf):
        cf.bc_off += Target.unpack_branch(instr)


    def _instr_import(self, instr, cf):
        mod = self.get_mod(cf.pc.mod.imps[Target.unpack_import(instr)])
        mod.import_(self)
        cf.stack_push(mod)
        cf.bc_off += Target.INTSIZE


    def _instr_dict(self, instr, cf):
        ne = Target.unpack_dict(instr)
        l = cf.stack_get_slice_del(cf.stackpe - ne * 2)
        cf.stack_push(Builtins.Con_Dict(self, l))
        cf.bc_off += Target.INTSIZE


    def _instr_dup(self, instr, cf):
        cf.stack_push(cf.stack_get(cf.stackpe - 1))
        cf.bc_off += Target.INTSIZE


    def _instr_pull(self, instr, cf):
        i = Target.unpack_pull(instr)
        cf.stack_push(cf.stack_pop_n(i))
        cf.bc_off += Target.INTSIZE


    def _instr_builtin_lookup(self, instr, cf):
        bl = Target.unpack_builtin_lookup(instr)
        cf.stack_push(self.get_builtin(bl))
        cf.bc_off += Target.INTSIZE


    def _instr_assign_slot(self, instr, cf):
        o = cf.stack_pop()
        v = cf.stack_get(cf.stackpe - 1)
        nm_start, nm_size = Target.unpack_assign_slot(instr)
        nm = Target.extract_str(cf.pc.mod.bc, nm_start + cf.bc_off, nm_size)
        o.set_slot(self, nm, v)
        cf.bc_off += Target.align(nm_start + nm_size)


    def _instr_eyield(self, instr, cf):
        o = cf.stack_pop()
        is_fail_up, resume_bc_off = self._read_failure_frame(cf)
        self._remove_failure_frame(cf)
        prev_gfp = cf.gfp
        egf = Stack_Generator_EYield_Frame(prev_gfp, resume_bc_off)
        cf.gfp = cf.stackpe
        cf.stack_push(egf)
        # At this point the Con_Stack looks like:
        #   [..., <gen obj 1>, ..., <gen obj n>, <eyield frame>]
        gen_objs_s = prev_gfp + 1 # start of generator objects
        if cf.ffp > gen_objs_s:
            gen_objs_s = cf.ffp + 1
        gen_objs_e = cf.stackpe - 1
        assert gen_objs_s >= 0
        assert gen_objs_e >= gen_objs_s
        cf.stack_extend(cf.stack_get_slice(gen_objs_s, gen_objs_e))
        cf.stack_push(o)
        cf.bc_off += Target.INTSIZE


    def _instr_add_exception_frame(self, instr, cf):
        j = Target.unpack_add_exception_frame(instr)
        self._add_exception_frame(cf, cf.bc_off + j)
        cf.bc_off += Target.INTSIZE


    def _instr_remove_exception_frame(self, instr, cf):
        self._remove_exception_frame(cf)
        cf.bc_off += Target.INTSIZE


    def _instr_raise(self, instr, cf):
        self.raise_(cf.stack_pop())


    @jit.unroll_safe
    def _instr_unpack_args(self, instr, cf):
        num_fargs, has_vargs = Target.unpack_unpack_args(instr)
        num_fargs = jit.promote(num_fargs)
        has_vargs = jit.promote(has_vargs)
        if not has_vargs:
            nargs = jit.promote(cf.nargs)
        else:
            nargs = cf.nargs
        if nargs > num_fargs and not has_vargs:
            msg = "Too many parameters (%d passed, but a maximum of %d allowed)." % \
              (nargs, num_fargs)
            self.raise_helper("Parameters_Exception", [Builtins.Con_String(self, msg)])

        if num_fargs > 0:
            arg_offset = cf.bc_off + Target.INTSIZE + num_fargs * Target.INTSIZE
            for i in range(num_fargs - 1, -1, -1):
                arg_offset -= Target.INTSIZE
                arg_info = Target.read_word(cf.pc.mod.bc, arg_offset)
                if i >= nargs:
                    if not Target.unpack_unpack_args_is_mandatory(arg_info):
                        msg = "No value passed for parameter %d." % (i + 1)
                        self.raise_helper("Parameters_Exception", [Builtins.Con_String(self, msg)])
                else:
                    if nargs > num_fargs:
                        o = cf.stack_pop_n(nargs - num_fargs)
                    else:
                        o = cf.stack_pop()
                    assert isinstance(o, Builtins.Con_Object)
                    cf.closure.vars[Target.unpack_unpack_args_arg_num(arg_info)] = o

        if has_vargs:
            arg_offset = cf.bc_off + Target.INTSIZE + num_fargs * Target.INTSIZE
            arg_info = Target.read_word(cf.pc.mod.bc, arg_offset)
            if nargs <= num_fargs:
                l = []
            else:
                j = cf.stackpe
                i = j - (nargs - num_fargs)
                assert i >= 0 and j >= 0
                l = cf.stack_get_slice(i, j)
                cf.stackpe = i + 1
            cf.closure.vars[Target.unpack_unpack_args_arg_num(arg_info)] = Builtins.Con_List(self, l)
            cf.bc_off += Target.INTSIZE + (num_fargs + 1) * Target.INTSIZE
        else:
            cf.bc_off += Target.INTSIZE + num_fargs * Target.INTSIZE


    def _instr_set(self, instr, cf):
        ne = Target.unpack_set(instr)
        l = cf.stack_get_slice_del(cf.stackpe - ne)
        cf.stack_push(Builtins.Con_Set(self, l))
        cf.bc_off += Target.INTSIZE


    def _instr_const_get(self, instr, cf):
        const_num = Target.unpack_constant_get(instr)
        cf.stack_push(cf.pc.mod.get_const(self, const_num))
        cf.bc_off += Target.INTSIZE


    @jit.unroll_safe
    def _instr_unpack_assign(self, instr, cf):
        o = cf.stack_get(cf.stackpe - 1)
        o = Builtins.type_check_list(self, o)
        ne = len(o.l)
        if ne != Target.unpack_unpack_assign(instr):
            self.raise_helper("Unpack_Exception", \
              [Builtins.Con_Int(self, Target.unpack_unpack_assign(instr)), \
               Builtins.Con_Int(self, ne)])
        for i in range(ne - 1, -1, -1):
            cf.stack_push(o.l[i])
        cf.bc_off += Target.INTSIZE


    def _instr_branch_if_not_fail(self, instr, cf):
        if cf.stack_pop() is self.get_builtin(Builtins.BUILTIN_FAIL_OBJ):
            cf.bc_off += Target.INTSIZE
        else:
            j = Target.unpack_branch_if_not_fail(instr)
            cf.bc_off += j


    def _instr_branch_if_fail(self, instr, cf):
        if cf.stack_pop() is not self.get_builtin(Builtins.BUILTIN_FAIL_OBJ):
            cf.bc_off += Target.INTSIZE
        else:
            j = Target.unpack_branch_if_not_fail(instr)
            cf.bc_off += j


    def _instr_cmp(self, instr, cf):
        rhs = cf.stack_pop()
        lhs = cf.stack_pop()
        
        it = Target.get_instr(instr)
        if it == Target.CON_INSTR_EQ:
            r = lhs.eq(self, rhs)
        elif it == Target.CON_INSTR_LE:
            r = lhs.le(self, rhs)
        elif it == Target.CON_INSTR_NEQ:
            r = lhs.neq(self, rhs)
        elif it == Target.CON_INSTR_LE_EQ:
            r = lhs.le_eq(self, rhs)
        elif it == Target.CON_INSTR_GR_EQ:
            r = lhs.gr_eq(self, rhs)
        elif it == Target.CON_INSTR_GT:
            r = lhs.gt(self, rhs)
        else:
            raise Exception("XXX")
        
        if r:
            cf.stack_push(rhs)
            cf.bc_off += Target.INTSIZE
        else:
            self._fail_now(cf)


    def _instr_calc(self, instr, cf):
        rhs = cf.stack_pop()
        lhs = cf.stack_pop()
        
        it = Target.get_instr(instr)
        if it == Target.CON_INSTR_ADD:
            r = lhs.add(self, rhs)
        else:
            assert it == Target.CON_INSTR_SUBTRACT
            r = lhs.subtract(self, rhs)

        cf.stack_push(r)
        cf.bc_off += Target.INTSIZE


    def _instr_module_lookup(self, instr, cf):
        o = cf.stack_pop()
        nm_start, nm_size = Target.unpack_mod_lookup(instr)
        nm = Target.extract_str(cf.pc.mod.bc, cf.bc_off + nm_start, nm_size)
        if isinstance(o, Builtins.Con_Module):
            v = o.get_defn(self, nm)
        else:
            v = self.get_slot_apply(o, "get_defn", [Builtins.Con_String(self, nm)])
        cf.stack_push(v)
        cf.bc_off += Target.align(nm_start + nm_size)


    ################################################################################################
    # Frame operations
    #
    
    def _add_continuation_frame(self, func, nargs):
        if not isinstance(func, Builtins.Con_Func):
            self.raise_helper("Apply_Exception", [func])
        func = jit.promote(func) # XXX this will promote lambdas, which will be inefficient

        pc = func.pc
        if isinstance(pc, BC_PC):
            bc_off = pc.off
        else:
            bc_off = -1 

        closure = Closure(func.container_closure, func.num_vars)

        if func.max_stack_size > nargs:
            max_stack_size = func.max_stack_size
        elif nargs == 0:
            # We make the stack size at least 1 so that RPython functions have room for 1 generator
            # frame. If they need more than that, they'll have to be clever.
            max_stack_size = 1
        else:
            max_stack_size = nargs

        cf = Stack_Continuation_Frame(self.cur_cf, func, pc, max_stack_size, nargs, bc_off,
          closure)
        self.cur_cf = cf
        
        return cf


    def _remove_continuation_frame(self):
        self.cur_cf = self.cur_cf.parent


    def _remove_generator_frame(self, cf):
        gf = cf.stack_get(cf.gfp)
        cf.stack_del_from(cf.gfp)
        if isinstance(gf, Stack_Generator_Frame):
            cf.gfp = gf.prev_gfp
        else:
            assert isinstance(gf, Stack_Generator_EYield_Frame)
            cf.gfp = gf.prev_gfp


    def _add_failure_frame(self, cf, is_fail_up, new_off=-1):
        ff = Stack_Failure_Frame()

        ff.is_fail_up = is_fail_up
        ff.prev_ffp = cf.ffp
        ff.prev_gfp = cf.gfp
        ff.fail_to_off = new_off

        cf.gfp = -1
        cf.ffp = cf.stackpe
        cf.stack_push(ff)
        

    @jit.unroll_safe
    def _remove_failure_frame(self, cf):
        ffp = cf.ffp
        ff = cf.stack_get(ffp)
        assert isinstance(ff, Stack_Failure_Frame)
        cf.stack_del_from(ffp)
        cf.ffp = ff.prev_ffp
        cf.gfp = ff.prev_gfp


    def _read_failure_frame(self, cf):
        assert cf.ffp >= 0
        ff = cf.stack_get(cf.ffp)
        assert isinstance(ff, Stack_Failure_Frame)

        return (ff.is_fail_up, ff.fail_to_off)


    @jit.unroll_safe
    def _fail_now(self, cf):
        while 1:
            is_fail_up, fail_to_off = self._read_failure_frame(cf)
            if is_fail_up:
                if cf.gfp == -1:
                    self._remove_failure_frame(cf)
                    continue
                has_rg = False
                gf = cf.stack_get(cf.gfp)
                if isinstance(gf, Stack_Generator_EYield_Frame):
                    self._remove_generator_frame(cf)
                    cf.bc_off = gf.resume_bc_off
                    return
                else:
                    assert isinstance(gf, Stack_Generator_Frame)
                    o = self.apply_pump(True)
                    if o is not None:
                        cf = self.cur_cf
                        cf.stack_push(o)
                        cf.bc_off = gf.resume_bc_off
                        return
            else:
                cf.bc_off = fail_to_off
                self._remove_failure_frame(cf)
                return


    def _add_exception_frame(self, cf, bc_off):
        ef = Stack_Exception_Frame(bc_off, cf.ffp, cf.gfp, cf.xfp)
        cf.xfp = cf.stackpe
        cf.stack_push(ef)


    def _remove_exception_frame(self, cf):
        ef = cf.stack_get(cf.xfp)
        cf.stack_del_from(cf.xfp)
        assert isinstance(ef, Stack_Exception_Frame)
        cf.ffp = ef.prev_ffp
        cf.gfp = ef.prev_gfp
        cf.xfp = ef.prev_xfp



####################################################################################################
# Stack frame classes
#

class Stack_Continuation_Frame(Con_Thingy):
    __slots__ = ("parent", "stack", "stackpe", "func", "pc", "nargs", "bc_off", "closure", "ffp",
      "gfp", "xfp", "returned")
    _immutable_fields_ = ("parent", "stack", "ff_cache", "func", "closure", "pc", "nargs")
    _virtualizable2_ = ("parent", "bc_off", "stack[*]", "closure", "stackpe", "ffp", "gfp")

    def __init__(self, parent, func, pc, max_stack_size, nargs, bc_off, closure):
        self = jit.hint(self, access_directly=True, fresh_virtualizable=True)
        self.parent = parent
        self.stack = [None] * max_stack_size
        debug.make_sure_not_resized(self.stack)
        self.func = func
        self.pc = pc
        self.nargs = nargs # Number of arguments passed to this continuation
        self.bc_off = bc_off # -1 for Py modules
        self.closure = closure
        self.returned = False

        # stackpe always points to the element *after* the end of the stack (this makes a lot of
        # stack-based operations quicker)
        self.stackpe = 0
        # ffp, gfp, and xfp all point *to* the frame they refer to
        self.ffp = self.gfp = self.xfp = -1


    def stack_get(self, i):
        assert i >= 0
        jit.promote(i)
        return self.stack[i]


    @jit.unroll_safe
    def stack_get_slice(self, i, j):
        assert i >= 0 and j >= i
        l = [None] * (j - i)
        a = 0
        jit.promote(i)
        for k in range(i, j):
            l[a] = self.stack[k]
            a += 1
        return l


    @jit.unroll_safe
    def stack_get_slice_del(self, i):
        assert i >= 0
        l = [None] * (self.stackpe - i)
        a = 0
        jit.promote(i)
        for k in range(i, self.stackpe):
            l[a] = self.stack[k]
            self.stack[k] = None
            a += 1
        self.stackpe = i
        return l


    def stack_set(self, i, o):
        assert i >= 0
        jit.promote(i)
        self.stack[i] = o


    @jit.unroll_safe
    def stack_extend(self, l):
        for x in l:
            self.stack_set(self.stackpe, x)
            self.stackpe += 1


    def stack_push(self, x):
        assert self.stackpe < len(self.stack)
        self.stack_set(self.stackpe, x)
        self.stackpe += 1


    def stack_pop(self):
        assert self.stackpe > 0
        self.stackpe -= 1
        o = self.stack_get(self.stackpe)
        self.stack_set(self.stackpe, None)
        return o


    # Pop an item n items from the end of self.stack.

    @jit.unroll_safe
    def stack_pop_n(self, n):
        assert n < self.stackpe
        i = self.stackpe - 1 - n
        o = self.stack_get(i)
        # Shuffle the stack down
        self.stackpe -= 1
        for j in range(i, self.stackpe):
            self.stack_set(j, self.stack_get(j + 1))
        self.stack_set(self.stackpe, None)
        # If the frame pointers come after the popped item, they need to be rewritten
        if self.ffp > i:
            self.ffp -= 1
        if self.gfp > i:
            self.gfp -= 1
        if self.xfp > i:
            self.xfp -= 1

        return o


    @jit.unroll_safe
    def stack_del_from(self, i):
        for j in range(i, self.stackpe):
            self.stack_set(j, None)
        self.stackpe = i



class Stack_Failure_Frame(Con_Thingy):
    __slots__ = ("is_fail_up", "prev_ffp", "prev_gfp", "fail_to_off")
    # Because failure frames can be reused, they have no immutable fields.

    def __repr__(self):
        if self.is_fail_up:
            return "<Fail up frame>"
        else:
            return "<Failure frame>"


class Stack_Generator_Frame(Con_Thingy):
    __slots__ = ("prev_gfp", "resume_bc_off", "returned", "gen", "saved_cf")
    _immutable_fields_ = ("prev_gfp", "resume_bc_off")
    
    def __init__(self, prev_gfp, resume_bc_off):
        self.prev_gfp = prev_gfp
        self.resume_bc_off = resume_bc_off
        self.returned = False


class Stack_Generator_EYield_Frame(Con_Thingy):
    __slots__ = ("prev_gfp", "resume_bc_off")
    _immutable_fields_ = ("prev_gfp", "resume_bc_off")
    
    def __init__(self, prev_gfp, resume_bc_off):
        self.prev_gfp = prev_gfp
        self.resume_bc_off = resume_bc_off


class Stack_Exception_Frame(Con_Thingy):
    __slots__ = ("bc_off", "prev_ffp", "prev_gfp", "prev_xfp")
    _immutable_fields_ = ("bc_off", "prev_ffp", "prev_gfp", "prev_xfp")
    
    def __init__(self, bc_off, prev_ffp, prev_gfp, prev_xfp):
        self.bc_off = bc_off
        self.prev_ffp = prev_ffp
        self.prev_gfp = prev_gfp
        self.prev_xfp = prev_xfp



####################################################################################################
# Misc
#

class Closure:
    __slots__ = ("parent", "vars")
    _immutable_fields = ("parent", "vars")

    def __init__(self, parent, num_vars):
        self.parent = parent
        self.vars = [None] * num_vars
        debug.make_sure_not_resized(self.vars)


class Con_Raise_Exception(Exception):
    _immutable_ = True

    def __init__(self, ex_obj):
        self.ex_obj = ex_obj


global_vm = VM()

def new_vm(vm_path, argv):
    global_vm.init(vm_path, argv)
    return global_vm
