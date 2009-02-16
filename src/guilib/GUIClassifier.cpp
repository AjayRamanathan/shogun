/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Written (W) 1999-2008 Soeren Sonnenburg
 * Written (W) 1999-2008 Gunnar Raetsch
 * Copyright (C) 1999-2008 Fraunhofer Institute FIRST and Max-Planck-Society
 */

#include "lib/config.h"

#ifndef HAVE_SWIG
#include "lib/io.h"

#include "guilib/GUIClassifier.h"
#include "interface/SGInterface.h"

#include "features/SparseFeatures.h"
#include "features/RealFileFeatures.h"
#include "features/Labels.h"

#include "classifier/KNN.h"
#include "clustering/KMeans.h"
#include "clustering/Hierarchical.h"
#include "classifier/PluginEstimate.h"

#include "classifier/LDA.h"
#include "classifier/LPM.h"
#include "classifier/LPBoost.h"
#include "classifier/Perceptron.h"
#include "classifier/KernelPerceptron.h"

#include "classifier/LinearClassifier.h"
#include "classifier/SparseLinearClassifier.h"

#ifdef USE_SVMLIGHT
#include "classifier/svm/SVM_light.h"
#include "regression/svr/SVR_light.h"
#endif //USE_SVMLIGHT

#include "classifier/svm/LibSVM.h"
#include "classifier/svm/GPBTSVM.h"
#include "classifier/svm/LibSVMOneClass.h"
#include "classifier/svm/LibSVMMultiClass.h"

#include "regression/svr/LibSVR.h"
#include "regression/KRR.h"

#include "classifier/svm/LibLinear.h"
#include "classifier/svm/MPDSVM.h"
#include "classifier/svm/GNPPSVM.h"
#include "classifier/svm/GMNPSVM.h"

#include "classifier/svm/SVMLin.h"
#include "classifier/svm/SubGradientSVM.h"
#include "classifier/SubGradientLPM.h"
#include "classifier/svm/SVMOcas.h"
#include "classifier/svm/SVMSGD.h"
#include "classifier/svm/WDSVMOcas.h"

CGUIClassifier::CGUIClassifier(CSGInterface* ui_)
: CSGObject(), ui(ui_)
{
	classifier=NULL;
	max_train_time=0;

    // Perceptron parameters
	perceptron_learnrate=0.1;
	perceptron_maxiter=1000;

    // SVM parameters
	svm_qpsize=41;
	svm_bufsize=3000;
	svm_max_qpsize=1000;
	svm_mkl_norm=1;
	svm_C1=1;
	svm_C2=1;
	svm_C_mkl=0;
	svm_weight_epsilon=1e-5;
	svm_epsilon=1e-5;
	svm_tube_epsilon=1e-2;
	svm_nu=0.5;
	svm_use_shrinking = true ;

	svm_use_bias = true;
	svm_use_mkl = false ;
	svm_use_batch_computation = true ;
	svm_use_linadd = true ;
	svm_use_precompute = false ;
	svm_use_precompute_subkernel = false ;
	svm_use_precompute_subkernel_light = false ;
	svm_do_auc_maximization = false ;

	// KRR parameters
	krr_tau=1;
}

CGUIClassifier::~CGUIClassifier()
{
	delete classifier;
}

