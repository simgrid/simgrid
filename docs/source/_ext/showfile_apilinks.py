# -*- coding: utf-8 -*-
"""
API-linking for showfile-generated code blocks (Python, Java, C, C++).

Turns `var.method(...)`, `var->method(...)`, `Class::method(...)` and bare `name(...)` calls found inside `.. showfile::` blocks
into links to the matching entry in the API docs, when resolution is unambiguous.

Resolution order for a qualified call (`var.method`, `var->method`, `Class::method`):
  1. per-block hint     (:hints: var=Type on the showfile/example-tab block)
  2. global hint        (`showfile_hints` dict in conf.py)
  3. for `::`-calls only: the left-hand side itself, if it's a known class/ namespace short name (`Activity::self()` --
     "Activity" is not a hinted variable, it *is* the type)
  4. symbol table lookup by bare method/function name -- only if exactly 1 candidate exists in that language

Resolution for bare `name(...)`: direct lookup by short name, only if exactly 1 candidate.

A hint (or a `Class::` prefix) may itself be a short, unqualified type name ("Exec", not "simgrid::s4u::Exec") -- it goes
through the same "exactly 1 candidate or warn" resolution as everything else, against a class/struct- only symbol table.

Anything ambiguous (2+ candidates, no hint) emits a Sphinx warning at build time and is left unlinked. That warning is the
signal to add a :hints: entry (or a showfile_hints entry in conf.py, for a name that's ambiguous everywhere).

## How it works

Symbol tables are built once, per language, from each domain's `get_objects()` on 'env-updated', without digging into the domain
internals.

 - python  -- env.domains['py']    (autodoc), join='.',  seps=['.'] 
 - java    -- env.domains['java']  (javadomain.py),  join='.', seps=['.'] 
 - cpp     -- env.domains['cpp']   (Breathe),  join='::', seps=['.', '->', '::'] 
 - c       -- env.domains['c']     (Breathe),  flat (no classes): bare-name only

Linking happens at HTML-write time, by wrapping `PygmentsBridge.highlight_block`: after the normal Pygments call produces
highlighted HTML, we regex-substitute identifiers that resolve to an API entry with `<a>` tags.

Per-block hints travel from the directive (read phase) to the highlighter (write phase) as a plain attribute stashed on the
literal_block node itself (`node['showfile_hints'] = {...}`) by showfile.py's ShowFileDirective.
`PygmentsBridge.highlight_block` receives that same node as its `location` argument (Sphinx passes `location=node` from
visit_literal_block), so we can read the hints back off it directly -- no shared/global per-block state.

## Known limitations

- Regex-based matching on Pygments' HTML output, not a real parser per language. Verify against a real build for each lexer:
  this assumes Pygments consistently tags identifiers as `<span class="n">` and `.`/`->`/`::`  as `<span class="o">` for the
  C/C++/Java lexers, the same way it does for Python. Worth spot-checking the generated HTML once per language before trusting
  this in production. A miss just means no link, never a wrong link.
- In C, constructions such as `struct_ptr->something(...)` are not handled, but it's not used in SimGrid.
- No overload disambiguation beyond what each domain already does itself (e.g. Java's `javamethod` keeps only the
  last-registered overload -- see javadomain.py's own documented limitation). A link may land on the "wrong" overload's anchor;
  it still lands on the right method's doc entry.
"""

import re

from docutils import nodes
from sphinx.highlighting import PygmentsBridge
from sphinx.util import logging

logger = logging.getLogger(__name__)

# ---------------------------------------------------------------------------
# Per-language configuration
# ---------------------------------------------------------------------------

# Canonicalize whatever string ends up in :language:/lang -- ShowFileDirective defaults to the full word "python", but
# example-tab emits Pygments' short aliases ("py", "cpp", "c", "java"). Both must resolve to the same key here.
_LANG_ALIASES = {
    'python': 'python', 'py': 'python', 'py3': 'python',
    'java': 'java',
    'cpp': 'cpp', 'c++': 'cpp', 'cxx': 'cpp',
    'c': 'c',
}

