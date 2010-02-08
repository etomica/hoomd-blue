/*
Highly Optimized Object-oriented Many-particle Dynamics -- Blue Edition
(HOOMD-blue) Open Source Software License Copyright 2008, 2009 Ames Laboratory
Iowa State University and The Regents of the University of Michigan All rights
reserved.

HOOMD-blue may contain modifications ("Contributions") provided, and to which
copyright is held, by various Contributors who have granted The Regents of the
University of Michigan the right to modify and/or distribute such Contributions.

Redistribution and use of HOOMD-blue, in source and binary forms, with or
without modification, are permitted, provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright notice, this
list of conditions, and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice, this
list of conditions, and the following disclaimer in the documentation and/or
other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of HOOMD-blue's
contributors may be used to endorse or promote products derived from this
software without specific prior written permission.

Disclaimer

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND/OR
ANY WARRANTIES THAT THIS SOFTWARE IS FREE OF INFRINGEMENT ARE DISCLAIMED.

IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// $Id$
// $URL$
// Maintainer: joaander

#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4103 4244 )
#endif

#include <fstream>

#include "TablePotential.h"
#include "BinnedNeighborList.h"
#ifdef ENABLE_CUDA
#include "TablePotentialGPU.h"
#endif

using namespace std;
using namespace boost;

//! Name the unit test module
#define BOOST_TEST_MODULE TablePotentialTests
#include "boost_utf_configure.h"

/*! \file table_potential.cc
    \brief Implements unit tests for TablePotential and descendants
    \ingroup unit_tests
*/

//! Typedef'd TablePotential factory
typedef boost::function<shared_ptr<TablePotential> (shared_ptr<SystemDefinition> sysdef,
                                                    shared_ptr<NeighborList> nlist,
                                                    unsigned int width)> table_potential_creator;

