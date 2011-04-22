/*
  This file is part of MADNESS.

  Copyright (C) 2007,2010 Oak Ridge National Laboratory

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

  For more information please contact:

  Robert J. Harrison
  Oak Ridge National Laboratory
  One Bethel Valley Road
  P.O. Box 2008, MS-6367

  email: harrisonrj@ornl.gov
  tel:   865-241-3937
  fax:   865-572-0680

  $Id$
*/

#ifndef SEPREP_H_
#define SEPREP_H_

#include "mra/srconf.h"
#include "tensor/tensor.h"
#include <stdexcept>

#define bench 0

namespace madness {


	/**
	 * This is a separated representation as described in BM2005:
	 * Beylkin, Mohlenkamp, Siam J Sci Comput, 2005 vol. 26 (6) pp. 2133-2159
	 * for dimensions d
	 *  f(x) = sum_m^r s^m_k1 * s^m_k2 * .. * s^m_k(d)
	 *
	 * A SepRep keeps all the information about the tensor --
	 * the data are stored in SRConf.
	 *
	 * For now we will only use square tensors
	 */
	template<typename T>
	class SepRep {
	public:

		typedef Tensor<T> tensorT;

	private:
		/// the actual data stored in a vector of length rank_
		SRConf<T> configs_;

		/// the machine precision
		static const double machinePrecision=1.e-14;

		/// sqrt of the machine precision
		static const double sqrtMachinePrecision=1.e-7;

		static const double facReduce=1.e-4;

	public:
		/// default ctor, an invalid tensor
		SepRep() : configs_() {
			print("There is no default ctor for SepRep w/o TensorType");
			MADNESS_ASSERT(0);
		}

		/// default ctor, an invalid tensor
		SepRep(const TensorType& tt) : configs_(tt) {}

		/// ctor w/ configs, shallow
		SepRep(const SRConf<T>& config) : configs_(config) {}

		/// the default ctor with a given representation, is a valid tensor
		SepRep(const TensorType& tt, const unsigned int& maxk, const unsigned int& dim)
			: configs_(SRConf<T>(dim,maxk,tt)) {
			// construct empty SRConf
		}

		/// ctor with a polynomial values
		SepRep(const Tensor<T>& values, const double& eps, const TensorType& tt)
			: configs_(SRConf<T>(values.ndim(),values.dim(0),tt)) {

			MADNESS_ASSERT(values.ndim()>0);

			// adapt form of values
			std::vector<long> d(configs_.dim_eff(),configs_.kVec());
			Tensor<T> values_eff=values.reshape(d);

			// direct reduction on the polynomial values on the Tensor
			if (this->tensor_type()==TT_3D) {
				this->doReduceRank(eps,values_eff);
			} else if (this->tensor_type()==TT_2D) {
				this->computeSVD(eps,values_eff);
			} else {
				MADNESS_ASSERT(0);
			}
			MADNESS_ASSERT(this->configs_.has_structure() or this->rank()==0);

		}

		/// copy ctor (shallow)
		SepRep(const SepRep& rhs)
			: configs_(rhs.configs_)	{
		}

		/// deep copy of rhs by deep copying rhs.configs
		friend SepRep<T> copy(const SepRep<T>& rhs) {
			return SepRep<T>(copy(rhs.configs_));
		}

		/// dtor
		~SepRep() {};

		/// assignment operator
		SepRep& operator=(const SepRep& rhs) {
			configs_=rhs.configs_;
			return *this;
		}

		/// add another SepRep to this one
		SepRep& operator+=(const SepRep& rhs) {

			MADNESS_ASSERT(this->tensor_type()==rhs.tensor_type());
			MADNESS_ASSERT(this->get_k()==rhs.get_k());
			MADNESS_ASSERT(this->dim()==rhs.dim());
			MADNESS_ASSERT(&(*this)!=&rhs);

			configs_+=rhs.configs_;
			return *this;
		}

		/// substract another SepRep from this one (tested)
		SepRep& operator-=(const SepRep& rhs) {

			MADNESS_ASSERT(tensor_type()==rhs.tensor_type());
			MADNESS_ASSERT(this->get_k()==rhs.get_k());
			MADNESS_ASSERT(&(*this)!=&rhs);
			configs_-=rhs.configs_;

			return *this;
		}