_DOMAIN_FOR_LANG = {'python': 'py', 'java': 'java', 'cpp': 'cpp', 'c': 'c'}
_JOIN_FOR_LANG = {'python': '.', 'java': '.', 'cpp': '::', 'c': None}
_SEPS_FOR_LANG = {'python': ['.'], 'java': ['.'], 'cpp': ['.', '->', '::'], 'c': []}
_CLASS_TYPES_FOR_LANG = {
    'python': {'class'},
    'java': {'class'},
    'cpp': {'class', 'struct'},
    'c': set(),
}

# Breathe/the cpp domain also registers each function's individual parameters (and template parameters) as their own domain
# objects, e.g. 'simgrid::s4u::Disk::read_async::size' with objtype 'functionParam' -- so that Doxygen-style docs can
# cross-reference a single parameter. These are never something a shown code call should resolve to (a call to `size(...)` isn't
# a call to the *parameter* named size on some unrelated function), so they're excluded outright rather than polluting the
# method/function tables.
_EXCLUDE_TYPES_FOR_LANG = {
    'python': set(),
    'java': set(),
    'cpp': {'functionParam', 'templateParam'},
    'c': {'functionParam'},
}

# How each separator appears in Pygments' HTML output.
_SEP_HTML = {'.': r'\.', '->': r'-&gt;', '::': r'::'}
_SEP_LITERAL = {'.': '.', '->': '-&gt;', '::': '::'}


def _preceded_by_separator(html, pos, lang):
    """True if the text right before `pos` ends with '<span class="o">SEP</span>' for one of this language's separators --
    i.e. `pos` is the *method* half of a qualified call already handled (or attempted) by the call regex, and must NOT be
    re-attempted as a bare name (that's the double-warning bug)."""
    for sep in _SEPS_FOR_LANG[lang]:
        tag = '<span class="o">{}</span>'.format(_SEP_LITERAL[sep])
        if html[:pos].endswith(tag):
            return True
    return False


# ---------------------------------------------------------------------------
# Symbol tables (built once, after all docs are read), one set per language
# ---------------------------------------------------------------------------

_symbol_table = {lang: {} for lang in _LANG_ALIASES.values()}       # lang -> {qualname: (docname, anchor)}
_by_short_name = {lang: {} for lang in _LANG_ALIASES.values()}      # lang -> {short_name: [qualname, ...]}
_class_by_short_name = {lang: {} for lang in _LANG_ALIASES.values()}  # lang -> {short_class_name: [qualname, ...]}


def build_symbol_table(app, env):
    for lang in _symbol_table:
        _symbol_table[lang].clear()
        _by_short_name[lang].clear()
        _class_by_short_name[lang].clear()

        domain = env.domains.get(_DOMAIN_FOR_LANG[lang])
        if domain is None:
            continue

        class_types = _CLASS_TYPES_FOR_LANG[lang]
        exclude_types = _EXCLUDE_TYPES_FOR_LANG[lang]
        for name, dispname, objtype, docname, anchor, priority in domain.get_objects():
            if objtype in exclude_types:
                continue
            _symbol_table[lang][name] = (docname, anchor)
            short = name.rsplit(_JOIN_FOR_LANG[lang] or '.', 1)[-1] if _JOIN_FOR_LANG[lang] else name
            _by_short_name[lang].setdefault(short, []).append(name)
            if objtype in class_types:
                _class_by_short_name[lang].setdefault(short, []).append(name)


# ---------------------------------------------------------------------------
# Resolution
# ---------------------------------------------------------------------------

def _resolve_from(table, short_name, kind, source_label):
    candidates = sorted(set(table.get(short_name, [])))
    if len(candidates) == 1:
        return candidates[0]
    if len(candidates) > 1:
        logger.warning(
            "%s: ambiguous %s reference '%s'. %d candidates: %s "
            "Add a :hints: entry on the block, or a showfile_hints entry in conf.py.",
            source_label, kind, short_name, len(candidates), ", ".join(candidates))
    return None


