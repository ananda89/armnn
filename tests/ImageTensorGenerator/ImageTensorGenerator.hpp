//
// Copyright © 2017 Arm Ltd. All rights reserved.
// SPDX-License-Identifier: MIT
//

#include "../InferenceTestImage.hpp"
#include "Permute.hpp"
#include <armnn/TypesUtils.hpp>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>

struct NormalizationParameters
{
    float scale{ 1.0 };
    std::array<float, 3> mean{ { 0.0, 0.0, 0.0 } };
    std::array<float, 3> stddev{ { 1.0, 1.0, 1.0 } };
};

enum class SupportedFrontend
{
    Caffe      = 0,
    TensorFlow = 1,
    TFLite     = 2,
};

// Get normalization parameters.
// Note that different flavours of models have different normalization methods.
// This tool currently only supports Caffe, TF and TFLite models
NormalizationParameters GetNormalizationParameters(const SupportedFrontend& modelFormat,
                                                   const armnn::DataType& outputType)
{
    NormalizationParameters normParams;
    // Explicitly set default parameters
    normParams.scale  = 1.0;
    normParams.mean   = { 0.0, 0.0, 0.0 };
    normParams.stddev = { 1.0, 1.0, 1.0 };
    switch (modelFormat)
    {
        case SupportedFrontend::Caffe:
            break;
        case SupportedFrontend::TensorFlow:
        case SupportedFrontend::TFLite:
        default:
            switch (outputType)
            {
                case armnn::DataType::Float32:
                    normParams.scale = 127.5;
                    normParams.mean  = { 1.0, 1.0, 1.0 };
                    break;
                case armnn::DataType::Signed32:
                    normParams.mean = { 128.0, 128.0, 128.0 };
                    break;
                case armnn::DataType::QuantisedAsymm8:
                default:
                    break;
            }
            break;
    }
    return normParams;
}

// Prepare raw image tensor data by loading the image from imagePath and preprocessing it.
template <typename ElemType>
std::vector<ElemType> PrepareImageTensor(const std::string& imagePath,
                                         unsigned int newWidth,
                                         unsigned int newHeight,
                                         const NormalizationParameters& normParams,
                                         unsigned int batchSize                = 1,
                                         const armnn::DataLayout& outputLayout = armnn::DataLayout::NHWC);

// Prepare float32 image tensor
template <>
std::vector<float> PrepareImageTensor<float>(const std::string& imagePath,
                                             unsigned int newWidth,
                                             unsigned int newHeight,
                                             const NormalizationParameters& normParams,
                                             unsigned int batchSize,
                                             const armnn::DataLayout& outputLayout)
{
    // Generate image tensor
    std::vector<float> imageData;
    InferenceTestImage testImage(imagePath.c_str());
    if (newWidth == 0)
    {
        newWidth = testImage.GetWidth();
    }
    if (newHeight == 0)
    {
        newHeight = testImage.GetHeight();
    }
    // Resize the image to new width and height or keep at original dimensions if the new width and height are specified
    // as 0 Centre/Normalise the image.
    imageData = testImage.Resize(newWidth, newHeight, CHECK_LOCATION(),
                                 InferenceTestImage::ResizingMethods::BilinearAndNormalized, normParams.mean,
                                 normParams.stddev, normParams.scale);
    if (outputLayout == armnn::DataLayout::NCHW)
    {
        // Convert to NCHW format
        const armnn::PermutationVector NHWCToArmNN = { 0, 2, 3, 1 };
        armnn::TensorShape dstShape({ batchSize, 3, newHeight, newWidth });
        std::vector<float> tempImage(imageData.size());
        armnnUtils::Permute(dstShape, NHWCToArmNN, imageData.data(), tempImage.data(), sizeof(float));
        imageData.swap(tempImage);
    }
    return imageData;
}

// Prepare int32 image tensor
template <>
std::vector<int> PrepareImageTensor<int>(const std::string& imagePath,
                                         unsigned int newWidth,
                                         unsigned int newHeight,
                                         const NormalizationParameters& normParams,
                                         unsigned int batchSize,
                                         const armnn::DataLayout& outputLayout)
{
    // Get float32 image tensor
    std::vector<float> imageDataFloat =
        PrepareImageTensor<float>(imagePath, newWidth, newHeight, normParams, batchSize, outputLayout);
    // Convert to int32 image tensor with static cast
    std::vector<int> imageDataInt;
    imageDataInt.reserve(imageDataFloat.size());
    std::transform(imageDataFloat.begin(), imageDataFloat.end(), std::back_inserter(imageDataInt),
                   [](float val) { return static_cast<int>(val); });
    return imageDataInt;
}

// Prepare qasymm8 image tensor
template <>
std::vector<uint8_t> PrepareImageTensor<uint8_t>(const std::string& imagePath,
                                                 unsigned int newWidth,
                                                 unsigned int newHeight,
                                                 const NormalizationParameters& normParams,
                                                 unsigned int batchSize,
                                                 const armnn::DataLayout& outputLayout)
{
    // Get float32 image tensor
    std::vector<float> imageDataFloat =
        PrepareImageTensor<float>(imagePath, newWidth, newHeight, normParams, batchSize, outputLayout);
    std::vector<uint8_t> imageDataQasymm8;
    imageDataQasymm8.reserve(imageDataFloat.size());
    // Convert to uint8 image tensor with static cast
    std::transform(imageDataFloat.begin(), imageDataFloat.end(), std::back_inserter(imageDataQasymm8),
                   [](float val) { return static_cast<uint8_t>(val); });
    return imageDataQasymm8;
}

// Write image tensor to ofstream
template <typename ElemType>
void WriteImageTensorImpl(const std::vector<ElemType>& imageData, std::ofstream& imageTensorFile)
{
    std::copy(imageData.begin(), imageData.end(), std::ostream_iterator<ElemType>(imageTensorFile, " "));
}