		/// return a slice of this (deep copy)
		SepRep operator()(const std::vector<Slice>& s) const {

			// consistency check
			MADNESS_ASSERT(s.size()==this->dim());
			MADNESS_ASSERT(s[0].step==1);

			// fast return if possible
			if (this->rank()==0) {
				int k_new=s[0].end-s[0].start+1;
				return SepRep<T> (this->tensor_type(),k_new,this->dim());
			}

			MADNESS_ASSERT(this->configs_.has_structure());

			// get dimensions
			const TensorType tt=this->tensor_type();
			const int merged_dim=this->configs_.dim_per_vector();
			const int dim_eff=this->configs_.dim_eff();
			const int rank=this->rank();
			int k_new=s[0].end-s[0].start+1;
			if (s[0].end<0) k_new+=this->get_k();

			// get and reshape the vectors, slice and re-reshape again;
			// this is shallow
			const SepRep<T>& sr=*this;

			std::vector<Tensor<T> > vectors(dim_eff,Tensor<T>());
			for (int idim=0; idim<dim_eff; idim++) {

				// assignment from/to slice is deep-copy
				if (merged_dim==1) {
					if (rank>0) {
						vectors[idim]=copy(sr.configs_.ref_vector(idim)(Slice(0,rank-1),s[idim]));
					} else {
						vectors[idim]=Tensor<T>(0,s[idim].end-s[idim].start+1);
					}
				} else if (merged_dim==2) {
					if (rank>0) {
						vectors[idim]=copy(sr.configs_.ref_vector(idim)(Slice(0,rank-1),s[2*idim],s[2*idim+1]));
					} else {
						vectors[idim]=Tensor<T>(0,s[2*idim].end-s[2*idim].start+1,s[2*idim+1].end-s[2*idim+1].start+1);
					}
				} else if (merged_dim==3) {
					if (rank>0) {
						vectors[idim]=copy(sr.configs_.ref_vector(idim)(Slice(0,rank-1),s[3*idim],s[3*idim+1],s[3*idim+2]));
					} else {
						vectors[idim]=Tensor<T>(0,s[3*idim].end-s[3*idim].start+1,s[3*idim+1].end-s[3*idim+1].start+1,
								s[3*idim+2].end-s[3*idim+2].start+1);

					}
				} else MADNESS_ASSERT(0);
			}

			// work-around for rank==0
			Tensor<double> weights;
			if (rank>0) {
				weights=copy(this->configs_.weights_);
			} else {
				weights=Tensor<double>(int(0));
			}
			const SRConf<T> conf(weights,vectors,this->dim(),k_new,tt);

			return SepRep<T>(conf);

		}

		/// same as operator+=, but handles non-conforming vectors (i.e. slices)
		void inplace_add(const SepRep<T>& rhs, const std::vector<Slice>& lhs_s,
				const std::vector<Slice>& rhs_s, const double alpha, const double beta) {


			// fast return if possible
			if (rhs.rank()==0) return;

			// no fast return possible!!!
//			if (this->rank()==0) {
//				// this is a deep copy
//				*this=rhs(rhs_s);
//				return;
//			}

			this->configs_.inplace_add(rhs.configs_,lhs_s,rhs_s, alpha, beta);
		}

		/// is this a valid tensor? Note that the rank might still be zero.
		bool is_valid() const {

			bool it_is=this->dim()>0;

			// double check
			if (it_is) MADNESS_ASSERT((this->get_k()>0) || (this->tensor_type()!=TT_NONE));
			return it_is;
		}

		/// make this zero
		void zeroOut() {this->configs_.zeroOut();};

		/// return if the underlying SRConf is a vector
		bool isVector() const {return configs_.isVector();};

		/// multiply with a scalar
		void scale(const double& dfac) {configs_.scale(dfac);};

		/// don't multiply with a complex number
		void scale(const double_complex& dfac) {
			print("no seprep::scale with double_complex");
			MADNESS_ASSERT(0);
		};

		/// normalize
		void normalize() {configs_.normalize();};

		void fillrandom() {configs_.fillWithRandom();}

		/// return the condition number, i.e. the largest EV^2 divided by machine precision
		double conditionNumber() const {

			double largeEV=configs_.maxWeight();

			// sqrt(rank()) comes with Gauss error analysis
		//	const double condition=sqrt(rank()) * std::max(largeEV*largeEV*machinePrecision,machinePrecision*1.0);
			const double condition=std::max(largeEV*largeEV*machinePrecision,machinePrecision*1.0);

			return condition;
		}

		/// reserve enough space to hold maxRank configurations
		void reserve(const unsigned int& r) {configs_.ensureSpace(r);};

		/// return the separation rank
		unsigned int rank() const {return configs_.rank();};

		/// return the dimension
		unsigned int dim() const {return configs_.dim();};

		/// return the polynomial order
		unsigned int get_k() const {return configs_.get_k();};

		/// return the number length of the underlying vectors
		unsigned int kVec() const {return configs_.kVec();};

		/// return the number of coefficients
		unsigned int nCoeff() const {return configs_.nCoeff();};

		/// return the representation
		TensorType tensor_type() const {return configs_.type();};

		/// return a vector of Tensor views of an SRConf
	//	const std::vector<tensor<T> > refSRConf(const unsigned int& srIndex) const;

		/// return the weight of an SRConf
		double weight(const unsigned int& i) const {return configs_.weights(i);};


