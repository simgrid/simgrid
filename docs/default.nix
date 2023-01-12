# Nix package for SimGrid's sphinx documentation.
# Example usage: nix-build ./default.nix -A simgrid-doc && firefox result/index.html
{ pkgs ? import (fetchTarball {
    url = "https://github.com/NixOS/nixpkgs/archive/21.05.tar.gz";
    sha256 = "1ckzhh24mgz6jd1xhfgx0i9mijk6xjqxwsshnvq789xsavrmsc36";
  }) {}
}:

let
  pythonPackages = pkgs.python3Packages;
  buildPythonPackage = pythonPackages.buildPythonPackage;

  self = rec {
    # the desired package
    simgrid-doc = pkgs.stdenv.mkDerivation rec {
      name = "simgrid-doc";
      src = ./..;
      buildInputs = [
        pkgs.doxygen
        pythonPackages.sphinx
        pythonPackages.sphinx_rtd_theme
        pythonPackages.breathe
        sphinx-tabs
      ];
      phases = [ "unpackPhase" "buildPhase" "installPhase" ];
      buildPhase = ''
        cd docs
        rm -rf build # this is not done in your directory, this is on the copy made by the Nix build daemon
        sphinx-build -b html source build
      '';
      installPhase = ''
        mkdir -p $out
        mv build/* $out/
      '';
    };

    sphinx-tabs = pythonPackages.buildPythonPackage rec {
      pname = "sphinx-tabs";
      version = "3.1.0";
      name = "${pname}-${version}";

      src = pythonPackages.fetchPypi {
        inherit pname version;
        sha256 = "0kv935qhml40mly33rk5am128g2ygqkfvizh33vf29hjkf32mvjy";
      };

      propagatedBuildInputs = with pythonPackages; [
        docutils
        pygments
        sphinx
      ];
    };

  };
in
  self
