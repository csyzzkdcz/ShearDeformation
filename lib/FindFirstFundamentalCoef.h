#ifndef FindFirstFundamentalCoef_h
#define FindFirstFundamentalCoef_h
#include <Eigen/Sparse>
#include <Eigen/Dense>
#include "external/alglib/stdafx.h"
#include "external/alglib/optimization.h"

class FindFirstFundamentalCoef
{
public:
//    FindFirstFundamentalCoef(): _is_initialized(false),ID_list(0,Eigen::Matrix2d()),
//    IID_list(0,Eigen::Matrix2d()), dID_index(0,std::vector<int>(0,0)),dIID_index(0,std::vector<int>(0,0)){}
    FindFirstFundamentalCoef(): _is_initialized(false){}
    ~FindFirstFundamentalCoef(){}

public:
    void get_func_grad(const alglib::real_1d_array &x, double &f, alglib::real_1d_array &df);
    void set_up(Eigen::MatrixXd VD, Eigen::MatrixXi F0, double YonungsModulus, double PoissonRatio, double thickness);
    void compute_first_fundamental_form(Eigen::MatrixXd VD, Eigen::MatrixXi F0, Eigen::VectorXd &sol, double YoungsModulus, double PoissonRatio, double thickness);

    void test_func_grad();

private:
    void compute_derivative_inv_mat(Eigen::Matrix2d A, std::vector<Eigen::Matrix2d > &dA); // Compute the derivative of A*A^T
    void compute_derivative_sqrt_det(Eigen::Matrix2d A, std::vector<double> &diff_sqrt_det); // Compute the derivative of A*A^T


private:
    bool _is_initialized;
    int _itr_times;
    std::vector<std::vector<int> > VF;  // list of lists of incident faces (adjacency list)
    std::vector<std::vector<int> > VFi; // list of lists of index of incidence within incident faces listed in VF
    Eigen::MatrixXi TT;             // The matrix which stores the information of adjacent faces
    Eigen::MatrixXi TTi;
    Eigen::MatrixXd V;
    Eigen::MatrixXi F;
    
    std::vector<Eigen::Matrix2d> ID_list;
    std::vector<Eigen::Matrix2d> IID_list;
//
    std::vector<std::vector<Eigen::Matrix2d > > dID_list; // The list to store the derivative of the first fundamental form. The order is determined by VF
    std::vector<std::vector<Eigen::Matrix2d > > dIID_list_neighbor;// The list to store the derivative of the first fundamental form. The order is determined by VF
    std::vector<std::vector<Eigen::Matrix2d > > dIID_list_2_neighbor; // The list to store the derivative of the first fundamental form. The order is determined by TT
    
    double _PoissonsRatio;
    double _YoungsModulus;
    double _thickness;
    double _alpha;
    double _beta;

};

#endif