		/// reduce the rank using SVD
		void computeSVD(const double& eps,const Tensor<T>& values_eff) {

			// SVD works only with matrices (2D)
			MADNESS_ASSERT(values_eff.ndim()==2);
			MADNESS_ASSERT(this->tensor_type()==TT_2D);

			// fast return if possible
			if (values_eff.normf()<eps*facReduce) return;

			// output from svd
			Tensor<T> U;
			Tensor<T> VT;
			Tensor< typename Tensor<T>::scalar_type > s;

			svd(values_eff,U,s,VT);

			// find the maximal singular value that's supposed to contribute
			// singular values are ordered (largest first)
			const double threshold=eps*eps*facReduce;
			double residual=0.0;
			long i;
			for (i=s.dim(0)-1; i>=0; i--) {
				residual+=s(i)*s(i);
				if (residual>threshold) break;
			}

			// convert SVD output to our convention
			if (i>=0) {
				this->configs_.weights_=s(Slice(0,i));
				this->configs_.vector_[0]=transpose(U(Slice(_),Slice(0,i)));
				this->configs_.vector_[1]=(VT(Slice(0,i),Slice(_)));
				this->configs_.rank_=i+1;
				MADNESS_ASSERT(this->configs_.kVec()==this->configs_.vector_[0].dim(1));
				MADNESS_ASSERT(this->configs_.rank()==this->configs_.vector_[0].dim(0));
				MADNESS_ASSERT(this->configs_.rank()==this->configs_.weights_.dim(0));
			}
			this->configs_.make_structure();
		}

		void reduceRank(const double& eps, const Tensor<T>& values=Tensor<T>()) {

			// direct reduction on the polynomial values on the Tensor
			if (this->tensor_type()==TT_3D) {
				this->doReduceRank(eps,values);
				this->configs_.make_structure();
			} else if (this->tensor_type()==TT_2D) {
				MADNESS_ASSERT(not values.has_data());
				Tensor<T> values=this->reconstructTensor();

				// adapt form of values
				std::vector<long> d(configs_.dim_eff(),configs_.kVec());
				Tensor<T> values_eff=values.reshape(d);

				this->computeSVD(eps,values_eff);
			} else {
				MADNESS_ASSERT(0);
			}
			MADNESS_ASSERT(this->configs_.has_structure() or this->rank()==0);
		}

		/// reduce the separation rank of this to a near optimal value
		/// follow section 3 in BM2005
		void doReduceRank(const double& eps, const Tensor<T>& values=Tensor<T>()){//,
//				const SepRep& trial2=SepRep()) {
			/*
			 * basic idea is to use the residual Frobenius norm to check
			 * convergence. Don't know if this is rigorous, probably not..
			 *
			 * convergence criterion: optimize the trial SepRep until the
			 * residual doesn't change any more
			 */

//			madness::print(values);

			/*
			 * figure out what to do:
			 * 	1. this exists and is to be reduced
			 * 	2. this doesn't exist, but values are provided
			 */
			SepRep& reference=*this;
			const bool haveSR=(reference.rank()!=0);
			const bool haveVal=(values.ndim()!=-1);

			// set factors
			double facSR=0.0;
			if (haveSR) facSR=1.0;

			double facVal=0.0;
			if (haveVal) facVal=1.0;

			// fast return if possible
			if ((not haveSR) and (not haveVal)) return;


//			reference.configs_=this->configs_.semi_flatten();
			reference.configs_.undo_structure();

//			const bool useTrial=(not (trial2.tensor_type()==TT_NONE));

//			timeReduce_.start();

			/*
			 * some constants
			 */

			// the threshold
			const double threshold=eps*facReduce;

			// what we expect the trial rank might be (engineering problem)
			const unsigned int maxTrialRank=218;
			const unsigned int maxloop=218;

			const bool print=false;
			double norm=1.0;

			const unsigned int config_dim=this->configs_.dim_eff();
			const unsigned int rG1=reference.rank();

			// scratch Tensor for this and the reference
			std::vector<Tensor<T> > B1(config_dim);
			for (unsigned int idim=0; idim<config_dim; idim++) B1[idim]=Tensor<T> (maxTrialRank,rG1);
			// scratch Tensor for this and the residual
			std::vector<Tensor<T> > B2(config_dim);
			for (unsigned int idim=0; idim<config_dim; idim++) B2[idim]=Tensor<T> (maxTrialRank,1);

			// set up a trial function
			SepRep trial(reference.tensor_type(),reference.get_k(),reference.dim());
			trial.configs_.undo_structure();
			trial.configs_.reserve(10);
//			trial.configs_.semi_flatten();
//			if (useTrial) trial=trial2;
//			else trial.configs_.ensureSpace(maxTrialRank);

			// and the residual
			SepRep residual(trial.tensor_type(),trial.get_k(),trial.dim());

			// loop while || F-G || > epsilon
			for (unsigned int iloop=0; iloop<maxloop; iloop++) {

				// compute the residual wrt reference minus trial
				residual.configs_.fillWithRandom(1);
				residual.configs_.undo_structure();

				residual.optimize(reference,facSR,trial,-1.0,values,facVal,threshold,50,B1,B2);

				// exit if residual is supposedly small
				norm=residual.FrobeniusNorm();
				if (print) printf("trial norm in reduceRank %d %12.8f\n", trial.rank(), norm);
#if bench
				if (iloop>5) break;
#else
				if (norm<threshold) break;
#endif
				// otherwise add residual to the trial function ..
//				trial.configs_.unflatten();
//				residual.configs_.unflatten();
				trial.configs_.make_structure();
				residual.configs_.make_structure();
				trial+=residual;
				trial.configs_.undo_structure();
				residual.configs_.undo_structure();
//				trial.configs_.semi_flatten();
//				residual.configs_.semi_flatten();

				// .. and optimize trial wrt the reference
				bool successful=trial.optimize(reference,facSR,residual,0.0,
								values,facVal,threshold,10,B1,B2);

				MADNESS_ASSERT(successful);

			}
			if (print) std::cout << "final trial norm in reduceRank " << trial.rank() << " " << norm  << std::endl;
			if (print) std::cout << "threshold " << threshold << std::endl;

#if !bench
			// check actual convergence
			if (norm>threshold) {
		//		trial.printCoeff("trial");
		//		this->printCoeff("failed SepRep");
				std::cout << "failed to reduce rank in SepRep::reduceRank() " << std::endl;
				printf("initial rank %d, trial rank %d\n",this->rank(), trial.rank());
				printf("residual's norm         %24.16f\n", norm);
				printf("this' condition number  %24.16f\n", this->conditionNumber());
				printf("trial' condition number %24.16f\n", trial.conditionNumber());
				printf("norm(this) %12.8f\n", this->FrobeniusNorm());
				printf("no convergence in SepRep::reduceRank()");
				MADNESS_ASSERT(0);
			}
#endif

			// copy shrinks the matrices
			*this=copy(trial.configs_);
			this->configs_.make_structure();
//			timeReduce_.end();

		}