//! performs some really basic checks on the TablePotential class
void table_potential_basic_test(table_potential_creator table_creator, ExecutionConfiguration exec_conf)
    {
#ifdef ENABLE_CUDA
    g_gpu_error_checking = true;
#endif
    
    // perform a basic test to see of the potential and force can be interpolated between two particles
    shared_ptr<SystemDefinition> sysdef_2(new SystemDefinition(2, BoxDim(1000.0), 1, 0, 0, 0, 0, ExecutionConfiguration()));
    shared_ptr<ParticleData> pdata_2 = sysdef_2->getParticleData();
    
    ParticleDataArrays arrays = pdata_2->acquireReadWrite();
    arrays.x[0] = arrays.y[0] = arrays.z[0] = 0.0;
    arrays.x[1] = Scalar(1.0); arrays.y[1] = arrays.z[1] = 0.0;
    pdata_2->release();
    
    shared_ptr<NeighborList> nlist_2(new NeighborList(sysdef_2, Scalar(7.0), Scalar(0.8)));
    shared_ptr<TablePotential> fc_2 = table_creator(sysdef_2, nlist_2, 3);
    
    // first check for proper initialization by seeing if the force and potential come out to be 0
    fc_2->compute(0);
    
    ForceDataArrays force_arrays = fc_2->acquire();
    MY_BOOST_CHECK_SMALL(force_arrays.fx[0], tol_small);
    MY_BOOST_CHECK_SMALL(force_arrays.fy[0], tol_small);
    MY_BOOST_CHECK_SMALL(force_arrays.fz[0], tol_small);
    MY_BOOST_CHECK_SMALL(force_arrays.pe[0], tol_small);
    MY_BOOST_CHECK_SMALL(force_arrays.virial[0], tol_small);
    
    MY_BOOST_CHECK_SMALL(force_arrays.fx[1], tol_small);
    MY_BOOST_CHECK_SMALL(force_arrays.fy[1], tol_small);
    MY_BOOST_CHECK_SMALL(force_arrays.fz[1], tol_small);
    MY_BOOST_CHECK_SMALL(force_arrays.pe[1], tol_small);
    MY_BOOST_CHECK_SMALL(force_arrays.virial[1], tol_small);
    
    // specify a table to interpolate
    vector<float> V, F;
    V.push_back(10.0);  F.push_back(1.0);
    V.push_back(21.0);  F.push_back(6.0);
    V.push_back(5.0);   F.push_back(2.0);
    fc_2->setTable(0, 0, V, F, 2.0, 4.0);
    
    // compute the forces again and check that they are still 0
    fc_2->compute(1);
    
    force_arrays = fc_2->acquire();
    MY_BOOST_CHECK_SMALL(force_arrays.fx[0], tol_small);
    MY_BOOST_CHECK_SMALL(force_arrays.fy[0], tol_small);
    MY_BOOST_CHECK_SMALL(force_arrays.fz[0], tol_small);
    MY_BOOST_CHECK_SMALL(force_arrays.pe[0], tol_small);
    MY_BOOST_CHECK_SMALL(force_arrays.virial[0], tol_small);
    
    MY_BOOST_CHECK_SMALL(force_arrays.fx[1], tol_small);
    MY_BOOST_CHECK_SMALL(force_arrays.fy[1], tol_small);
    MY_BOOST_CHECK_SMALL(force_arrays.fz[1], tol_small);
    MY_BOOST_CHECK_SMALL(force_arrays.pe[1], tol_small);
    MY_BOOST_CHECK_SMALL(force_arrays.virial[1], tol_small);
    
    // now go to rmin and check for the correct force value
    arrays = pdata_2->acquireReadWrite();
    arrays.x[1] = Scalar(2.0);
    pdata_2->release();
    
    fc_2->compute(2);
    
    force_arrays = fc_2->acquire();
    MY_BOOST_CHECK_CLOSE(force_arrays.fx[0], -1.0, tol);
    MY_BOOST_CHECK_SMALL(force_arrays.fy[0], tol_small);
    MY_BOOST_CHECK_SMALL(force_arrays.fz[0], tol_small);
    MY_BOOST_CHECK_CLOSE(force_arrays.pe[0], 5.0, tol);
    MY_BOOST_CHECK_CLOSE(force_arrays.virial[0], (1.0 / 6.0) * 2.0, tol);
    
    MY_BOOST_CHECK_CLOSE(force_arrays.fx[1], 1.0, tol);
    MY_BOOST_CHECK_SMALL(force_arrays.fy[1], tol_small);
    MY_BOOST_CHECK_SMALL(force_arrays.fz[1], tol_small);
    MY_BOOST_CHECK_CLOSE(force_arrays.pe[1], 5.0, tol);
    MY_BOOST_CHECK_CLOSE(force_arrays.virial[1], (1.0 / 6.0) * 2.0, tol);
    
    // go halfway in-between two points
    arrays = pdata_2->acquireReadWrite();
    arrays.y[1] = Scalar(3.5);
    arrays.x[1] = Scalar(0.0);
    pdata_2->release();
    
    // check the forces
    fc_2->compute(3);
    force_arrays = fc_2->acquire();
    MY_BOOST_CHECK_CLOSE(force_arrays.fy[0], -4.0, tol);
    MY_BOOST_CHECK_SMALL(force_arrays.fx[0], tol_small);
    MY_BOOST_CHECK_SMALL(force_arrays.fz[0], tol_small);
    MY_BOOST_CHECK_CLOSE(force_arrays.pe[0], 13.0/2.0, tol);
    MY_BOOST_CHECK_CLOSE(force_arrays.virial[0], (1.0 / 6.0) * 4.0 * 3.5, tol);
    
    MY_BOOST_CHECK_CLOSE(force_arrays.fy[1], 4.0, tol);
    MY_BOOST_CHECK_SMALL(force_arrays.fx[1], tol_small);
    MY_BOOST_CHECK_SMALL(force_arrays.fz[1], tol_small);
    MY_BOOST_CHECK_CLOSE(force_arrays.pe[1], 13.0 / 2.0, tol);
    MY_BOOST_CHECK_CLOSE(force_arrays.virial[1], (1.0 / 6.0) * 4.0 * 3.5, tol);
    
    // and now check for when r > rmax
    arrays = pdata_2->acquireReadWrite();
    arrays.z[1] = Scalar(4.0);
    pdata_2->release();
    
    // compute and check
    fc_2->compute(4);
    
    force_arrays = fc_2->acquire();
    MY_BOOST_CHECK_SMALL(force_arrays.fx[0], tol_small);
    MY_BOOST_CHECK_SMALL(force_arrays.fy[0], tol_small);
    MY_BOOST_CHECK_SMALL(force_arrays.fz[0], tol_small);
    MY_BOOST_CHECK_SMALL(force_arrays.pe[0], tol_small);
    MY_BOOST_CHECK_SMALL(force_arrays.virial[0], tol_small);
    
    MY_BOOST_CHECK_SMALL(force_arrays.fx[1], tol_small);
    MY_BOOST_CHECK_SMALL(force_arrays.fy[1], tol_small);
    MY_BOOST_CHECK_SMALL(force_arrays.fz[1], tol_small);
    MY_BOOST_CHECK_SMALL(force_arrays.pe[1], tol_small);
    MY_BOOST_CHECK_SMALL(force_arrays.virial[1], tol_small);
    }

