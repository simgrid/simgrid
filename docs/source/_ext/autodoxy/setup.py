from setuptools import setup, Extension

with open("README", "r") as fh:
    long_description = fh.read()
    
setup(
    name="autodoxy",
    version="0.0.1",
    author="Martin Quinson",
#    author_email="author@example.com",
    description="A bridge between the autodoc of Python and Doxygen of C/C++",
    long_description=long_description,
    long_description_content_type="text/plain",
    url="https://framagit.org/simgrid/simgrid/docs/source/_ext/autodoxy",
    packages=setuptools.find_packages(),
    classifiers=[
      "Programming Language :: Python :: 3",
      "License :: OSI Approved :: MIT License",
      "Operating System :: OS Independent",
    ],
    python_requires='>=3.6',
)