		/// overlap between two SepReps (Frobenius inner product)
		friend T overlap(const SepRep<T>& rhs, const SepRep<T>& lhs)  {

	        // fast return if possible
	        if ((lhs.rank()==0) or (rhs.rank()==0)) return 0.0;

	        MADNESS_ASSERT(compatible(lhs,rhs));
	        MADNESS_ASSERT(lhs.tensor_type()==rhs.tensor_type());

	        const T ovlp=overlap(lhs.configs_,rhs.configs_);
	        return ovlp;
		}


		/// return the Frobenius norm of this
		double FrobeniusNorm() const {

			// fast return if possible
			if (this->rank()==0) return 0.0;

//			double ovlp=overlap(this->configs_,this->configs_);
			double ovlp=std::abs(overlap(*this,*this));

			// estimate the accuracy one can expect
			const double condition=this->conditionNumber();

			if (sqrt(fabs(ovlp))<condition) {
		//	if (fabs(ovlp)<condition) {
		//		std::cout << "truncating due to condition " << condition << std::endl;
				ovlp=0.0;
			}
			if (ovlp<-condition) {
				std::cout << "norm<0.0 in SepRep::FrobeniusNorm() " << ovlp << std::endl;
				printf(" condition number       :  %24.16f\n half machine precision :  %24.16f\n",
						condition,sqrtMachinePrecision);
			}

			return sqrt(ovlp);
		}

		/// print this' coefficients
		void printCoeff(const std::string title) const {
			print("printing SepRep",title);
			print(configs_.weights_);
			for (unsigned int idim=0; idim<this->configs_.dim_eff(); idim++) {
				print("coefficients for dimension",idim);
				print(configs_.vector_[idim]);
			}
		}

		/// print the weights
//		void printWeights(const std::string) const;