bool CGUIClassifier::new_classifier(CHAR* name, INT d, INT from_d)
{
	if (strcmp(name,"LIBSVM_ONECLASS")==0)
	{
		delete classifier;
		classifier = new CLibSVMOneClass();
		SG_INFO("created SVMlibsvm object for oneclass\n");
	}
	else if (strcmp(name,"LIBSVM_MULTICLASS")==0)
	{
		delete classifier;
		classifier = new CLibSVMMultiClass();
		SG_INFO("created SVMlibsvm object for multiclass\n");
	}
	else if (strcmp(name,"LIBSVM")==0)
	{
		delete classifier;
		classifier= new CLibSVM();
		SG_INFO("created SVMlibsvm object\n") ;
	}
#ifdef USE_SVMLIGHT
	else if ((strcmp(name,"LIGHT")==0) || (strcmp(name,"SVMLIGHT")==0))
	{
		delete classifier;
		classifier= new CSVMLight();
		SG_INFO("created SVMLight object\n") ;
	}
	else if (strcmp(name,"SVRLIGHT")==0)
	{
		delete classifier;
		classifier= new CSVRLight();
		SG_INFO("created SVRLight object\n") ;
	}
#endif //USE_SVMLIGHT
	else if (strcmp(name,"GPBTSVM")==0)
	{
		delete classifier;
		classifier= new CGPBTSVM();
		SG_INFO("created GPBT-SVM object\n") ;
	}
	else if (strcmp(name,"MPDSVM")==0)
	{
		delete classifier;
		classifier= new CMPDSVM();
		SG_INFO("created MPD-SVM object\n") ;
	}
	else if (strcmp(name,"GNPPSVM")==0)
	{
		delete classifier;
		classifier= new CGNPPSVM();
		SG_INFO("created GNPP-SVM object\n") ;
	}
	else if (strcmp(name,"GMNPSVM")==0)
	{
		delete classifier;
		classifier= new CGMNPSVM();
		SG_INFO("created GMNP-SVM object\n") ;
	}
	else if (strcmp(name,"LIBSVR")==0)
	{
		delete classifier;
		classifier= new CLibSVR();
		SG_INFO("created SVRlibsvm object\n") ;
	}
#ifdef HAVE_LAPACK
	else if (strcmp(name, "KRR")==0)
	{
		delete classifier;
		classifier=new CKRR(krr_tau, ui->ui_kernel->get_kernel(),
			ui->ui_labels->get_train_labels());
		SG_INFO("created KRR object %p\n", classifier);
	}
#endif //HAVE_LAPACK
	else if (strcmp(name,"KERNELPERCEPTRON")==0)
	{
		delete classifier;
		classifier= new CKernelPerceptron();
		SG_INFO("created Kernel Perceptron object\n") ;
	}
	else if (strcmp(name,"PERCEPTRON")==0)
	{
		delete classifier;
		classifier= new CPerceptron();
		SG_INFO("created Perceptron object\n") ;
	}
#ifdef HAVE_LAPACK
	else if (strcmp(name,"LIBLINEAR_LR")==0)
	{
		delete classifier;
		classifier= new CLibLinear(LR);
		((CLibLinear*) classifier)->set_C(svm_C1, svm_C2);
		((CLibLinear*) classifier)->set_epsilon(svm_epsilon);
		((CLibLinear*) classifier)->set_bias_enabled(svm_use_bias);
		SG_INFO("created LibLinear logistic regression object\n") ;
	}
	else if (strcmp(name,"LIBLINEAR_L2")==0)
	{
		delete classifier;
		classifier= new CLibLinear(L2);
		((CLibLinear*) classifier)->set_C(svm_C1, svm_C2);
		((CLibLinear*) classifier)->set_epsilon(svm_epsilon);
		((CLibLinear*) classifier)->set_bias_enabled(svm_use_bias);
		SG_INFO("created LibLinear l2 loss object\n") ;
	}
	else if (strcmp(name,"LDA")==0)
	{
		delete classifier;
		classifier= new CLDA();
		SG_INFO("created LDA object\n") ;
	}
#endif //HAVE_LAPACK
#ifdef USE_CPLEX
	else if (strcmp(name,"LPM")==0)
	{
		delete classifier;
		classifier= new CLPM();
		((CLPM*) classifier)->set_C(svm_C1, svm_C2);
		((CLPM*) classifier)->set_epsilon(svm_epsilon);
		((CLPM*) classifier)->set_bias_enabled(svm_use_bias);
		((CLPM*) classifier)->set_max_train_time(max_train_time);
		SG_INFO("created LPM object\n") ;
	}
	else if (strcmp(name,"LPBOOST")==0)
	{
		delete classifier;
		classifier= new CLPBoost();
		((CLPBoost*) classifier)->set_C(svm_C1, svm_C2);
		((CLPBoost*) classifier)->set_epsilon(svm_epsilon);
		((CLPBoost*) classifier)->set_bias_enabled(svm_use_bias);
		((CLPBoost*) classifier)->set_max_train_time(max_train_time);
		SG_INFO("created LPBoost object\n") ;
	}
	else if (strcmp(name,"SUBGRADIENTLPM")==0)
	{
		delete classifier;
		classifier= new CSubGradientLPM();

		((CSubGradientLPM*) classifier)->set_bias_enabled(svm_use_bias);
		((CSubGradientLPM*) classifier)->set_qpsize(svm_qpsize);
		((CSubGradientLPM*) classifier)->set_qpsize_max(svm_max_qpsize);
		((CSubGradientLPM*) classifier)->set_C(svm_C1, svm_C2);
		((CSubGradientLPM*) classifier)->set_epsilon(svm_epsilon);
		((CSubGradientLPM*) classifier)->set_max_train_time(max_train_time);
		SG_INFO("created Subgradient LPM object\n") ;
	}
#endif //USE_CPLEX
	else if (strncmp(name,"KNN", strlen("KNN"))==0)
	{
		delete classifier;
		classifier= new CKNN();
		SG_INFO("created KNN object\n") ;
	}
	else if (strncmp(name,"KMEANS", strlen("KMEANS"))==0)
	{
		delete classifier;
		classifier= new CKMeans();
		SG_INFO("created KMeans object\n") ;
	}
	else if (strncmp(name,"HIERARCHICAL", strlen("HIERARCHICAL"))==0)
	{
		delete classifier;
		classifier= new CHierarchical();
		SG_INFO("created Hierarchical clustering object\n") ;
	}
	else if (strcmp(name,"SVMLIN")==0)
	{
		delete classifier;
		classifier= new CSVMLin();
		((CSVMLin*) classifier)->set_C(svm_C1, svm_C2);
		((CSVMLin*) classifier)->set_epsilon(svm_epsilon);
		((CSVMLin*) classifier)->set_bias_enabled(svm_use_bias);
		SG_INFO("created SVMLin object\n") ;
	}
	else if (strcmp(name,"SUBGRADIENTSVM")==0)
	{
		delete classifier;
		classifier= new CSubGradientSVM();

		((CSubGradientSVM*) classifier)->set_bias_enabled(svm_use_bias);
		((CSubGradientSVM*) classifier)->set_qpsize(svm_qpsize);
		((CSubGradientSVM*) classifier)->set_qpsize_max(svm_max_qpsize);
		((CSubGradientSVM*) classifier)->set_C(svm_C1, svm_C2);
		((CSubGradientSVM*) classifier)->set_epsilon(svm_epsilon);
		((CSubGradientSVM*) classifier)->set_max_train_time(max_train_time);
		SG_INFO("created Subgradient SVM object\n") ;
	}
	else if (strncmp(name,"WDSVMOCAS", strlen("WDSVMOCAS"))==0)
	{
		delete classifier;
		classifier= new CWDSVMOcas(SVM_OCAS);

		((CWDSVMOcas*) classifier)->set_degree(d, from_d);
		((CWDSVMOcas*) classifier)->set_C(svm_C1, svm_C2);
		((CWDSVMOcas*) classifier)->set_epsilon(svm_epsilon);
		((CWDSVMOcas*) classifier)->set_bufsize(svm_bufsize);
		SG_INFO("created Weighted Degree Kernel SVM Ocas(OCAS) object of order %d (from order:%d)\n", d, from_d) ;
	}
	else if (strcmp(name,"SVMOCAS")==0)
	{
		delete classifier;
		classifier= new CSVMOcas(SVM_OCAS);

		((CSVMOcas*) classifier)->set_C(svm_C1, svm_C2);
		((CSVMOcas*) classifier)->set_epsilon(svm_epsilon);
		((CSVMOcas*) classifier)->set_bufsize(svm_bufsize);
		((CSVMOcas*) classifier)->set_bias_enabled(svm_use_bias);
		SG_INFO("created SVM Ocas(OCAS) object\n") ;
	}
	else if (strcmp(name,"SVMSGD")==0)
	{
		delete classifier;
		classifier= new CSVMSGD(svm_C1);
		((CSVMSGD*) classifier)->set_bias_enabled(svm_use_bias);
		SG_INFO("created SVM SGD object\n") ;
	}
	else if (strcmp(name,"SVMBMRM")==0 || (strcmp(name,"SVMPERF")==0))
	{
		delete classifier;
		classifier= new CSVMOcas(SVM_BMRM);

		((CSVMOcas*) classifier)->set_C(svm_C1, svm_C2);
		((CSVMOcas*) classifier)->set_epsilon(svm_epsilon);
		((CSVMOcas*) classifier)->set_bufsize(svm_bufsize);
		((CSVMOcas*) classifier)->set_bias_enabled(svm_use_bias);
		SG_INFO("created SVM Ocas(BMRM/PERF) object\n") ;
	}
	else
	{
		SG_ERROR("Unknown classifier %s.\n", name);
		return false;
	}

	return (classifier!=NULL);
}

