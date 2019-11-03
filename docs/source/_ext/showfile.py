# -*- coding: utf-8 -*-

import os
from docutils.parsers.rst import Directive, directives
from docutils import nodes
from docutils.statemachine import StringList
from sphinx.util.osutil import copyfile
from sphinx.util import logging

class ShowFileDirective(Directive):
    """
    Show a file or propose it to download.
    """

    has_content = False
    optional_arguments = 1
    option_spec = {
      'language': directives.unchanged
    }

    def run(self):

        filename = self.arguments[0]
        language = "python"
        if 'language' in self.options:
            language = self.options['language']

        logger = logging.getLogger(__name__)
#        logger.info('showfile {} in {}'.format(filename, language))

        new_content = [
          '.. toggle-header::',
          '   :header: View {}'.format(filename),
          '',
          '   `Download {} <https://framagit.org/simgrid/simgrid/tree/{}>`_'.format(os.path.basename(filename), filename),
          '',
          '   .. literalinclude:: ../../{}'.format(filename),
          '      :language: {}'.format(language),
          ''
        ]

        for idx, line in enumerate(new_content):
#            logger.info('{} {}'.format(idx,line))
            self.content.data.insert(idx, line)
            self.content.items.insert(idx, (None, idx))

        node = nodes.container()
        self.state.nested_parse(self.content, self.content_offset, node)
        return node.children

class ExampleTabDirective(Directive):
    """
    A group-tab for a given language, in the presentation of the examples.
    """
    has_content = True
    optional_arguments = 0
    mandatory_argument = 0

    def run(self):
        self.assert_has_content()

        filename = self.content[0].strip()
        self.content.trim_start(1)

        (language, langcode) = (None, None)
        if filename[-3:] == '.py':
            language = 'Python'
            langcode = 'py'
        elif filename[-4:] == '.cpp':
            language = 'C++'
            langcode = 'cpp'
        elif filename[-4:] == '.xml':
            language = 'XML'
            langcode = 'xml'
        else:
            raise Exception("Unknown language '{}'. Please choose '.cpp', '.py' or '.xml'".format(language))

        for idx, line in enumerate(self.content.data):
            self.content.data[idx] = '   ' + line

        for idx, line in enumerate([
            '.. group-tab:: {}'.format(language),
            '   ']):
            self.content.data.insert(idx, line)
            self.content.items.insert(idx, (None, idx))

        for line in [
            '',
            '   .. showfile:: {}'.format(filename),
            '      :language: {}'.format(langcode),
            '']:
            idx = len(self.content.data)
            self.content.data.insert(idx, line)
            self.content.items.insert(idx, (None, idx))

#        logger = logging.getLogger(__name__)
#        logger.info('------------------')
#        for line in self.content.data:
#            logger.info('{}'.format(line))

        node = nodes.container()
        self.state.nested_parse(self.content, self.content_offset, node)
        return node.children

def setup(app):
    app.add_directive('showfile', ShowFileDirective)
    app.add_directive('example-tab', ExampleTabDirective)

