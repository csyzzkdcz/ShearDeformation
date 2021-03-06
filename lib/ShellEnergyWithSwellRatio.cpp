//
// Created by Zhen Chen on 10/3/18.
//
#include <unsupported/Eigen/MatrixFunctions>
#include <igl/readOBJ.h>
#include <iostream>
#include <iomanip>
//#include <boost/math/special_functions/jacobi_elliptic.hpp>
#include "elliptic_functions.h"
#include "external/alglib/stdafx.h"
#include "external/alglib/specialfunctions.h"
#include "ShellEnergyWithSwellRatio.h"
#include "GeometryFeature.h"
#include "MeshConnection.h"
#include "ComputeCoefficient.h"

void ShellEnergyWithSwellRatio::streching_energy(Eigen::MatrixXd V, Eigen::MatrixXd V0, Eigen::MatrixXi F, double YoungsModulus, double PossionRatio, double thickness, double &E, Eigen::VectorXd &dE)
{
    double alpha = YoungsModulus*PossionRatio/((1+PossionRatio)*(1-PossionRatio));
    double beta = YoungsModulus/(2.0*(1+PossionRatio));
    E = 0;
    std::vector<std::vector<int> > VF;  // list of lists of incident faces (adjacency list)
    std::vector<std::vector<int> > VFi; // list of lists of index of incidence within incident faces listed in VF
    std::vector<Eigen::Matrix2d> IU_list; // The list of the first fundamental form of the original surface
    std::vector<Eigen::Matrix2d> ID_list;// The list of the first fundamental form of the current surface
    std::vector<double> dA_list;
    // get the information of vertex-triangle adjacency
    MeshConnection::vertex_triangle_adjacency(V0.rows(), F, VF, VFi);
    
    // Calculate the energy
    for(int i=0;i<F.rows();i++)
    {
        Eigen::Matrix2d I_D, I_U;
        GeoFeature::calculate_first_fundamental_form(V.row(F(i,0)),V.row(F(i,1)),V.row(F(i,2)),I_D);
        GeoFeature::calculate_first_fundamental_form(V0.row(F(i,0)),V0.row(F(i,1)),V0.row(F(i,2)),I_U);
        //Eigen::Vector3d barycenter = (V0.row(F(i,0)) + V0.row(F(i,1)) + V0.row(F(i,2)))/3.0;
        //double omega = compute_omega(0.025, barycenter(0), barycenter(1), 1.0/sqrt(2));
        I_U = ((1-_ratio) + _ratio*_omega_list(i)) * I_U;
        double dA = sqrt(I_U.determinant())/2.0; // 1/2 is the area of the parametric space
        dA_list.push_back(dA);
        IU_list.push_back(I_U);
        ID_list.push_back(I_D);
        
        Eigen::Matrix2d Q = I_U.inverse()*(I_D-I_U);
        E+=1.0/4.0 * thickness * dA * (alpha*0.5*(Q.trace()*Q.trace())+beta*(Q*Q).trace());
    }
    // Calculate the gradient
    dE.resize(3*V.rows());
    dE.setZero();
    for (int i=0;i<V.rows();i++)
    {
        
        for (int j = 0; j < VF[i].size(); j++) // Consider the faces which contains the vertex
        {
            Eigen::Matrix2d Q;
            std::vector<Eigen::Matrix2d> dI, dQ;
            int f = VF[i][j];
            int fi = VFi[i][j];
            GeoFeature::diff_first_fundamental_form(V.row(F(f, 0)), V.row(F(f, 1)), V.row(F(f, 2)), fi, dI);
            Q = IU_list[f].inverse() * (ID_list[f] - IU_list[f]);
            dQ.resize(3);
            for (int k = 0; k < 3; k++)
            {
                dQ[k] = IU_list[f].inverse() * dI[k];
                dE(3 * i + k) = dE(3 * i + k) + 1.0 / 2.0 * thickness * dA_list[f] *
                (0.5 * alpha * Q.trace() * dQ[k].trace() +
                 beta * (Q * dQ[k]).trace());
            }
        }
    }
    // std::cout<<dE.norm()<<std::endl;
}