bool CGUIClassifier::train_svm()
{
	CSVM* svm= (CSVM*) classifier;
	if (!svm)
		SG_ERROR("No SVM available.\n");

	bool oneclass=(svm->get_classifier_type()==CT_LIBSVMONECLASS);
	CLabels* trainlabels=NULL;
	if(!oneclass)
		trainlabels=ui->ui_labels->get_train_labels();
	else
		SG_INFO("Training one class svm.\n");
	if (!trainlabels && !oneclass)
		SG_ERROR("No trainlabels available.\n");

	CKernel* kernel=ui->ui_kernel->get_kernel();
	if (!kernel)
		SG_ERROR("No kernel available.\n");
	if (!ui->ui_kernel->is_initialized() || !kernel->get_lhs())
		SG_ERROR("Kernel not initialized.\n");

	INT num_vec=kernel->get_lhs()->get_num_vectors();
	if (!oneclass && trainlabels->get_num_labels() != num_vec)
		SG_ERROR("Number of train labels (%d) and training vectors (%d) differs!\n", trainlabels->get_num_labels(), num_vec);

	SG_INFO("Starting SVM training on %ld vectors using C1=%lf C2=%lf epsilon=%lf\n", num_vec, svm_C1, svm_C2, svm_epsilon);

	svm->set_bias_enabled(svm_use_bias);
	svm->set_weight_epsilon(svm_weight_epsilon);
	svm->set_mkl_norm(svm_mkl_norm);
	svm->set_epsilon(svm_epsilon);
	svm->set_max_train_time(max_train_time);
	svm->set_tube_epsilon(svm_tube_epsilon);
	svm->set_nu(svm_nu);
	svm->set_C_mkl(svm_C_mkl);
	svm->set_C(svm_C1, svm_C2);
	svm->set_qpsize(svm_qpsize);
	svm->set_mkl_enabled(svm_use_mkl);
	svm->set_shrinking_enabled(svm_use_shrinking);
	svm->set_linadd_enabled(svm_use_linadd);
	svm->set_batch_computation_enabled(svm_use_batch_computation);
	if(!oneclass)
		((CKernelMachine*) svm)->set_labels(trainlabels);
	((CKernelMachine*) svm)->set_kernel(kernel);
	((CSVM*) svm)->set_precomputed_subkernels_enabled(svm_use_precompute_subkernel_light);
	kernel->set_precompute_matrix(svm_use_precompute, svm_use_precompute_subkernel);

#ifdef USE_SVMLIGHT
	if (svm_do_auc_maximization)
		((CSVMLight*)svm)->setup_auc_maximization();
#endif //USE_SVMLIGHT

	bool result=svm->train();
	kernel->set_precompute_matrix(false,false);

	return result;
}

