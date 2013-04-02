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


from rpython.rlib import rarithmetic, objectmodel
from rpython.rtyper.lltypesystem import llmemory, lltype, rffi
from rpython.rtyper.tool import rffi_platform as platform
from rpython.rtyper.annlowlevel import llhelper
from rpython.translator.tool.cbuild import ExternalCompilationInfo
from Builtins import *
from Core import *
import Config, Target


eci                        = ExternalCompilationInfo(includes=["libxml/parser.h"], \
                               include_dirs=Config.LIBXML2_INCLUDE_DIRS, \
                               library_dirs=Config.LIBXML2_LIBRARY_DIRS, \
                               libraries=Config.LIBXML2_LIBRARIES, \
                               link_extra=Config.LIBXML2_LINK_FLAGS, \
                               link_files=[Config.LIBXML2_A])
xmlCharP                   = lltype.Ptr(lltype.Array(rffi.UCHAR, hints={'nolength': True}))
xmlCharPP                  = lltype.Ptr(lltype.Array(xmlCharP, hints={'nolength': True}))
charactersSAXFunc          = lltype.FuncType((rffi.VOIDP, xmlCharP, rffi.INT), lltype.Void)
charactersSAXFuncP         = lltype.Ptr(charactersSAXFunc)
startElementNsSAX2Func     = lltype.FuncType((rffi.VOIDP, xmlCharP, xmlCharP, xmlCharP, rffi.INT, \
                               xmlCharPP, rffi.INT, rffi.INT, xmlCharPP), lltype.Void)
startElementNsSAX2FuncP    = lltype.Ptr(startElementNsSAX2Func)
endElementNsSAX2Func       = lltype.FuncType((rffi.VOIDP, xmlCharP, xmlCharP, xmlCharP), lltype.Void)
endElementNsSAX2FuncP      = lltype.Ptr(endElementNsSAX2Func)

class CConfig:
    _compilation_info_     = eci
    xmlSAXHandler          = platform.Struct("struct _xmlSAXHandler", \
                               [("characters", charactersSAXFuncP), ("initialized", rffi.UINT), \
                               ("startElementNs", startElementNsSAX2FuncP), \
                               ("endElementNs", endElementNsSAX2FuncP)])
    XML_SAX2_MAGIC         = platform.DefinedConstantInteger("XML_SAX2_MAGIC")

cconfig = platform.configure(CConfig)
XML_SAX2_MAGIC             = cconfig["XML_SAX2_MAGIC"]
xmlSAXHandler              = cconfig["xmlSAXHandler"]
xmlSAXHandlerP             = lltype.Ptr(xmlSAXHandler)
xmlSAXUserParseMemory      = rffi.llexternal("xmlSAXUserParseMemory", \
                               [xmlSAXHandlerP, rffi.VOIDP, rffi.CCHARP, rffi.INT], rffi.INT, \
                               compilation_info=eci)




def init(vm):
    return new_c_con_module(vm, "libXML2", "libXML2", __file__, import_, \
      ["XML_Exception", "parse"])


@con_object_proc
def import_(vm):
    (mod,),_ = vm.decode_args("O")

    class_class = vm.get_builtin(BUILTIN_CLASS_CLASS)
    user_exception_class = vm.get_builtin(BUILTIN_EXCEPTIONS_MODULE). \
      get_defn(vm, "User_Exception")
    xml_exception = vm.get_slot_apply(class_class, "new", \
      [Con_String(vm, "XML_Exception"), Con_List(vm, [user_exception_class]), mod])
    mod.set_defn(vm, "XML_Exception", xml_exception)

    new_c_con_func_for_mod(vm, "parse", parse, mod)

    return vm.get_builtin(BUILTIN_NULL_OBJ)


