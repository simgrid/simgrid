#
# Copyright 2012-2015 Bronto Software, Inc. and contributors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import re
import string

from docutils import nodes
from docutils.parsers.rst import Directive, directives

from sphinx import addnodes, version_info
from sphinx.roles import XRefRole
from sphinx.locale import _
from sphinx.domains import Domain, ObjType
from sphinx.directives import ObjectDescription
from sphinx.util.nodes import make_refnode
from sphinx.util.docfields import Field, TypedField, GroupedField

import javalang

import javasphinx.extdoc as extdoc
import javasphinx.formatter as formatter
import javasphinx.util as util

# Classes in java.lang. These are available without an import.
java_dot_lang = set([
    'AbstractMethodError', 'Appendable', 'ArithmeticException',
    'ArrayIndexOutOfBoundsException', 'ArrayStoreException', 'AssertionError',
    'AutoCloseable', 'Boolean', 'BootstrapMethodError', 'Byte', 'Character',
    'CharSequence', 'Class', 'ClassCastException', 'ClassCircularityError',
    'ClassFormatError', 'ClassLoader', 'ClassNotFoundException', 'ClassValue',
    'Cloneable', 'CloneNotSupportedException', 'Comparable', 'Compiler',
    'Deprecated', 'Double', 'Enum', 'EnumConstantNotPresentException', 'Error',
    'Exception', 'ExceptionInInitializerError', 'Float', 'IllegalAccessError',
    'IllegalAccessException', 'IllegalArgumentException',
    'IllegalMonitorStateException', 'IllegalStateException',
    'IllegalThreadStateException', 'IncompatibleClassChangeError',
    'IndexOutOfBoundsException', 'InheritableThreadLocal', 'InstantiationError',
    'InstantiationException', 'Integer', 'InternalError', 'InterruptedException',
    'Iterable', 'LinkageError', 'Long', 'Math', 'NegativeArraySizeException',
    'NoClassDefFoundError', 'NoSuchFieldError', 'NoSuchFieldException',
    'NoSuchMethodError', 'NoSuchMethodException', 'NullPointerException', 'Number',
    'NumberFormatException', 'Object', 'OutOfMemoryError', 'Override', 'Package',
    'Process', 'ProcessBuilder', 'Readable', 'ReflectiveOperationException',
    'Runnable', 'Runtime', 'RuntimeException', 'RuntimePermission', 'SafeVarargs',
    'SecurityException', 'SecurityManager', 'Short', 'StackOverflowError',
    'StackTraceElement', 'StrictMath', 'String', 'StringBuffer', 'StringBuilder',
    'StringIndexOutOfBoundsException', 'SuppressWarnings', 'System', 'Thread',
    'ThreadDeath', 'ThreadGroup', 'ThreadLocal', 'Throwable',
    'TypeNotPresentException', 'UnknownError', 'UnsatisfiedLinkError',
    'UnsupportedClassVersionError', 'UnsupportedOperationException', 'VerifyError',
    'VirtualMachineError', 'Void'])