bool CGUIClassifier::train_clustering(INT k, INT max_iter)
{
	bool result=false;
	CDistance* distance=ui->ui_distance->get_distance();

	if (!distance)
		SG_ERROR("No distance available\n");

	((CDistanceMachine*) classifier)->set_distance(distance);

	EClassifierType type=classifier->get_classifier_type();
	switch (type)
	{
		case CT_KMEANS:
		{
			((CKMeans*) classifier)->set_k(k);
			((CKMeans*) classifier)->set_max_iter(max_iter);
			result=((CKMeans*) classifier)->train();
			break;
		}
		case CT_HIERARCHICAL:
		{
			((CHierarchical*) classifier)->set_merges(k);
			result=((CHierarchical*) classifier)->train();
			break;
		}
		default:
			SG_ERROR("Unknown clustering type %d\n", type);
	}

	return result;
}

bool CGUIClassifier::train_knn(INT k)
{
	CLabels* trainlabels=ui->ui_labels->get_train_labels();
	CDistance* distance=ui->ui_distance->get_distance();

	bool result=false;

	if (trainlabels)
	{
		if (distance)
		{
			((CKNN*) classifier)->set_labels(trainlabels);
			((CKNN*) classifier)->set_distance(distance);
			((CKNN*) classifier)->set_k(k);
			result=((CKNN*) classifier)->train();
		}
		else
			SG_ERROR("No distance available.\n");
	}
	else
		SG_ERROR("No labels available\n");

	return result;
}

bool CGUIClassifier::train_linear(DREAL gamma)
{
	CFeatures* trainfeatures=ui->ui_features->get_train_features();
	CLabels* trainlabels=ui->ui_labels->get_train_labels();

	bool result=false;

	if (!trainfeatures)
		SG_ERROR("No trainfeatures available.\n");
	if (trainfeatures->get_feature_class()!=C_SIMPLE ||
		trainfeatures->get_feature_type()!=F_DREAL)
		SG_ERROR("Trainfeatures are not of class SIMPLE type REAL.\n");

	if (!trainlabels)
		SG_ERROR("No labels available\n");

	if (classifier->get_classifier_type()==CT_PERCEPTRON)
	{
		((CPerceptron*) classifier)->set_learn_rate(perceptron_learnrate);
		((CPerceptron*) classifier)->set_max_iter(perceptron_maxiter);
	}

#ifdef HAVE_LAPACK
	if (classifier->get_classifier_type()==CT_LDA)
		((CLDA*) classifier)->set_gamma(gamma);
#endif

	((CLinearClassifier*) classifier)->set_labels(trainlabels);
	((CLinearClassifier*) classifier)->set_features((CRealFeatures*) trainfeatures);
	result=((CLinearClassifier*) classifier)->train();

	return result;
}

bool CGUIClassifier::train_wdocas()
{
	CFeatures* trainfeatures=ui->ui_features->get_train_features();
	CLabels* trainlabels=ui->ui_labels->get_train_labels();

	bool result=false;

	if (!trainfeatures)
		SG_ERROR("No trainfeatures available.\n");

	if (trainfeatures->get_feature_class()!=C_STRING ||
			trainfeatures->get_feature_type()!=F_BYTE )
		SG_ERROR("Trainfeatures are not of class STRING type BYTE.\n");

	if (!trainlabels)
		SG_ERROR("No labels available.\n");

	((CWDSVMOcas*) classifier)->set_labels(trainlabels);
	((CWDSVMOcas*) classifier)->set_features((CStringFeatures<BYTE>*) trainfeatures);
	result=((CWDSVMOcas*) classifier)->train();

	return result;
}

bool CGUIClassifier::train_sparse_linear()
{
	CFeatures* trainfeatures=ui->ui_features->get_train_features();
	CLabels* trainlabels=ui->ui_labels->get_train_labels();

	bool result=false;

	if (!trainfeatures)
		SG_ERROR("No trainfeatures available.\n");

	if (trainfeatures->get_feature_class()!=C_SPARSE ||
		trainfeatures->get_feature_type()!=F_DREAL)
		SG_ERROR("Trainfeatures are not of class SPARSE type REAL.\n");

	if (!trainlabels)
		SG_ERROR("No labels available.\n");

	((CSparseLinearClassifier*) classifier)->set_labels(trainlabels);
	((CSparseLinearClassifier*) classifier)->set_features((CSparseFeatures<DREAL>*) trainfeatures);
	result=((CSparseLinearClassifier*) classifier)->train();

	return result;
}

