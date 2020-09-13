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

from __future__ import print_function, unicode_literals

try:
   import cPickle as pickle
except:
   import pickle

import hashlib
import logging
import sys
import os
import os.path

from optparse import OptionParser

import javalang

import javasphinx.compiler as compiler
import javasphinx.util as util

def encode_output(s):
   if isinstance(s, str):
      return s
   else:
      return s.encode('utf-8')

def find_source_files(input_path, excludes):
    """ Get a list of filenames for all Java source files within the given
    directory.

    """

    java_files = []

    input_path = os.path.normpath(os.path.abspath(input_path))

    for dirpath, dirnames, filenames in os.walk(input_path):
        if is_excluded(dirpath, excludes):
            del dirnames[:]
            continue

        for filename in filenames:
            if filename.endswith(".java"):
                java_files.append(os.path.join(dirpath, filename))

    return java_files

def write_toc(packages, opts):
    doc = util.Document()
    doc.add_heading(opts.toc_title, '=')

    toc = util.Directive('toctree')
    toc.add_option('maxdepth', '2')
    doc.add_object(toc)

    for package in sorted(packages.keys()):
        toc.add_content("%s/package-index\n" % package.replace('.', '/'))

    filename = 'packages.' + opts.suffix
    fullpath = os.path.join(opts.destdir, filename)

    if os.path.exists(fullpath) and not (opts.force or opts.update):
        sys.stderr.write(fullpath + ' already exists. Use -f to overwrite.\n')
        sys.exit(1)

    f = open(fullpath, 'w')
    f.write(encode_output(doc.build()))
    f.close()

def write_documents(packages, documents, sources, opts):
    package_contents = dict()

    # Write individual documents
    for fullname, (package, name, document) in documents.items():
        if is_package_info_doc(name):
            continue

        package_path = package.replace('.', os.sep)
        filebasename = name.replace('.', '-')
        filename = filebasename + '.' + opts.suffix
        dirpath = os.path.join(opts.destdir, package_path)
        fullpath = os.path.join(dirpath, filename)

        if not os.path.exists(dirpath):
            os.makedirs(dirpath)
        elif os.path.exists(fullpath) and not (opts.force or opts.update):
            sys.stderr.write(fullpath + ' already exists. Use -f to overwrite.\n')
            sys.exit(1)

        # Add to package indexes
        package_contents.setdefault(package, list()).append(filebasename)

        if opts.update and os.path.exists(fullpath):
            # If the destination file is newer than the source file than skip
            # writing it out
            source_mod_time = os.stat(sources[fullname]).st_mtime
            dest_mod_time = os.stat(fullpath).st_mtime

            if source_mod_time < dest_mod_time:
                continue

        f = open(fullpath, 'w')
        f.write(encode_output(document))
        f.close()

    # Write package-index for each package
    for package, classes in package_contents.items():
        doc = util.Document()
        doc.add_heading(package, '=')

        #Adds the package documentation (if any)
        if packages[package] != '':
            documentation = packages[package]
            doc.add_line("\n%s" % documentation)

        doc.add_object(util.Directive('java:package', package))

        toc = util.Directive('toctree')
        toc.add_option('maxdepth', '1')

        classes.sort()
        for filebasename in classes:
            toc.add_content(filebasename + '\n')
        doc.add_object(toc)

        package_path = package.replace('.', os.sep)
        filename = 'package-index.' + opts.suffix
        dirpath = os.path.join(opts.destdir, package_path)
        fullpath = os.path.join(dirpath, filename)

        if not os.path.exists(dirpath):
            os.makedirs(dirpath)
        elif os.path.exists(fullpath) and not (opts.force or opts.update):
            sys.stderr.write(fullpath + ' already exists. Use -f to overwrite.\n')
            sys.exit(1)

        f = open(fullpath, 'w')
        f.write(encode_output(doc.build()))
        f.close()

def get_newer(a, b):
    if not os.path.exists(a):
        return b

    if not os.path.exists(b):
        return a

    a_mtime = int(os.stat(a).st_mtime)
    b_mtime = int(os.stat(b).st_mtime)

    if a_mtime < b_mtime:
        return b

    return a

def format_syntax_error(e):
    rest = ""
    if e.at.position:
        value = e.at.value
        pos = e.at.position
        rest = ' at %s line %d, character %d' % (value, pos[0], pos[1])
    return e.description + rest

def generate_from_source_file(doc_compiler, source_file, cache_dir):
    if cache_dir:
        fingerprint = hashlib.md5(source_file.encode()).hexdigest()
        cache_file = os.path.join(cache_dir, 'parsed-' + fingerprint + '.p')

        if get_newer(source_file, cache_file) == cache_file:
            return pickle.load(open(cache_file, 'rb'))
    else:
        cache_file = None

    f = open(source_file)
    source = f.read()
    f.close()

    try:
        ast = javalang.parse.parse(source)
    except javalang.parser.JavaSyntaxError as e:
        util.error('Syntax error in %s: %s', source_file, format_syntax_error(e))
    except Exception:
        util.unexpected('Unexpected exception while parsing %s', source_file)

    documents = {}
    try:
        if source_file.endswith("package-info.java"):
            if ast.package is not None:
                documentation = doc_compiler.compile_docblock(ast.package)
                documents[ast.package.name] = (ast.package.name, 'package-info', documentation)
        else:
            documents = doc_compiler.compile(ast)
    except Exception:
        util.unexpected('Unexpected exception while compiling %s', source_file)

    if cache_file:
        dump_file = open(cache_file, 'wb')
        pickle.dump(documents, dump_file)
        dump_file.close()

    return documents

