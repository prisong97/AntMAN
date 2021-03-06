/*
 *  AntMAN Package
 *  Unit test
 *
 */



#define BOOST_TEST_MODULE UnivariateNormalTest

#include <boost/test/included/unit_test.hpp>
#include <PriorPoisson.h>
#include <MixtureUnivariateNormal.h>

void test_Mixture_UnivariateNormal(long niter, long burnin, long thin) {


	static const arma::vec y_uvn =  { 9.172,  9.350,  9.483,  9.558,  9.775, 10.227, 10.406, 16.084, 16.170, 18.419,18.552,
			18.600, 18.927, 19.052, 19.070, 19.330, 19.343, 19.349, 19.440, 19.473,19.529, 19.541, 19.547, 19.663,
			19.846, 19.856, 19.863, 19.914, 19.918, 19.973,19.989, 20.166, 20.175, 20.179, 20.196, 20.215, 20.221,
			20.415, 20.629, 20.795,20.821, 20.846, 20.875, 20.986, 21.137, 21.492, 21.701, 21.814, 21.921, 21.960,
			22.185, 22.209, 22.242, 22.249, 22.314, 22.374, 22.495, 22.746, 22.747, 22.888,22.914, 23.206, 23.241,
			23.263, 23.484, 23.538, 23.542, 23.666, 23.706, 23.711,24.129, 24.285, 24.289, 24.366, 24.717, 24.990, 25.633, 26.960,
			26.995, 32.065,32.789, 34.279 };
	// TODO : Need to go over variable and see what should be checked
	PriorPoisson *prior = new PriorPoisson(poisson_gamma_h_param_t(2,1,1,0.00001),poisson_gamma_q_param_t(3,1,1));
	MixtureUnivariateNormal * mixture = new MixtureUnivariateNormal (20.83146, 0.3333333, 4.222222, 3.661027);
	cluster_indices_t initial_clustering (y_uvn.size());
	AntMANLogger * logger = new AntMANLogger(std::vector<std::string>(), (niter - burnin) / thin );

	auto start_gibbs           = std::chrono::system_clock::now();
	mixture->fit(y_uvn , initial_clustering, false, prior , niter ,burnin ,thin , false , logger);
	auto end_gibbs             = std::chrono::system_clock::now();
	auto elapsed_gibbs         = end_gibbs - start_gibbs;
	auto total_gibbs           = elapsed_gibbs.count() / 1000000.0;
	COUT_STREAM << "Total time: " << total_gibbs << "ms"  << std::endl ;


}

BOOST_AUTO_TEST_SUITE( test_suite_univariate_normal_dataset )

BOOST_AUTO_TEST_CASE( test_univariate_normal_100_10_10 )
{
	test_Mixture_UnivariateNormal(100, 10, 10);
}


BOOST_AUTO_TEST_SUITE_END()