class JavaObject(ObjectDescription):
    option_spec = {
        'noindex': directives.flag,
        'package': directives.unchanged,
        'outertype': directives.unchanged
    }

    def _build_ref_node(self, target):
        ref = addnodes.pending_xref('', refdomain='java', reftype='type', reftarget=target, modname=None, classname=None)
        ref['java:outertype'] = self.get_type()

        package = self.env.temp_data.get('java:imports', dict()).get(target, None)

        if not package and target in java_dot_lang:
            package = 'java.lang'

        if package:
            ref['java:imported'] = True
            ref['java:package'] = package
        else:
            ref['java:imported'] = False
            ref['java:package'] = self.get_package()

        return ref

    def _build_type_node(self, typ):
        if isinstance(typ, javalang.tree.ReferenceType):
            if typ.dimensions:
                dim = '[]' * len(typ.dimensions)
            else:
                dim = ''

            target = typ.name
            parts = []

            while typ:
                ref_node = self._build_ref_node(target)
                ref_node += nodes.Text(typ.name, typ.name)
                parts.append(ref_node)

                if typ.arguments:
                    parts.append(nodes.Text('<', '<'))

                    first = True
                    for type_arg in typ.arguments:
                        if first:
                            first = False
                        else:
                            parts.append(nodes.Text(', ', ', '))

                        if type_arg.pattern_type == '?':
                            parts.append(nodes.Text('?', '?'))
                        else:
                            if type_arg.pattern_type:
                                s = '? %s ' % (type_arg.pattern_type,)
                                parts.append(nodes.Text(s, s))
                            parts.extend(self._build_type_node(type_arg.type))

                    parts.append(nodes.Text('>', '>'))

                typ = typ.sub_type

                if typ:
                    target = target + '.' + typ.name
                    parts.append(nodes.Text('.', '.'))
                elif dim:
                    parts.append(nodes.Text(dim, dim))

            return parts

        type_repr = formatter.output_type(typ).build()
        return [nodes.Text(type_repr, type_repr)]

    def _build_type_node_list(self, types):
        parts = self._build_type_node(types[0])
        for typ in types[1:]:
            parts.append(nodes.Text(', ', ', '))
            parts.extend(self._build_type_node(typ))
        return parts

    def handle_signature(self, sig, signode):
        handle_name = 'handle_%s_signature' % (self.objtype,)
        handle = getattr(self, handle_name, None)

        if handle:
            return handle(sig, signode)
        raise NotImplementedError

    def get_index_text(self, package, typ, name):
        raise NotImplementedError

    def get_package(self):
        return self.options.get('package', self.env.temp_data.get('java:package'))

    def get_type(self):
        return self.options.get('outertype', '.'.join(self.env.temp_data.get('java:outertype', [])))

    def add_target_and_index(self, name, sig, signode):
        package = self.get_package()
        typ = self.get_type()

        fullname = '.'.join(filter(None, (package, typ, name)))
        basename = fullname.partition('(')[0]

        # note target
        if fullname not in self.state.document.ids:
            signode['names'].append(fullname)
            signode['ids'].append(fullname)
            signode['first'] = (not self.names)
            self.state.document.note_explicit_target(signode)

            objects = self.env.domaindata['java']['objects']
            if fullname in objects:
                self.state_machine.reporter.warning(
                    'duplicate object description of %s, ' % fullname +
                    'other instance in ' + self.env.doc2path(objects[fullname][0]) +
                    ', use :noindex: for one of them',
                    line=self.lineno)

            objects[fullname] = (self.env.docname, self.objtype, basename)

        indextext = self.get_index_text(package, typ, name)
        if indextext:
            self.indexnode['entries'].append(_create_indexnode(indextext, fullname))

    def before_content(self):
        self.set_type = False

        if self.objtype == 'type' and self.names:
            self.set_type = True
            self.env.temp_data.setdefault('java:outertype', list()).append(self.names[0])

    def after_content(self):
        if self.set_type:
            self.env.temp_data['java:outertype'].pop()