void ShellEnergyWithSwellRatio::bending_energy(Eigen::MatrixXd V, Eigen::MatrixXd V0, Eigen::MatrixXi F, double YoungsModulus, double PossionRatio, double thickness, double &E, Eigen::VectorXd &dE)
{
    double alpha = YoungsModulus*PossionRatio/((1+PossionRatio)*(1-PossionRatio));
    double beta = YoungsModulus/(2.0*(1+PossionRatio));
    E = 0;
    std::vector<std::vector<int> > VF;  // list of lists of incident faces (adjacency list)
    std::vector<std::vector<int> > VFi; // list of lists of index of incidence within incident faces listed in VF
    Eigen::MatrixXi TT;             // The matrix which stores the information of adjacent faces
    Eigen::MatrixXi TTi;
    std::vector<Eigen::Matrix2d> IU_list; // The list of the first fundamental form of the original surface
    std::vector<Eigen::Matrix2d> IIU_list; // The list of the first fundamental form of the current surface
    std::vector<Eigen::Matrix2d> IID_list;// The list of the second fundamental form of the current surface
    std::vector<double> dA_list;
    std::vector<int> index(3);
    std::vector<Eigen::Vector3d> adjacent_points(3);
    std::vector<Eigen::Vector3d> adjacent_points_U(3);
    std::vector<bool> real_pts(3);
    
    MeshConnection::vertex_triangle_adjacency(V.rows(), F, VF, VFi);
    MeshConnection::triangle_triangle_adjacency(F, TT, TTi);
    
    // Calculate the energy
    for(int i=0;i<F.rows();i++)
    {
        Eigen::Matrix2d I_U, II_U, II_D;
        GeoFeature::calculate_first_fundamental_form(V0.row(F(i,0)),V0.row(F(i,1)),V0.row(F(i,2)),I_U);
        // Eigen::Vector3d barycenter = (V0.row(F(i,0)) + V0.row(F(i,1)) + V0.row(F(i,2)))/3.0;
        // double omega = compute_omega(0.025, barycenter(0), barycenter(1), 1.0/sqrt(2));
        I_U = ((1-_ratio) + _ratio*_omega_list(i)) * I_U;
        double dA = sqrt(I_U.determinant())/2.0; // 1/2 is the area of the parametric space
        dA_list.push_back(dA);
        IU_list.push_back(I_U);
        for(int k=0;k<3;k++)
        {
            if(TT(i,k)!=-1)
            {
                index[(k+2)%3] = F( TT(i,k), (TTi(i,k)+2)%3 );
                adjacent_points[(k+2)%3] = V.row(index[(k+2)%3]);
                adjacent_points_U[(k+2)%3] = V0.row(index[(k+2)%3]);
                real_pts[(k+2)%3] = true;
            }
            else
            {
                index[(k+2)%3] = -1; // Boundary edge
                adjacent_points[(k+2)%3] << -1,-1,-1;
                adjacent_points_U[(k+2)%3] << -1,-1,-1;
                real_pts[(k+2)%3] = false;
            }
        }
        
        GeoFeature::calculate_second_fundamental_form(V.row(F(i,0)), V.row(F(i,1)), V.row(F(i,2)), adjacent_points[0], adjacent_points[1], adjacent_points[2], real_pts, II_D);
        GeoFeature::calculate_second_fundamental_form(V0.row(F(i,0)), V0.row(F(i,1)), V0.row(F(i,2)), adjacent_points_U[0], adjacent_points_U[1], adjacent_points_U[2], real_pts, II_U);
        
        IIU_list.push_back(II_U);
        IID_list.push_back(II_D);
        
        Eigen::Matrix2d Q = I_U.inverse()*(II_D-II_U);
        
        E+=1.0/12.0 * pow(thickness,3) * dA * (alpha*0.5*(Q.trace()*Q.trace())+beta*(Q*Q).trace());
    }
    
    // Calculate the gradient
    dE.resize(3*V.rows());
    dE.setZero();
    for(int i=0;i<V.rows();i++)
    {
        for(int j=0;j<VF[i].size();j++) // Consider the faces which contains the vertex
        {
            Eigen::Matrix2d Q;
            std::vector<Eigen::Matrix2d> dII,dQ;
            int f = VF[i][j];
            int fi = VFi[i][j];
            for(int k=0;k<3;k++)
            {
                if(TT(f,k)!=-1)
                {
                    index[(k+2)%3] = F( TT(f,k), (TTi(f,k)+2)%3 );
                    adjacent_points[(k+2)%3] = V.row(index[(k+2)%3]);
                    real_pts[(k+2)%3] = true;
                }
                else
                {
                    index[(k+2)%3] = -1; // Boundary edge
                    adjacent_points[(k+2)%3] << -1,-1,-1;
                    real_pts[(k+2)%3] = false;
                }
            }
            GeoFeature::diff_second_fundamental_form(V.row(F(f, 0)), V.row(F(f, 1)), V.row(F(f, 2)), adjacent_points[0], adjacent_points[1], adjacent_points[2], fi, real_pts, dII);
            Q = IU_list[f].inverse()*(IID_list[f]-IIU_list[f]);
            dQ.resize(3);
            for(int k=0;k<3;k++)
            {
                dQ[k] = IU_list[f].inverse()*dII[k];
                dE(3 * i + k) = dE(3 * i + k) + 1.0 / 6.0 * pow(thickness,3.0) * dA_list[f] *
                (0.5 * alpha * Q.trace() * dQ[k].trace() +
                 beta * (Q * dQ[k]).trace());
            }
            
            if(TT(f,(fi+1)%3)!=-1) // Doesn't belong to the boundary
            {
                int fa = TT(f,(fi+1)%3);
                // We need to consider the gradient of II_{fa} w.r.t V
                int start_ind = (TTi(f,(fi+1)%3) + 2) % 3 + 3;
                for(int k=0;k<3;k++)
                {
                    if(TT(fa,k)!=-1)
                    {
                        index[(k+2)%3] = F( TT(fa,k), (TTi(fa,k)+2)%3 );
                        adjacent_points[(k+2)%3] = V.row(index[(k+2)%3]);
                        real_pts[(k+2)%3] = true;
                    }
                    else
                    {
                        index[(k+2)%3] = -1; // Boundary edge
                        adjacent_points[(k+2)%3] << -1,-1,-1;
                        real_pts[(k+2)%3] = false;
                    }
                }
                GeoFeature::diff_second_fundamental_form(V.row(F(fa, 0)), V.row(F(fa, 1)), V.row(F(fa, 2)), adjacent_points[0], adjacent_points[1], adjacent_points[2], start_ind, real_pts, dII);
                Q = IU_list[fa].inverse()*(IID_list[fa]-IIU_list[fa]);
                dQ.resize(3);
                for(int k=0;k<3;k++)
                {
                    dQ[k] = IU_list[fa].inverse()*dII[k];
                    dE(3 * i + k) = dE(3 * i + k) + 1.0 / 6.0 * pow(thickness,3.0) * dA_list[fa] *
                    (0.5 * alpha * Q.trace() * dQ[k].trace() +
                     beta * (Q * dQ[k]).trace());
                }
            }
        }
    }
}

