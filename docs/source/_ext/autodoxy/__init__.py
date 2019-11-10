import os.path
from lxml import etree as ET
from sphinx.errors import ExtensionError


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
    from autodoxy import DoxygenClassDocumenter, DoxygenMethodDocumenter, DoxygenTypeDocumenter
    from autodoxy.autosummary import DoxygenAutosummary, DoxygenAutoEnum
    from autodoxy.autosummary.generate import process_generate_options

    app.connect("builder-inited", set_doxygen_xml)
    app.connect("builder-inited", process_generate_options)

    app.setup_extension('sphinx.ext.autodoc')
    app.setup_extension('sphinx.ext.autosummary')

    app.add_autodocumenter(DoxygenClassDocumenter)
    app.add_autodocumenter(DoxygenMethodDocumenter)
    app.add_autodocumenter(DoxygenTypeDocumenter)
    app.add_config_value("doxygen_xml", "", 'env')
    app.add_config_value('autosummary_toctree', '', 'html')

    app.add_directive('autodoxysummary', DoxygenAutosummary)
    app.add_directive('autodoxyenum', DoxygenAutoEnum)

    return {'version': sphinx.__display_version__, 'parallel_read_safe': True}