class JavaMethod(JavaObject):
    doc_field_types = [
        TypedField('parameter', label=_('Parameters'),
                   names=('param', 'parameter', 'arg', 'argument'),
                   typerolename='type', typenames=('type',)),
        Field('returnvalue', label=_('Returns'), has_arg=False,
              names=('returns', 'return')),
        GroupedField('throws', names=('throws',), label=_('Throws'), rolename='type')
    ]

    def handle_method_signature(self, sig, signode):
        try:
            member = javalang.parse.parse_member_signature(sig)
        except javalang.parser.JavaSyntaxError:
            raise self.error("syntax error in method signature")

        if not isinstance(member, javalang.tree.MethodDeclaration):
            raise self.error("expected method declaration")

        mods = formatter.output_modifiers(member.modifiers).build()
        signode += nodes.Text(mods + ' ', mods + ' ')

        if member.type_parameters:
            type_params = formatter.output_type_params(member.type_parameters).build()
            signode += nodes.Text(type_params, type_params)
            signode += nodes.Text(' ', ' ')

        rnode = addnodes.desc_type('', '')
        rnode += self._build_type_node(member.return_type)

        signode += rnode
        signode += nodes.Text(' ', ' ')
        signode += addnodes.desc_name(member.name, member.name)

        paramlist = addnodes.desc_parameterlist()
        for parameter in member.parameters:
            param = addnodes.desc_parameter('', '', noemph=True)
            param += self._build_type_node(parameter.type)

            if parameter.varargs:
                param += nodes.Text('...', '')

            param += nodes.emphasis(' ' + parameter.name, ' ' + parameter.name)
            paramlist += param
        signode += paramlist

        param_reprs = [formatter.output_type(param.type, with_generics=False).build() for param in member.parameters]
        return member.name + '(' + ', '.join(param_reprs) + ')'

    def get_index_text(self, package, typ, name):
        return _('%s (Java method)' % (name,))

class JavaConstructor(JavaObject):
    doc_field_types = [
        TypedField('parameter', label=_('Parameters'),
                   names=('param', 'parameter', 'arg', 'argument'),
                   typerolename='type', typenames=('type',)),
        GroupedField('throws', names=('throws',), label=_('Throws'))
    ]

    def handle_constructor_signature(self, sig, signode):
        try:
            member = javalang.parse.parse_constructor_signature(sig)
        except javalang.parser.JavaSyntaxError:
            raise self.error("syntax error in constructor signature")

        if not isinstance(member, javalang.tree.ConstructorDeclaration):
            raise self.error("expected constructor declaration")

        mods = formatter.output_modifiers(member.modifiers).build()
        signode += nodes.Text(mods + ' ', mods + ' ')

        signode += addnodes.desc_name(member.name, member.name)

        paramlist = addnodes.desc_parameterlist()
        for parameter in member.parameters:
            param = addnodes.desc_parameter('', '', noemph=True)
            param += self._build_type_node(parameter.type)

            if parameter.varargs:
                param += nodes.Text('...', '')

            param += nodes.emphasis(' ' + parameter.name, ' ' + parameter.name)
            paramlist += param
        signode += paramlist

        param_reprs = [formatter.output_type(param.type, with_generics=False).build() for param in member.parameters]
        return '%s(%s)' % (member.name, ', '.join(param_reprs))

    def get_index_text(self, package, typ, name):
        return _('%s (Java constructor)' % (name,))

class JavaType(JavaObject):
    doc_field_types = [
        GroupedField('parameter', names=('param',), label=_('Parameters'))
    ]

    declaration_type = None

    def handle_type_signature(self, sig, signode):
        try:
            member = javalang.parse.parse_type_signature(sig)
        except javalang.parser.JavaSyntaxError:
            raise self.error("syntax error in field signature")

        if isinstance(member, javalang.tree.ClassDeclaration):
            self.declaration_type = 'class'
        elif isinstance(member, javalang.tree.InterfaceDeclaration):
            self.declaration_type = 'interface'
        elif isinstance(member, javalang.tree.EnumDeclaration):
            self.declaration_type = 'enum'
        elif isinstance(member, javalang.tree.AnnotationDeclaration):
            self.declaration_type = 'annotation'
        else:
            raise self.error("expected type declaration")

        mods = formatter.output_modifiers(member.modifiers).build()
        signode += nodes.Text(mods + ' ', mods + ' ')

        if self.declaration_type == 'class':
            signode += nodes.Text('class ', 'class ')
        elif self.declaration_type == 'interface':
            signode += nodes.Text('interface ', 'interface ')
        elif self.declaration_type == 'enum':
            signode += nodes.Text('enum ', 'enum ')
        elif self.declaration_type == 'annotation':
            signode += nodes.Text('@interface ', '@interface ')

        signode += addnodes.desc_name(member.name, member.name)

        if self.declaration_type in ('class', 'interface') and member.type_parameters:
            type_params = formatter.output_type_params(member.type_parameters).build()
            signode += nodes.Text(type_params, type_params)

        if self.declaration_type == 'class':
            if member.extends:
                extends = ' extends '
                signode += nodes.Text(extends, extends)
                signode += self._build_type_node(member.extends)
            if member.implements:
                implements = ' implements '
                signode += nodes.Text(implements, implements)
                signode += self._build_type_node_list(member.implements)
        elif self.declaration_type == 'interface':
            if member.extends:
                extends = ' extends '
                signode += nodes.Text(extends, extends)
                signode += self._build_type_node_list(member.extends)
        elif self.declaration_type == 'enum':
            if member.implements:
                implements = ' implements '
                signode += nodes.Text(implements, implements)
                signode += self._build_type_node_list(member.implements)

        return member.name

    def get_index_text(self, package, typ, name):
        return _('%s (Java %s)' % (name, self.declaration_type))