bool CGUIClassifier::test(CHAR* filename_out, CHAR* filename_roc)
{
	FILE* file_out=stdout;
	FILE* file_roc=NULL;

	if (filename_out)
	{
		file_out=fopen(filename_out, "w");
		if (!file_out)
			SG_ERROR("Could not open file %s.\n", filename_out);

		if (filename_roc)
		{
			file_roc=fopen(filename_roc, "w");

			if (!file_roc)
				SG_ERROR("Could not open file %s.\n", filename_roc);
		}
	}

	CLabels* testlabels=ui->ui_labels->get_test_labels();
	CFeatures* trainfeatures=ui->ui_features->get_train_features();
	CFeatures* testfeatures=ui->ui_features->get_test_features();
	SG_DEBUG("I:training: %ld examples each %ld features\n", ((CRealFeatures*) trainfeatures)->get_num_vectors(), ((CRealFeatures*) trainfeatures)->get_num_features());
	SG_DEBUG("I:testing: %ld examples each %ld features\n", ((CRealFeatures*) testfeatures)->get_num_vectors(), ((CRealFeatures*) testfeatures)->get_num_features());

	if (!classifier)
		SG_ERROR("No svm available.\n");
	if (!trainfeatures)
		SG_ERROR("No training features available.\n");
	if (!testfeatures)
		SG_ERROR("No test features available.\n");
	if (!testlabels)
		SG_ERROR("No test labels available.\n");
	if (!ui->ui_kernel->is_initialized())
		SG_ERROR("Kernel not initialized.\n");

	SG_INFO("Starting svm testing.\n");
	((CKernelMachine*) classifier)->set_labels(testlabels);
	((CKernelMachine*) classifier)->set_kernel(ui->ui_kernel->get_kernel());
	ui->ui_kernel->get_kernel()->set_precompute_matrix(false,false);
	((CKernelMachine*) classifier)->set_batch_computation_enabled(svm_use_batch_computation);

	CLabels* predictions= classifier->classify();

	INT len=0;
	DREAL* output= predictions->get_labels(len);
	INT total=testfeatures->get_num_vectors();
	INT* label=testlabels->get_int_labels(len);
	ASSERT(label);

	SG_DEBUG("len:%d total:%d\n", len, total);
	ASSERT(len==total);

	ui->ui_math->evaluate_results(output, label, total, file_out, file_roc);

	if (file_roc)
		fclose(file_roc);
	if ((file_out) && (file_out!=stdout))
		fclose(file_out);

	delete[] output;
	delete[] label;
	return true;
}

bool CGUIClassifier::load(CHAR* filename, CHAR* type)
{
	bool result=false;

	if (new_classifier(type))
	{
		FILE* model_file=fopen(filename, "r");

		if (model_file)
		{
			if (classifier && classifier->load(model_file))
			{
				SG_DEBUG("file successfully read.\n");
				result=true;
			}
			else
				SG_ERROR("SVM/Classifier creation/loading failed on file %s.\n", filename);

			fclose(model_file);
		}
		else
			SG_ERROR("Opening file %s failed.\n", filename);

		return result;
	}
	else
		SG_ERROR("Type %s of SVM/Classifier unknown.\n", type);

	return false;
}

bool CGUIClassifier::save(CHAR* param)
{
	bool result=false;
	param=CIO::skip_spaces(param);

	if (classifier)
	{
		FILE* file=fopen(param, "w");

		if ((!file) ||	(!classifier->save(file)))
			printf("writing to file %s failed!\n", param);
		else
		{
			printf("successfully written classifier into \"%s\" !\n", param);
			result=true;
		}

		if (file)
			fclose(file);
	}
	else
		SG_ERROR("create classifier first\n");

	return result;
}

bool CGUIClassifier::set_perceptron_parameters(DREAL learnrate, INT maxiter)
{
	if (learnrate<=0)
		perceptron_learnrate=0.01;
	else
		perceptron_learnrate=learnrate;

	if (maxiter<=0)
		perceptron_maxiter=1000;
	else
		perceptron_maxiter=maxiter;
	SG_INFO("Setting to perceptron parameters (learnrate %f and maxiter: %d\n", perceptron_learnrate, perceptron_maxiter);

	return true;
}

bool CGUIClassifier::set_svm_epsilon(DREAL epsilon)
{
	if (epsilon<0)
		svm_epsilon=1e-4;
	else
		svm_epsilon=epsilon;
	SG_INFO("Set to svm_epsilon=%f.\n", svm_epsilon);

	return true;
}

bool CGUIClassifier::set_max_train_time(DREAL max)
{
	if (max>0)
	{
		max_train_time=max;
		SG_INFO("Set to max_train_time=%f.\n", max_train_time);
	}
	else
		SG_INFO("Disabling max_train_time.\n");

	return true;
}

bool CGUIClassifier::set_svr_tube_epsilon(DREAL tube_epsilon)
{
	if (tube_epsilon<0)
		svm_tube_epsilon=1e-2;
	svm_tube_epsilon=tube_epsilon;

	((CSVM*) classifier)->set_tube_epsilon(svm_tube_epsilon);
	SG_INFO("Set to svr_tube_epsilon=%f.\n", svm_tube_epsilon);

	return true;
}

bool CGUIClassifier::set_svm_one_class_nu(DREAL nu)
{
	if (nu<0 || nu>1)
		svm_nu=0.5;
	SG_INFO("Set to nu=%f.\n", svm_nu);

	return true;
}