def resolve_call(lang, var, sep, method, hints, ignore_qualified, source_label):
    """Resolve `var.method`, `var->method` or `Class::method`. Returns (docname, anchor) or None."""
    if (var, method) in ignore_qualified:
        return None  # explicitly excluded via :ignore: -- skip silently, no warning

    join = _JOIN_FOR_LANG[lang]
    if join is None:  # flat/no-OOP language (C): nothing to do here
        return None

    type_short = hints.get(var)
    if type_short is None and sep == '::':
        # `Class::method()` -- the left-hand side *is* the type, no hint needed.
        type_short = var

    if type_short is None:
        # No hint and not a `::` call: fall back to a bare method-name lookup.
        qualname = _resolve_from(_by_short_name[lang], method, "method", source_label)
        return _symbol_table[lang].get(qualname) if qualname else None

    qualified_type = _resolve_from(_class_by_short_name[lang], type_short, "class", source_label)
    if qualified_type is None:
        return None  # already warned inside _resolve_from if ambiguous; silently skip if unknown

    qualname = qualified_type + join + method
    hit = _symbol_table[lang].get(qualname)
    if hit is None:
        logger.warning(
            "showfile: hint/qualifier says '%s' is a '%s', but '%s' has no member '%s' "
            "in the API docs (in %s).", var, qualified_type, qualified_type, method, source_label)
    return hit


def resolve_bare(lang, name, ignore_bare, source_label):
    if name in ignore_bare:
        return None  # explicitly excluded via :ignore: -- skip silently, no warning
    qualname = _resolve_from(_by_short_name[lang], name, "function", source_label)
    return _symbol_table[lang].get(qualname) if qualname else None


# ---------------------------------------------------------------------------
# HTML post-processing
# ---------------------------------------------------------------------------

_BARE_RE = re.compile(
    r'<span class="n">(?P<name>\w+)</span>'
    r'(?=<span class="p">\()'
)


def _call_re_for(lang):
    seps = _SEPS_FOR_LANG[lang]
    if not seps:
        return None
    sep_pattern = '|'.join(_SEP_HTML[s] for s in seps)
    return re.compile(
        r'<span class="n">(?P<var>\w+)</span>'
        r'<span class="o">(?P<sep>' + sep_pattern + r')</span>'
        r'<span class="n">(?P<method>\w+)</span>'
        r'(?=<span class="p">\()'
    )


_CALL_RE_CACHE = {lang: _call_re_for(lang) for lang in _LANG_ALIASES.values()}


def _line_at(text, pos):
    return text.count('\n', 0, pos) + 1


def _plain_call_re_for(lang):
    seps = _SEPS_FOR_LANG[lang]
    if not seps:
        return None
    sep_pattern = '|'.join(re.escape(s) for s in seps)
    return re.compile(r'\b(?P<var>\w+)(?:' + sep_pattern + r')(?P<method>\w+)\s*\(')


_PLAIN_CALL_RE_CACHE = {lang: _plain_call_re_for(lang) for lang in _LANG_ALIASES.values()}


def _plain_call_lines(source, lang):
    """Line numbers of each qualified-call occurrence in the *raw* source, in the same left-to-right order the HTML call regex will match them in."""
    call_re = _PLAIN_CALL_RE_CACHE.get(lang)
    if call_re is None:
        return []
    return [_line_at(source, m.start('var')) for m in call_re.finditer(source)]


def _plain_bare_lines(source, lang):
    """Same, but for bare name(...) occurrences that are NOT the tail of a qualified call (those are covered by _plain_call_lines instead) -- 
    must mirror _preceded_by_separator's filtering so the two queues line up with what the HTML passes will actually process."""
    seps = _SEPS_FOR_LANG[lang]
    lines = []
    for m in re.finditer(r'\b(?P<name>\w+)\s*\(', source):
        start = m.start('name')
        if any(source[:start].endswith(sep) for sep in seps):
            continue
        lines.append(_line_at(source, start))
    return lines


