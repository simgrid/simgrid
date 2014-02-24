#!/bin/sh

# Copyright (c) 2013-2014. The SimGrid Team.
# All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

(find ./ -name '*.[ch]' | xargs perl tools/spell/lspell2.pl tools/spell/sg_stopwords.txt ; find ./ -name '*.[ch]' | xargs perl tools/spell/lspell.pl tools/spell/sg_stopwords.txt ) | grep -v ':' | sort -f | uniq
