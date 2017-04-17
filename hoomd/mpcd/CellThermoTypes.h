// Copyright (c) 2009-2017 The Regents of the University of Michigan
// This file is part of the HOOMD-blue project, released under the BSD 3-Clause License.

// Maintainer: mphoward

/*!
 * \file mpcd/CellThermoTypes.h
 * \brief Defines enums for mpcd::CellThermoCompute
 */

#ifndef MPCD_CELL_THERMO_TYPES_H_
#define MPCD_CELL_THERMO_TYPES_H_

#include "hoomd/HOOMDMath.h"

#ifdef NVCC
#define DEVICE __device__ __forceinline__
#else
#define DEVICE
#endif

namespace mpcd
{
namespace detail
{

//! Indexes into 1d array containing scalar thermo properties
/*!
 * The last element contains the total number of properties.
 */
struct thermo_index
    {
    enum property
        {
        momentum_x=0,   //!< Net momentum in x
        momentum_y,     //!< Net momentum in y
        momentum_z,     //!< Net momentum in z
        energy,         //!< Net kinetic energy
        temperature,    //!< Average temperature
        num_quantities  //!< Total number of thermo quantities
        };
    };

struct CellVelocityPackOp
    {
    typedef Scalar4 element;

    DEVICE element pack(const Scalar4& val) const
        {
        return val;
        }

    DEVICE Scalar4 unpack(const element& e, const Scalar4& val) const
        {
        return make_scalar4(e.x + val.x, e.y + val.y, e.z + val.z, e.w + val.w);
        }
    };

struct CellEnergyPackOp
    {
    typedef struct
        {
        Scalar energy;
        unsigned int np;
        } element;

    DEVICE element pack(const Scalar3& val) const
        {
        element e;
        e.energy = val.x;
        e.np = __scalar_as_int(val.z);
        return e;
        }

    DEVICE Scalar3 unpack(const element& e, const Scalar3& val) const
        {
        return make_scalar3(e.energy + val.x,
                            val.y,
                            __int_as_scalar(__scalar_as_int(val.z) + e.np));
        }
    };

} // end namespace detail
} // end namespace mpcd

#undef DEVICE

#endif // MPCD_CELL_THERMO_TYPES_H_
