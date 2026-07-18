"""
Minimalistic Java domain for Sphinx, only sufficient for SimGrid.

Goal: mimic the *shape* of what Breathe does for C++, but scoped to Java and without routing anything through the cpp domain.

This is a prototype: signature parsing is regex-based (not a real Java grammar), and only
class/method/constructor/field/enum-constant are handled. Good enough to replace ".. doxygenfunction::
org::simgrid::s4u::X::y()" in the docs.

## What it does

To render a class and its public methods (filtering out SWIG-generated content such as `swigCPtr`), do:

   .. javaclass:: org.simgrid.s4u.Activity
      :members:

To render one method as we do a lot in app_s4u.rst, do:

   .. javamethod:: org.simgrid.s4u.Activity.suspend
   
To link to a class or a method (similar to :cpp:class: and :cpp:func:) do:

   :java:class:`...`

   :java:meth:`...`  

## What it deliberately doesn't do (yet)

- No real Java grammar (generics, annotations, arrays-of-generics not handled). Only good enough for SimGrid's codebase.
- No overload disambiguation: `javamethod` renders every overload found under that name.
- No `index.xml`-based auto-discovery; `javaclass` resolves `refid` by mangling the qualified name the way Doxygen does,
  matching the existing files. Fine for a known/bounded set of classes.
- No interfaces/enums/fields rendering beyond what's shown here.

## Next steps to make it production-ready

1. Extend `DoxygenJavaReader` to also render fields/enums (`Activity.State` is referenced but not itself rendered here).
2. Swap the regex-based Doxygen reading for something closer to Breathe's more defensive XML handling (missing files, malformed
   argstrings).
3. Migrate `app_s4u.rst`'s Java tabs from `doxygenclass`/`doxygenfunction` to `javaclass`/`javamethod` — mechanical find/replace
   given the two directives take the same dotted qualified name Doxygen already uses.
4. Decide on visual styling (CSS) to match the rest of the SimGrid docs theme for the `xref java java-class` classes.


"""
import os
import re
import xml.etree.ElementTree as ET

from docutils import nodes
from docutils.parsers.rst import Directive, directives
from sphinx.domains import Domain, ObjType
from sphinx.roles import XRefRole
from sphinx.util.nodes import make_refnode


# ---------------------------------------------------------------------------
# Doxygen XML reading helpers
# ---------------------------------------------------------------------------

def _text(el):
    """Flatten a Doxygen description element (briefdescription, type...) to text."""
    if el is None:
        return ""
    return "".join(el.itertext()).strip()


class JavaDoxygenError(Exception):
    """Raised for anything that goes wrong while reading Doxygen XML for a
    Java compound, so callers can turn it into a clean Sphinx warning
    instead of letting a raw traceback blow up the whole build."""