@con_object_proc
def parse(vm):
    (xml_o, nodes_mod),_ = vm.decode_args("SM")
    assert isinstance(xml_o, Con_String)
    
    with lltype.scoped_alloc(xmlSAXHandler, zero=True) as h:
        h.c_initialized = rffi.r_uint(XML_SAX2_MAGIC)
        h.c_characters = llhelper(charactersSAXFuncP, _characters)
        h.c_startElementNs = llhelper(startElementNsSAX2FuncP, _start_element)
        h.c_endElementNs = llhelper(endElementNsSAX2FuncP, _end_element)
        docs_eo = Con_List(vm, [])
        _storage_hack.push(_Store(vm, [docs_eo], nodes_mod))
        r = xmlSAXUserParseMemory(h, lltype.nullptr(rffi.VOIDP.TO), xml_o.v, len(xml_o.v))
    if r < 0 or len(_storage_hack.peek().elems_stack) != 1:
        raise Exception("XXX")
    _storage_hack.pop()
    doc_o = vm.get_slot_apply(nodes_mod.get_defn(vm, "Doc"), "new", [docs_eo])
    
    return doc_o


class _Store:
    __slots__ = ("vm", "elems_stack", "nodes_mod")
    _immutable_slots_ = ("vm", "elems_stack", "nodes_mod")

    def __init__(self, vm, elems_stack, nodes_mod):
        self.vm = vm
        self.elems_stack = elems_stack
        self.nodes_mod = nodes_mod


# A global storage to be able to recover W_CallbackPtr object out of number
class _Storage_Hack:
    def __init__(self):
        self._stores = []

    def push(self, store):
        self._stores.append(store)

    def peek(self):
        return self._stores[-1]

    def pop(self):
        self._stores.pop()

_storage_hack = _Storage_Hack()


def _characters(ctx, chrs, chrs_len):
    st = _storage_hack.peek()
    vm = st.vm
    s_o = Con_String(vm, \
      rffi.charpsize2str(rffi.cast(rffi.CCHARP, chrs), rarithmetic.intmask(chrs_len)))
    vm.get_slot_apply(st.elems_stack[-1], "append", [s_o])


def _start_element(ctx, localname, prefix, URI, nb_namespaces, namespaces, nb_attributes, nb_defaulted, attributes):
    st = _storage_hack.peek()
    vm = st.vm
    nodes_mod = st.nodes_mod

    name_o = Con_String(vm, rffi.charp2str(rffi.cast(rffi.CCHARP, localname)))
    if prefix:
        prefix_o = Con_String(vm, rffi.charp2str(rffi.cast(rffi.CCHARP, prefix)))
        namespace_o = Con_String(vm, rffi.charp2str(rffi.cast(rffi.CCHARP, URI)))
    else:
        prefix_o = namespace_o = Con_String(vm, "")

    attributes_l = []
    current_attr = attributes
    for i in range(int(nb_attributes) + int(nb_defaulted)):
        attr_name_o = Con_String(vm, rffi.charp2str(rffi.cast(rffi.CCHARP, current_attr[0])))
        attr_len = rffi.cast(lltype.Unsigned, \
          current_attr[4]) - rffi.cast(lltype.Unsigned, current_attr[3])
        attr_val_o = Con_String(vm, \
          rffi.charpsize2str(rffi.cast(rffi.CCHARP, current_attr[3]), rarithmetic.intmask(attr_len)))
            
        if current_attr[1]:
            attr_prefix_o = Con_String(vm, rffi.charp2str(rffi.cast(rffi.CCHARP, current_attr[1])))
            attr_namespace_o = Con_String(vm, rffi.charp2str(rffi.cast(rffi.CCHARP, current_attr[2])))
        else:
            attr_prefix_o = attr_namespace_o = Con_String(vm, "")
        
        attr_o = vm.get_slot_apply(nodes_mod.get_defn(vm, "Attr"), "new", \
          [attr_name_o, attr_val_o, attr_prefix_o, attr_namespace_o])
        attributes_l.append(attr_o)
        
        current_attr = rffi.ptradd(current_attr, 5)
    attributes_o = Con_Set(vm, attributes_l)
    elem_o = vm.get_slot_apply(nodes_mod.get_defn(vm, "Elem"), "new", \
      [name_o, attributes_o, prefix_o, namespace_o])
    vm.get_slot_apply(st.elems_stack[-1], "append", [elem_o])
    st.elems_stack.append(elem_o)


def _end_element(ctx, localname, prefix, URI):
    st = _storage_hack.peek()
    st.elems_stack.pop()