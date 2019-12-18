"""
This is autodoxy, a sphinx extension providing autodoc-like directives
that are feed with Doxygen files.

It is highly inspired from the autodoc_doxygen sphinx extension, but
adapted to my own needs in SimGrid.
https://github.com/rmcgibbo/sphinxcontrib-autodoc_doxygen

Licence: MIT
Copyright (c) 2015 Robert T. McGibbon
Copyright (c) 2019 Martin Quinson
"""
from __future__ import print_function, absolute_import, division

import os.path
import re
import sys

from six import itervalues
from lxml import etree as ET
from sphinx.ext.autodoc import Documenter, AutoDirective, members_option, ALL
from sphinx.errors import ExtensionError
from sphinx.util import logging


##########################################################################
# XML utils
##########################################################################
def format_xml_paragraph(xmlnode):
    """Format an Doxygen XML segment (principally a detaileddescription)
    as a paragraph for inclusion in the rst document

    Parameters
    ----------
    xmlnode

    Returns
    -------
    lines
        A list of lines.
    """
    return [l.rstrip() for l in _DoxygenXmlParagraphFormatter().generic_visit(xmlnode).lines]


class _DoxygenXmlParagraphFormatter(object):
    # This class follows the model of the stdlib's ast.NodeVisitor for tree traversal
    # where you dispatch on the element type to a different method for each node
    # during the traverse.

    # It's supposed to handle paragraphs, references, preformatted text (code blocks), and lists.

    def __init__(self):
        self.lines = ['']
        self.continue_line = False

    def visit(self, node):
        method = 'visit_' + node.tag
        visitor = getattr(self, method, self.generic_visit)
        return visitor(node)

    def generic_visit(self, node):
        for child in node.getchildren():
            self.visit(child)
        return self

    def visit_ref(self, node):
        ref = get_doxygen_root().findall('.//*[@id="%s"]' % node.get('refid'))
        if ref:
            ref = ref[0]
            if ref.tag == 'memberdef':
                parent = ref.xpath('./ancestor::compounddef/compoundname')[0].text
                name = ref.find('./name').text
                real_name = parent + '::' + name
            elif ref.tag in ('compounddef', 'enumvalue'):
                name_node = ref.find('./name')
                real_name = name_node.text if name_node is not None else ''
            else:
                raise NotImplementedError(ref.tag)
        else:
            real_name = None

        val = [':cpp:any:`', node.text]
        if real_name:
            val.extend((' <', real_name, '>`'))
        else:
            val.append('`')
        if node.tail is not None:
            val.append(node.tail)

        self.lines[-1] += ''.join(val)

    def visit_para(self, node):
        if node.text is not None:
            if self.continue_line:
                self.lines[-1] += node.text
            else:
                self.lines.append(node.text)
        self.generic_visit(node)
        self.lines.append('')
        self.continue_line = False

    def visit_verbatim(self, node):
        if node.text is not None:
            # remove the leading ' *' of any lines
            lines = [re.sub('^\s*\*','', l) for l in node.text.split('\n')]
            # Merge each paragraph together
            text = re.sub("\n\n", "PaRaGrraphSplit", '\n'.join(lines))
            text = re.sub('\n', '', text)
            lines = text.split('PaRaGrraphSplit')

            # merge content to the built doc
            if self.continue_line:
                self.lines[-1] += lines[0]
                lines = lines[1:]
            for l in lines:
                self.lines.append('')
                self.lines.append(l)
        self.generic_visit(node)
        self.lines.append('')
        self.continue_line = False

    def visit_parametername(self, node):
        if 'direction' in node.attrib:
            direction = '[%s] ' % node.get('direction')
        else:
            direction = ''

        self.lines.append('**%s** -- %s' % (
            node.text, direction))
        self.continue_line = True

    def visit_parameterlist(self, node):
        lines = [l for l in type(self)().generic_visit(node).lines if l is not '']
        self.lines.extend([':parameters:', ''] + ['* %s' % l for l in lines] + [''])

    def visit_simplesect(self, node):
        if node.get('kind') == 'return':
            self.lines.append(':returns: ')
            self.continue_line = True
        self.generic_visit(node)

    def visit_listitem(self, node):
        self.lines.append('   - ')
        self.continue_line = True
        self.generic_visit(node)

    def visit_preformatted(self, node):
        segment = [node.text if node.text is not None else '']
        for n in node.getchildren():
            segment.append(n.text)
            if n.tail is not None:
                segment.append(n.tail)

        lines = ''.join(segment).split('\n')
        self.lines.extend(('.. code-block:: C++', ''))
        self.lines.extend(['  ' + l for l in lines])

    def visit_computeroutput(self, node):
        c = node.find('preformatted')
        if c is not None:
            return self.visit_preformatted(c)
        return self.visit_preformatted(node)

    def visit_xrefsect(self, node):
        if node.find('xreftitle').text == 'Deprecated':
            sublines = type(self)().generic_visit(node).lines
            self.lines.extend(['.. admonition:: Deprecated'] + ['   ' + s for s in sublines])
        else:
            raise ValueError(node)

    def visit_subscript(self, node):
        self.lines[-1] += '\ :sub:`%s` %s' % (node.text, node.tail)