		/// reconstruct this to return a full tensor
		Tensor<T> reconstructTensor() const {

			/*
			 * reconstruct the tensor first to the configurational dimension,
			 * then to the real dimension
			 */


			// for convenience
			const unsigned int conf_dim=this->configs_.dim_eff();
			const unsigned int conf_k=this->kVec();			// possibly k,k*k,..
			const unsigned int rank=this->rank();
			long d[TENSOR_MAXDIM];

			// fast return if possible
			if (rank==0) {
				// reshape the tensor to the really required one
				const long k=this->get_k();
				const long dim=this->dim();
				for (long i=0; i<dim; i++) d[i] = k;

				return Tensor<T> (dim,d,true);
			}

			// set up result Tensor (in configurational dimensions)
			for (long i=0; i<conf_dim; i++) d[i] = conf_k;
			Tensor<T> s(conf_dim,d,true);

			// flatten this
			SepRep<T> sr=*this;
			sr.configs_.undo_structure();
//			sr.configs_.semi_flatten();

			// and a scratch Tensor
			Tensor<T>  scr(rank);
			Tensor<T>  scr1(rank);
			Tensor<T>  scr2(rank);

			if (conf_dim==1) {
				MADNESS_ASSERT(0);

				for (unsigned int i0=0; i0<conf_k; i0++) {
					scr=sr.configs_.weights_;
//					scr.emul(F[0][i0]);
					T buffer=scr.sum();
					s(i0)=buffer;
				}

			} else if (conf_dim==2) {

//				tensorT weight_matrix(rank,rank);
//				for (unsigned int r=0; r<rank; r++) {
//					weight_matrix(r,r)=this->weight(r);
//				}
//				s=inner(weight_matrix,sr.configs_.refVector(0));
//				s=inner(s,sr.configs_.refVector(1),0,0);
				tensorT sscr=copy(sr.configs_.ref_vector(0)(sr.configs_.c0()));
				for (unsigned int r=0; r<rank; r++) {
					const double w=this->weight(r);
					for (unsigned int k=0; k<conf_k; k++) {
						sscr(r,k)*=w;
					}
				}
				inner_result(sscr,sr.configs_.ref_vector(1)(sr.configs_.c0()),0,0,s);

//				for (unsigned int i0=0; i0<conf_k; i0++) {
//					scr=copy(sr.configs_.weights_);
//					scr.emul(sr.configs_.refVector(0)(Slice(_),Slice(i0,i0,0)));
//					for (unsigned int i1=0; i1<conf_k; i1++) {
//						scr1=copy(scr);
//						scr1.emul(sr.configs_.refVector(1)(Slice(_),Slice(i1,i1,0)));
//						s(i0,i1)=scr1.sum();
//					}
//				}


			} else if (conf_dim==3) {

				for (unsigned int i0=0; i0<conf_k; i0++) {
					scr=copy(sr.configs_.weights_);
					scr.emul(sr.configs_.ref_vector(0)(Slice(0,rank-1),Slice(i0,i0,0)));
					for (unsigned int i1=0; i1<conf_k; i1++) {
						scr1=copy(scr);
						scr1.emul(sr.configs_.ref_vector(1)(Slice(0,rank-1),Slice(i1,i1,0)));
						for (unsigned int i2=0; i2<conf_k; i2++) {
							scr2=copy(scr1);
							scr2.emul(sr.configs_.ref_vector(2)(Slice(0,rank-1),Slice(i2,i2,0)));
							s(i0,i1,i2)=scr2.sum();
						}
					}
				}
			} else {
				print("only config_dim=1,2,3 in SepRep::reconstructTensor");
				MADNESS_ASSERT(0);
			}


			// reshape the tensor to the really required one
			const long k=this->get_k();
			const long dim=this->dim();
			for (long i=0; i<dim; i++) d[i] = k;

			Tensor<T> s2=s.reshape(dim,d);

			return s2;
		}

		/// reconstruct this to full rank, and accumulate into t
		void accumulate_into(Tensor<T>& t, const double fac) const {

			// fast return if possible
			if (this->rank()==0) return;

			MADNESS_ASSERT(t.iscontiguous());

			// for convenience
			const unsigned int conf_dim=this->configs_.dim_eff();
			const unsigned int conf_k=this->kVec();			// possibly k,k*k,..
			const unsigned int rank=this->rank();
			long d[TENSOR_MAXDIM];

			// set up result Tensor (in configurational dimensions)
			for (long i=0; i<conf_dim; i++) d[i] = conf_k;
			Tensor<T> s=t.reshape(conf_dim,d);

			// flatten this
			SepRep<T> sr=*this;
			sr.configs_.undo_structure();

			if (conf_dim==2) {

				tensorT sscr=copy(sr.configs_.ref_vector(0)(sr.configs_.c0()));
				if (fac!=1.0) sscr.scale(fac);
				for (unsigned int r=0; r<rank; r++) {
					const double w=this->weight(r);
					for (unsigned int k=0; k<conf_k; k++) {
						sscr(r,k)*=w;
					}
				}
				inner_result(sscr,sr.configs_.ref_vector(1)(sr.configs_.c0()),0,0,s);

				// transpose
//				long dd[2];
//				dd[0]=rank;
//				dd[1]=conf_k;
//				tensorT sscr2(2,dd,false);
//				fast_transpose(rank,conf_k,sscr.ptr(),sscr2.ptr());
//				fast_transpose(rank,conf_k,sr.configs_.refVector(1).ptr(),sscr.ptr());
//				tensorT st=sscr.reshape(conf_k,rank);
//				tensorT s2t=sscr2.reshape(conf_k,rank);
//				inner_result(s2t,st,1,1,s);


			} else {
				print("only config_dim=2 in SepRep::accumulate_into");
				MADNESS_ASSERT(0);
			}

		}

