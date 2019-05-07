/*
 * MixtureUnivariateNormal.hpp
 *
 *  Created on: Mar 8, 2019
 *      Author: toky
 */

#ifndef PROBITFMMNEW_SRC_MIXTUREUNIVARIATENORMAL_HPP_
#define PROBITFMMNEW_SRC_MIXTUREUNIVARIATENORMAL_HPP_

#include <RcppArmadillo.h>
#include "Mixture.hpp"

class Mixture_UnivariateNormal: public UnivariateMixture  {

	// Parametric Prior
	double _m0, _k0, _nu0, _sig02;

	//Tau
	std::vector <double> _mu_current;
	std::vector <double> _sig2_current ;

public :
	Mixture_UnivariateNormal (const double m0, const double k0, const double nu0, const double sig02) : _m0 (m0), _k0 (k0), _nu0 (nu0), _sig02  (sig02){}

	virtual void init_tau (const input_t & y, const int M) {
		 _mu_current.resize(M);
		 _sig2_current.resize(M);

		 const double nu0   = _nu0;
		 const double m0    = _m0;
		 const double k0    = _k0;
		 const double sig02 = _sig02;

		 const double scale_loc=std::pow(nu0/2*sig02,-1.0);

		 for(int l=0;l<M;l++){
			 _sig2_current[l] = pow(R::rgamma(nu0/2,scale_loc),-1);
			 _mu_current[l]   = R::rnorm(m0, pow(_sig2_current[l]/k0,0.5));
		 }

	}

	virtual cluster_indices_t  up_ci(const  input_t & y,
			const long M,
			const Rcpp::NumericVector & S_current) {
		const int n = y.size();

		const std::vector<double>& mu_current = _mu_current;
		const std::vector<double>& sig2_current = _sig2_current;

		cluster_indices_t ci_current(n);
		Rcpp::NumericVector Log_S_current = log(S_current);
		std::vector<double> pow_sig2_current (M);
		std::vector<double> log_pow_sig2_current (M);

		Rcpp::NumericVector random_u   = Rcpp::runif(n,0.0,1.0 );


		for(int l=0;l<M;l++){
			pow_sig2_current[l]     = pow(sig2_current[l],0.5);
			log_pow_sig2_current[l] = log(pow_sig2_current[l]);
		}


		omp_set_dynamic(0);
	    #pragma omp parallel for num_threads(4)
		for (int i=0; i < n; i++) {

			arma::vec pesi(M);
			double max_lpesi=-INFINITY;

			for(int l=0;l<M;l++){

				// Speed-up 30% using homemade dnorm
				 //double ldensi = R::dnorm(y[i],mu_current[l],pow_sig2_current[l],true);
				 const double x       = fabs ( (y[i] - mu_current[l]) / pow_sig2_current[l] ) ;
				 const double ldensi  = -(M_LN_SQRT_2PI + 0.5 * x * x + log_pow_sig2_current[l]);

				 pesi[l]=Log_S_current[l]+ldensi;
				 if(max_lpesi<pesi[l]){max_lpesi=pesi[l];}
			}

			// I put the weights in natural scale and re-normalize then
			pesi = exp(pesi - max_lpesi);
			pesi = pesi / sum(pesi);

			const double u = random_u[i];
			double cdf = 0.0;
			unsigned int ii = 0;
			while (u >= cdf) { // This loop assumes (correctly) that R::runif(0,1) never return 1.
				cdf += pesi[ii++];
			}
			ci_current[i] = ii;
		}

		return  ci_current ;
	}

	 virtual allocation_result up_allocated_nonallocated (
			const int K ,
			const int M ,
			const cluster_indices_t & ci_current ,
			const cluster_indices_t & ci_star  ,
			const double gamma_current,
			const double U_current,
			const  input_t & y ) {

			const long   n     = y.size();
			const double nu0   = _nu0;
			const double k0    = _k0;
			const double m0    = _m0;
			const double sig02 = _sig02;


			//Allocation_result output ;
			std::vector<double> sig2_current(M);
			std::vector<double> mu_current(M);

			Rcpp::NumericVector S_current    = Rcpp::NumericVector(M);

			cluster_indices_t ci_reorder(y.size());
			ci_reorder.fill(-1);
			std::vector<int>    nj(K);
			std::map< int, std::vector<int> > clusters_indices;

			for(int i=0;i<n;i++){
				clusters_indices[ci_current[i]].push_back(i);
			}

			for(int local_index=0;local_index<K;local_index++){
				const int key = ci_star[local_index];
				nj[local_index] = clusters_indices[key].size();
				for (auto v : clusters_indices[key]) {
					ci_reorder[v]=local_index;
				}
			}

			for(int l=0; l<K;l++){
				// Find the index of the data in the l-th cluster
				std::vector<int> & which_ind=clusters_indices [ci_star[l]];

				//Prepare the variable that will contain the data in the cluster
				arma::colvec y_l = arma::colvec(nj[l]);
				//Separate the data in each cluster and rename the cluster
				for(int it=0;it<nj[l];it++){
					y_l[it]=y[which_ind[it]];

				}

				// Call the parametric function that update the
				// parameter in each cluster
				// See the boxes 4.c.i and 4.b.i of
				// Figure 1 in the paper

				const int njl = y_l.n_elem; // This is the number of data in the cluster. I hope so


				//Since in our case the full conditionals are in closed form
				//they are Normal-inverse-gamma



				//Firs compute the posterior parameters

				const double ysum= arma::sum(y_l);

				//double ysum= arma::accu(y); //is there any difference between sun and accu un Armadillo?

				const double ybar=ysum/((double) njl); // cast?
				const double s2= arma::var(y_l);
				// Then the parameters of the posteriorr  Normal-inverse-gamma a posteriori are
				const double kn = k0+  (double) njl; // maybe the cast at double in the sum is not needed
				const double mn = (ysum+m0*k0)/kn;
				const double nun = nu0+(double) njl;  // maybe the cast at  double in the sum is not needed

				const double s2n=1/nun*(nu0*sig02+(njl-1)*s2+k0*njl/kn* std::pow(ybar-m0,2.0));

				const double alphan = nun/2.0;
				const double scalen  = 2.0/(nun*s2n);


				const double sig2_l = pow(R::rgamma(alphan,scalen),-1.0);
				const double mu_l  = R::rnorm(mn, pow(sig2_l/kn,0.5));

				sig2_current[l]=sig2_l; // In case 4 we have to update a matrix
				mu_current[l]=mu_l;

				// TODO[OPTIMIZE ME] : Cannot split or the random value are completly different
				// Update the Jumps of the allocated part of the process
				S_current[l]=R::rgamma(nj[l]+gamma_current,1./(U_current+1.0));

			}

			// Fill non-allocated

			const double alpha_loc=nu0/2;
			const double scale_loc=std::pow(nu0/2*sig02,-1.0);

			for(int l=K; l<M;l++){
				sig2_current[l] = pow(R::rgamma(alpha_loc,scale_loc),-1);
				mu_current[l] =	R::rnorm(m0,pow(sig2_current[l]/k0,0.5));
				S_current[l]=R::rgamma(gamma_current,1./(U_current+1.0));
			}



			_mu_current = mu_current;
			_sig2_current = sig2_current;

			return allocation_result(ci_reorder , nj , S_current);


	 }

};



#endif /* PROBITFMMNEW_SRC_MIXTUREUNIVARIATENORMAL_HPP_ */