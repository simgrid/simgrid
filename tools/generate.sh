#!/bin/sh
# Generate files from a given dwarf.h

cat "$1" | grep DW_TAG_ | sed 's/.*\(DW_TAG_[^ ]*\) = \(0x[0-9a-f]*\).*/case \2: return "\1";/' > src/mc/mc_dwarf_tagnames.h
cat "$1" | grep DW_AT_ | sed 's/.*\(DW_AT_[^ ]*\) = \(0x[0-9a-f]*\).*/case \2: return "\1";/' >  src/mc/mc_dwarf_attrnames.h
