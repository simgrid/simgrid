# -*- coding: utf-8 -*-

# Useful doc: https://www.sphinx-doc.org/en/master/extdev/markupapi.html
# Example: https://www.sphinx-doc.org/en/master/development/tutorials/recipe.html

import os
from docutils.parsers.rst import Directive, directives
from docutils import nodes
from docutils.statemachine import StringList
from sphinx.util.osutil import copyfile
from sphinx.util import logging

CSS_FILE = 'showfile.css'
JS_FILE = 'showfile.js'

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

class ToggleDirective(Directive):
    has_content = True
    option_spec = {'header': directives.unchanged}
    optional_arguments = 1

    def run(self):
        node = nodes.container()
        node['classes'].append('toggle-content')

        par = nodes.container()
        par['classes'].append('toggle-header')
        if self.arguments and self.arguments[0]:
            par['classes'].append(self.arguments[0])

        self.state.nested_parse(StringList([self.options["header"]]), self.content_offset, par)
        self.state.nested_parse(self.content, self.content_offset, node)

        return [par, node]

def add_assets(app):
    app.add_stylesheet(CSS_FILE)
    app.add_javascript(JS_FILE)


def copy_assets(app, exception):
    if app.builder.name not in ['html', 'readthedocs'] or exception:
        return
    logger = logging.getLogger(__name__)
    logger.info('Copying showfile stylesheet/javascript... ', nonl=True)
    dest = os.path.join(app.builder.outdir, '_static', CSS_FILE)
    source = os.path.join(os.path.abspath(os.path.dirname(__file__)), CSS_FILE)
    copyfile(source, dest)
    dest = os.path.join(app.builder.outdir, '_static', JS_FILE)
    source = os.path.join(os.path.abspath(os.path.dirname(__file__)), JS_FILE)
    copyfile(source, dest)
    logger.info('done')

def setup(app):
    app.add_directive('toggle-header', ToggleDirective)
    app.add_directive('showfile', ShowFileDirective)
    app.add_directive('example-tab', ExampleTabDirective)

    app.connect('builder-inited', add_assets)
    app.connect('build-finished', copy_assets)

