%define DOCSTR
"The `Kernel` module gathers all kernels available in the SHOGUN toolkit."
%enddef

%module(docstring=DOCSTR) Kernel
%{
#define SWIG_FILE_WITH_INIT
#include "kernel/Kernel.h"
%}

#ifdef HAVE_DOXYGEN
%include "kernel/Kernel_doxygen.i"
#endif

%include "lib/common.i"
%include "lib/swig_typemaps.i"

#ifdef HAVE_PYTHON
%init %{
	  import_array();
%}
%feature("autodoc", "get_kernel_matrix(self) -> numpy 2dim array of float") get_kernel_matrix;
#endif

%apply (DREAL** ARGOUT2, INT* DIM1, INT* DIM2) {(DREAL** dst, INT* m, INT* n)};

%rename(Kernel) CKernel;
%feature("autodoc","0");

%include "lib/ShogunException.i"
%include "lib/io.i"
%include "base/Version.i"
%include "base/Parallel.i"
%include "base/SGObject.i"
%include "kernel/Kernel.h"

%include "kernel/StringKernel.i" 
%include "kernel/SimpleKernel.i"
%include "kernel/SparseKernel.i"

%include "kernel/ConstKernel.i" 
%include "kernel/DiagKernel.i" 
%include "kernel/LinearKernel.i"
%include "kernel/PolyKernel.i"
%include "kernel/GaussianKernel.i"
%include "kernel/GaussianShortRealKernel.i"
%include "kernel/GaussianShiftKernel.i"
%include "kernel/SigmoidKernel.i"
%include "kernel/SparseGaussianKernel.i"
%include "kernel/SparseLinearKernel.i"
%include "kernel/SparsePolyKernel.i"

%include "kernel/Chi2Kernel.i"
%include "kernel/AUCKernel.i"

%include "kernel/WeightedDegreeStringKernel.i" 
%include "kernel/WeightedDegreePositionStringKernel.i" 
%include "kernel/CommUlongStringKernel.i" 
%include "kernel/CommWordStringKernel.i" 
%include "kernel/WeightedCommWordStringKernel.i" 
%include "kernel/MindyGramKernel.i"
%include "kernel/FixedDegreeStringKernel.i"
%include "kernel/HistogramWordKernel.i"
%include "kernel/LinearByteKernel.i"
%include "kernel/LinearStringKernel.i"
%include "kernel/LocalAlignmentStringKernel.i"
%include "kernel/LinearWordKernel.i"

%include "kernel/LocalityImprovedStringKernel.i"
%include "kernel/PolyMatchStringKernel.i"
%include "kernel/PolyMatchWordKernel.i"
%include "kernel/SalzbergWordKernel.i"
%include "kernel/SimpleLocalityImprovedStringKernel.i"
%include "kernel/WordMatchKernel.i"

%include "kernel/CombinedKernel.i"
%include "kernel/CustomKernel.i"
%include "kernel/DistanceKernel.i"