bool CGUIClassifier::set_svm_mkl_parameters(DREAL weight_epsilon, DREAL C_mkl, INT mkl_norm)
{
	if (weight_epsilon<0)
		svm_weight_epsilon=1e-4;
	if (C_mkl<0)
		svm_C_mkl=0;
	if (mkl_norm!=1 || mkl_norm!=2)
		svm_mkl_norm=1;

	svm_weight_epsilon=weight_epsilon;
	svm_C_mkl=C_mkl;
	svm_mkl_norm=mkl_norm;

	SG_INFO("Set to weight_epsilon=%f.\n", svm_weight_epsilon);
	SG_INFO("Set to C_mkl=%f.\n", svm_C_mkl);
	SG_INFO("Set to mkl_norm=%d.\n", svm_mkl_norm);

	return true;
}

bool CGUIClassifier::set_svm_C(DREAL C1, DREAL C2)
{
	if (C1<0)
		svm_C1=1.0;
	else
		svm_C1=C1;

	if (C2<0)
		svm_C2=svm_C1;
	else
		svm_C2=C2;

	SG_INFO("Set to C1=%f C2=%f.\n", svm_C1, svm_C2);

	return true;
}

bool CGUIClassifier::set_svm_qpsize(INT qpsize)
{
	if (qpsize<2)
		svm_qpsize=41;
	else
		svm_qpsize=qpsize;
	SG_INFO("Set qpsize to svm_qpsize=%d.\n", svm_qpsize);

	return true;
}

bool CGUIClassifier::set_svm_max_qpsize(INT max_qpsize)
{
	if (max_qpsize<50)
		svm_max_qpsize=50;
	else
		svm_max_qpsize=max_qpsize;
	SG_INFO("Set max qpsize to svm_max_qpsize=%d.\n", svm_max_qpsize);

	return true;
}

bool CGUIClassifier::set_svm_bufsize(INT bufsize)
{
	if (svm_bufsize<0)
		svm_bufsize=3000;
	else
		svm_bufsize=bufsize;
	SG_INFO("Set bufsize to svm_bufsize=%d.\n", svm_bufsize);

	return true ;
}

bool CGUIClassifier::set_svm_mkl_enabled(bool enabled)
{
	svm_use_mkl=enabled;
	if (svm_use_mkl)
		SG_INFO("Enabling MKL optimization.\n");
	else
		SG_INFO("Disabling MKL optimization.\n");

	return true;
}

bool CGUIClassifier::set_svm_shrinking_enabled(bool enabled)
{
	svm_use_shrinking=enabled;
	if (svm_use_shrinking)
		SG_INFO("Enabling shrinking optimization.\n");
	else
		SG_INFO("Disabling shrinking optimization.\n");

	return true;
}

bool CGUIClassifier::set_svm_batch_computation_enabled(bool enabled)
{
	svm_use_batch_computation=enabled;
	if (svm_use_batch_computation)
		SG_INFO("Enabling batch computation.\n");
	else
		SG_INFO("Disabling batch computation.\n");

	return true;
}

bool CGUIClassifier::set_svm_precompute_enabled(INT precompute)
{
	svm_use_precompute=(precompute==1);
	svm_use_precompute_subkernel=(precompute==2);
	svm_use_precompute_subkernel_light=(precompute==3);

	if (svm_use_precompute)
		SG_INFO("Enabling Kernel Matrix Precomputation.\n");
	else
		SG_INFO("Disabling Kernel Matrix Precomputation.\n");

	if (svm_use_precompute_subkernel)
		SG_INFO("Enabling Subkernel Matrix Precomputation.\n");
	else
		SG_INFO("Disabling Subkernel Matrix Precomputation.\n");

	if (svm_use_precompute_subkernel_light)
		SG_INFO("Enabling Subkernel Matrix Precomputation by SVM Light.\n");
	else
		SG_INFO("Disabling Subkernel Matrix Precomputation by SVM Light.\n");

	return true;
}

bool CGUIClassifier::set_svm_linadd_enabled(bool enabled)
{
	svm_use_linadd=enabled;
	if (svm_use_linadd)
		SG_INFO("Enabling LINADD optimization.\n");
	else
		SG_INFO("Disabling LINADD optimization.\n");

	return true;
}

bool CGUIClassifier::set_svm_bias_enabled(bool enabled)
{
	svm_use_bias=enabled;
	if (svm_use_bias)
		SG_INFO("Enabling svm bias.\n");
	else
		SG_INFO("Disabling svm bias.\n");

	return true;
}

bool CGUIClassifier::set_do_auc_maximization(bool do_auc)
{
	svm_do_auc_maximization=do_auc;

	if (svm_do_auc_maximization)
		SG_INFO("Enabling AUC maximization.\n");
	else
		SG_INFO("Disabling AUC maximization.\n");

	return true;
}

CLabels* CGUIClassifier::classify(CLabels* output)
{
	ASSERT(classifier);

	switch (classifier->get_classifier_type())
	{
		case CT_LIGHT:
		case CT_LIBSVM:
		case CT_MPD:
		case CT_GPBT:
		case CT_CPLEXSVM:
		case CT_GMNPSVM:
		case CT_GNPPSVM:
		case CT_KERNELPERCEPTRON:
		case CT_LIBSVR:
		case CT_LIBSVMMULTICLASS:
		case CT_LIBSVMONECLASS:
		case CT_SVRLIGHT:
		case CT_KRR:
			return classify_kernelmachine(output);
		case CT_KNN:
			return classify_distancemachine(output);
		case CT_PERCEPTRON:
		case CT_LDA:
			return classify_linear(output);
		case CT_SVMLIN:
		case CT_SVMPERF:
		case CT_SUBGRADIENTSVM:
		case CT_SVMOCAS:
		case CT_SVMSGD:
		case CT_LPM:
		case CT_LPBOOST:
		case CT_SUBGRADIENTLPM:
		case CT_LIBLINEAR:
			return classify_sparse_linear(output);
		case CT_WDSVMOCAS:
			return classify_byte_linear(output);
		default:
			SG_ERROR("unknown classifier type\n");
			break;
	};

	return false;
}

