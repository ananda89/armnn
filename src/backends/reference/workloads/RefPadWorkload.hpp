//
// Copyright © 2017 Arm Ltd. All rights reserved.
// SPDX-License-Identifier: MIT
//

#pragma once

#include <backendsCommon/Workload.hpp>
#include <backendsCommon/WorkloadData.hpp>

#include <armnn/TypesUtils.hpp>

namespace armnn
{

template <armnn::DataType DataType>
class RefPadWorkload : public TypedWorkload<PadQueueDescriptor, DataType>
{
public:

    static const std::string& GetName()
    {
        static const std::string name = std::string("RefPad") + GetDataTypeName(DataType) + "Workload";
        return name;
    }

    using TypedWorkload<PadQueueDescriptor, DataType>::m_Data;
    using TypedWorkload<PadQueueDescriptor, DataType>::TypedWorkload;

    void Execute() const override;
};

using RefPadBFloat16Workload = RefPadWorkload<DataType::BFloat16>;
using RefPadFloat32Workload =  RefPadWorkload<DataType::Float32>;
using RefPadFloat16Workload =  RefPadWorkload<DataType::Float16>;
using RefPadQAsymmS8Workload = RefPadWorkload<DataType::QAsymmS8>;
using RefPadQAsymm8Workload =  RefPadWorkload<DataType::QAsymmU8>;
using RefPadQSymm16Workload =  RefPadWorkload<DataType::QSymmS16>;

} //namespace armnn