		/// inplace add
		void update_by(const SepRep<T>& rhs) {
			if (rhs.rank()==1) configs_.rank1_update_slow(rhs.configs_,1.0);
			else {
				MADNESS_EXCEPTION("rhs rank != 1",0);
			}
		}

		void finalize_accumulate() {configs_.finalize_accumulate();}

		/// check compatibility
		friend bool compatible(const SepRep& rhs, const SepRep& lhs) {
	//		return ((rhs.rep_==lhs.rep_) and (rhs.maxk_==lhs.maxk_) and (rhs.dim()==lhs.dim()));
			return ((rhs.tensor_type()==lhs.tensor_type()) and (rhs.get_k()==lhs.get_k()) and (rhs.dim()==lhs.dim()));
		};

		/// check consistency of this
		bool consistent() const {
			print("no SepRep.h::consistent()");
			MADNESS_ASSERT(0);
			return true;
		}

		/// compute a best one-term approximation wrt to this
		SepRep oneTermApprox(const double& eps, std::vector<Tensor<T> >& B1) const {

			/*
			 * return a SepRep that represents this with only one term
			 */

			// new random SepRep of rank 1 and optimize wrt this
			SepRep residual(this->tensor_type(),this->get_k(),this->dim());
			SepRep dummy(residual);

			residual.configs_.fillWithRandom();

			Tensor<T> t1;
			Tensor<T> B2;
//			std::vector<Tensor<T> > B2(B1);

			// optimize the new SepRep wrt the residual; *this is the residual
			// maxloop can be large because we have an additional stopping criterion
			const unsigned int maxloop=50;
			bool successful=residual.optimize(*this,1.0,dummy,0.0,t1,0.0,eps,maxloop,B1,B2);
			if (not successful) {
				std::cout << "NaNs in oneTermApprox " << std::endl;
				MADNESS_ASSERT(0);
			}
			return residual;
		}

		/// cast this into a SepRep with different maxk
//		SepRep cast(const unsigned int maxk) const;

		/// transform the Legendre coefficients with the tensor
		SepRep<T> transform(const Tensor<T> c) const {
			return SepRep<T> (this->configs_.transform(c));
		}

		/// transform the Legendre coefficients with the tensor
		SepRep<T> general_transform(const Tensor<T> c[]) const {
			return SepRep<T> (this->configs_.general_transform(c));
		}

		/// inner product
		SepRep<T> transform_dir(const Tensor<T>& c, const int& axis) const {

			return this->configs_.transform_dir(c,axis);
		}

	private:

		/// optimize this wrt reference, and return the error norm
		bool optimize(const SepRep& ref1, const double& fac1,
				const SepRep& ref2, const double fac2,
				const Tensor<T> & ref3, const double fac3,
				const double& eps, const unsigned int& maxloop,
				std::vector<Tensor<T> >& B1, std::vector<Tensor<T> >& B2) {


			// for convenience
			const unsigned int config_dim=this->configs_.dim_eff();
			const unsigned int rF=this->rank();
			const unsigned int rG1=ref1.rank();
			const unsigned int rG2=ref2.rank();

			// some checks
			if (fac1!=0) MADNESS_ASSERT(compatible(*this,ref1));
			if (fac2!=0) MADNESS_ASSERT(compatible(ref1,ref2));

			double oldnorm=1.0;

			// reshape the scratch Tensor
			for (unsigned int idim=0; idim<config_dim; idim++) {
//				B1[idim].reshape(rF,rG1);
//				B2[idim].reshape(rF,rG2);
				B1[idim]=Tensor<T>(rF,rG1);
				B2[idim]=Tensor<T>(rF,rG2);				// 0.2 sec
			}

			// keep optimizing until either the norm doesn't change
			// or we hit some max number of runs
			unsigned int iloop=0;
			for (iloop=0; iloop<maxloop; iloop++) {

				// optimize once for all dimensions
				try {
					this->generalizedALS(ref1,fac1,ref2,fac2,ref3,fac3,B1,B2);
				} catch (std::runtime_error) {
					throw std::runtime_error("rank reduction failed");
					return false;
				}

				/*
				 * for residuals: also exit if norm vanishes, or if
				 * norm doesn't change any more
				 */
		//		const double norm=fabs(this->configs_.weights(this->rank()-1));
				const double norm=this->FrobeniusNorm();
				if (iloop>1) {
					const double ratio=oldnorm/norm;
		//			std::cout << "  ratio " << ratio << " norm " << norm << std::endl;
					if (fabs(ratio-1.0)<0.003) break;
				}
				oldnorm=norm;

			}
			return true;
		}

