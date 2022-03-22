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

import javalang

import javasphinx.formatter as formatter
import javasphinx.util as util
import javasphinx.htmlrst as htmlrst

class JavadocRestCompiler(object):
    """ Javadoc to ReST compiler. Builds ReST documentation from a Java syntax
    tree. """

    def __init__(self, filter=None, member_headers=True, parser='lxml'):
        if filter:
            self.filter = filter
        else:
            self.filter = self.__default_filter

        self.converter = htmlrst.Converter(parser)

        self.member_headers = member_headers

    def __default_filter(self, node):
        """Excludes private members and those tagged "@hide" / "@exclude" in their
        docblocks.

        """

        if not isinstance(node, javalang.tree.Declaration):
            return False

        if 'private' in node.modifiers:
            return False

        if isinstance(node, javalang.tree.Documented) and node.documentation:
            doc = javalang.javadoc.parse(node.documentation)
            if 'hide' in doc.tags or 'exclude' in doc.tags:
                return False

        return True

    def __html_to_rst(self, s):
        return self.converter.convert(s)

    def __output_doc(self, documented):
        if not isinstance(documented, javalang.tree.Documented):
            raise ValueError('node not documented')

        output = util.Document()

        if not documented.documentation:
            return output

        doc = javalang.javadoc.parse(documented.documentation)

        if doc.description:
            output.add(self.__html_to_rst(doc.description))
            output.clear()

        if doc.authors:
            output.add_line(':author: %s' % (self.__html_to_rst(', '.join(doc.authors)),))

        for name, value in doc.params:
            output.add_line(':param %s: %s' % (name, self.__html_to_rst(value)))

        for exception in doc.throws:
            description = doc.throws[exception]
            output.add_line(':throws %s: %s' % (exception, self.__html_to_rst(description)))

        if doc.return_doc:
            output.add_line(':return: %s' % (self.__html_to_rst(doc.return_doc),))

        if doc.tags.get('see'):
            output.clear()

            see_also = ', '.join(self.__output_see(see) for see in doc.tags['see'])
            output.add_line('**See also:** %s' % (see_also,))

        return output

    def __output_see(self, see):
        """ Convert the argument to a @see tag to rest """

        if see.startswith('<a href'):
            # HTML link -- <a href="...">...</a>
            return self.__html_to_rst(see)
        if '"' in see:
            # Plain text
            return see

        # Type reference (default)
        return ':java:ref:`%s`' % (see.replace('#', '.').replace(' ', ''),)

    def compile_type(self, declaration):
        signature = util.StringBuilder()
        formatter.output_declaration(declaration, signature)

        doc = self.__output_doc(declaration)

        directive = util.Directive('java:type', signature.build())
        directive.add_content(doc)

        return directive

    def compile_enum_constant(self, enum, constant):
        signature = util.StringBuilder()

        for annotation in constant.annotations:
            formatter.output_annotation(annotation, signature)

        # All enum constants are public, static, and final
        signature.append('public static final ')
        signature.append(enum)
        signature.append(' ')
        signature.append(constant.name)

        doc = self.__output_doc(constant)

        directive = util.Directive('java:field', signature.build())
        directive.add_content(doc)

        return directive

    def compile_field(self, field):
        signature = util.StringBuilder()

        for annotation in field.annotations:
            formatter.output_annotation(annotation, signature)

        formatter.output_modifiers(field.modifiers, signature)
        signature.append(' ')

        formatter.output_type(field.type, signature)
        signature.append(' ')
        signature.append(field.declarators[0].name)

        doc = self.__output_doc(field)

        directive = util.Directive('java:field', signature.build())
        directive.add_content(doc)

        return directive

    def compile_constructor(self, constructor):
        signature = util.StringBuilder()

        for annotation in constructor.annotations:
            formatter.output_annotation(annotation, signature)

        formatter.output_modifiers(constructor.modifiers, signature)
        signature.append(' ')

        if constructor.type_parameters:
            formatter.output_type_params(constructor.type_parameters, signature)
            signature.append(' ')

        signature.append(constructor.name)

        signature.append('(')
        formatter.output_list(formatter.output_formal_param, constructor.parameters, signature, ', ')
        signature.append(')')

        if constructor.throws:
            signature.append(' throws ')
            formatter.output_list(formatter.output_exception, constructor.throws, signature, ', ')

        doc = self.__output_doc(constructor)

        directive = util.Directive('java:constructor', signature.build())
        directive.add_content(doc)

        return directive

    def compile_method(self, method):
        signature = util.StringBuilder()

        for annotation in method.annotations:
            formatter.output_annotation(annotation, signature)

        formatter.output_modifiers(method.modifiers, signature)
        signature.append(' ')

        if method.type_parameters:
            formatter.output_type_params(method.type_parameters, signature)
            signature.append(' ')

        formatter.output_type(method.return_type, signature)
        signature.append(' ')

        signature.append(method.name)

        signature.append('(')
        formatter.output_list(formatter.output_formal_param, method.parameters, signature, ', ')
        signature.append(')')

        if method.throws:
            signature.append(' throws ')
            formatter.output_list(formatter.output_exception, method.throws, signature, ', ')

        doc = self.__output_doc(method)

        directive = util.Directive('java:method', signature.build())
        directive.add_content(doc)

        return directive

    def compile_type_document(self, imports_block, package, name, declaration):
        """ Compile a complete document, documenting a type and its members """

        outer_type = name.rpartition('.')[0]

        document = util.Document()
        document.add(imports_block)
        document.add_heading(name, '=')

        method_summary = util.StringBuilder()
        document.add_object(method_summary)

        package_dir = util.Directive('java:package', package)
        package_dir.add_option('noindex')
        document.add_object(package_dir)

        # Add type-level documentation
        type_dir = self.compile_type(declaration)
        if outer_type:
            type_dir.add_option('outertype', outer_type)
        document.add_object(type_dir)

        if isinstance(declaration, javalang.tree.EnumDeclaration):
            enum_constants = list(declaration.body.constants)
            enum_constants.sort(key=lambda c: c.name)

            document.add_heading('Enum Constants')
            for enum_constant in enum_constants:
                if self.member_headers:
                    document.add_heading(enum_constant.name, '^')
                c = self.compile_enum_constant(name, enum_constant)
                c.add_option('outertype', name)
                document.add_object(c)

        fields = list(filter(self.filter, declaration.fields))
        if fields:
            document.add_heading('Fields', '-')
            fields.sort(key=lambda f: f.declarators[0].name)
            for field in fields:
                if self.member_headers:
                    document.add_heading(field.declarators[0].name, '^')
                f = self.compile_field(field)
                f.add_option('outertype', name)
                document.add_object(f)

        constructors = list(filter(self.filter, declaration.constructors))
        if constructors:
            document.add_heading('Constructors', '-')
            constructors.sort(key=lambda c: c.name)
            for constructor in constructors:
                if self.member_headers:
                    document.add_heading(constructor.name, '^')
                c = self.compile_constructor(constructor)
                c.add_option('outertype', name)
                document.add_object(c)

        methods = list(filter(self.filter, declaration.methods))
        if methods:
            document.add_heading('Methods', '-')
            methods.sort(key=lambda m: m.name)
            for method in methods:
                if self.member_headers:
                    document.add_heading(method.name, '^')
                m = self.compile_method(method)
                m.add_option('outertype', name)
                document.add_object(m)

        return document

    def compile(self, ast):
        """ Compile autodocs for the given Java syntax tree. Documents will be
        returned documenting each separate type. """

        documents = {}

        imports = util.StringBuilder()
        for imp in ast.imports:
            if imp.static or imp.wildcard:
                continue

            package_parts = []
            cls_parts = []

            for part in imp.path.split('.'):
                if cls_parts or part[0].isupper():
                    cls_parts.append(part)
                else:
                    package_parts.append(part)


            # If the import's final part wasn't capitalized,
            # append it to the class parts anyway so sphinx doesn't complain.
            if cls_parts == []:
                cls_parts.append(package_parts.pop())

            package = '.'.join(package_parts)
            cls = '.'.join(cls_parts)

            imports.append(util.Directive('java:import', package + ' ' + cls).build())
        import_block = imports.build()

        if not ast.package:
            raise ValueError('File must have package declaration')

        package = ast.package.name
        type_declarations = []
        for path, node in ast.filter(javalang.tree.TypeDeclaration):
            if not self.filter(node):
                continue

            classes = [n.name for n in path if isinstance(n, javalang.tree.TypeDeclaration)]
            classes.append(node.name)

            name = '.'.join(classes)
            type_declarations.append((package, name, node))

        for package, name, declaration in type_declarations:
            full_name = package + '.' + name
            document = self.compile_type_document(import_block, package, name, declaration)
            documents[full_name] = (package, name, document.build())
        return documents

    def compile_docblock(self, documented):
        ''' Compiles a single, standalone docblock. '''
        return self.__output_doc(documented).build()