##########################################################################
# Directives
##########################################################################


class DoxygenDocumenter(Documenter):
    # Variables to store the names of the object being documented. modname and fullname are redundant,
    # and objpath is always the empty list. This is inelegant, but we need to work with the superclass.

    fullname = None  # example: "OpenMM::NonbondedForce" or "OpenMM::NonbondedForce::methodName""
    modname = None   # example: "OpenMM::NonbondedForce" or "OpenMM::NonbondedForce::methodName""
    objname = None   # example: "NonbondedForce"  or "methodName"
    objpath = []     # always the empty list
    object = None    # the xml node for the object

    option_spec = {
        'members': members_option,
    }

    def __init__(self, directive, name, indent=u'', id=None):
        super(DoxygenDocumenter, self).__init__(directive, name, indent)
        if id is not None:
            self.parse_id(id)

    def parse_id(self, id):
        return False

    def parse_name(self):
        """Determine what module to import and what attribute to document.
        Returns True and sets *self.modname*, *self.objname*, *self.fullname*,
        if parsing and resolving was successful.
        """
        # To view the context and order in which all of these methods get called,
        # See, Documenter.generate(). That's the main "entry point" that first
        # calls parse_name(), follwed by import_object(), format_signature(),
        # add_directive_header(), and then add_content() (which calls get_doc())

        # If we were provided a prototype, that must be an overloaded function. Save it.
        self.argsstring = None
        if "(" in self.name:
            (self.name, self.argsstring) = self.name.split('(', 1)
            self.argsstring = "({}".format(self.argsstring)

        # methods in the superclass sometimes use '.' to join namespace/class
        # names with method names, and we don't want that.
        self.name = self.name.replace('.', '::')
        self.fullname = self.name
        self.modname = self.fullname
        self.objpath = []

        if '::' in self.name:
            parts = self.name.split('::')
            self.klassname = parts[-2]
            self.objname = parts[-1]
        else:
            self.objname = self.name
            self.klassname = ""

        return True

    def add_directive_header(self, sig):
        """Add the directive header and options to the generated content."""
        domain = getattr(self, 'domain', 'cpp')
        directive = getattr(self, 'directivetype', self.objtype)
        name = self.format_name()
        sourcename = self.get_sourcename()
        self.add_line(u'.. %s:%s:: %s%s' % (domain, directive, name, sig),
                      sourcename)

    def document_members(self, all_members=False):
        """Generate reST for member documentation.
        If *all_members* is True, do all members, else those given by
        *self.options.members*.
        """
        want_all = all_members or self.options.inherited_members or \
            self.options.members is ALL
        # find out which members are documentable
        members_check_module, members = self.get_object_members(want_all)

        # remove members given by exclude-members
        if self.options.exclude_members:
            members = [(membername, member) for (membername, member) in members
                       if membername not in self.options.exclude_members]

        # document non-skipped members
        memberdocumenters = []
        for (mname, member, isattr) in self.filter_members(members, want_all):
            classes = [cls for cls in itervalues(AutoDirective._registry)
                       if cls.can_document_member(member, mname, isattr, self)]
            if not classes:
                # don't know how to document this member
                continue

            # prefer the documenter with the highest priority
            classes.sort(key=lambda cls: cls.priority)

            documenter = classes[-1](self.directive, mname, indent=self.indent, id=member.get('id'))
            memberdocumenters.append((documenter, isattr))

        for documenter, isattr in memberdocumenters:
            documenter.generate(
                all_members=True, real_modname=self.real_modname,
                check_module=members_check_module and not isattr)

        # reset current objects
        self.env.temp_data['autodoc:module'] = None
        self.env.temp_data['autodoc:class'] = None