		/// perform the alternating least squares algorithm directly on function values,
		/// minus the difference SR
		void generalizedALS(const SepRep& ref1, const double& fac1,
				const SepRep& ref2, const double& fac2,
				const Tensor<T> & ref3, const double& fac3,
				std::vector<Tensor<T> >& B1, std::vector<Tensor<T> >& B2) {

			// for convenience
			const unsigned int dim=this->configs_.dim_eff();
			const unsigned int kvec=this->kVec();
			SepRep& trial=*this;

			const bool have1=(fac1!=0.0 and ref1.rank()>0);
			const bool have2=(fac2!=0.0 and ref2.rank()>0);
			const bool have3=(fac3!=0.0);

			/*
			 * rF is the rank of the trial SepRep
			 * rG1 is the rank of the SepRep ref1
			 * rG2 is the rank of the SepRep ref2
			 */
			const unsigned int rF=trial.rank();
			const unsigned int rG1=ref1.rank();
			const unsigned int rG2=ref2.rank();

			// some checks
			MADNESS_ASSERT(ref1.tensor_type()==this->tensor_type());
			if (have2) MADNESS_ASSERT(ref2.tensor_type()==this->tensor_type());
			if (have2) MADNESS_ASSERT(compatible(ref1,ref2));
			MADNESS_ASSERT(dim==B1.size());
			MADNESS_ASSERT(dim==B2.size());
			MADNESS_ASSERT(rG1>=0);
			MADNESS_ASSERT(rG2>=0);
			MADNESS_ASSERT(rF>0);
			MADNESS_ASSERT(B1[0].dim(0)==rF);
			MADNESS_ASSERT(B1[0].dim(1)==rG1);
			MADNESS_ASSERT(B2[0].dim(0)==rF);
			MADNESS_ASSERT(B2[0].dim(1)==rG2);

			// for controlling the condition number, sec. (3.2) of BM2005
			const double alpha=machinePrecision;
			Tensor<T> unity(trial.rank(),trial.rank());
			for (unsigned int i=0; i<trial.rank(); i++) {
				unity(i,i)=alpha;
			}

			// some scratch Tensors
			Tensor<T>  fvec(kvec);
			Tensor<T>  vecb(rF,kvec);
//			Tensor<T>  vecb(kvec,rF);
			// no copy ctor here for B, since it is shallow!
//			std::vector<Tensor<T> > B(dim,Tensor<T> (rF,rF));
			std::vector<Tensor<T> > B(dim);
			for (unsigned int idim=0; idim<dim; idim++) B[idim]=Tensor<T> (rF,rF);


			/*
			 * first make all factors of the two B matrices of eq. (3.3) BM2005,
			 * 	- B is the overlap <F | F> for each dimension
			 * 	- B_GF is the overlap <F | G> for each dimension
			 * 	- as B[idim] and B_GF[idim] is not required, use it as scratch for constructing
			 * 		the product \Prod_{dim\idim} <F | G>
			 * 	- include the weights in B_GF[idim], and construct the vectors b as
			 * 		b(l',k) = B_GF(l',l) * F(l,k)
			 */

			// leave out idim=0, since it not required in the first alteration
			for (unsigned int idim=1; idim<dim; idim++) {
				if (have1) makeB(B1[idim],idim,trial.configs_,ref1.configs_);
				if (have2) makeB(B2[idim],idim,trial.configs_,ref2.configs_);
				makeB(B[idim],idim,trial.configs_,trial.configs_);
				B[idim]+=unity;
//				print("B[idim]");
//				print(B[idim]);
			}

			// next loop over all dimensions
			for (unsigned int idim=0; idim<dim; idim++) {

				// reconstruct B and B_GF for the dimension that has been
				// altered before, include the unit matrix
				if (idim>0) {
					if (have1) makeB(B1[idim-1],idim-1,trial.configs_,ref1.configs_);
					if (have2) makeB(B2[idim-1],idim-1,trial.configs_,ref2.configs_);
					makeB(B[idim-1],idim-1,trial.configs_,trial.configs_);
					B[idim-1]+=unity;

				}

				// construct the products of the B's and B_GF's
				B1[idim]=1.0;
				B2[idim]=1.0;
				B[idim]=1.0;
				for (unsigned int jdim=0; jdim<dim; jdim++) {
					if (jdim!=idim) {
						if (have1) B1[idim].emul(B1[jdim]);
						if (have2) B2[idim].emul(B2[jdim]);
						B[idim].emul(B[jdim]);
					}
				}


				/*
				 * now construct the b vector of eq (3.4) BM2005
				 */
				vecb.fill(0.0);

				// bring the quantity \prod_(i/=k) < G|F > in some efficient form
				// it is independent of jk, and include the weights
				if (have1) {
					for (unsigned int l=0; l<rG1; l++) {
						const double w=ref1.configs_.weights(l);
						for (unsigned int l1=0; l1<rF; l1++) {
							B1[idim](l1,l)*=w*fac1;
						}
					}
					Tensor<T> tmp=ref1.configs_.ref_vector(idim)(ref1.configs_.c0());
					vecb+=madness::inner(B1[idim],tmp);
				}
				if (have2) {
					for (unsigned int l=0; l<rG2; l++) {
						const double w=ref2.configs_.weights(l);
						for (unsigned int l1=0; l1<rF; l1++) {
							B2[idim](l1,l)*=w*fac2;
						}
					}
					Tensor<T>  tmp=ref2.configs_.ref_vector(idim)(ref2.configs_.c0());
					vecb+=madness::inner(B2[idim],tmp);
				}

				/*
				 * now construct the b(r,k) vector for the Tensor values
				 */
				if (have3) {

					MADNESS_ASSERT((dim==3) or (dim==2));
					if (dim==3) {

						// b[jk][rF] += inner( s[jk][k1,k2] , [rF][\prod[k1,k2])

						// reorder s[k1,k2,k3] to s[jk][k1,k2]
						// need to cycle backwards..
						const Tensor<T> weights=copy(ref3.cycledim(dim-idim,0,dim-1));
						const Tensor<T> w=weights.reshape(kvec,kvec*kvec);

						const unsigned int idim0=(idim+1)%dim;
						const unsigned int idim1=(idim+2)%dim;

						// set up \Prod_{i\neq k} <G | F>
						Tensor<T> prod(rF,kvec,kvec);
						for (unsigned int r=0; r<rF; r++) {
							for (unsigned int i0=0; i0<kvec; i0++) {
								const T F1=trial.configs_.vector_[idim0](r,i0);
								for (unsigned int i1=0; i1<kvec; i1++) {
									const T F2=trial.configs_.vector_[idim1](r,i1);
									prod(r,i0,i1)=F1*F2;
								}
							}
						}
						const Tensor<T> p=prod.reshape(rF,kvec*kvec);

						// compute the contrib to b
						vecb+=madness::inner(p,w,-1,-1);

					} else if (dim==2) {

						// b[jk][rF] += inner( s[jk][k1,k2] , [rF][\prod[k1,k2])

						// reorder s[k1,k2,k3] to s[jk][k1,k2]
						// need to cycle backwards..
						const Tensor<T> weights=copy(ref3.cycledim(dim-idim,0,dim-1));
						const Tensor<T> w=weights.reshape(kvec,kvec);

						const unsigned int idim0=(idim+1)%dim;

						// set up \Prod_{i\neq k} <G | F>
						Tensor<T> prod(rF,kvec);
						for (unsigned int r=0; r<rF; r++) {
							for (unsigned int i0=0; i0<kvec; i0++) {
								const T F1=trial.configs_.vector_[idim0](r,i0);
								prod(r,i0)=F1;
							}
						}
						const Tensor<T> p=prod.reshape(rF,kvec);

						// compute the contrib to b
						vecb+=madness::inner(p,w,-1,-1);
					}

				}


				// solve the linear system
				// note that gesv requires: vecb(kvec,rF) -> vecb(rF,kvec)p;
				// x can be empty for now
				Tensor<T> x;

				try {
#if !bench
					gesv(B[idim],vecb,x);
#else
					x=vecb;
					x=1.0;
#endif

				} catch (std::exception) {
					print("gesv failed..");
					print(B);
					print(vecb);
					MADNESS_ASSERT(0);
				}

				vecb=x;

				for (unsigned int l=0; l<rF; l++) {

					// calculate the new weights s_
					typename madness::Tensor<T>::float_scalar_type norm=0.0;
					for (unsigned int jk=0; jk<kvec; jk++) {
						const T val=vecb(l,jk);
						norm+= madness::detail::mynorm(val);
//						print("norm in ALS", norm);
					}

					/*
					 * check for NaNs somewhere
					 */
					if (not (norm==norm)) {
						std::cout << "NaNs in ALS" << std::endl;
						print("B[idim]",B[idim]);
		//				vecbb.print("old vecb");
						std::cout << "idim " << idim << std::endl;
						std::cout << "weight_l in NaN; norm: " << norm << std::endl;
						throw std::runtime_error("NaNs in ALS");
						MADNESS_ASSERT(0);
					}
					MADNESS_ASSERT(norm>=0.0);

					/*
					 * if trial is a residual the weights might be zero;
					 * fill the vectors F with random numbers, not with zeros,
					 * otherwise they will screw up the ALS algorithm in the next
					 * iteration!
					 */
					double weight_l=sqrt(norm);
//					print("weight in ALS", weight_l);
					if (norm==0.0) {
						weight_l=0.0;
						fvec.fillrandom();
						fvec=1.0;
						fvec.scale(0.01);
					} else {
						for (unsigned int jk=0; jk<kvec; jk++) {
							const T c_jk_l=vecb(l,jk);
							fvec(jk)=c_jk_l/weight_l;
						}
					}
//					print("fvec in ALS", fvec);

					// use this->kVec(), as this might be an SVR vector or
					// an SPR operator
					trial.configs_.reassign(idim,l,weight_l,fvec,ref1.kVec());
				}
			}
		}


	};


}



#endif /* SEPREP_H_ */