class DoxygenJavaReader:
    """Reads one compound Java class/interface/enum XML file produced by
    Doxygen, plus (optionally) the project-wide index.xml for refid lookup."""

    # SWIG generates a lot of plumbing (swigCPtr, swigCMemOwnBase, the
    # (long cPtr, boolean cMemoryOwn) constructor, getCPtr...). None of that
    # is part of the actual user-facing API, so we filter it out.
    _SWIG_NOISE = re.compile(r"^(swig|getCPtr$)", re.IGNORECASE)

    # SWIG renders Java enum-like classes as a set of
    # "public static final <ClassName> CONST = new <ClassName>(\"CONST\");"
    # fields. We detect and render those as enum constants rather than as
    # regular fields.
    _ENUM_CONST_TYPE = re.compile(r"^final\s+(\S+)$")

    def __init__(self, xml_dir):
        self.xml_dir = xml_dir
        self._index_cache = None  # qualname -> refid, built lazily from index.xml

    # -- index.xml based lookup ------------------------------------------------

    def _load_index(self):
        """Build a {qualname: refid} map for every Java class/interface
        compound listed in index.xml. Used as the authoritative lookup so we
        don't rely solely on guessing Doxygen's id-mangling scheme."""
        if self._index_cache is not None:
            return self._index_cache

        index_path = os.path.join(self.xml_dir, "index.xml")
        cache = {}
        try:
            tree = ET.parse(index_path)
        except (OSError, ET.ParseError) as exc:
            raise JavaDoxygenError(
                f"could not read Doxygen index at '{index_path}': {exc}. "
                "Did the doxygen run configured for Java bindings complete?") from exc

        for compound in tree.getroot().findall("compound"):
            if compound.get("kind") not in ("class", "interface"):
                continue
            name_el = compound.find("name")
            if name_el is None or "::" not in (name_el.text or ""):
                continue  # not a namespaced Java-style name, skip
            qualname = name_el.text.replace("::", ".")
            cache[qualname] = compound.get("refid")

        self._index_cache = cache
        return cache

    def refid_for_qualname(self, qualname):
        """Resolve a dotted Java qualified name to its Doxygen refid.

        Tries index.xml first (authoritative), and falls back to the naive
        name-mangling Doxygen normally uses, in case index.xml is
        unavailable or doesn't (yet) list the compound for some reason.
        """
        try:
            index = self._load_index()
        except JavaDoxygenError:
            index = {}

        refid = index.get(qualname)
        if refid:
            return refid

        # Fallback: guess the mangled id the way Doxygen usually builds it.
        return "class" + qualname.replace(".", "_1_1")

    # -- compound (class/interface/enum-like) reading ---------------------------

    def read_class(self, qualname):
        refid = self.refid_for_qualname(qualname)
        path = os.path.join(self.xml_dir, refid + ".xml")

        try:
            tree = ET.parse(path)
        except OSError as exc:
            raise JavaDoxygenError(
                f"no Doxygen XML found for Java class '{qualname}' "
                f"(looked for '{path}'). Check the spelling, or that "
                "doxygen was run with the Java bindings in its INPUT.") from exc
        except ET.ParseError as exc:
            raise JavaDoxygenError(f"malformed Doxygen XML in '{path}': {exc}") from exc

        compound = tree.getroot().find("compounddef")
        if compound is None:
            raise JavaDoxygenError(f"'{path}' has no <compounddef>, is this a valid Doxygen XML file?")
        if compound.get("language") != "Java":
            raise JavaDoxygenError(
                f"'{qualname}' resolves to a non-Java Doxygen compound "
                f"(language={compound.get('language')!r}); use doxygenclass instead.")

        qualname = _text(compound.find("compoundname")).replace("::", ".")
        brief = _text(compound.find("briefdescription"))
        detailed = _text(compound.find("detaileddescription"))

        methods, fields, enum_constants = [], [], []
        for sectiondef in compound.findall("sectiondef"):
            kind = sectiondef.get("kind", "")
            if not kind.startswith("public"):
                continue  # only the public API is interesting to end users
            for memberdef in sectiondef.findall("memberdef"):
                name = _text(memberdef.find("name"))
                if self._SWIG_NOISE.match(name):
                    continue
                member_kind = memberdef.get("kind")
                if member_kind == "function":
                    methods.append(self._read_method(memberdef, qualname))
                elif member_kind == "variable":
                    rtype = _text(memberdef.find("type"))
                    m = self._ENUM_CONST_TYPE.match(rtype)
                    if kind == "public-static-attrib" and m and m.group(1).rsplit("::", 1)[-1] == qualname.rsplit(".", 1)[-1]:
                        enum_constants.append(name)
                    else:
                        fields.append(self._read_field(memberdef, qualname))

        return {
            "qualname": qualname,
            "brief": brief,
            "detailed": detailed,
            "methods": methods,
            "fields": fields,
            "enum_constants": enum_constants,
        }

    def _read_method(self, memberdef, class_qualname):
        name = _text(memberdef.find("name"))
        rtype = _text(memberdef.find("type"))
        is_ctor = (rtype == "") and (name == class_qualname.split(".")[-1])

        params = []
        for param in memberdef.findall("param"):
            ptype = _text(param.find("type"))
            pname = _text(param.find("declname"))
            if not ptype and not pname:
                continue  # tolerate odd/incomplete <param> blocks rather than emit "  "
            params.append((ptype, pname))

        # Doxygen usually keeps <param> and argsstring in sync; if a weird
        # signature ever leaves <param> empty while argsstring isn't, fall
        # back to showing the raw argsstring instead of silently rendering
        # an empty "()" that would misrepresent the method.
        argsstring = _text(memberdef.find("argsstring"))
        raw_sig_fallback = argsstring if (not params and argsstring not in ("", "()")) else None

        return {
            "name": name,
            "qualname": f"{class_qualname}.{name}",
            "is_ctor": is_ctor,
            "return_type": rtype,
            "params": params,
            "raw_sig_fallback": raw_sig_fallback,
            "brief": _text(memberdef.find("briefdescription")),
            "detailed": _text(memberdef.find("detaileddescription")),
        }

    def _read_field(self, memberdef, class_qualname):
        name = _text(memberdef.find("name"))
        return {
            "name": name,
            "qualname": f"{class_qualname}.{name}",
            "type": _text(memberdef.find("type")),
            "brief": _text(memberdef.find("briefdescription")),
            "detailed": _text(memberdef.find("detaileddescription")),
        }


# ---------------------------------------------------------------------------
# Rendering helpers shared by both directives
# ---------------------------------------------------------------------------

def _method_signature(m):
    if m["raw_sig_fallback"] is not None:
        sig = m["raw_sig_fallback"]
    else:
        sig = "(" + ", ".join(f"{t} {n}".strip() for t, n in m["params"]) + ")"
    label = m["name"] if m["is_ctor"] else f"{m['return_type']} {m['name']}".strip()
    return f"{label}{sig}"