class JavaField(JavaObject):
    def handle_field_signature(self, sig, signode):
        try:
            member = javalang.parse.parse_member_signature(sig)
        except javalang.parser.JavaSyntaxError:
            raise self.error("syntax error in field signature")

        if not isinstance(member, javalang.tree.FieldDeclaration):
            raise self.error("expected field declaration")

        mods = formatter.output_modifiers(member.modifiers).build()
        signode += nodes.Text(mods + ' ', mods + ' ')

        tnode = addnodes.desc_type('', '')
        tnode += self._build_type_node(member.type)

        signode += tnode
        signode += nodes.Text(' ', ' ')

        if len(member.declarators) > 1:
            self.error('only one field may be documented at a time')

        declarator = member.declarators[0]
        signode += addnodes.desc_name(declarator.name, declarator.name)

        dim = '[]' * len(declarator.dimensions)
        signode += nodes.Text(dim)

        if declarator.initializer and isinstance(declarator.initializer, javalang.tree.Literal):
            signode += nodes.Text(' = ' + declarator.initializer.value)

        return declarator.name

    def get_index_text(self, package, typ, name):
        return _('%s (Java field)' % (name,))

class JavaPackage(Directive):
    """
    Directive to mark description of a new package.
    """

    has_content = False
    required_arguments = 1
    optional_arguments = 0
    final_argument_whitespace = False
    option_spec = {
        'noindex': directives.flag,
    }

    def run(self):
        env = self.state.document.settings.env
        package = self.arguments[0].strip()
        noindex = 'noindex' in self.options
        env.temp_data['java:package'] = package
        env.domaindata['java']['objects'][package] = (env.docname, 'package', package)
        ret = []

        if not noindex:
            targetnode = nodes.target('', '', ids=['package-' + package], ismod=True)
            self.state.document.note_explicit_target(targetnode)

            # the platform and synopsis aren't printed; in fact, they are only
            # used in the modindex currently
            ret.append(targetnode)

            indextext = _('%s (package)') % (package,)
            inode = addnodes.index(entries=[_create_indexnode(indextext, 'package-' + package)])
            ret.append(inode)

        return ret

class JavaImport(Directive):
    """
    This directive is just to tell Sphinx the source of a referenced type.
    """

    has_content = False
    required_arguments = 2
    optional_arguments = 0
    final_argument_whitespace = False
    option_spec = {}

    def run(self):
        env = self.state.document.settings.env
        package, typename = self.arguments

        env.temp_data.setdefault('java:imports', dict())[typename] = package
        return []