CLabels* CGUIClassifier::classify_kernelmachine(CLabels* output)
{
	CFeatures* trainfeatures=ui->ui_features->get_train_features();
	CFeatures* testfeatures=ui->ui_features->get_test_features();
	ui->ui_kernel->get_kernel()->set_precompute_matrix(false,false);

	if (!classifier)
		SG_ERROR("No kernelmachine available.\n");
	if (!trainfeatures)
		SG_ERROR("No training features available.\n");
	if (!testfeatures)
		SG_ERROR("No test features available.\n");
	if (!ui->ui_kernel->is_initialized())
		SG_ERROR("Kernel not initialized.\n");

	CKernelMachine* km=(CKernelMachine*) classifier;
	km->set_kernel(ui->ui_kernel->get_kernel());
	ui->ui_kernel->get_kernel()->set_precompute_matrix(false,false);
	km->set_batch_computation_enabled(svm_use_batch_computation);

	SG_INFO("Starting kernel machine testing.\n");
	return classifier->classify(output);
}

bool CGUIClassifier::get_trained_classifier(DREAL* &weights, INT &rows, INT &cols,
		DREAL*& bias, INT& brows, INT& bcols)
{
	ASSERT(classifier);

	switch (classifier->get_classifier_type())
	{
		case CT_LIGHT:
		case CT_LIBSVM:
		case CT_MPD:
		case CT_GPBT:
		case CT_CPLEXSVM:
		case CT_GMNPSVM:
		case CT_GNPPSVM:
		case CT_KERNELPERCEPTRON:
		case CT_LIBSVR:
		case CT_LIBSVMMULTICLASS:
		case CT_LIBSVMONECLASS:
		case CT_SVRLIGHT:
		case CT_KRR:
			return get_svm(weights, rows, cols, bias, brows, bcols);
			break;
		case CT_PERCEPTRON:
		case CT_LDA:
			return get_linear(weights, rows, cols, bias, brows, bcols);
			break;
		case CT_KMEANS:
		case CT_HIERARCHICAL:
			return get_clustering(weights, rows, cols, bias, brows, bcols);
			break;
		case CT_KNN:
			SG_ERROR("not implemented");
			break;
		case CT_LPM:
		case CT_LPBOOST:
		case CT_SUBGRADIENTLPM:
		case CT_SVMOCAS:
		case CT_SVMSGD:
		case CT_SVMLIN:
		case CT_SVMPERF:
		case CT_SUBGRADIENTSVM:
		case CT_LIBLINEAR:
			return get_sparse_linear(weights, rows, cols, bias, brows, bcols);
			break;
		default:
			SG_ERROR("unknown classifier type\n");
			break;
	};
	return false;
}

bool CGUIClassifier::get_svm(DREAL* &weights, INT& rows, INT& cols,
		DREAL*& bias, INT& brows, INT& bcols)
{
	CSVM* svm=(CSVM*) classifier;

	if (svm)
	{
		brows=1;
		bcols=1;
		bias=new DREAL[1];
		*bias=svm->get_bias();

		rows=svm->get_num_support_vectors();
		cols=2;
		weights=new DREAL[rows*cols];

		for (int i=0; i<rows; i++)
		{
			weights[i]=svm->get_alpha(i);
			weights[i+rows]=svm->get_support_vector(i);
		}

		return true;
	}

	return false;
}

bool CGUIClassifier::get_clustering(DREAL* &centers, INT& rows, INT& cols,
		DREAL*& radi, INT& brows, INT& bcols)
{
	if (!classifier)
		return false;

	switch (classifier->get_classifier_type())
	{
		case CT_KMEANS:
		{
			CKMeans* clustering=(CKMeans*) classifier;

			bcols=1;
			clustering->get_radi(radi, brows);

			cols=1;
			clustering->get_centers(centers, rows, cols);
			break;
		}

		case CT_HIERARCHICAL:
		{
			CHierarchical* clustering=(CHierarchical*) classifier;

			// radi == merge_distances, centers == pairs
			bcols=1;
			clustering->get_merge_distance(radi, brows);

			INT* p=NULL;
			clustering->get_pairs(p, rows, cols);
			centers=new DREAL[rows*cols]; // FIXME memleak
			for (INT i=0; i<rows*cols; i++)
				centers[i]=(DREAL) p[i];

			break;
		}

		default:
			SG_ERROR("internal error - unknown clustering type\n");
	}

	return true;
}

bool CGUIClassifier::get_linear(DREAL* &weights, INT& rows, INT& cols,
		DREAL*& bias, INT& brows, INT& bcols)
{
	CLinearClassifier* linear=(CLinearClassifier*) classifier;

	if (!linear)
		return false;

	bias=new DREAL[1];
	*bias=linear->get_bias();
	brows=1;
	bcols=1;

	cols=1;
	linear->get_w(&weights, &rows);
	return true;
}

