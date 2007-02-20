/*
  This file is part of the CSDP 5.0 package developed by Dr. Brian Borchers.
  Which can be distributed under the LGPL licence. The source code and 
  manual can be downloaded at http://euler.nmt.edu/~brian/csdp.html.

  This include file contains declarations for a number of parameters that 
  can affect the performance of CSDP.  You can adjust these parameters by 
 
    1. #include "parameters.h" in your code.
    2. Declare struct paramstruc params;
    3. Call init_params(params); to get default values.
    4. Change the value of the parameter that you're interested in.



  */

struct paramstruc {
  double axtol;
  double atytol;
  double objtol;
  double pinftol;
  double dinftol;
  int maxiter;
  double minstepfrac;
  double maxstepfrac;
  double minstepp;
  double minstepd;
  int usexzgap;
  int tweakgap;
  int affine;
  int perturbobj;
  int fastmode;
};