class JavaXRefRole(XRefRole):
    def process_link(self, env, refnode, has_explicit_title, title, target):
        refnode['java:outertype'] = '.'.join(env.temp_data.get('java:outertype', list()))

        target = target.lstrip('~')

        # Strip a method component from the target
        basetype = target
        if '(' in basetype:
            basetype = basetype.partition('(')[0]
            if '.' in basetype:
                basetype = basetype.rpartition('.')[0]

        package = env.temp_data.get('java:imports', dict()).get(basetype, None)

        if package:
            refnode['java:imported'] = True
            refnode['java:package'] = package
        else:
            refnode['java:imported'] = False
            refnode['java:package'] = env.temp_data.get('java:package')

        if not has_explicit_title:
            # if the first character is a tilde, don't display the module/class
            # parts of the contents
            if title[0:1] == '~':
                title = title.partition('(')[0]
                title = title[1:]
                dot = title.rfind('.')
                if dot != -1:
                    title = title[dot+1:]

        return title, target

class JavaDomain(Domain):
    """Java language domain."""
    name = 'java'
    label = 'Java'

    object_types = {
        'package':     ObjType(_('package'), 'package', 'ref'),
        'type':        ObjType(_('type'), 'type', 'ref'),
        'field':       ObjType(_('field'), 'field', 'ref'),
        'constructor': ObjType(_('constructor'), 'construct', 'ref'),
        'method':      ObjType(_('method'), 'meth', 'ref')
    }

    directives = {
        'package':        JavaPackage,
        'type':           JavaType,
        'field':          JavaField,
        'constructor':    JavaConstructor,
        'method':         JavaMethod,
        'import':         JavaImport
    }

    roles = {
        'package':   JavaXRefRole(),
        'type':      JavaXRefRole(),
        'field':     JavaXRefRole(),
        'construct': JavaXRefRole(),
        'meth':      JavaXRefRole(),
        'ref':       JavaXRefRole(),
    }

    initial_data = {
        'objects': {},  # fullname -> docname, objtype, basename
    }

    def clear_doc(self, docname):
        objects = dict(self.data['objects'])

        for fullname, (fn, _, _) in objects.items():
            if fn == docname:
                del self.data['objects'][fullname]

    def resolve_xref(self, env, fromdocname, builder, typ, target, node, contnode):
        objects = self.data['objects']
        package = node.get('java:package')
        imported = node.get('java:imported')
        type_context = node.get('java:outertype')

        # Partial function to make building the response easier
        make_ref = lambda fullname: make_refnode(builder, fromdocname, objects[fullname][0], fullname, contnode, fullname)

        # Check for fully qualified references
        if target in objects:
            return make_ref(target)

        # Try with package name prefixed
        if package:
            fullname = package + '.' + target
            if fullname in objects:
                return make_ref(fullname)

        # Try with package and type prefixed
        if package and type_context:
            fullname = package + '.' + type_context + '.' + target
            if fullname in objects:
                return make_ref(fullname)

        # Try to find a matching suffix
        suffix = '.' + target
        basename_match = None
        basename_suffix = suffix.partition('(')[0]

        for fullname, (_, _, basename) in objects.items():
            if fullname.endswith(suffix):
                return make_ref(fullname)
            if basename.endswith(basename_suffix):
                basename_match = fullname

        if basename_match:
            return make_ref(basename_match)

        # Try creating an external documentation reference
        ref = extdoc.get_javadoc_ref(self.env, target, target)

        if not ref and target in java_dot_lang:
            fulltarget = 'java.lang.' + target
            ref = extdoc.get_javadoc_ref(self.env, fulltarget, fulltarget)

        # If the target was imported try with the package prefixed
        if not ref and imported:
            fulltarget = package + '.' + target
            ref = extdoc.get_javadoc_ref(self.env, fulltarget, fulltarget)

        if ref:
            ref.append(contnode)
            return ref
        return None

    def get_objects(self):
        for refname, (docname, typ, _) in self.data['objects'].items():
            yield (refname, refname, typ, docname, refname, 1)


def _create_indexnode(indextext, fullname):
    # See https://github.com/sphinx-doc/sphinx/issues/2673
    if version_info < (1, 4):
        return ('single', indextext, fullname, '')
    return ('single', indextext, fullname, '', None)