class DoxygenClassDocumenter(DoxygenDocumenter):
    objtype = 'doxyclass'
    directivetype = 'class'
    domain = 'cpp'
    priority = 100

    option_spec = {
        'members': members_option,
    }

    @classmethod
    def can_document_member(cls, member, membername, isattr, parent):
        # this method is only called from Documenter.document_members
        # when a higher level documenter (module or namespace) is trying
        # to choose the appropriate documenter for each of its lower-level
        # members. Currently not implemented since we don't have a higher-level
        # doumenter like a DoxygenNamespaceDocumenter.
        return False

    def import_object(self):
        """Import the object and set it as *self.object*.  In the call sequence, this
        is executed right after parse_name(), so it can use *self.fullname*, *self.objname*,
        and *self.modname*.

        Returns True if successful, False if an error occurred.
        """
        xpath_query = './/compoundname[text()="%s"]/..' % self.fullname
        match = get_doxygen_root().xpath(xpath_query)
        if len(match) != 1:
            raise ExtensionError('[autodoxy] could not find class (fullname="%s"). I tried'
                                 'the following xpath: "%s"' % (self.fullname, xpath_query))

        self.object = match[0]
        return True

    def format_signature(self):
        return ''

    def format_name(self):
        return self.fullname

    def get_doc(self, encoding):
        detaileddescription = self.object.find('detaileddescription')
        doc = [format_xml_paragraph(detaileddescription)]
        return doc

    def get_object_members(self, want_all):
        all_members = self.object.xpath('.//sectiondef[@kind="public-func" '
            'or @kind="public-static-func"]/memberdef[@kind="function"]')

        if want_all:
            return False, ((m.find('name').text, m) for m in all_members)
        if not self.options.members:
            return False, []
        return False, ((m.find('name').text, m) for m in all_members if m.find('name').text in self.options.members)

    def filter_members(self, members, want_all):
        ret = []
        for (membername, member) in members:
            ret.append((membername, member, False))
        return ret

    def document_members(self, all_members=False):
        super(DoxygenClassDocumenter, self).document_members(all_members=all_members)
        # Uncomment to view the generated rst for the class.
        # print('\n'.join(self.directive.result))

class DoxygenMethodDocumenter(DoxygenDocumenter):
    objtype = 'doxymethod'
    directivetype = 'function'
    domain = 'cpp'
    priority = 100

    @classmethod
    def can_document_member(cls, member, membername, isattr, parent):
        if ET.iselement(member) and member.tag == 'memberdef' and member.get('kind') == 'function':
            return True
        return False

    def parse_id(self, id):
        xp = './/*[@id="%s"]' % id
        match = get_doxygen_root().xpath(xp)
        if len(match) > 0:
            match = match[0]
            self.fullname = match.find('./definition').text.split()[-1]
            self.modname = self.fullname
            self.objname = match.find('./name').text
            self.object = match
        return False

    def import_object(self):
        if ET.iselement(self.object):
            # self.object already set from DoxygenDocumenter.parse_name(),
            # caused by passing in the `id` of the node instead of just a
            # classname or method name
            return True

        (obj, meth) = self.fullname.rsplit('::', 1)

        xpath_query_noparam = ('.//compoundname[text()="{:s}"]/../sectiondef[@kind="public-func" or @kind="public-static-func"]'
                               '/memberdef[@kind="function"]/name[text()="{:s}"]/..').format(obj, meth)
        xpath_query = ""
