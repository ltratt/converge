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


from pypy.config.pypyoption import get_pypy_config
from pypy.rlib import debug, jit
from pypy.rlib.rstacklet import StackletThread
from pypy.rpython.lltypesystem import lltype, llmemory, rffi

from Core import *
import Builtins, Target




DEBUG = False



def get_printable_location(bc_off, mod_bc, pc):
    instr = Target.read_word(mod_bc, bc_off)
    it = Target.get_instr(instr)
    return "%s:%s at offset %s. bytecode: %s" % (pc.mod, pc.off, bc_off, Target.INSTR_NAMES[it])

jitdriver = jit.JitDriver(greens=["bc_off", "mod_bc", "pc"], reds=["prev_bc_off", "cf", "self"],
                          get_printable_location=get_printable_location)


class VM(object):
    __slots__ = ("builtins", "cf_stack", "mods", "spare_ff", "pypy_config", "st")
    _immutable_fields = ("builtins", "cf_stack", "mods")

    def __init__(self): 
        self.builtins = [None] * Builtins.NUM_BUILTINS
        self.mods = {}
        self.cf_stack = []
        # Continually allocating and deallocating failure frames is expensive, which is particularly
        # annoying as most never have any impact. spare_ff is used as a simple one-off cache of a
        # failure frame object, so that instead of being continually allocated and deallocated, in
        # many cases we can move a pointer from spare_ff to a continuation frame's stack, which is
        # cheap.
        self.spare_ff = None
        self.pypy_config = None


    def init(self):
        self.st = StackletThread(self.pypy_config)
        
        import Modules
        for init_func in Modules.BUILTIN_MODULES:
            self.set_mod(init_func(self))

        Builtins.bootstrap_con_object(self)
        Builtins.bootstrap_con_class(self)
        Builtins.bootstrap_con_dict(self)
        Builtins.bootstrap_con_int(self)
        Builtins.bootstrap_con_list(self)
        Builtins.bootstrap_con_module(self)
        Builtins.bootstrap_con_set(self)
        Builtins.bootstrap_con_string(self)
        Builtins.bootstrap_con_exception(self)

        self.get_mod("Exceptions").import_(self)
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


    def get_mod(self, mod_id):
        m = self.mods.get(mod_id, None)
        if m is None:
            self.raise_helper("Import_Exception", [Builtins.Con_String(self, mod_id)])
        if not m.initialized:
            m.import_(self)
        return m


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
            self._cf_stack_push(cf, func.o)
        else: 
            cf = self._add_continuation_frame(func, nargs)

        if args:
            self._cf_stack_extend(cf, list(args))

        try:
            self.execute(self.st.get_null_handle())
        except Con_Return_Exception:
            pass
        
        self._remove_continuation_frame()
        
        o = self._cf_stack_pop(cf)
        if o is self.get_builtin(Builtins.BUILTIN_FAIL_OBJ):
            o = None
        if not allow_fail and o is None:
            raise Exception("XXX")
        
        return o, cf.closure[-1]


    def _apply_pump(self, remove_generator_frames=False):
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

        cf = self.cf_stack[-1]
        if cf.gfp == -1:
            # We're in case 1) from above.
            ct = self.st.new(switch_hack)
            cf = self.cf_stack[-1]
        else:
            # We're in case 2) from above.
            self._cf_stack_del_from(cf, cf.gfp + 1)
            gf = cf.stack[cf.gfp]
            assert isinstance(gf, Stack_Generator_Frame)
            if gf.returned:
                return None
            cf = gf.saved_cf
            self.cf_stack.append(cf)
            ct = self.st.switch(gf.ct)

        o = self._cf_stack_pop(cf)
        if cf.returned:
            self.st.destroy(ct)
            self._remove_continuation_frame()
            self._remove_generator_frame(self.cf_stack[-1])
        else:
            saved_cf = self.cf_stack.pop()
            cf = self.cf_stack[-1]

            # At this point cf.stack looks like:
            #   [..., <gen obj 1>, ..., <gen obj n>, <generator frame>]

            gf = cf.stack[cf.gfp]
            assert isinstance(gf, Stack_Generator_Frame)
            gf.ct = ct
            gf.saved_cf = saved_cf
            if gf.prev_gfp > cf.ffp:
                i = gf.prev_gfp + 1
            else:
                i = cf.ffp + 1
            j = cf.gfp
            assert i >= 0 and j >= i
            self._cf_stack_extend(cf, cf.stack[i : j])

            # At this point cf.stack looks like:
            #   [..., <gen obj 1>, ..., <gen obj n>, <generator frame>, <gen obj 1>, ...,
            #     <gen obj n>]
        
        if o is self.get_builtin(Builtins.BUILTIN_FAIL_OBJ):
            # Currently the failure of a function is signalled in the bytecode by returning the
            # FAIL object.
            return None

        return o


    @jit.unroll_safe
    def decode_args(self, mand="", opt="", vargs=False):
        cf = self.cf_stack[-1]
        nargs = cf.nargs # Number of arguments passed

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
        
            o = cf.stack[cf.stackpe - nargs + i]
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
                elif t == "I":
                    Builtins.type_check_int(self, o)
                elif t == "L":
                    Builtins.type_check_list(self, o)
                elif t == "M":
                    Builtins.type_check_module(self, o)
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
                vap[j - i] = cf.stack[cf.stackpe - nargs + j]
        else:
            vap = None

        self._cf_stack_del_from(cf, cf.stackpe - nargs)
        
        return (nrmp, vap)


    def get_funcs_mod(self):
        cf = self.cf_stack[-1]
        return cf.pc.mod


    def return_(self, obj):
        assert isinstance(obj, Builtins.Con_Object)
        cf = self.cf_stack[-1]
        self._cf_stack_push(cf, obj)
        cf.returned = True
        if cf.ct:
            self.st.switch(cf.ct)
        else:
            raise Con_Return_Exception()


    def yield_(self, obj):
        cf = self.cf_stack[-1]
        if not cf.ct:
            # If there's no continuation to return back to, this yield becomes in effect a return.
            self.return_(obj)

        self._cf_stack_push(cf, obj)
        cf.ct = self.st.switch(cf.ct)


    def raise_(self, ex):
        ex = Builtins.type_check_exception(self, ex)
        if ex.call_chain is None:
            cc = [] # Call chain
            i = len(self.cf_stack) - 1
            while i >= 0:
                cf = self.cf_stack[i]
                cc.append((cf.pc, cf.bc_off))
                i -= 1
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

    def execute(self, ct):
        cf = self.cf_stack[-1]
        cf.ct = ct
        pc = cf.pc
        #print cf.func.name.v, pc.mod.id_
        if isinstance(pc, Py_PC):
            try:
                pc.f(self)
            except Con_Raise_Exception, e:
                if cf.xfp == -1:
                    # There is no exception handler, so kill this continuation frame and propagate
                    # the exception
                    self._remove_continuation_frame()
                    raise
                raise Exception("XXX")
        else:
            self.bc_loop(cf)
        
        return ct


    def bc_loop(self, cf):
        pc = cf.pc
        assert isinstance(pc, BC_PC)
        mod_bc = pc.mod.bc
        prev_bc_off = -1
        while 1:
            try:
                bc_off = cf.bc_off
                if prev_bc_off != -1 and prev_bc_off > bc_off:
                    jitdriver.can_enter_jit(bc_off=bc_off, mod_bc=mod_bc, cf=cf, prev_bc_off=prev_bc_off, pc=pc, self=self)
                jitdriver.jit_merge_point(bc_off=bc_off, mod_bc=mod_bc, cf=cf, prev_bc_off=prev_bc_off, pc=pc, self=self)
                assert cf is self.cf_stack[-1]
                prev_bc_off = bc_off
                instr = Target.read_word(mod_bc, bc_off)
                it = Target.get_instr(instr)
                #x = cf.stackpe; assert x >= 0; print "%s %s %d [stackpe:%d ffp:%d gfp:%d xfp:%d]" % (Target.INSTR_NAMES[instr & 0xFF], str(cf.stack[:x]), bc_off, cf.stackpe, cf.ffp, cf.gfp, cf.xfp)
                if it == Target.CON_INSTR_VAR_LOOKUP:
                    self._instr_var_lookup(instr, cf)
                elif it == Target.CON_INSTR_VAR_ASSIGN:
                    self._instr_var_assign(instr, cf)
                elif it == Target.CON_INSTR_INT:
                    self._instr_int(instr, cf)
                elif it == Target.CON_INSTR_ADD_FAILURE_FRAME:
                    self._instr_add_failure_frame(instr, cf)
                elif it == Target.CON_INSTR_ADD_FAIL_UP_FRAME:
                    self._instr_add_fail_up_frame(instr, cf)
                elif it == Target.CON_INSTR_REMOVE_FAILURE_FRAME:
                    self._instr_remove_failure_frame(instr, cf)
                elif it == Target.CON_INSTR_IS_ASSIGNED:
                    self._instr_is_assigned(instr, cf)
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
                    self._instr_return(instr, cf)
                elif it == Target.CON_INSTR_BRANCH:
                    self._instr_branch(instr, cf)
                elif it == Target.CON_INSTR_YIELD:
                    self._instr_yield(instr, cf)
                elif it == Target.CON_INSTR_IMPORT:
                    self._instr_import(instr, cf)
                elif it == Target.CON_INSTR_DICT:
                    self._instr_dict(instr, cf)
                elif it == Target.CON_INSTR_DUP:
                    self._instr_dup(instr, cf)
                elif it == Target.CON_INSTR_PULL:
                    self._instr_pull(instr, cf)
                elif it == Target.CON_INSTR_STRING:
                    self._instr_string(instr, cf)
                elif it == Target.CON_INSTR_BUILTIN_LOOKUP:
                    self._instr_builtin_lookup(instr, cf)
                elif it == Target.CON_INSTR_ASSIGN_SLOT:
                    self._instr_assign_slot(instr, cf)
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
                elif it == Target.CON_INSTR_CONST_SET:
                    self._instr_const_set(instr, cf)
                elif it == Target.CON_INSTR_PRE_SLOT_LOOKUP_APPLY:
                    # In the C Converge VM, this instruction is used to avoid a very expensive path
                    # through the VM; it's currently unclear whether this VM will suffer from the
                    # same problem. Until we are more sure, we simply use the normal slot lookup
                    # function, which has the correct semantics, but may perhaps not be fully
                    # optimised.
                    self._instr_slot_lookup(instr, cf)
                elif it == Target.CON_INSTR_BRANCH_IF_NOT_FAIL:
                    self._instr_branch_if_not_fail(instr, cf)
                elif it == Target.CON_INSTR_EQ or it == Target.CON_INSTR_LE \
                  or it == Target.CON_INSTR_NEQ or it == Target.CON_INSTR_LE_EQ \
                  or it == Target.CON_INSTR_GR_EQ or it == Target.CON_INSTR_GT:
                    self._instr_cmp(instr, cf)
                elif it == Target.CON_INSTR_ADD or it == Target.CON_INSTR_SUBTRACT:
                    self._instr_calc(instr, cf)
                elif it == Target.CON_INSTR_MODULE_LOOKUP:
                    self._instr_module_lookup(instr, cf)
                else:
                    print it, cf.stack
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
                ef = cf.stack[cf.xfp]
                assert isinstance(ef, Stack_Exception_Frame)
                self._remove_exception_frame(cf)
                self._cf_stack_push(cf, e.ex_obj)
                cf.bc_off = ef.bc_off


    def _instr_var_lookup(self, instr, cf):
        closure_off, var_num = Target.unpack_var_lookup(instr)
        v = cf.closure[closure_off - 1][var_num]
        if v is None:
            raise Exception("XXX")
        self._cf_stack_push(cf, v)
        cf.bc_off += Target.INTSIZE


    def _instr_var_assign(self, instr, cf):
        closure_off, var_num = Target.unpack_var_assign(instr)
        cf.closure[len(cf.closure) - 1 - closure_off][var_num] = cf.stack[cf.stackpe - 1]
        cf.bc_off += Target.INTSIZE


    def _instr_int(self, instr, cf):
        self._cf_stack_push(cf, Builtins.Con_Int(self, Target.unpack_int(instr)))
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


    def _instr_is_assigned(self, instr, cf):
        closure_off, var_num = Target.unpack_var_lookup(instr)
        if cf.closure[len(cf.closure) - 1 - closure_off][var_num] is not None:
            pc = cf.pc
            assert isinstance(pc, BC_PC)
            mod_bc = pc.mod.bc
            instr2 = Target.read_word(mod_bc, cf.bc_off + Target.INTSIZE)
            cf.bc_off += Target.unpack_is_assigned(instr2)
        else:
            cf.bc_off += Target.INTSIZE + Target.INTSIZE


    def _instr_pop(self, instr, cf):
        self._cf_stack_pop(cf)
        cf.bc_off += Target.INTSIZE


    def _instr_list(self, instr, cf):
        ne = Target.unpack_list(instr)
        i = cf.stackpe - ne
        assert i >= 0
        j = cf.stackpe
        assert j >= 0
        l = cf.stack[i : j]
        self._cf_stack_del_from(cf, i)
        self._cf_stack_push(cf, Builtins.Con_List(self, l))
        cf.bc_off += Target.INTSIZE


    def _instr_slot_lookup(self, instr, cf):
        o = self._cf_stack_pop(cf)
        nm_start, nm_size = Target.unpack_slot_lookup(instr)
        nm = Target.extract_str(cf.pc.mod.bc, nm_start + cf.bc_off, nm_size)
        self._cf_stack_push(cf, o.get_slot(self, nm))
        cf.bc_off += Target.align(nm_start + nm_size)


    @jit.unroll_safe
    def _instr_apply(self, instr, cf):
        is_fail_up, _ = self._read_failure_frame()
        num_args = Target.unpack_apply(instr)
        fp = cf.stackpe - num_args - 1
        func = cf.stack[fp]
        args = [None] * num_args
        for i in range(0, num_args):
            args[i] = cf.stack[cf.stackpe - num_args + i]
            cf.stack[cf.stackpe - num_args + i] = None
        cf.stackpe -= num_args

        if is_fail_up:
            gf = Stack_Generator_Frame(cf.gfp, cf.bc_off + Target.INTSIZE)
            cf.stack[fp] = gf
            cf.gfp = fp
            new_cf = self._add_continuation_frame(func, len(args), True)
            self._cf_stack_extend(new_cf, args)
            o = self._apply_pump()
            if o is None:
                self._fail_now(cf)
                return
        else:
            cf.stack[fp] = None
            cf.stackpe -= 1
            o, _ = self.apply_closure(func, args, True)
            if o is None:
                self._fail_now(cf)
                return
        self._cf_stack_push(cf, o)
        cf.bc_off += Target.INTSIZE


    def _instr_fail_now(self, instr, cf):
        self._fail_now(cf)


    def _instr_func_defn(self, instr, cf):
        is_bound, max_stack_size = Target.unpack_func_defn(instr)
        np_o = self._cf_stack_pop(cf)
        assert isinstance(np_o, Builtins.Con_Int)
        nv_o = self._cf_stack_pop(cf)
        assert isinstance(nv_o, Builtins.Con_Int)
        name = self._cf_stack_pop(cf)
        new_pc = BC_PC(cf.pc.mod, cf.bc_off + 2 * Target.INTSIZE)
        container = cf.func.get_slot(self, "container")
        f = Builtins.Con_Func(self, name, is_bound, new_pc, max_stack_size, nv_o.v, \
          container, cf.closure)
        self._cf_stack_push(cf, f)
        cf.bc_off += Target.INTSIZE


    def _instr_return(self, instr, cf):
        self.return_(self._cf_stack_pop(cf))
        # Won't get here


    def _instr_branch(self, instr, cf):
        cf.bc_off += Target.unpack_branch(instr)


    def _instr_yield(self, instr, cf):
        self.yield_(self._cf_stack_pop(cf))
        cf.bc_off += Target.INTSIZE


    def _instr_import(self, instr, cf):
        mod = self.get_mod(cf.pc.mod.imps[Target.unpack_import(instr)])
        self._cf_stack_push(cf, mod)
        cf.bc_off += Target.INTSIZE


    def _instr_dict(self, instr, cf):
        ne = Target.unpack_dict(instr)
        i = cf.stackpe - ne * 2
        assert i >= 0
        j = cf.stackpe
        assert j >= 0
        l = cf.stack[i : j]
        self._cf_stack_del_from(cf, i)
        self._cf_stack_push(cf, Builtins.Con_Dict(self, l))
        cf.bc_off += Target.INTSIZE


    def _instr_dup(self, instr, cf):
        self._cf_stack_push(cf, cf.stack[cf.stackpe - 1])
        cf.bc_off += Target.INTSIZE


    def _instr_pull(self, instr, cf):
        i = Target.unpack_pull(instr)
        self._cf_stack_push(cf, self._cf_stack_pop_n(cf, i))
        cf.bc_off += Target.INTSIZE


    def _instr_string(self, instr, cf):
        str_start, str_size = Target.unpack_string(instr)
        str_off = cf.bc_off + str_start
        assert str_off > 0 and str_size >= 0
        str_ = rffi.charpsize2str(rffi.ptradd(cf.pc.mod.bc, str_off), str_size)
        self._cf_stack_push(cf, Builtins.Con_String(self, str_))
        cf.bc_off += Target.align(str_start + str_size)


    def _instr_builtin_lookup(self, instr, cf):
        bl = Target.unpack_builtin_lookup(instr)
        self._cf_stack_push(cf, self.get_builtin(bl))
        cf.bc_off += Target.INTSIZE


    def _instr_assign_slot(self, instr, cf):
        o = self._cf_stack_pop(cf)
        v = cf.stack[cf.stackpe - 1]
        nm_start, nm_size = Target.unpack_assign_slot(instr)
        nm = Target.extract_str(cf.pc.mod.bc, nm_start + cf.bc_off, nm_size)
        o.set_slot(self, nm, v)
        cf.bc_off += Target.align(nm_start + nm_size)


    def _instr_add_exception_frame(self, instr, cf):
        j = Target.unpack_add_exception_frame(instr)
        self._add_exception_frame(cf, cf.bc_off + j)
        cf.bc_off += Target.INTSIZE


    def _instr_remove_exception_frame(self, instr, cf):
        self._remove_exception_frame(cf)
        cf.bc_off += Target.INTSIZE


    def _instr_raise(self, instr, cf):
        self.raise_(self._cf_stack_pop(cf))


    @jit.unroll_safe
    def _instr_unpack_args(self, instr, cf):
        num_fargs, has_vargs = Target.unpack_unpack_args(instr)
        nargs = cf.nargs
        if nargs > num_fargs and not has_vargs:
            raise Exception("XXX")

        if num_fargs > 0:
            arg_offset = cf.bc_off + Target.INTSIZE + num_fargs * Target.INTSIZE
            for i in range(num_fargs - 1, -1, -1):
                arg_offset -= Target.INTSIZE
                arg_info = Target.read_word(cf.pc.mod.bc, arg_offset)
                if i >= nargs:
                    if not Target.unpack_unpack_args_is_mandatory(arg_info):
                        raise Exception("XXX")
                else:
                    if nargs > num_fargs:
                        o = self._cf_stack_pop_n(cf, nargs - num_fargs)
                    else:
                        o = self._cf_stack_pop(cf)
                    assert isinstance(o, Builtins.Con_Object)
                    cf.closure[-1][Target.unpack_unpack_args_arg_num(arg_info)] = o

        if has_vargs:
            arg_offset = cf.bc_off + Target.INTSIZE + num_fargs * Target.INTSIZE
            arg_info = Target.read_word(cf.pc.mod.bc, arg_offset)
            l = []
            if nargs <= num_fargs:
                l = []
            else:
                j = cf.stackpe
                i = j - (nargs - num_fargs)
                assert i >= 0 and j >= 0
                l = cf.stack[i : j]
                cf.stackpe = i + 1
            cf.closure[-1][Target.unpack_unpack_args_arg_num(arg_info)] = Builtins.Con_List(self, l)
            cf.bc_off += Target.INTSIZE + (num_fargs + 1) * Target.INTSIZE
        else:
            cf.bc_off += Target.INTSIZE + num_fargs * Target.INTSIZE


    @jit.unroll_safe
    def _instr_set(self, instr, cf):
        ne = Target.unpack_set(instr)
        i = cf.stackpe - ne
        assert i >= 0
        j = cf.stackpe
        assert j >= 0
        l = cf.stack[i : j]
        self._cf_stack_del_from(cf, cf.stackpe - ne)
        self._cf_stack_push(cf, Builtins.Con_Set(self, l))
        cf.bc_off += Target.INTSIZE


    def _instr_const_get(self, instr, cf):
        const_num = Target.unpack_constant_get(instr)
        v = cf.pc.mod.consts[const_num]
        if v is not None:
            self._cf_stack_push(cf, v)
            cf.bc_off += Target.INTSIZE
        else:
            self._add_failure_frame(cf, False, cf.bc_off)
            cf.bc_off = cf.pc.mod.get_const_create_off(self, const_num)


    def _instr_const_set(self, instr, cf):
        const_num = Target.unpack_constant_set(instr)
        cf.pc.mod.consts[const_num] = self._cf_stack_pop(cf)
        cf.bc_off += Target.INTSIZE


    def _instr_branch_if_not_fail(self, instr, cf):
        if self._cf_stack_pop(cf) is self.get_builtin(Builtins.BUILTIN_FAIL_OBJ):
            cf.bc_off += Target.INTSIZE
        else:
            j = Target.unpack_branch_if_not_fail(instr)
            cf.bc_off += j


    def _instr_cmp(self, instr, cf):
        rhs = self._cf_stack_pop(cf)
        lhs = self._cf_stack_pop(cf)
        
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
            self._cf_stack_push(cf, rhs)
            cf.bc_off += Target.INTSIZE
        else:
            self._fail_now(cf)


    def _instr_calc(self, instr, cf):
        rhs = self._cf_stack_pop(cf)
        lhs = self._cf_stack_pop(cf)
        
        it = Target.get_instr(instr)
        if it == Target.CON_INSTR_ADD:
            r = lhs.add(self, rhs)
        else:
            assert it == Target.CON_INSTR_SUBTRACT
            r = lhs.subtract(self, rhs)

        self._cf_stack_push(cf, r)
        cf.bc_off += Target.INTSIZE


    def _instr_module_lookup(self, instr, cf):
        o = self._cf_stack_pop(cf)
        if not isinstance(o, Builtins.Con_Module):
            raise Exception("XXX")
        nm_start, nm_size = Target.unpack_mod_lookup(instr)
        nm = Target.extract_str(cf.pc.mod.bc, cf.bc_off + nm_start, nm_size)
        self._cf_stack_push(cf, o.get_defn(self, nm))
        cf.bc_off += Target.align(nm_start + nm_size)


    ################################################################################################
    # cf.stack operations
    #
    
    def _cf_stack_push(self, cf, x):
        cf.stack[cf.stackpe] = x
        cf.stackpe += 1


    @jit.unroll_safe
    def _cf_stack_extend(self, cf, l):
        for x in l:
            cf.stack[cf.stackpe] = x
            cf.stackpe += 1


    def _cf_stack_pop(self, cf):
        assert cf.stackpe > 0
        cf.stackpe -= 1
        o = cf.stack[cf.stackpe]
        cf.stack[cf.stackpe] = None
        return o


    # Pop an item n items from the end of cf.stack.

    def _cf_stack_pop_n(self, cf, n):
        assert n < cf.stackpe
        i = cf.stackpe - 1 - n
        o = cf.stack[i]
        # Shuffle the stack down
        cf.stackpe -= 1
        for j in range(i, cf.stackpe):
            cf.stack[j] = cf.stack[j + 1]
        cf.stack[cf.stackpe] = None
        # If the frame pointers come after the popped item, they need to be rewritten
        if cf.ffp > i:
            cf.ffp -= 1
        if cf.gfp > i:
            cf.gfp -= 1
        if cf.xfp > i:
            cf.xfp -= 1

        return o


    @jit.unroll_safe
    def _cf_stack_del_from(self, cf, i):
        cf__stack = cf.stack
        for j in range(i, cf.stackpe):
            cf__stack[j] = None
        cf.stackpe = i


    @jit.unroll_safe
    def _cf_stack_insert(self, cf, i, x):
        for j in range(cf.stackpe, i, -1):
            cf.stack[j] = cf.stack[j - 1]
        cf.stack[i] = x
    

    def _add_continuation_frame(self, func, nargs, resumable=False):
        if not isinstance(func, Builtins.Con_Func):
            print func
            raise Exception("XXX")

        pc = func.pc
        if isinstance(pc, BC_PC):
            bc_off = pc.off
        else:
            bc_off = -1 

        if func.container_closure is not None:
            closure = func.container_closure + [[None] * func.num_vars]
        else:
            closure = [[None] * func.num_vars]

        if func.max_stack_size > nargs:
            max_stack_size = func.max_stack_size
        else:
            max_stack_size = nargs

        cf = Stack_Continuation_Frame(func, pc, max_stack_size, nargs, bc_off, closure, resumable)
        self.cf_stack.append(cf)
        
        return cf


    def _remove_continuation_frame(self):
        del self.cf_stack[-1]


    def _remove_generator_frame(self, cf):
        gf = cf.stack[cf.gfp]
        assert isinstance(gf, Stack_Generator_Frame)

        self._cf_stack_del_from(cf, cf.gfp)
        cf.gfp = gf.prev_gfp


    def _add_failure_frame(self, cf, is_fail_up, new_off=-1):
        ff = self.spare_ff
        if ff:
            self.spare_ff = None
        else:
            ff = Stack_Failure_Frame()

        ff.is_fail_up = is_fail_up
        ff.prev_ffp = cf.ffp
        ff.prev_gfp = cf.gfp
        ff.fail_to_off = new_off

        cf.gfp = -1
        cf.ffp = cf.stackpe
        self._cf_stack_push(cf, ff)
        

    def _remove_failure_frame(self, cf):
        ffp = cf.ffp
        ff = cf.stack[ffp]
        assert isinstance(ff, Stack_Failure_Frame)
        self._cf_stack_del_from(cf, ffp)
        cf.ffp = ff.prev_ffp

        self.spare_ff = ff


    def _read_failure_frame(self):
        cf = self.cf_stack[-1]
        assert cf.ffp >= 0
        ff = cf.stack[cf.ffp]
        assert isinstance(ff, Stack_Failure_Frame)

        return (ff.is_fail_up, ff.fail_to_off)


    @jit.unroll_safe
    def _fail_now(self, cf):
        while 1:
            is_fail_up, fail_to_off = self._read_failure_frame()
            if is_fail_up:
                has_rg = False
                if cf.gfp > -1:
                    gf = cf.stack[cf.gfp]
                    assert isinstance(gf, Stack_Generator_Frame)
                    has_rg = not gf.returned
                
                if has_rg:
                    o = self._apply_pump(True)
                    if o is not None:
                        cf = self.cf_stack[-1]
                        self._cf_stack_push(cf, o)
                        cf.bc_off = gf.resume_bc_off
                        return
                else:
                    self._remove_failure_frame(cf)
            else:
                cf.bc_off = fail_to_off
                self._remove_failure_frame(cf)
                break


    def _add_exception_frame(self, cf, bc_off):
        ef = Stack_Exception_Frame(bc_off, cf.ffp, cf.gfp, cf.xfp)
        cf.xfp = cf.stackpe
        self._cf_stack_push(cf, ef)


    def _remove_exception_frame(self, cf):
        ef = cf.stack[cf.xfp]
        assert isinstance(ef, Stack_Exception_Frame)
        cf.ffp = ef.prev_ffp
        cf.gfp = ef.prev_gfp
        cf.xfp = ef.prev_xfp
        cf.stack[cf.xfp] = None
        cf.stackpe -= 1