def _render_method(m, domain, env, target_id=None):
    target_id = target_id or f"java-method-{m['qualname']}-{id(m)}"
    domain.data["methods"][m["qualname"]] = (env.docname, target_id)
    para = nodes.paragraph(ids=[target_id])
    para += nodes.literal(text=_method_signature(m))
    if m["brief"]:
        para += nodes.Text("  " + m["brief"])
    return para


def _render_field(f, domain, env):
    target_id = f"java-field-{f['qualname']}"
    domain.data["fields"][f["qualname"]] = (env.docname, target_id)
    para = nodes.paragraph(ids=[target_id])
    para += nodes.literal(text=f"{f['type']} {f['name']}".strip())
    if f["brief"]:
        para += nodes.Text("  " + f["brief"])
    return para


def _reader_error_node(state_machine, lineno, exc):
    """Turn a JavaDoxygenError into a normal Sphinx warning/error node
    instead of letting the exception abort the whole build."""
    return state_machine.reporter.error(str(exc), line=lineno)


# ---------------------------------------------------------------------------
# Directives
# ---------------------------------------------------------------------------

class JavaClassDirective(Directive):
    """.. javaclass:: org.simgrid.s4u.Activity
       :members:
    """

    has_content = False
    required_arguments = 1
    option_spec = {"members": directives.flag}

    def run(self):
        env = self.state.document.settings.env
        qualname = self.arguments[0].strip()
        reader = DoxygenJavaReader(env.config.javadomain_xml_dir)
        domain = env.get_domain("java")

        try:
            data = reader.read_class(qualname)
        except JavaDoxygenError as exc:
            return [_reader_error_node(self.state_machine, self.lineno, exc)]

        target_id = f"java-class-{qualname}"
        domain.data["classes"][qualname] = (env.docname, target_id)

        section = nodes.section(ids=[target_id])
        section += nodes.title(text=f"class {qualname}")
        if data["brief"] or data["detailed"]:
            section += nodes.paragraph(text=(data["brief"] + " " + data["detailed"]).strip())

        if "members" in self.options:
            if data["enum_constants"]:
                section += nodes.paragraph(text="Enum constants: " + ", ".join(data["enum_constants"]))
            for f in data["fields"]:
                section += _render_field(f, domain, env)
            for m in data["methods"]:
                section += _render_method(m, domain, env, target_id=f"java-method-{m['qualname']}")

        return [section]


class JavaFunctionDirective(Directive):
    """.. javamethod:: org.simgrid.s4u.Activity.suspend"""

    has_content = False
    required_arguments = 1

    def run(self):
        env = self.state.document.settings.env
        qualname = self.arguments[0].strip()
        class_qualname, _, method_name = qualname.rpartition(".")
        reader = DoxygenJavaReader(env.config.javadomain_xml_dir)
        domain = env.get_domain("java")

        try:
            data = reader.read_class(class_qualname)
        except JavaDoxygenError as exc:
            return [_reader_error_node(self.state_machine, self.lineno, exc)]

        matches = [m for m in data["methods"] if m["name"] == method_name]
        if not matches:
            msg = self.state_machine.reporter.error(
                f"javamethod: '{method_name}' not found in {class_qualname}", line=self.lineno)
            return [msg]

        return [_render_method(m, domain, env) for m in matches]


# ---------------------------------------------------------------------------
# Domain
# ---------------------------------------------------------------------------

class JavaDomain(Domain):
    name = "java"
    label = "Java"
    object_types = {
        "class": ObjType("class", "class"),
        "method": ObjType("method", "meth"),
        "field": ObjType("field", "field"),
    }
    directives = {}
    roles = {
        "class": XRefRole(),
        "meth": XRefRole(),
        "field": XRefRole(),
    }
    initial_data = {"classes": {}, "methods": {}, "fields": {}}

    def resolve_xref(self, env, fromdocname, builder, typ, target, node, contnode):
        table = {"class": "classes", "meth": "methods", "field": "fields"}.get(typ)
        entry = self.data.get(table, {}).get(target) if table else None
        if entry is None:
            return None
        todocname, target_id = entry
        return make_refnode(builder, fromdocname, todocname, target_id, contnode, target)

    def get_objects(self):
        for kind, key in (("class", "classes"), ("method", "methods"), ("field", "fields")):
            for qualname, (docname, target_id) in self.data[key].items():
                yield qualname, qualname, kind, docname, target_id, 1


def setup(app):
    app.add_config_value("javadomain_xml_dir", None, "env")
    app.add_domain(JavaDomain)
    # Registered as bare top-level directives (not "java:javaclass") so the docs read like the existing ".. doxygenclass::"/"..
    # doxygenfunction::" directives, for a minimal-diff migration.
    app.add_directive("javaclass", JavaClassDirective)
    app.add_directive("javamethod", JavaFunctionDirective)
    return {"version": "0.2", "parallel_read_safe": True}