def link_identifiers(lang, source, html, hints, ignore_qualified, ignore_bare, builder, from_docname, source_label):
    """Post-process Pygments-highlighted HTML, wrapping resolvable identifiers in <a>."""

    call_lines = iter(_plain_call_lines(source, lang))
    bare_lines = iter(_plain_bare_lines(source, lang))

    def _labeled(line_iter):
        line = next(line_iter, None)
        location_bit = ":{}".format(line) if line else ""
        return "{}{} (shown via showfile)".format(source_label, location_bit)

    def _link(docname, anchor, inner_html):
        url = builder.get_relative_uri(from_docname, docname) + '#' + anchor
        return '<a class="showfile-apilink" href="{url}">{inner}</a>'.format(url=url, inner=inner_html)

    call_re = _CALL_RE_CACHE.get(lang)
    if call_re is not None:
        def _call_sub(m):
            label = _labeled(call_lines)
            hit = resolve_call(lang, m.group('var'), m.group('sep'), m.group('method'),
                                hints, ignore_qualified, label)
            if hit is None:
                return m.group(0)
            docname, anchor = hit
            prefix = '<span class="n">{var}</span><span class="o">{sep}</span>'.format(
                var=m.group('var'), sep=m.group('sep'))
            return prefix + _link(docname, anchor, '<span class="n">{}</span>'.format(m.group('method')))

        html = call_re.sub(_call_sub, html)

    def _bare_sub(m):
        if _preceded_by_separator(html, m.start(), lang):
            # This identifier is the method half of a qualified call the call regex already handled (resolved, or warned about
            # and left as-is) a few lines up -- don't re-attempt it as a bare name, that's what caused every ambiguous call to
            # warn twice.
            return m.group(0)
        label = _labeled(bare_lines)
        hit = resolve_bare(lang, m.group('name'), ignore_bare, label)
        if hit is None:
            return m.group(0)
        docname, anchor = hit
        return _link(docname, anchor, '<span class="n">{}</span>'.format(m.group('name')))

    return _BARE_RE.sub(_bare_sub, html)


# ---------------------------------------------------------------------------
# Wiring
# ---------------------------------------------------------------------------

_orig_highlight_block = PygmentsBridge.highlight_block
_app_ref = {}  # holds the running Sphinx app, set once at builder-inited


def _patched_highlight_block(self, source, lang, opts=None, force=False, location=None, **kwargs):
    html = _orig_highlight_block(self, source, lang, opts=opts, force=force, location=location, **kwargs)

    app = _app_ref.get('app')
    canonical_lang = _LANG_ALIASES.get(lang)
    if app is None or canonical_lang is None or not isinstance(location, nodes.Element):
        return html
    if not location.get('showfile_apilink', False):
        # Not a showfile-generated block (e.g. an unrelated .. code-block:: python
        # elsewhere in the docs) -- leave it alone entirely.
        return html

    hints = dict(app.config.showfile_hints or {})
    hints.update(location.get('showfile_hints', {}) or {})  # per-block hints win

    ignore_qualified, ignore_bare = location.get('showfile_ignore', (set(), set()))

    source_file = location.get('showfile_source_file', '<unknown file>')
    source_label = "'{}'".format(source_file)

    from_docname = app.builder.current_docname
    return link_identifiers(canonical_lang, source, html, hints, ignore_qualified, ignore_bare,
                             app.builder, from_docname, source_label)


PygmentsBridge.highlight_block = _patched_highlight_block


def _remember_app(app):
    _app_ref['app'] = app


def setup(app):
    app.add_config_value('showfile_hints', {}, 'env')  # global var-name -> type-name hints
    app.connect('builder-inited', _remember_app)
    app.connect('env-updated', build_symbol_table)
    return {'version': '0.2', 'parallel_read_safe': True, 'parallel_write_safe': False}
