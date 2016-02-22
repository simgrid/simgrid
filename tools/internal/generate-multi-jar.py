#!/usr/bin/env python

# Copyright (c) 2013-2014. The SimGrid Team.
# All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
Tool to generate the simgrid.jar
"""
import urllib2
import zipfile
import tempfile
import os
import sys
import re

dists = ['https://ci.inria.fr/simgrid/job/SimGrid-Multi/build_mode=Debug,label=%s/lastSuccessfulBuild/artifact/build/SimGrid-3.11/simgrid.jar' % dist
         for dist in ["small-debian-32", "small-debian-64", "small-freebsd-64-clang"]]
dists.append(
    'https://ci.inria.fr/simgrid/job/Simgrid-Mult-Win7/build_mode=Debug,label=windows-64/lastSuccessfulBuild/artifact/build/simgrid.jar')


class SimJar(object):

    def __init__(self, filename='simgrid.jar'):
        self.zipfile = zipfile.ZipFile(filename, 'w')
        self.done = set()
        self.git_version = None

    def addJar(self, filename):
        with zipfile.ZipFile(filename) as zf:
            platform = None
            arch = None
            git_version = None
            for z in zf.infolist():
                path = filter(None, z.filename.split('/'))
                if len(path) == 3 and path[0] == 'NATIVE':
                    platform, arch = path[1:3]
                elif z.filename == 'META-INF/MANIFEST.MF':
                    zf.read('META-INF/MANIFEST.MF')
                    git_version = re.findall(
                        r"Implementation-Version: \"(.*?)\"", zf.read('META-INF/MANIFEST.MF'))

            assert platform is not None and git_version is not None, "Jar file not valid (%s, %s)" % (
                platform, git_version)
            print "Adding: %s %s" % (platform, arch)
            if self.git_version is None:
                self.git_version = git_version
            elif self.git_version != git_version:
                print "WARNING: Assembling jar of various commits (%s vs %s)" % (self.git_version, git_version)

            for info in zf.infolist():
                if info.filename not in self.done:
                    self.done.add(info.filename)
                    self.zipfile.writestr(info, zf.read(info.filename))

    def addByUrl(self, url):
        data = urllib2.urlopen(url)
        f = tempfile.NamedTemporaryFile(delete=False)
        f.write(data.read())
        f.close()
        self.addJar(f.name)
        os.unlink(f.name)

    def close(self):
        self.zipfile.close()

if __name__ == "__main__":
    jar = SimJar()
    for dist in dists:
        jar.addByUrl(dist)
    for a in sys.argv[1:]:
        jar.addJar(a)
    jar.close()
