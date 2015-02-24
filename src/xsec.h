#ifndef XSEC_H
#define XSEC_H

#include "pheno.h"
#include <stdexcept>
//PDF package
#include <mstwpdf.h>
//Numerical integration
#include <gsl/gsl_monte.h>
#include <gsl/gsl_monte_vegas.h>


namespace pheno{
  
  
  //*******************************************************//
  //            Class for partonic cross sections          //
  //*******************************************************//

  
  class PartonXSec{
    ///Class for calculcation of partonic cross sections of f_in f_in~ --> f_out f_out~
    private:
      int pdgin;
      //Pointer to the model in use
      pheno::zpmodel *  _model;      
      //Numerical factors for partial cross sections
      double numGam;
      double numZ;
      double numZp;
      double numGamZ;
      double numGamZp;
      double numZZp;
      //Partial cross sections (Zp part can be called->public)
      double sigGam(double Ecm);
      double sigZ(double Ecm);
      double sigGamZ(double Ecm);
      double sigGamZp(double Ecm);
      double sigZZp(double Ecm);
    public:
      int pdg_in();
      //Partonic cross sections
      double sigZp(double Ecm);
      double sigSM(double Ecm);
      double sigInt(double Ecm);
      double sigTot(double Ecm);
      //Class Constructor: Give model as reference &model
      PartonXSec(fundamental::fermionExt* f_in, fundamental::fermionExt* f_out, pheno::zpmodel* p_model);
      ///Give the two fermions and the model as reference to the constructor!
  };

  
  
  
  
  
  
  
  //*********************************************************//
  //            Classes for hadronic cross sections          //
  //*********************************************************//

  

  
  class HadronXSec{
    ///Class for calculcation of partonic cross sections of p p --> f_out f_out~
    private:
      double Epp;
      //Internal parton cross sections: no top, as pdf negligible
      PartonXSec* dxsec;
      PartonXSec* uxsec;
      PartonXSec* sxsec;
      PartonXSec* cxsec;
      PartonXSec* bxsec;
      c_mstwpdf* pdf;
      //Subroutines for pdf convoluted cross sections: Specify partial cross section in functor object
      template<class PartialCrossX> double pdf_xsec(PartonXSec* pxsec, double q, double x);
//       template<class PartialCrossX> double monte_pdf(double x[], size_t dim, void * p);
      template<class PartialCrossX> double pdfconvoluted(PartonXSec* pxsec, double Ecm);
    public:
      //Hadronic cross sections
      double sigTot(double Ecm);
      //Constructor and destructor to take care of memory allocations
      HadronXSec(fundamental::fermionExt* f_out, pheno::zpmodel* p_model, char* pdf_grid_file, double Ecoll=8000.);
      ~HadronXSec();    
  };
  
  

  
  //Implementation of the member templates
  template<class PartialCrossX> 
  double pheno::HadronXSec::pdf_xsec(PartonXSec* pxsec, double q, double x)
  {
    PartialCrossX f;
    return f(pxsec, q) * pheno::HadronXSec::pdf->parton(pxsec->pdg_in(), x, q);
  }
  
  
  
  //Parameter struct for integrable function
  struct parameters{ PartonXSec* px; c_mstwpdf* ppdf; double Ecm; double Ecoll; double cross_section; };
  
  
  
  
  //Implementation of gsl integrable function
  inline double monte_pdf(double x[], size_t dim, void * p)
  {
    //Check that integration vector is one dimensional
    if(dim!=1)
      throw std::runtime_error("ERROR: Integration dim!=1");
    //Get initialization parameters
    struct parameters * pars = (struct parameters *)p;
    //Define Bjorken x
    double x1 = x[0];
    double x2 = pars->Ecm * pars->Ecm/(x1 * pars->Ecoll * pars->Ecoll);
    //Allocate cross section functor f
    return pars->cross_section * 
           pars->ppdf->parton(   pars->px->pdg_in(),  x1, pars->Ecm )/x1 * 
           pars->ppdf->parton(  -pars->px->pdg_in(),  x2, pars->Ecm )/x2;
  }
  
  
  
  
  
  //Function that integrates cross section 
  template<class PartialCrossX> 
  double pheno::HadronXSec::pdfconvoluted(PartonXSec* pxsec, double Ecm)
  {
    PartialCrossX f;
    struct parameters local_pars = {pxsec, pdf, Ecm, Epp, f(pxsec, Ecm)};
    //Define a gsl_monte_function to pass to the integrator
    gsl_monte_function F;
    F.f = &monte_pdf;
    F.dim = 1;
    F.params = &local_pars;
    
    //Parameter and result objects
    size_t calls = 10000;
    double res, err;
    double xl[1] = { Ecm*Ecm/(Epp*Epp) };  //lower integration limit
    double xu[1] = { 1 };

    //Initialize random number gen for monte carlo
    gsl_rng_env_setup ();

    const gsl_rng_type *T;
    gsl_rng *r;
    T = gsl_rng_default;
    r = gsl_rng_alloc (T);

    //Integration
    gsl_monte_vegas_state *s = gsl_monte_vegas_alloc (1);
    gsl_monte_vegas_integrate (&F, xl, xu, 1, calls, r, s, &res, &err);
    
    return res;
  }
  
  
  
  
  
}

#endif