#        print("fullname {}".format(self.fullname))
        if self.argsstring != None:
            xpath_query = ('.//compoundname[text()="{:s}"]/../sectiondef[@kind="public-func" or @kind="public-static-func"]'
                           '/memberdef[@kind="function" and argsstring/text()="{:s}"]/name[text()="{:s}"]/..').format(obj,self.argsstring,meth)
        else:
            xpath_query = xpath_query_noparam
        match = get_doxygen_root().xpath(xpath_query)
        if len(match) == 0:
            logger = logging.getLogger(__name__)

            if self.argsstring != None:
                candidates = get_doxygen_root().xpath(xpath_query_noparam)
                if len(candidates) == 1:
                    logger.warning("[autodoxy] Using method '{}::{}{}' instead of '{}::{}{}'. You may want to drop your specification of the signature, or to fix it."
                                   .format(obj, meth, candidates[0].find('argsstring').text, obj, meth, self.argsstring))
                    self.object = candidates[0]
                    return True
                logger.warning("[autodoxy] WARNING: Could not find method {}::{}{}".format(obj, meth, self.argsstring))
                for cand in candidates:
                    logger.warning("[autodoxy] WARNING:   Existing candidate: {}::{}{}".format(obj, meth, cand.find('argsstring').text))
            else:
                logger.warning("[autodoxy] WARNING: could not find method {}::{} in Doxygen files".format(obj, meth))
            return False
        self.object = match[0]
        return True

    def get_doc(self, encoding):
        detaileddescription = self.object.find('detaileddescription')
        doc = [format_xml_paragraph(detaileddescription)]
        return doc

    def format_name(self):
        def text(el):
            if el.text is not None:
                return el.text
            return ''

        def tail(el):
            if el.tail is not None:
                return el.tail
            return ''

        rtype_el = self.object.find('type')
        rtype_el_ref = rtype_el.find('ref')
        if rtype_el_ref is not None:
            rtype = text(rtype_el) + text(rtype_el_ref) + tail(rtype_el_ref)
        else:
            rtype = rtype_el.text

 #       print("rtype: {}".format(rtype))
        signame = (rtype and (rtype + ' ') or '') + self.klassname + "::"+ self.objname
        return self.format_template_name() + signame

    def format_template_name(self):
        types = [e.text for e in self.object.findall('templateparamlist/param/type')]
        if len(types) == 0:
            return ''
        ret = 'template <%s>' % ','.join(types)
#        print ("template: {}".format(ret))
        return ret

    def format_signature(self):
        args = self.object.find('argsstring').text
        return args

    def document_members(self, all_members=False):
        pass

class DoxygenVariableDocumenter(DoxygenDocumenter):
    objtype = 'doxyvar'
    directivetype = 'var'
    domain = 'cpp'
    priority = 100

    @classmethod
    def can_document_member(cls, member, membername, isattr, parent):
        if ET.iselement(member) and member.tag == 'memberdef' and member.get('kind') == 'variable':
            return True
        return False

    def import_object(self):
        if ET.iselement(self.object):
            # self.object already set from DoxygenDocumenter.parse_name(),
            # caused by passing in the `id` of the node instead of just a
            # classname or method name
            return True

        (obj, var) = self.fullname.rsplit('::', 1)

        xpath_query = ('.//compoundname[text()="{:s}"]/../sectiondef[@kind="public-attrib" or @kind="public-static-attrib"]'
                       '/memberdef[@kind="variable"]/name[text()="{:s}"]/..').format(obj, var)
