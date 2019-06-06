

# python3 setup.py sdist # Build a source distrib (building binary distribs is complex on linux)

# twine upload --repository-url https://test.pypi.org/legacy/ dist/simgrid-*.tar.gz # Upload to test
# pip3 install --user --index-url https://test.pypi.org/simple  simgrid

# Once it works, upload to the real infra.  /!\ you cannot modify a file once uploaded
# twine upload dist/simgrid-*.tar.gz

import os
import re
import sys
import platform
import subprocess

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
from distutils.version import LooseVersion


class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuild(build_ext):
    def run(self):
        try:
            out = subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError("CMake must be installed to build the following extensions: " +
                               ", ".join(e.name for e in self.extensions))
        if not os.path.exists("MANIFEST.in"):
            raise RuntimeError("Please generate a MANIFEST.in file (configure simgrid, and copy it here if you build out of tree)")

        if platform.system() == "Windows":
            cmake_version = LooseVersion(re.search(r'version\s*([\d.]+)', out.decode()).group(1))
            if cmake_version < '3.1.0':
                raise RuntimeError("CMake >= 3.1.0 is required on Windows")

        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
        cmake_args = ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + extdir,
                      '-DPYTHON_EXECUTABLE=' + sys.executable,
                      '-Denable_smpi=OFF',
                      '-Denable_java=OFF',
                      '-Denable_python=ON',
                      '-Dminimal-bindings=ON']

        cfg = 'Debug' if self.debug else 'Release'
        build_args = ['--config', cfg]

        if platform.system() == "Windows":
            cmake_args += ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{}={}'.format(cfg.upper(), extdir)]
            if sys.maxsize > 2**32:
                cmake_args += ['-A', 'x64']
            build_args += ['--', '/m']
        else:
            cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]
            build_args += ['--', '-j4']

        env = os.environ.copy()
        env['CXXFLAGS'] = '{} -DVERSION_INFO=\\"{}\\"'.format(env.get('CXXFLAGS', ''),
                                                              self.distribution.get_version())
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        subprocess.check_call(['cmake', ext.sourcedir] + cmake_args, cwd=self.build_temp, env=env)
        subprocess.check_call(['cmake', '--build', '.'] + build_args, cwd=self.build_temp)

setup(
    name='simgrid',
    version='3.22.4',
    author='Da SimGrid Team',
    author_email='simgrid-devel@lists.gforge.inria.fr',
    description='Toolkit for scalable simulation of distributed applications',
    long_description=("SimGrid is a scientific instrument to study the behavior of "
                      "large-scale distributed systems such as Grids, Clouds, HPC or P2P "
                      "systems. It can be used to evaluate heuristics, prototype applications "
                      "or even assess legacy MPI applications."),
    ext_modules=[CMakeExtension('simgrid')],
    cmdclass=dict(build_ext=CMakeBuild),
    zip_safe=False,
    classifiers=[
      "Development Status :: 4 - Beta",
      "Environment :: Console",
      "Intended Audience :: Education",
      "Intended Audience :: Developers",
      "Intended Audience :: Science/Research",
      "Intended Audience :: System Administrators",
      "License :: OSI Approved :: GNU Lesser General Public License v2 (LGPLv2)",
      "Operating System :: POSIX",
      "Operating System :: MacOS",
      "Operating System :: Microsoft :: Windows",
      "Programming Language :: Python :: 3",
      "Programming Language :: C++",
      "Programming Language :: C",
      "Programming Language :: Fortran",
      "Programming Language :: Java",
      "Topic :: System :: Distributed Computing",
      "Topic :: System :: Systems Administration",
    ],
    url="https://simgrid.org",
    project_urls={
      'Tracker': 'https://framagit.org/simgrid/simgrid/issues/',
      'Source':  'https://framagit.org/simgrid/simgrid/',
      'Documentation': 'https://simgrid.org/doc/latest/',
    },
)
