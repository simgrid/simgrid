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
#        self.assert_has_content()

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

def setup(app):
    app.add_directive('showfile', ShowFileDirective)
