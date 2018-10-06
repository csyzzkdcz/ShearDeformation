//
//  GeometryFeature.cpp
//  ShearDeformation_bin
//
//  Created by Zhen Chen on 10/5/18.
//

#include <iostream>
#include "GeometryFeature.h"


void GeoFeature::face_normal(Eigen::Vector3d V0, Eigen::Vector3d V1, Eigen::Vector3d V2, int start_ind, Eigen::Vector3d &nf, std::vector<Eigen::Vector3d> &dn)
{
    if (start_ind != 0 && start_ind != 1 && start_ind != 2)
    {
        std::cout << "The differential vertex should lie on the triangle" << std::endl;
        return;
    }
    nf = ((V1-V0).cross(V2-V0)).normalized();
    Eigen::Matrix3d W;
    W << 1,0,0,
    0,1,0,
    0,0,1;
    double A = (V1-V0).cross(V2-V0).norm();
    dn.resize(3);
    Eigen::Vector3d e;
    if(start_ind==0)
    {
        e=V2-V1;
    }
    else if(start_ind==1)
    {
        e=V0-V2;
    }
    else
    {
        e=V1-V0;
    }
    for(int i=0;i<3;i++)
    {
        dn[i] = nf.dot(W.row(i)) * 1.0 / A * e.cross(nf);
    }
    
}

void GeoFeature::calculate_first_fundamental_form(Eigen::Vector3d V0, Eigen::Vector3d V1, Eigen::Vector3d V2,Eigen::Matrix2d &I)
{
    I(0,0) = (V1-V0).dot(V1-V0);
    I(1,0) = (V1-V0).dot(V2-V0);
    I(0,1) = (V1-V0).dot(V2-V0);
    I(1,1) = (V2-V0).dot(V2-V0);
}

void GeoFeature::diff_first_fundamental_form(Eigen::Vector3d V0, Eigen::Vector3d V1, Eigen::Vector3d V2, int start_ind,std::vector<Eigen::Matrix2d> &dI)
{
    if (start_ind != 0 && start_ind != 1 && start_ind != 2 && start_ind != -1)
    {
        std::cout << "The differential vertex should lie on the triangle or set start_ind as -1" << std::endl;
        return;
    }
    Eigen::Vector3d A11, A12, A21, A22;
    if (start_ind == 0)
    {
        A11 = -2 * (V1 - V0);
        A12 = -(V1 + V2 - 2 * V0);
        A21 = -(V1 + V2 - 2 * V0);
        A22 = -2 * (V2 - V0);
    }
    else if (start_ind == 1)
    {
        A11 = 2 * (V1 - V0);
        A12 = V2 - V0;
        A21 = V2 - V0;
        A22 << 0, 0, 0;
    }
    else if (start_ind == 2)
    {
        A11 << 0, 0, 0;
        A12 = V1 - V0;
        A21 = V1 - V0;
        A22 = 2 * (V2 - V0);
    }
    else
    {
        A11 << 0, 0, 0;
        A12 << 0, 0, 0;
        A21 << 0, 0, 0;
        A22 << 0, 0, 0;
    }
    dI.resize(3);
    for(int k=0;k<3;k++)
    {
        dI[k] << A11(k), A12(k),
        A21(k), A22(k);
    }
    
}

void GeoFeature::calculate_second_fundamental_form(Eigen::Vector3d V0, Eigen::Vector3d V1, Eigen::Vector3d V2, Eigen::Vector3d V_0, Eigen::Vector3d V_1, Eigen::Vector3d V_2, Eigen::Matrix2d &II)
{
    Eigen::Vector3d nf = ((V1-V0).cross(V2-V0)).normalized();
    Eigen::Vector3d nf0 = ((V_0-V1).cross(V2-V1)).normalized();
    Eigen::Vector3d nf1 = ((V_1-V2).cross(V0-V2)).normalized();
    Eigen::Vector3d nf2 = ((V_2-V0).cross(V1-V0)).normalized();
    
    Eigen::Vector3d n0 = (nf+nf0).normalized();
    Eigen::Vector3d n1 = (nf+nf1).normalized();
    Eigen::Vector3d n2 = (nf+nf2).normalized();
    
    II(0,0) = 2*(V1-V0).dot(n1-n0);
    II(0,1) = 2*(V1-V0).dot(n2-n0);
    II(1,0) = 2*(V2-V0).dot(n1-n0);
    II(1,1) = 2*(V2-V0).dot(n2-n0);
}


