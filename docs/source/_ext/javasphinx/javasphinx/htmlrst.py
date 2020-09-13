#
# Copyright 2013-2015 Bronto Software, Inc. and contributors
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

from __future__ import unicode_literals
from builtins import str

import collections
import re

from xml.sax.saxutils import escape as html_escape
from bs4 import BeautifulSoup

Cell = collections.namedtuple('Cell', ['type', 'rowspan', 'colspan', 'contents'])

class Converter(object):
    def __init__(self, parser):
        self._unknown_tags = set()
        self._clear = '\n\n..\n\n'

        # Regular expressions
        self._preprocess_anchors = re.compile(r'<a\s+name\s*=\s*["\']?(.+?)["\']?\s*>')
        self._post_process_empty_lines = re.compile(r'^\s+$', re.MULTILINE)
        self._post_process_compress_lines = re.compile(r'\n{3,}')
        self._whitespace_with_newline = re.compile(r'[\s\n]+')
        self._whitespace = re.compile(r'\s+')
        self._html_tag = re.compile(r'<.*?>')

        self._preprocess_entity = re.compile(r'&(nbsp|lt|gt|amp)([^;]|[\n])')
        self._parser = parser

    # --------------------------------------------------------------------------
    # ---- reST Utility Methods ----

    def _unicode(self, s):
        if isinstance(s, unicode):
            return s
        else:
            return unicode(s, 'utf8')

    def _separate(self, s):
        return u'\n\n' + s + u'\n\n'

    def _escape_inline(self, s):
        return '\\ ' + s + '\\ '

    def _inline(self, tag, s):
        # Seems fishy if our inline markup spans lines. We will instead just return
        # the string as is
        if '\n' in s:
            return s

        s = s.strip()

        if not s:
            return s

        return self._escape_inline(tag + s.strip() + tag)

    def _role(self, role, s, label=None):
        if label:
            return self._escape_inline(':%s:`%s <%s>`' % (role, label, s))
        else:
            return self._escape_inline(':%s:`%s`' % (role, s))

    def _directive(self, directive, body=None):
        header = '\n\n.. %s::\n\n' % (directive,)

        if body:
            return header + self._left_justify(body, 3) + '\n\n'
        else:
            return header + '\n'

    def _hyperlink(self, target, label):
        return self._escape_inline('`%s <%s>`_' % (label, target))

    def _listing(self, marker, items):
        items = [self._left_justify(item, len(marker) + 1) for item in items]
        items = [marker + item[len(marker):] for item in items]
        return self._separate('..') + self._separate('\n'.join(items))

    def _left_justify(self, s, indent=0):
        lines = [l.rstrip() for l in s.split('\n')]
        indents = [len(l) - len(l.lstrip()) for l in lines if l]

        if not indents:
            return s

        shift = indent - min(indents)

        if shift < 0:
            return '\n'.join(l[-shift:] for l in lines)
        else:
            prefix = ' ' * shift
            return '\n'.join(prefix + l for l in lines)

    def _compress_whitespace(self, s, replace=' ', newlines=True):
        if newlines:
            return self._whitespace_with_newline.sub(replace, s)
        else:
            return self._whitespace.sub(replace, s)

    # --------------------------------------------------------------------------
    # ---- DOM Tree Processing ----

    def _process_table_cells(self, table):
        """ Compile all the table cells.

        Returns a list of rows. The rows may have different lengths because of
        column spans.

        """

        rows = []

        for i, tr in enumerate(table.find_all('tr')):
            row = []

            for c in tr.contents:
                cell_type = getattr(c, 'name', None)

                if cell_type not in ('td', 'th'):
                    continue

                rowspan = int(c.attrs.get('rowspan', 1))
                colspan = int(c.attrs.get('colspan', 1))
                contents = self._process_children(c).strip()

                if cell_type == 'th' and i > 0:
                    contents = self._inline('**', contents)

                row.append(Cell(cell_type, rowspan, colspan, contents))

            rows.append(row)

        return rows

    def _process_table(self, node):
        rows = self._process_table_cells(node)

        if not rows:
            return ''

        table_num_columns = max(sum(c.colspan for c in row) for row in rows)

        normalized = []

        for row in rows:
            row_num_columns = sum(c.colspan for c in row)

            if row_num_columns < table_num_columns:
                cell_type = row[-1].type if row else 'td'
                row.append(Cell(cell_type, 1, table_num_columns - row_num_columns, ''))

        col_widths = [0] * table_num_columns
        row_heights = [0] * len(rows)

        for i, row in enumerate(rows):
            j = 0
            for cell in row:
                current_w = sum(col_widths[j:j + cell.colspan])
                required_w = max(len(l) for l in cell.contents.split('\n'))

                if required_w > current_w:
                    additional = required_w - current_w
                    col_widths[j] += additional - (cell.colspan - 1) * (additional // cell.colspan)
                    for jj in range(j + 1, j + cell.colspan):
                        col_widths[jj] += (additional // cell.colspan)

                current_h = row_heights[i]
                required_h = len(cell.contents.split('\n'))

                if required_h > current_h:
                    row_heights[i] = required_h

                j += cell.colspan

        row_sep = '+' + '+'.join('-' * (l + 2) for l in col_widths) + '+'
        header_sep = '+' + '+'.join('=' * (l + 2) for l in col_widths) + '+'
        lines = [row_sep]

        for i, row in enumerate(rows):
            for y in range(0, row_heights[i]):
                line = []
                j = 0
                for c in row:
                    w = sum(n + 3 for n in col_widths[j:j+c.colspan]) - 2
                    h = row_heights[i]

                    line.append('| ')
                    cell_lines = c.contents.split('\n')
                    content = cell_lines[y] if y < len(cell_lines) else ''
                    line.append(content.ljust(w))

                    j += c.colspan

                line.append('|')
                lines.append(''.join(line))

            if i == 0 and all(c.type == 'th' for c in row):
                lines.append(header_sep)
            else:
                lines.append(row_sep)

        return self._separate('\n'.join(lines))

    def _process_children(self, node):
        parts = []
        is_newline = False

        for c in node.contents:
            part = self._process(c)

            if is_newline:
                part = part.lstrip()

            if part:
                parts.append(part)
                is_newline = part.endswith('\n')

        return ''.join(parts)

    def _process_text(self, node):
        return ''.join(node.strings)

    def _process(self, node):
        if isinstance(node, str):
            return self._compress_whitespace(node)

        simple_tags = {
            'b'      : lambda s: self._inline('**', s),
            'strong' : lambda s: self._inline('**', s),
            'i'      : lambda s: self._inline('*', s),
            'em'     : lambda s: self._inline('*', s),
            'tt'     : lambda s: self._inline('``', s),
            'code'   : lambda s: self._inline('``', s),
            'h1'     : lambda s: self._inline('**', s),
            'h2'     : lambda s: self._inline('**', s),
            'h3'     : lambda s: self._inline('**', s),
            'h4'     : lambda s: self._inline('**', s),
            'h5'     : lambda s: self._inline('**', s),
            'h6'     : lambda s: self._inline('**', s),
            'sub'    : lambda s: self._role('sub', s),
            'sup'    : lambda s: self._role('sup', s),
            'hr'     : lambda s: self._separate('') # Transitions not allowed
            }

        if node.name in simple_tags:
            return simple_tags[node.name](self._process_text(node))

        if node.name == 'p':
            return self._separate(self._process_children(node).strip())

        if node.name == 'pre':
            return self._directive('parsed-literal', self._process_text(node))

        if node.name == 'a':
            if 'name' in node.attrs:
                return self._separate('.. _' + node['name'] + ':')
            elif 'href' in node.attrs:
                target = node['href']
                label = self._compress_whitespace(self._process_text(node).strip('\n'))

                if target.startswith('#'):
                    return self._role('ref', target[1:], label)
                elif target.startswith('@'):
                    return self._role('java:ref', target[1:], label)
                else:
                    return self._hyperlink(target, label)

        if node.name == 'ul':
            items = [self._process(n) for n in node.find_all('li', recursive=False)]
            return self._listing('*', items)

        if node.name == 'ol':
            items = [self._process(n) for n in node.find_all('li', recursive=False)]
            return self._listing('#.', items)

        if node.name == 'li':
            s = self._process_children(node)
            s = s.strip()

            # If it's multiline clear the end to correcly support nested lists
            if '\n' in s:
                s = s + '\n\n'

            return s

        if node.name == 'table':
            return self._process_table(node)

        self._unknown_tags.add(node.name)

        return self._process_children(node)

    # --------------------------------------------------------------------------
    # ---- HTML Preprocessing ----

    def _preprocess_inline_javadoc_replace(self, tag, f, s):
        parts = []

        start = '{@' + tag
        start_length = len(start)

        i = s.find(start)
        j = 0

        while i != -1:
            parts.append(s[j:i])

            # Find a closing bracket such that the brackets are balanced between
            # them. This is necessary since code examples containing { and } are
            # commonly wrapped in {@code ...} tags

            try:
                j = s.find('}', i + start_length) + 1
                while s.count('{', i, j) != s.count('}', i, j):
                    j = s.index('}', j) + 1
            except ValueError:
                raise ValueError('Unbalanced {} brackets in ' + tag + ' tag')

            parts.append(f(s[i + start_length:j - 1].strip()))
            i = s.find(start, j)

        parts.append(s[j:])

        return ''.join(parts)

    def _preprocess_replace_javadoc_link(self, s):
        s = self._compress_whitespace(s)

        target = None
        label = ''

        if ' ' not in s:
            target = s
        else:
            i = s.find(' ')

            while s.count('(', 0, i) != s.count(')', 0, i):
                i = s.find(' ', i + 1)

                if i == -1:
                    i = len(s)
                    break

            target = s[:i]
            label = s[i:]

        if target[0] == '#':
            target = target[1:]

        target = target.replace('#', '.').replace(' ', '').strip()

        # Strip HTML tags from the target
        target = self._html_tag.sub('', target)

        label = label.strip()

        return '<a href="@%s">%s</a>' % (target, label)

    def _preprocess_close_anchor_tags(self, s):
        # Add closing tags to all anchors so they are better handled by the parser
        return self._preprocess_anchors.sub(r'<a name="\1"></a>', s)

    def _preprocess_fix_entities(self, s):
        return self._preprocess_entity.sub(r'&\1;\2', s)

    def _preprocess(self, s_html):
        to_tag = lambda t: lambda m: '<%s>%s</%s>' % (t, html_escape(m), t)
        s_html = self._preprocess_inline_javadoc_replace('code', to_tag('code'), s_html)
        s_html = self._preprocess_inline_javadoc_replace('literal', to_tag('span'), s_html)
        s_html = self._preprocess_inline_javadoc_replace('docRoot', lambda m: '', s_html)
        s_html = self._preprocess_inline_javadoc_replace('linkplain', self._preprocess_replace_javadoc_link, s_html)
        s_html = self._preprocess_inline_javadoc_replace('link', self._preprocess_replace_javadoc_link, s_html)

        # Make sure all anchor tags are closed
        s_html = self._preprocess_close_anchor_tags(s_html)

        # Fix up some entitities without closing ;
        s_html = self._preprocess_fix_entities(s_html)

        return s_html

    # --------------------------------------------------------------------------
    # ---- Conversion entry point ----

    def convert(self, s_html):
        if not isinstance(s_html, str):
            s_html = str(s_html, 'utf8')

        s_html = self._preprocess(s_html)

        if not s_html.strip():
            return ''

        soup = BeautifulSoup(s_html, self._parser)
        top = soup.html.body

        result = self._process_children(top)

        # Post processing
        result = self._post_process_empty_lines.sub('', result)
        result = self._post_process_compress_lines.sub('\n\n', result)
        result = result.strip()

        return result
