#!/usr/bin/env sh

# Copyright (c) 2013-2019. The SimGrid Team.
# All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

SAVEIFS="$IFS"
LISTSEP="$(printf '\b')"

# Create a temporary file, with its name of the form $1_XXX$2, where XXX is replaced by an unique string.
# $1: prefix, $2: suffix
mymktemp () {
    local_tmp=$(mktemp --suffix="$2" "$1_XXXXXXXXXX" 2> /dev/null)
    if [ -z "$local_tmp" ]; then
        # mktemp failed (unsupported --suffix ?), try unsafe mode
        local_tmp=$(mktemp -u "$1_XXXXXXXXXX" 2> /dev/null)
        if [ -z "$local_tmp" ]; then
            # mktemp failed again (doesn't exist ?), try very unsafe mode
            if [ -z "${mymktemp_seq}" ]; then
                mymktemp_seq=$(date +%d%H%M%S)
            fi
            local_tmp="$1_$$x${mymktemp_seq}"
            mymktemp_seq=$((mymktemp_seq + 1))
        fi
        local_tmp="${local_tmp}$2"
        # create temp file, and exit if it existed before
        sh -C -c "true > \"${local_tmp}\"" || exit 1
    fi
    echo "${local_tmp}"
}

# Add a word to the end of a list (words separated by LISTSEP)
# $1: list, $2...: words to add
list_add () {
    local_list="$1"
    shift
    if [ $# -gt 0 ]; then
        eval local_content=\"\${$local_list}\"
        IFS="$LISTSEP"
        local_newcontent="$*"
        IFS="$SAVEIFS"
        if [ -z "$local_content" ]; then
            local_content="$local_newcontent"
        else
            local_content="$local_content${LISTSEP}$local_newcontent"
        fi
        eval $local_list=\"\${local_content}\"
    fi
}

# Like list_add, but only if first word to add ($2) is not empty
list_add_not_empty () {
    if [ -n "$2" ]; then
        list_add "$@"
    fi
}

# Set contents of a list (words separated by LISTSEP)
# $1: list, $2...: words to set
list_set () {
    eval $1=""
    list_add "$@"
}

# Get the content of a list: positional parameters ($1, $2, ...) are set to the content of the list
# $1: list
# usage:  eval $(list_get list)
list_get () {
    printf 'IFS="'\$'LISTSEP"; eval set -- \$%s; IFS="'\$'SAVEIFS"\n' "$1"
}