bool CGUIClassifier::get_sparse_linear(DREAL* &weights, INT& rows, INT& cols,
		DREAL*& bias, INT& brows, INT& bcols)
{
	CSparseLinearClassifier* linear=(CSparseLinearClassifier*) classifier;

	if (!linear)
		return false;

	bias=new DREAL[1];
	*bias=linear->get_bias();
	brows=1;
	bcols=1;

	cols=1;
	linear->get_w(&weights, &rows);
	return true;
}


CLabels* CGUIClassifier::classify_distancemachine(CLabels* output)
{
	CFeatures* trainfeatures=ui->ui_features->get_train_features();
	CFeatures* testfeatures=ui->ui_features->get_test_features();
	ui->ui_distance->get_distance()->set_precompute_matrix(false);

	if (!classifier)
	{
		SG_ERROR("no kernelmachine available\n") ;
		return NULL;
	}
	if (!trainfeatures)
	{
		SG_ERROR("no training features available\n") ;
		return NULL;
	}

	if (!testfeatures)
	{
		SG_ERROR("no test features available\n") ;
		return NULL;
	}

	if (!ui->ui_distance->is_initialized())
	{
		SG_ERROR("distance not initialized\n") ;
		return NULL;
	}
	  
	((CDistanceMachine*) classifier)->set_distance(
		ui->ui_distance->get_distance());
	ui->ui_distance->get_distance()->set_precompute_matrix(false);
	SG_INFO("starting distance machine testing\n") ;
	return classifier->classify(output);
}


CLabels* CGUIClassifier::classify_linear(CLabels* output)
{
	CFeatures* testfeatures=ui->ui_features->get_test_features();

	if (!classifier)
	{
		SG_ERROR("no classifier available\n") ;
		return NULL;
	}
	if (!testfeatures)
	{
		SG_ERROR("no test features available\n") ;
		return NULL;
	}
	if (testfeatures->get_feature_class() != C_SIMPLE ||
			testfeatures->get_feature_type() != F_DREAL )
	{
		SG_ERROR("testfeatures not of class SIMPLE type REAL\n") ;
		return false ;
	}

	((CLinearClassifier*) classifier)->set_features((CRealFeatures*) testfeatures);
	SG_INFO("starting linear classifier testing\n") ;
	return classifier->classify(output);
}

CLabels* CGUIClassifier::classify_byte_linear(CLabels* output)
{
	CFeatures* testfeatures=ui->ui_features->get_test_features();

	if (!classifier)
	{
		SG_ERROR("no svm available\n") ;
		return NULL;
	}
	if (!testfeatures)
	{
		SG_ERROR("no test features available\n") ;
		return NULL;
	}
	if (testfeatures->get_feature_class() != C_STRING ||
			testfeatures->get_feature_type() != F_BYTE )
	{
		SG_ERROR("testfeatures not of class STRING type BYTE\n") ;
		return false ;
	}

	((CWDSVMOcas*) classifier)->set_features((CStringFeatures<BYTE>*) testfeatures);
	SG_INFO("starting linear classifier testing\n") ;
	return classifier->classify(output);
}

CLabels* CGUIClassifier::classify_sparse_linear(CLabels* output)
{
	CFeatures* testfeatures=ui->ui_features->get_test_features();

	if (!classifier)
	{
		SG_ERROR("no svm available\n") ;
		return NULL;
	}
	if (!testfeatures)
	{
		SG_ERROR("no test features available\n") ;
		return NULL;
	}
	if (testfeatures->get_feature_class() != C_SPARSE ||
			testfeatures->get_feature_type() != F_DREAL )
	{
		SG_ERROR("testfeatures not of class SPARSE type REAL\n") ;
		return false ;
	}

	((CSparseLinearClassifier*) classifier)->set_features((CSparseFeatures<DREAL>*) testfeatures);
	SG_INFO("starting linear classifier testing\n") ;
	return classifier->classify(output);
}

bool CGUIClassifier::classify_example(INT idx, DREAL &result)
{
	CFeatures* trainfeatures=ui->ui_features->get_train_features();
	CFeatures* testfeatures=ui->ui_features->get_test_features();
	ui->ui_kernel->get_kernel()->set_precompute_matrix(false,false);

	if (!classifier)
	{
		SG_ERROR("no svm available\n") ;
		return false;
	}
	if (!trainfeatures)
	{
		SG_ERROR("no training features available\n") ;
		return false;
	}

	if (!testfeatures)
	{
		SG_ERROR("no test features available\n") ;
		return false;
	}

	if (!ui->ui_kernel->is_initialized())
	{
		SG_ERROR("kernel not initialized\n") ;
		return false;
	}

	((CKernelMachine*) classifier)->set_kernel(
		ui->ui_kernel->get_kernel());

	result=classifier->classify_example(idx);
	return true ;
}


bool CGUIClassifier::set_krr_tau(DREAL tau)
{
	krr_tau=tau;
#ifdef HAVE_LAPACK
	((CKRR*) classifier)->set_tau(krr_tau);
	SG_INFO("Set to krr_tau=%f.\n", krr_tau);
#endif
	return true;
}

#endif
