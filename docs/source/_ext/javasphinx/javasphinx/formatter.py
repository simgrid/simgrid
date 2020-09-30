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

"""
Convert Java syntax tree nodes to string representations.

"""

import javalang

from .util import StringBuilder

# The order for displaying modifiers
__modifiers_order = ('public', 'protected', 'private', 'static', 'abstract', 'final',
                     'native', 'synchronized', 'transient', 'volatile', 'strictfp')

def formatter(f):
    def _f(node, output=None, **kwargs):
        if output is None:
            output = StringBuilder()

        f(node, output, **kwargs)
        return output
    return _f

def output_list(f, items, output=None, sep=', '):
    if items:
        f(items[0], output)
        for item in items[1:]:
            output.append(sep)
            f(item, output)

@formatter
def output_annotation(annotation, output):
    output.append('@')
    output.append(annotation.name)
    output.append(' ')

@formatter
def output_type(type, output, with_generics=True):
    if not type:
        output.append('void')
        return

    if type.dimensions:
        dim = '[]' * len(type.dimensions)
    else:
        dim = ''

    if isinstance(type, javalang.tree.BasicType):
        output.append(type.name)
    else:
        while type:
            output.append(type.name)

            if with_generics:
                output_type_args(type.arguments, output)

            type = type.sub_type

            if type:
                output.append('.')
    output.append(dim)

@formatter
def output_exception(exception, output):
    output.append(exception)

@formatter
def output_type_arg(type_arg, output):
    if type_arg.pattern_type == '?':
        output.append('?')
    else:
        if type_arg.pattern_type:
            output.append('? ')
            output.append(type_arg.pattern_type)
            output.append(' ')

        output_type(type_arg.type, output)

@formatter
def output_type_args(type_args, output):
    if type_args:
        output.append('<')
        output_list(output_type_arg, type_args, output, ', ')
        output.append('>')

@formatter
def output_type_param(type_param, output):
    output.append(type_param.name)

    if type_param.extends:
        output.append(' extends ')
        output_list(output_type, type_param.extends, output, ' & ')

@formatter
def output_type_params(type_params, output):
    if type_params:
        output.append('<')
        output_list(output_type_param, type_params, output, ', ')
        output.append('>')

@formatter
def output_declaration(declaration, output):
    for annotation in declaration.annotations:
        output_annotation(annotation, output)

    output_modifiers(declaration.modifiers, output)
    output.append(' ')

    if isinstance(declaration, javalang.tree.ClassDeclaration):
        output.append('class ')
    elif isinstance(declaration, javalang.tree.EnumDeclaration):
        output.append('enum ')
    elif isinstance(declaration, javalang.tree.InterfaceDeclaration):
        output.append('interface ')
    elif isinstance(declaration, javalang.tree.AnnotationDeclaration):
        output.append('@interface ')

    output.append(declaration.name)

    if isinstance(declaration, (javalang.tree.ClassDeclaration, javalang.tree.InterfaceDeclaration)):
        output_type_params(declaration.type_parameters, output)

    if isinstance(declaration, javalang.tree.ClassDeclaration) and declaration.extends:
        output.append(' extends ')
        output_type(declaration.extends, output)

    if isinstance(declaration, javalang.tree.InterfaceDeclaration) and declaration.extends:
        output.append(' extends ')
        output_list(output_type, declaration.extends, output, ', ')

    if isinstance(declaration, (javalang.tree.ClassDeclaration, javalang.tree.EnumDeclaration)) and declaration.implements:
        output.append(' implements ')
        output_list(output_type, declaration.implements, output, ', ')

@formatter
def output_formal_param(param, output):
    output_type(param.type, output)

    if param.varargs:
        output.append('...')

    output.append(' ')
    output.append(param.name)

@formatter
def output_modifiers(modifiers, output):
    ordered_modifiers = [mod for mod in __modifiers_order if mod in modifiers]
    output_list(lambda mod, output: output.append(mod), ordered_modifiers, output, ' ')
