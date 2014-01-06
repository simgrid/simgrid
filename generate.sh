#!/bin/sh
# Generate files from dwarf.h

(for tag in $(grep -o 'DW_TAG_[^ ]*' /usr/include/dwarf.h) ; do
    echo "case $tag: return \"$tag\";"
done) > src/mc/mc_dwarf_tagnames.h

(for attr in $(grep -o 'DW_AT_[^ ]*' /usr/include/dwarf.h) ; do
    echo "case $attr: return \"$attr\";"
done) > src/mc/mc_dwarf_attrnames.h