####################################################################################################
# Stack frame classes
#

class Stack_Continuation_Frame(Con_Thingy):
    __slots__ = ("stack", "stackpe", "ff_cache", "ff_cachesz", "func", "pc", "nargs", "bc_off",
      "closure", "ct", "ffp", "gfp", "xfp", "resumable", "returned")
    _immutable_fields_ = ("stack", "ff_cache", "func", "closure", "pc", "nargs", "resumable")

    def __init__(self, func, pc, max_stack_size, nargs, bc_off, closure, resumable):
        self.stack = [None] * max_stack_size
        debug.make_sure_not_resized(self.stack)
        self.func = func
        self.pc = pc
        self.nargs = nargs # Number of arguments passed to this continuation
        self.bc_off = bc_off # -1 for Py modules
        self.closure = closure
        self.resumable = resumable
        self.returned = False

        # stackpe always points to the element *after* the end of the stack (this makes a lot of
        # stack-based operations quicker)
        self.stackpe = 0
        # ffp, gfp, and xfp all point *to* the frame they refer to
        self.ffp = self.gfp = self.xfp = -1


class Stack_Failure_Frame(Con_Thingy):
    __slots__ = ("is_fail_up", "prev_ffp", "prev_gfp", "fail_to_off")
    # Because failure frames can be reused, they have no immutable fields.

    def __repr__(self):
        if self.is_fail_up:
            return "<Fail up frame>"
        else:
            return "<Failure frame>"


class Stack_Generator_Frame(Con_Thingy):
    __slots__ = ("prev_gfp", "resume_bc_off", "returned", "ct", "saved_cf")
    _immutable_fields_ = ("prev_gfp", "resume_bc_off")
    
    def __init__(self, prev_gfp, resume_bc_off):
        self.prev_gfp = prev_gfp
        self.resume_bc_off = resume_bc_off
        self.returned = False


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

class Con_Raise_Exception(Exception):
    _immutable_fields_ = ("ex_obj",)

    def __init__(self, ex_obj):
        self.ex_obj = ex_obj


class Con_Return_Exception(Exception):
    pass



def switch_hack(ct, arg):
    global_vm.execute(ct)
    return ct


global_vm = VM()

def new_vm():
    global_vm.init()
    return global_vm
