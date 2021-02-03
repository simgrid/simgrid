# -*- coding: utf-8 -*-
#
# Configuration file for the Sphinx documentation builder.
#
# This file does only contain a selection of the most common options. For a
# full list see the documentation:
# http://www.sphinx-doc.org/en/master/config

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
import os
import subprocess

# Search for our extensions too
import sys
sys.path.append(os.path.abspath('_ext'))

# -- Run doxygen on readthedocs.org ------------------------------------------

read_the_docs_build = os.environ.get('READTHEDOCS', None) == 'True'

if read_the_docs_build:
    subprocess.call('pwd', shell=True) # should be in docs/source
    subprocess.call('doxygen', shell=True)
    subprocess.call('javasphinx-apidoc --force -o java/ ../../src/bindings/java/org/simgrid/msg', shell=True)
    subprocess.call('rm java/packages.rst', shell=True)

# -- Project information -----------------------------------------------------

project = u'SimGrid'
copyright = u'2002-2021, The SimGrid Team'
author = u'The SimGrid Team'

# The short X.Y version
version = u'3.26.1'

# -- General configuration ---------------------------------------------------

# If your documentation needs a minimal Sphinx version, state it here.
#
# needs_sphinx = '1.0'

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    'sphinx.ext.todo',
    'breathe',
    'sphinx.ext.autodoc',
    'sphinx.ext.intersphinx',
    'sphinx.ext.autosummary',
    'sphinx_tabs.tabs',
    'javasphinx',
    'showfile',
    'autodoxy',
]

todo_include_todos = True

# Setup the breath extension
breathe_projects = {'simgrid': '../build/xml'}
breathe_default_project = "simgrid"

# Setup the autodoxy extension
doxygen_xml = os.path.join(os.path.dirname(__file__), "..", "build", "xml")
autodoxy_requalified_identifiers =  [ # The first element will be substituted into the second one if it's starting an element or preceded by a space
    ("Activity",       "simgrid::s4u::Activity"),
    ("Actor",          "simgrid::s4u::Actor"),
    ("ActorPtr",       "simgrid::s4u::ActorPtr"),
    ("Barrier",        "simgrid::s4u::Barrier"),
    ("BarrierPtr",     "simgrid::s4u::BarrierPtr"),
    ("Comm",           "simgrid::s4u::Comm"),
    ("CommPtr",        "simgrid::s4u::CommPtr"),
    ("ConditionVariable",    "simgrid::s4u::ConditionVariable"),
    ("ConditionVariablePtr", "simgrid::s4u::ConditionVariablePtr"),
    ("Disk",           "simgrid::s4u::Disk"),
    ("Exec",           "simgrid::s4u::Exec"),
    ("ExecPtr",        "simgrid::s4u::ExecPtr"),
    ("Host",           "simgrid::s4u::Host"),
    ("s4u::Host",      "simgrid::s4u::Host"),
    ("Engine",         "simgrid::s4u::Engine"),
    ("Io",             "simgrid::s4u::Io"),
    ("Link",           "simgrid::s4u::Link"),
    ("Mailbox",        "simgrid::s4u::Mailbox"),
    ("Mutex",          "simgrid::s4u::Mutex"),
    ("s4u::Mutex",     "simgrid::s4u::Mutex"),
    ("s4u::MutexPtr",  "simgrid::s4u::MutexPtr"),
    ("NetZone",        "simgrid::s4u::NetZone"),
    ("Semaphore",      "simgrid::s4u::Semaphore"),
    ("VirtualMachine", "simgrid::s4u::VirtualMachine"),
  ]

# Generate a warning for all a cross-reference (such as :func:`myfunc`) that cannot be found
nitpicky = True
nitpick_ignore = [
  ('cpp:identifier', 'boost'),
  ('cpp:identifier', 'boost::intrusive_ptr<Activity>'),
  ('cpp:identifier', 'boost::intrusive_ptr<Actor>'),
  ('cpp:identifier', 'boost::intrusive_ptr<Barrier>'),
  ('cpp:identifier', 'boost::intrusive_ptr<ConditionVariable>'),
  ('cpp:identifier', 'boost::intrusive_ptr<Comm>'),
  ('cpp:identifier', 'boost::intrusive_ptr<Exec>'),
  ('cpp:identifier', 'boost::intrusive_ptr<Io>'),
  ('cpp:identifier', 'boost::intrusive_ptr<Mutex>'),
  ('cpp:identifier', 'boost::intrusive_ptr<Semaphore>'),
  ('cpp:identifier', 'kernel'),
  ('cpp:identifier', 'kernel::activity'),
  ('cpp:identifier', 'kernel::profile'),
  ('cpp:identifier', 'kernel::resource'),
  ('cpp:identifier', 'kernel::routing'),
  ('cpp:identifier', 'simgrid'),
  ('cpp:identifier', 'simgrid::s4u'),
  ('cpp:identifier', 'simgrid::xbt'),
  ('cpp:identifier', 'this_actor'),
  ('cpp:identifier', 's4u'),
  ('cpp:identifier', 's4u_Actor'),
  ('cpp:identifier', 's4u_Barrier'),
  ('cpp:identifier', 's4u_ConditionVariable'),
  ('cpp:identifier', 's4u_Host'),
  ('cpp:identifier', 's4u_NetZone'),
  ('cpp:identifier', 's4u_Mutex'),
  ('cpp:identifier', 's4u_Link'),
  ('cpp:identifier', 's4u_Semaphore'),
  ('cpp:identifier', 's4u_VirtualMachine'),
  ('cpp:identifier', 's4u_VM'),
  ('cpp:identifier', 'size_t'),
  ('cpp:identifier', 'uint64_t'),
  ('cpp:identifier', 'xbt'),
  ('cpp:identifier', 'xbt::string'),
]

# For cross-ref generation
primary_domain = 'cpp'

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# The suffix(es) of source filenames.
# You can specify multiple suffix as a list of string: ['.rst', '.md']
source_suffix = '.rst'

# The master toctree document.
master_doc = 'index'

# The language for content autogenerated by Sphinx. Refer to documentation
# for a list of supported languages.
#
# This is also used if you do content translation via gettext catalogs.
# Usually you set "language" from the command line for these cases.
language = None

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path .
exclude_patterns = []

# The name of the Pygments (syntax highlighting) style to use.
pygments_style = 'sphinx'

# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = 'sphinx_rtd_theme'

# Theme options are theme-specific and customize the look and feel of a theme
# further.  For a list of options available for each theme, see the
# documentation.
#
html_theme_options = {
    'navigation_depth': 4,
    'sticky_navigation': True,
    'display_version': True,
    'includehidden': True,
}

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

# Custom sidebar templates, must be a dictionary that maps document names
# to template names.
#
# The default sidebars (for documents that don't match any pattern) are
# defined by theme itself.  Builtin themes are using these templates by
# default: ``['localtoc.html', 'relations.html', 'sourcelink.html',
# 'searchbox.html']``.
#
# html_sidebars = {'**': ['localtoc.html', 'relations.html', 'searchbox.html']}

# -- Options for HTMLHelp output ---------------------------------------------

# Output file base name for HTML help builder.
htmlhelp_basename = 'simgrid-doc'

# -- Options for GitLab integration ------------------------------------------

html_context = {
    "display_gitlab": True,  # Integrate Gitlab
    "gitlab_host": "framagit.org",
    "gitlab_user": "simgrid",
    "gitlab_repo": "simgrid",
    "gitlab_version": "master",  # Version
    "conf_py_path": "/docs/source/",  # Path in the checkout to the docs root
}

html_css_files = [
    'css/custom.css',
]

# -- Other options