void GeoFeature::diff_second_fundamental_form(Eigen::Vector3d V0, Eigen::Vector3d V1, Eigen::Vector3d V2, Eigen::Vector3d V_0, Eigen::Vector3d V_1, Eigen::Vector3d V_2, int start_ind, std::vector<Eigen::Matrix2d> &dII)
{
    Eigen::Matrix3d tri_vert; // start with V_{start_ind}
    tri_vert.row((0-start_ind)%3) = V0;
    tri_vert.row((1-start_ind)%3) = V1;
    tri_vert.row((2-start_ind)%3) = V2;
    
    Eigen::Matrix3d adjacent_tri_vert;
    adjacent_tri_vert.row((0-start_ind)%3) = V_0;
    adjacent_tri_vert.row((1-start_ind)%3) = V_1;
    adjacent_tri_vert.row((2-start_ind)%3) = V_2;
    
    Eigen::Vector3d nf;
    std::vector<Eigen::Vector3d> edge_n;
    std::vector<Eigen::Vector3d> dnf;
    std::vector<Eigen::Vector3d> adjacent_nf;
    std::vector<std::vector<Eigen::Vector3d>> diff_adjacent_nf;
    std::vector<std::vector<Eigen::Vector3d>> diff_edge_n;
    
    edge_n.resize(3);
    adjacent_nf.resize(3);
    diff_adjacent_nf.resize(3);
    diff_edge_n.resize(3);
    
    face_normal(tri_vert.row(0), tri_vert.row(1), tri_vert.row(2), 0, nf, dnf);
    
    for(int i=0;i<3;i++)
    {
        face_normal(tri_vert.row((i+2)%3), tri_vert.row((i+1)%3), adjacent_tri_vert.row(i), i-1 , adjacent_nf[i], diff_adjacent_nf[i]);
    }
    
    for(int i=0;i<3;i++)
    {
        edge_n[i] = (nf+adjacent_nf[i]).normalized();
        for(int j=0;j<3;j++)
        {
            diff_edge_n[i][j] = 1.0 / (nf+adjacent_nf[i]).norm() * (dnf[j]+diff_adjacent_nf[i][j]-(dnf[j]+diff_adjacent_nf[i][j]).dot(edge_n[i])*edge_n[i]);
        }
    }
    
    std::vector<Eigen::Vector3d> e(3);
    e[0] << 1,0,0;
    e[1] << 0,1,0;
    e[2] << 0,0,1;
    dII.resize(3);
    
    for(int k=0;k<3;k++)
    {
        dII[k](0,0) = -2*e[k].dot(edge_n[1]-edge_n[0]) + 2*(tri_vert.row(1)-tri_vert.row(0)).dot(diff_edge_n[1][k]-diff_edge_n[0][k]);
        dII[k](0,1) = -2*e[k].dot(edge_n[2]-edge_n[0]) + 2*(tri_vert.row(1)-tri_vert.row(0)).dot(diff_edge_n[2][k]-diff_edge_n[0][k]);
        dII[k](1,0) = -2*e[k].dot(edge_n[1]-edge_n[0]) + 2*(tri_vert.row(2)-tri_vert.row(0)).dot(diff_edge_n[1][k]-diff_edge_n[0][k]);
        dII[k](1,1) = -2*e[k].dot(edge_n[2]-edge_n[0]) + 2*(tri_vert.row(2)-tri_vert.row(0)).dot(diff_edge_n[2][k]-diff_edge_n[0][k]);
    }
    
}






void GeoFeature::test_face_normal()
{
    Eigen::Matrix3d V;
    Eigen::Matrix3d U;
    Eigen::Vector3d n,n1;
    std::vector<Eigen::Vector3d> dn;
    std::vector<Eigen::Vector3d> dn1;
    
    V << 1,0,0,
    0,0.5,0,
    0,0,2;
    
    U=V;
    
    Eigen::Vector3d w;
    w.setZero();
    
    for(int j=0;j<3;j++)
    {
        face_normal(V.row(0), V.row(1), V.row(2), j, n, dn);
        for(int i=0;i<3;i++)
        {
            for(int k=3;k<7;k++)
            {
                w(i) = pow(10,-k);
                U(j,i) = V(j,i)+w(i);
                face_normal(U.row(0), U.row(1), U.row(2), j, n1, dn1);
                
                Eigen::Vector3d diff_n = (n1-n) / w(i);
                std::cout<<"The test vertex is V"<<j<<std::endl;
                std::cout<<"The variable is the "<<i<<"th component"<<std::endl;
                std::cout<<"Eplison is: "<<w(i)<<std::endl;
                Eigen::Vector3d grad = (dn[0]*w(0)+dn[1]*w(1)+dn[2]*w(2))/w(i);
                for(int p=0;p<3;p++)
                {
                    std::cout<<"The "<<p<<"th component of the finite differecce is: "<<diff_n(p)<<std::endl;
                    std::cout<<"The "<<p<<"th component of the gradient is: "<<grad(p)<<std::endl;
                    std::cout<<"The difference is: "<<abs(diff_n(p)-grad(p))<<std::endl;
                    
                }
            }
            w.setZero();
            U=V;
        }
    }
    
}