def generate_documents(source_files, cache_dir, verbose, member_headers, parser):
    documents = {}
    sources = {}
    doc_compiler = compiler.JavadocRestCompiler(None, member_headers, parser)

    for source_file in source_files:
        if verbose:
            print('Processing', source_file)

        this_file_documents = generate_from_source_file(doc_compiler, source_file, cache_dir)
        for fullname in this_file_documents:
            sources[fullname] = source_file

        documents.update(this_file_documents)

    #Existing packages dict, where each key is a package name
    #and each value is the package documentation (if any)
    packages = {}

    #Gets the name of the package where the document was declared
    #and adds it to the packages dict with no documentation.
    #Package documentation, if any, will be collected from package-info.java files.
    for package, name, _ in documents.values():
        packages[package] = ""

    #Gets packages documentation from package-info.java documents (if any).
    for package, name, content in documents.values():
        if is_package_info_doc(name):
            packages[package] = content

    return packages, documents, sources

def normalize_excludes(rootpath, excludes):
    f_excludes = []
    for exclude in excludes:
        if not os.path.isabs(exclude) and not exclude.startswith(rootpath):
            exclude = os.path.join(rootpath, exclude)
        f_excludes.append(os.path.normpath(exclude) + os.path.sep)
    return f_excludes

def is_excluded(root, excludes):
    sep = os.path.sep
    if not root.endswith(sep):
        root += sep
    for exclude in excludes:
        if root.startswith(exclude):
            return True
    return False

def is_package_info_doc(document_name):
    ''' Checks if the name of a document represents a package-info.java file. '''
    return document_name == 'package-info'


def main(argv=sys.argv):
    logging.basicConfig(level=logging.WARN)

    parser = OptionParser(
        usage="""\
usage: %prog [options] -o <output_path> <input_path> [exclude_paths, ...]

Look recursively in <input_path> for Java sources files and create reST files
for all non-private classes, organized by package under <output_path>. A package
index (package-index.<ext>) will be created for each package, and a top level
table of contents will be generated named packages.<ext>.

Paths matching any of the given exclude_paths (interpreted as regular
expressions) will be skipped.

Note: By default this script will not overwrite already created files.""")

    parser.add_option('-o', '--output-dir', action='store', dest='destdir',
                      help='Directory to place all output', default='')
    parser.add_option('-f', '--force', action='store_true', dest='force',
                      help='Overwrite all files')
    parser.add_option('-c', '--cache-dir', action='store', dest='cache_dir',
                      help='Directory to stored cachable output')
    parser.add_option('-u', '--update', action='store_true', dest='update',
                      help='Overwrite new and changed files', default=False)
    parser.add_option('-T', '--no-toc', action='store_true', dest='notoc',
                      help='Don\'t create a table of contents file')
    parser.add_option('-t', '--title', dest='toc_title', default='Javadoc',
                      help='Title to use on table of contents')
    parser.add_option('--no-member-headers', action='store_false', default=True, dest='member_headers',
                      help='Don\'t generate headers for class members')
    parser.add_option('-s', '--suffix', action='store', dest='suffix',
                      help='file suffix (default: rst)', default='rst')
    parser.add_option('-I', '--include', action='append', dest='includes',
                      help='Additional input paths to scan', default=[])
    parser.add_option('-p', '--parser', dest='parser_lib', default='lxml',
                      help='Beautiful Soup---html parser library option.')
    parser.add_option('-v', '--verbose', action='store_true', dest='verbose',
                      help='verbose output')

    (opts, args) = parser.parse_args(argv[1:])

    if not args:
        parser.error('A source path is required.')

    rootpath, excludes = args[0], args[1:]

    input_paths = opts.includes
    input_paths.append(rootpath)

    if not opts.destdir:
        parser.error('An output directory is required.')

    if opts.suffix.startswith('.'):
        opts.suffix = opts.suffix[1:]

    for input_path in input_paths:
        if not os.path.isdir(input_path):
            sys.stderr.write('%s is not a directory.\n' % (input_path,))
            sys.exit(1)

    if not os.path.isdir(opts.destdir):
        os.makedirs(opts.destdir)

    if opts.cache_dir and not os.path.isdir(opts.cache_dir):
        os.makedirs(opts.cache_dir)

    excludes = normalize_excludes(rootpath, excludes)
    source_files = []

    for input_path in input_paths:
        source_files.extend(find_source_files(input_path, excludes))

    packages, documents, sources = generate_documents(source_files, opts.cache_dir, opts.verbose,
                                                      opts.member_headers, opts.parser_lib)

    write_documents(packages, documents, sources, opts)

    if not opts.notoc:
        write_toc(packages, opts)