//! checks to see if TablePotential correctly handles multiple types
void table_potential_type_test(table_potential_creator table_creator, ExecutionConfiguration exec_conf)
    {
#ifdef ENABLE_CUDA
    g_gpu_error_checking = true;
#endif
    
    // perform a basic test to see of the potential and force can be interpolated between two particles
    shared_ptr<SystemDefinition> sysdef(new SystemDefinition(4, BoxDim(1000.0), 2, 0, 0, 0, 0, ExecutionConfiguration()));
    shared_ptr<ParticleData> pdata = sysdef->getParticleData();
    
    ParticleDataArrays arrays = pdata->acquireReadWrite();
    arrays.x[0] = arrays.y[0] = arrays.z[0] = 0.0; arrays.type[0] = 0;
    arrays.x[1] = Scalar(1.5); arrays.y[1] = arrays.z[1] = 0.0; arrays.type[1] = 1;
    arrays.x[2] = 0.0; arrays.y[2] = Scalar(1.5); arrays.z[2] = 0.0; arrays.type[2] = 0;
    arrays.x[3] = Scalar(1.5); arrays.y[3] = Scalar(1.5); arrays.z[3] = 0.0; arrays.type[3] = 1;
    pdata->release();
    
    shared_ptr<NeighborList> nlist(new NeighborList(sysdef, Scalar(2.0), Scalar(0.8)));
    shared_ptr<TablePotential> fc = table_creator(sysdef, nlist, 3);
    
    // specify a table to interpolate
    vector<float> V, F;
    V.push_back(10.0);  F.push_back(1.0);
    V.push_back(20.0);  F.push_back(6.0);
    V.push_back(5.0);   F.push_back(2.0);
    fc->setTable(0, 0, V, F, 1.0, 2.0);
    
    // next type pair
    V.clear(); F.clear();
    V.push_back(20.0);  F.push_back(2.0);
    V.push_back(40.0);  F.push_back(12.0);
    V.push_back(10.0);   F.push_back(4.0);
    fc->setTable(0, 1, V, F, 0.0, 2.0);
    
    // next type pair
    V.clear(); F.clear();
    V.push_back(5.0);  F.push_back(0.5);
    V.push_back(10.0);  F.push_back(3.0);
    V.push_back(2.5);   F.push_back(2.0);
    fc->setTable(1, 1, V, F, 1.0, 2.0);
    
    // compute and check
    fc->compute(0);
    
    ForceDataArrays force_arrays = fc->acquire();
    MY_BOOST_CHECK_CLOSE(force_arrays.fx[0], -8.0, tol);
    MY_BOOST_CHECK_CLOSE(force_arrays.fy[0], -6.0, tol);
    MY_BOOST_CHECK_SMALL(force_arrays.fz[0], tol_small);
    MY_BOOST_CHECK_SMALL(force_arrays.pe[0], 10.0+25.0);
    MY_BOOST_CHECK_CLOSE(force_arrays.virial[0], (8*1.5+6*1.5)*1.0/6.0, tol);
    
    MY_BOOST_CHECK_CLOSE(force_arrays.fx[1], 8.0, tol);
    MY_BOOST_CHECK_CLOSE(force_arrays.fy[1], -3.0, tol);
    MY_BOOST_CHECK_SMALL(force_arrays.fz[1], tol_small);
    MY_BOOST_CHECK_CLOSE(force_arrays.pe[1], 25.0/2.0 + 5.0, tol);
    MY_BOOST_CHECK_CLOSE(force_arrays.virial[1], (8*1.5 + 3.0 * 1.5)*1.0/6.0, tol);
    
    MY_BOOST_CHECK_CLOSE(force_arrays.fx[2], -8.0, tol);
    MY_BOOST_CHECK_CLOSE(force_arrays.fy[2], 6.0, tol);
    MY_BOOST_CHECK_SMALL(force_arrays.fz[2], tol_small);
    MY_BOOST_CHECK_SMALL(force_arrays.pe[2], 10.0+25.0);
    MY_BOOST_CHECK_CLOSE(force_arrays.virial[2], (8*1.5+6*1.5)*1.0/6.0, tol);
    
    MY_BOOST_CHECK_CLOSE(force_arrays.fx[3], 8.0, tol);
    MY_BOOST_CHECK_CLOSE(force_arrays.fy[3], 3.0, tol);
    MY_BOOST_CHECK_SMALL(force_arrays.fz[3], tol_small);
    MY_BOOST_CHECK_CLOSE(force_arrays.pe[3], 25.0/2.0 + 5.0, tol);
    MY_BOOST_CHECK_CLOSE(force_arrays.virial[3], (8*1.5 + 3.0*1.5)*1.0/6.0, tol);
    }

