// Created by Zhen Chen on 10/3/18.


#include "ShellSimulation.h"
#include "ShellEnergy.h"
#include "ExternalEnergy.h"
#include <igl/readOBJ.h>
#include <fstream>

int times = 0;

void callback_func(const alglib::real_1d_array &x, double &f, alglib::real_1d_array &df, void* ptr)
{
    times++;
    std::cout<<times<<std::endl;
    ShellSimulation* callback = (ShellSimulation*) ptr;
    callback->energy_func_grad(x,f,df);
}

bool ShellSimulation::set_up_simulation(const std::string &prefix)
{
    std::string meshName = prefix + std::string("_geometry.obj");
    if (!igl::readOBJ(meshName, VU, F))
        return false;
    
    p_fixed_index.clear();
    p_fixed.clear();
    std::string clampedName = prefix + std::string("_clamped_vertices.dat");
    std::ifstream ifs(clampedName);
    int nclamped;
    ifs >> nclamped;
    char dummy;
    ifs >> dummy;
    if (!ifs)
        return false;
    for (int i = 0; i < nclamped; i++)
    {
        int vid;
        ifs >> vid;
        if (!ifs || vid < 0 || vid >= VU.rows())
            return false;
        p_fixed_index.push_back(vid);
        p_fixed.push_back(VU.row(vid));
    }
    if (!ifs)
        return false;
    
    Eigen::Vector3d initial_f;
    initial_f << 0,0,0;
    external_force.assign(VU.rows(),initial_f);
    std::string loadName = prefix + std::string("_loaded_vertices.dat");
    std::ifstream lfs(loadName);
    if (!lfs)
        return false;
    int nloaded;
    lfs >> nloaded;
    lfs >> dummy;
    if (!lfs)
        return false;
    for (int i = 0; i < nloaded; i++)
    {
        int vidx;
        lfs >> vidx;
        if (!lfs || vidx < 0 || vidx >= external_force.size())
            return false;
        for (int j = 0; j < 3; j++)
        {
            double val;
            lfs >> val;
            external_force[vidx](j) = val;
        }
    }
    if (!lfs)
        return false;
    
    std::string matName = prefix + std::string("_material.dat");
    std::ifstream mfs(matName);
    if (!mfs)
        return false;
    mfs >> _thickness;
    mfs >> _YoungsModulus;
    mfs >> _PoissonsRatio;
    if (!mfs)
        return false;
    _is_initialized = true;
    
    return true;
}

void ShellSimulation::compute_deformed_surface(Eigen::MatrixXd &V, Eigen::MatrixXi &F)
{
    if(!_is_initialized)
    {
        std::cout<<"Please call set_up_simualtion(strin prefix) to initialize first"<<std::endl;
        return;
    }
    alglib::real_1d_array x;
    double epsg = 1e-8;
    double epsf = 1e-8;
    double epsx = 1e-8;
    double stpmax = 0;
    alglib::ae_int_t maxits = 20000;
//    alglib::mincgstate state;
//    alglib::mincgreport rep;
    alglib::minlbfgsstate state;
    alglib::minlbfgsreport rep;
    x.setlength(3*V.rows());
    for(int i=0;i<V.rows();i++)
        for(int j=0;j<3;j++)
        {
            x[3*i+j] = V(i,j);
        }
    
    // Solve the optimization problem using alglib
    //    alglib::mincgcreate(x, state);
    //    alglib::mincgsetcond(state, epsg, epsf, epsx, maxits);
    //    alglib::mincgsetstpmax(state, stpmax);
    //    alglib::mincgoptimize(state, callback_func, NULL, this);
    //    alglib::mincgresults(state, x, rep);
    alglib::minlbfgscreate(x.length(),x, state);
    alglib::minlbfgssetcond(state, epsg, epsf, epsx, maxits);
    alglib::minlbfgssetstpmax(state, stpmax);
    alglib::minlbfgsoptimize(state, callback_func, NULL, this);
    alglib::minlbfgsresults(state, x, rep);
    for(int i=0;i<V.rows();i++)
    {
        V.row(i)<<x[3*i],x[3*i+1],x[3*i+2];
    }
    printf("%d\n", int(rep.terminationtype));
//    Rep     -   optimization report:
//    * Rep.TerminationType completetion code:
//    * -8    internal integrity control  detected  infinite
//    or NAN values in  function/gradient.  Abnormal
//    termination signalled.
//    * -7    gradient verification failed.
//    See MinLBFGSSetGradientCheck() for more information.
//        * -2    rounding errors prevent further improvement.
//        X contains best point found.
//        * -1    incorrect parameters were specified
//        *  1    relative function improvement is no more than
//        EpsF.
//        *  2    relative step is no more than EpsX.
//        *  4    gradient norm is no more than EpsG
//        *  5    MaxIts steps was taken
//        *  7    stopping conditions are too stringent,
//        further improvement is impossible
//        *  8    terminated by user who called minlbfgsrequesttermination().
//        X contains point which was "current accepted" when
//        termination request was submitted.
//        * Rep.IterationsCount contains iterations count
//        * NFEV countains number of function calculations
}

void ShellSimulation::energy_func_grad(const alglib::real_1d_array &x, double &f, alglib::real_1d_array &df)
{
    if(x.length()%3!=0)
    {
        std::cout<<"Error!"<<std::endl;
        return;
    }
    if(!_is_initialized)
    {
        std::cout<<"Please do initialization first!"<<std::endl;
        return;
    }
    //convert x to vertice format
    int v_num = int(x.length()/3);
    Eigen::MatrixXd V(v_num,3);
    for(int i=0;i<v_num;i++)
    {
        V.row(i)<<x[3*i],x[3*i+1],x[3*i+2];
    }
    auto op_shell = std::make_unique<ShellEnergy>();
    auto op_external = std::make_unique<ExternalEnergy>();
    double E_streching(0), E_external(0), E_bending(0);
    Eigen::VectorXd diff_f_streching,diff_f_external,diff_f_bending;
    op_shell->streching_energy(V,VU,F,_YoungsModulus,_PoissonsRatio,_thickness,E_streching,diff_f_streching);
    op_shell->bending_energy(V, VU, F, _YoungsModulus, _PoissonsRatio, _thickness, E_bending, diff_f_bending);
    op_external->external_energy(V, VU, external_force, E_external, diff_f_external);
    f = E_streching + E_bending + E_external;
    //    f = E_bending + E_streching;
    for(int i=0;i<df.length();i++)
    {
        df[i] = diff_f_streching(i) + diff_f_bending(i) + diff_f_external(i);
        //        df[i] = diff_f_streching(i) + diff_f_bending(i);
    }
    for(int i=0;i<p_fixed_index.size();i++)
    {
        for(int j=0;j<3;j++)
        {
            df[3*p_fixed_index[i]+j]=0;
        }
    }
    std::cout<<E_streching + E_bending + E_external<<std::endl;
    std::cout<<(diff_f_bending+diff_f_streching).norm()<<std::endl;
    std::cout<<(diff_f_external).norm()<<std::endl;
}


