from __future__ import print_function, absolute_import, division
from . import get_doxygen_root


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