//! TablePotential creator for unit tests
shared_ptr<TablePotential> base_class_table_creator(shared_ptr<SystemDefinition> sysdef,
                                                    shared_ptr<NeighborList> nlist,
                                                    unsigned int width)
    {
    return shared_ptr<TablePotential>(new TablePotential(sysdef, nlist, width));
    }

#ifdef ENABLE_CUDA
//! TablePotentialGPU creator for unit tests
shared_ptr<TablePotential> gpu_table_creator(shared_ptr<SystemDefinition> sysdef,
                                             shared_ptr<NeighborList> nlist,
                                             unsigned int width)
    {
    nlist->setStorageMode(NeighborList::full);
    shared_ptr<TablePotentialGPU> table(new TablePotentialGPU(sysdef, nlist, width));
    // the default block size kills valgrind :) reduce it
    table->setBlockSize(64);
    return table;
    }
#endif


//! boost test case for basic test on CPU
BOOST_AUTO_TEST_CASE( TablePotential_basic )
    {
    table_potential_creator table_creator_base = bind(base_class_table_creator, _1, _2, _3);
    table_potential_basic_test(table_creator_base, ExecutionConfiguration(ExecutionConfiguration::CPU));
    }

//! boost test case for type test on CPU
BOOST_AUTO_TEST_CASE( TablePotential_type )
    {
    table_potential_creator table_creator_base = bind(base_class_table_creator, _1, _2, _3);
    table_potential_type_test(table_creator_base, ExecutionConfiguration(ExecutionConfiguration::CPU));
    }

#ifdef ENABLE_CUDA
//! boost test case for basic test on GPU
BOOST_AUTO_TEST_CASE( TablePotentialGPU_basic )
    {
    table_potential_creator table_creator_gpu = bind(gpu_table_creator, _1, _2, _3);
    table_potential_basic_test(table_creator_gpu, ExecutionConfiguration(ExecutionConfiguration::GPU));
    }

//! boost test case for type test on GPU
BOOST_AUTO_TEST_CASE( TablePotentialGPU_type )
    {
    table_potential_creator table_creator_gpu = bind(gpu_table_creator, _1, _2, _3);
    table_potential_type_test(table_creator_gpu, ExecutionConfiguration(ExecutionConfiguration::GPU));
    }
#endif

#ifdef WIN32
#pragma warning( pop )
#endif
