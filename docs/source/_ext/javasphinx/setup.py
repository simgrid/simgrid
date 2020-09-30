#
# Copyright 2012-2015 Bronto Software, Inc. and contributors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

from setuptools import setup

setup(
    name = "javasphinx",
    packages = ["javasphinx"],
    version = "0.9.15",
    author = "Chris Thunes",
    author_email = "cthunes@brewtab.com",
    url = "http://github.com/bronto/javasphinx",
    description = "Sphinx extension for documenting Java projects",
    license = "Apache 2.0",
    classifiers = [
        "Programming Language :: Python",
        "Development Status :: 4 - Beta",
        "Operating System :: OS Independent",
        "License :: OSI Approved :: Apache Software License",
        "Intended Audience :: Developers",
        "Topic :: Software Development :: Libraries"
        ],
    install_requires=[
        "javalang>=0.10.1",
        "lxml",
        "beautifulsoup4",
        "future",
        "docutils",
        "sphinx"
    ],
    entry_points={
        'console_scripts': [
            'javasphinx-apidoc = javasphinx.apidoc:main'
            ]
        },
    long_description = """\
==========
javasphinx
==========

javasphinx is an extension to the Sphinx documentation system which adds support
for documenting Java projects. It includes a Java domain for writing
documentation manually and a javasphinx-apidoc utility which will automatically
generate API documentation from existing Javadoc markup.
"""
)