#        print("fullname {}".format(self.fullname))
        match = get_doxygen_root().xpath(xpath_query)
        if len(match) == 0:
            logger = logging.getLogger(__name__)

            logger.warning("[autodoxy] WARNING: could not find variable {}::{} in Doxygen files".format(obj, var))
            return False
        self.object = match[0]
        return True

    def parse_id(self, id):
        xp = './/*[@id="%s"]' % id
        match = get_doxygen_root().xpath(xp)
        if len(match) > 0:
            match = match[0]
            self.fullname = match.find('./definition').text.split()[-1]
            self.modname = self.fullname
            self.objname = match.find('./name').text
            self.object = match
        return False

    def format_name(self):
        def text(el):
            if el.text is not None:
                return el.text
            return ''

        def tail(el):
            if el.tail is not None:
                return el.tail
            return ''

        rtype_el = self.object.find('type')
        rtype_el_ref = rtype_el.find('ref')
        if rtype_el_ref is not None:
            rtype = text(rtype_el) + text(rtype_el_ref) + tail(rtype_el_ref)
        else:
            rtype = rtype_el.text

 #       print("rtype: {}".format(rtype))
        signame = (rtype and (rtype + ' ') or '') + self.klassname + "::" + self.objname
        return self.format_template_name() + signame

    def get_doc(self, encoding):
        detaileddescription = self.object.find('detaileddescription')
        doc = [format_xml_paragraph(detaileddescription)]
        return doc

    def format_template_name(self):
        types = [e.text for e in self.object.findall('templateparamlist/param/type')]
        if len(types) == 0:
            return ''
        ret = 'template <%s>' % ','.join(types)
#        print ("template: {}".format(ret))
        return ret

    def document_members(self, all_members=False):
        pass


##########################################################################
# Setup the extension
##########################################################################
def set_doxygen_xml(app):
    """Load all doxygen XML files from the app config variable
    `app.config.doxygen_xml` which should be a path to a directory
    containing doxygen xml output
    """
    err = ExtensionError(
        '[autodoxy] No doxygen xml output found in doxygen_xml="%s"' % app.config.doxygen_xml)

    if not os.path.isdir(app.config.doxygen_xml):
        raise err

    files = [os.path.join(app.config.doxygen_xml, f)
             for f in os.listdir(app.config.doxygen_xml)
             if f.lower().endswith('.xml') and not f.startswith('._')]
    if len(files) == 0:
        raise err

    setup.DOXYGEN_ROOT = ET.ElementTree(ET.Element('root')).getroot()
    for file in files:
        root = ET.parse(file).getroot()
        for node in root:
            setup.DOXYGEN_ROOT.append(node)


def get_doxygen_root():
    """Get the root element of the doxygen XML document.
    """
    if not hasattr(setup, 'DOXYGEN_ROOT'):
        setup.DOXYGEN_ROOT = ET.Element("root")  # dummy
    return setup.DOXYGEN_ROOT


def setup(app):
    import sphinx.ext.autosummary

    app.connect("builder-inited", set_doxygen_xml)
#    app.connect("builder-inited", process_generate_options)

    app.setup_extension('sphinx.ext.autodoc')
#    app.setup_extension('sphinx.ext.autosummary')

    app.add_autodocumenter(DoxygenClassDocumenter)
    app.add_autodocumenter(DoxygenMethodDocumenter)
    app.add_autodocumenter(DoxygenVariableDocumenter)
    app.add_config_value("doxygen_xml", "", True)

#    app.add_directive('autodoxysummary', DoxygenAutosummary)
#    app.add_directive('autodoxyenum', DoxygenAutoEnum)

    return {'version': sphinx.__display_version__, 'parallel_read_safe': True}