double ShellEnergyWithSwellRatio::compute_omega(double radius, double x, double y, double modulus)
{
//    std::complex<double> point(x/radius, y/radius);
////    std::cout<<boost::math::jacobi_sn(1.0/sqrt(2), point);
//    std::complex<double> sn,cn,dn;
//    double err;
//    jacobi_sn_cn_dn(point, modulus, sn, cn, dn, err);
//    double omega = 2 * pow(std::norm(dn*sn),2) / pow(1+pow(std::norm(cn),2),2);
//    return omega;
return 16.0*pow(radius,4.0) / pow(4*radius*radius + x*x + y*y,2.0);
}

void ShellEnergyWithSwellRatio::test_bending_energy()
{
    double YoungsModulus = 1e5;
    double PossionRatio = 0.3;
    double thickness = 1;
    
    Eigen::MatrixXd V0, V;
    Eigen::MatrixXi F0, F;
    
    igl::readOBJ("/Users/chenzhen/UT/Research/Projects/ShearDeformation/Models/rect-coarse.obj", V0, F0);
    igl::readOBJ("/Users/chenzhen/UT/Research/Projects/ShearDeformation/Models/rect-coarse_1.obj", V, F);
    double E, E1;
    Eigen::VectorXd dE, dE1;
    bending_energy(V, V0, F, YoungsModulus, PossionRatio, thickness, E, dE);
    
    Eigen::MatrixXd V1 = V;
    Eigen::Vector3d w;
    w.setZero();
    for(int i=0;i<V.rows();i++)
    {
        for(int j=0;j<3;j++)
        {
            Eigen::VectorXd e(3*V.rows());
            e.setZero();
            e(3*i+j)=1;
            for(int k=3;k<8;k++)
            {
                w(j) = pow(10,-k);
                V1(i,j) = V(i,j) + w(j);
                bending_energy(V1, V0, F, YoungsModulus, PossionRatio, thickness, E1, dE1);
                double diff_bending = (E1-E)/w(j);
                std::cout<<std::endl<<"Eplison is: "<<w(j)<<std::endl;
                std::cout<<"The direction is "<<w.transpose()<<std::endl;
                std::cout<<"The finite difference w.r.t. the vertex V"<<i<<" is: "<<std::endl;
                //                std::cout<<diff_bending<<std::endl;
                //                std::cout<<"The inner product of the gradient is: "<<std::endl;
                //                std::cout<<dE.dot(e)<<std::endl;
                std::cout<<"The difference is: "<<abs(diff_bending - dE.dot(e))<<std::endl;
            }
            w.setZero();
            V1 = V;
        }
    }
    
}
