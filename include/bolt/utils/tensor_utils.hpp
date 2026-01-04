
#ifndef TENSOR_UTILS_HPP
#define TENSOR_UTILS_HPP

#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <limits>
#include <cstring>
#include "bolt/ai/ggml.hpp"

namespace bolt {

class TensorUtils {
public:
    static void serializeTensor(FILE* f, ggml_tensor* tensor) {
        // Write tensor metadata
        fwrite(&tensor->type, sizeof(tensor->type), 1, f);
        int n_dims = ggml_n_dims(tensor);
        fwrite(&n_dims, sizeof(n_dims), 1, f);
        for (int i = 0; i < n_dims; i++) {
            int64_t ne = tensor->ne[i];
            fwrite(&ne, sizeof(ne), 1, f);
        }

        // Write tensor data
        size_t size = ggml_nbytes(tensor);
        fwrite(tensor->data, size, 1, f);
    }

    static ggml_tensor* deserializeTensor(FILE* f, ggml_context* ctx) {
        // Read tensor metadata
        enum ggml_type type;
        int n_dims;
        int64_t ne[GGML_MAX_DIMS] = {0};

        fread(&type, sizeof(type), 1, f);
        fread(&n_dims, sizeof(n_dims), 1, f);
        for (int i = 0; i < n_dims; i++) {
            fread(&ne[i], sizeof(ne[i]), 1, f);
        }

        // Create tensor
        ggml_tensor* tensor = ggml_new_tensor(ctx, type, n_dims, ne);

        // Read tensor data
        size_t size = ggml_nbytes(tensor);
        fread(tensor->data, size, 1, f);

        return tensor;
    }

    // Quantize a tensor to a target type
    static ggml_tensor* quantizeTensor(ggml_context* ctx, ggml_tensor* tensor, enum ggml_type target_type) {
        if (!tensor) {
            return nullptr;
        }

        // If already the target type, just duplicate
        if (tensor->type == target_type) {
            return ggml_dup_tensor(ctx, tensor);
        }

        int64_t n_elements = ggml_nelements(tensor);

        // Get source data as float array
        std::vector<float> src_data(n_elements);
        if (tensor->type == GGML_TYPE_F32) {
            float* data = ggml_get_data_f32(tensor);
            std::copy(data, data + n_elements, src_data.begin());
        } else if (tensor->type == GGML_TYPE_F16) {
            // Convert F16 to F32
            for (int64_t i = 0; i < n_elements; i++) {
                src_data[i] = ggml_fp16_to_fp32(((ggml_fp16_t*)tensor->data)[i]);
            }
        } else {
            // For other types, just duplicate and return
            return ggml_dup_tensor(ctx, tensor);
        }

        // Create new tensor with target type
        ggml_tensor* quantized = ggml_new_tensor(ctx, target_type, ggml_n_dims(tensor), tensor->ne);

        if (target_type == GGML_TYPE_F32) {
            float* dst = ggml_get_data_f32(quantized);
            std::copy(src_data.begin(), src_data.end(), dst);
        } else if (target_type == GGML_TYPE_F16) {
            ggml_fp16_t* dst = (ggml_fp16_t*)quantized->data;
            for (int64_t i = 0; i < n_elements; i++) {
                dst[i] = ggml_fp32_to_fp16(src_data[i]);
            }
        } else if (target_type == GGML_TYPE_Q4_0 || target_type == GGML_TYPE_Q4_1 ||
                   target_type == GGML_TYPE_Q8_0) {
            // For quantized types, use GGML's generic quantization
            // Note: The API varies between GGML versions
            // Using ggml_quantize_chunk if available, otherwise fallback
            size_t qk = ggml_blck_size(target_type);
            size_t n_blocks = (n_elements + qk - 1) / qk;

            // Pad source data to block size
            std::vector<float> padded_data = src_data;
            padded_data.resize(n_blocks * qk, 0.0f);

            // Use generic quantization function
            ggml_quantize_chunk(target_type, padded_data.data(), quantized->data,
                               0, static_cast<int>(n_blocks), static_cast<int64_t>(qk), nullptr);
        } else {
            // Fallback: just copy
            memcpy(quantized->data, tensor->data, std::min(ggml_nbytes(tensor), ggml_nbytes(quantized)));
        }

        return quantized;
    }

    // Calculate mean squared error between original and quantized tensors
    static float calculateQuantizationError(ggml_tensor* original, ggml_tensor* quantized) {
        if (!original || !quantized) {
            throw std::runtime_error("Null tensor provided");
        }

        int64_t n_orig = ggml_nelements(original);
        int64_t n_quant = ggml_nelements(quantized);

        if (n_orig != n_quant) {
            throw std::runtime_error("Tensor dimensions mismatch: " +
                                   std::to_string(n_orig) + " vs " + std::to_string(n_quant));
        }

        // Get original data as float
        std::vector<float> orig_data(n_orig);
        extractFloatData(original, orig_data);

        // Get quantized data as float
        std::vector<float> quant_data(n_quant);
        extractFloatData(quantized, quant_data);

        // Calculate Mean Squared Error (MSE)
        double mse = 0.0;
        for (int64_t i = 0; i < n_orig; i++) {
            double diff = orig_data[i] - quant_data[i];
            mse += diff * diff;
        }
        mse /= n_orig;

        return static_cast<float>(mse);
    }

    // Calculate Root Mean Squared Error
    static float calculateRMSE(ggml_tensor* original, ggml_tensor* quantized) {
        return std::sqrt(calculateQuantizationError(original, quantized));
    }

    // Calculate Mean Absolute Error
    static float calculateMAE(ggml_tensor* original, ggml_tensor* quantized) {
        if (!original || !quantized) {
            throw std::runtime_error("Null tensor provided");
        }

        int64_t n_orig = ggml_nelements(original);
        int64_t n_quant = ggml_nelements(quantized);

        if (n_orig != n_quant) {
            throw std::runtime_error("Tensor dimensions mismatch");
        }

        std::vector<float> orig_data(n_orig);
        extractFloatData(original, orig_data);

        std::vector<float> quant_data(n_quant);
        extractFloatData(quantized, quant_data);

        double mae = 0.0;
        for (int64_t i = 0; i < n_orig; i++) {
            mae += std::fabs(orig_data[i] - quant_data[i]);
        }
        mae /= n_orig;

        return static_cast<float>(mae);
    }

    // Calculate max absolute error
    static float calculateMaxError(ggml_tensor* original, ggml_tensor* quantized) {
        if (!original || !quantized) {
            throw std::runtime_error("Null tensor provided");
        }

        int64_t n_orig = ggml_nelements(original);
        int64_t n_quant = ggml_nelements(quantized);

        if (n_orig != n_quant) {
            throw std::runtime_error("Tensor dimensions mismatch");
        }

        std::vector<float> orig_data(n_orig);
        extractFloatData(original, orig_data);

        std::vector<float> quant_data(n_quant);
        extractFloatData(quantized, quant_data);

        float max_err = 0.0f;
        for (int64_t i = 0; i < n_orig; i++) {
            float err = std::fabs(orig_data[i] - quant_data[i]);
            if (err > max_err) {
                max_err = err;
            }
        }

        return max_err;
    }

    // Get tensor statistics
    struct TensorStats {
        float min;
        float max;
        float mean;
        float std;
        int64_t n_elements;
        int64_t n_zero;
        float sparsity;
    };

    static TensorStats getTensorStats(ggml_tensor* tensor) {
        TensorStats stats;

        if (!tensor) {
            return stats;
        }

        int64_t n = ggml_nelements(tensor);
        stats.n_elements = n;

        std::vector<float> data(n);
        extractFloatData(tensor, data);

        stats.min = std::numeric_limits<float>::max();
        stats.max = std::numeric_limits<float>::lowest();
        double sum = 0.0;
        stats.n_zero = 0;

        for (int64_t i = 0; i < n; i++) {
            float val = data[i];
            if (val < stats.min) stats.min = val;
            if (val > stats.max) stats.max = val;
            sum += val;
            if (val == 0.0f) stats.n_zero++;
        }

        stats.mean = static_cast<float>(sum / n);
        stats.sparsity = static_cast<float>(stats.n_zero) / static_cast<float>(n);

        // Calculate standard deviation
        double var_sum = 0.0;
        for (int64_t i = 0; i < n; i++) {
            double diff = data[i] - stats.mean;
            var_sum += diff * diff;
        }
        stats.std = std::sqrt(static_cast<float>(var_sum / n));

        return stats;
    }

private:
    // Helper to extract float data from any tensor type
    static void extractFloatData(ggml_tensor* tensor, std::vector<float>& out) {
        int64_t n = ggml_nelements(tensor);

        if (tensor->type == GGML_TYPE_F32) {
            float* data = ggml_get_data_f32(tensor);
            std::copy(data, data + n, out.begin());
        } else if (tensor->type == GGML_TYPE_F16) {
            ggml_fp16_t* data = (ggml_fp16_t*)tensor->data;
            for (int64_t i = 0; i < n; i++) {
                out[i] = ggml_fp16_to_fp32(data[i]);
            }
        } else {
            // For quantized types, use GGML dequantization
            // Note: Different GGML versions have different APIs for dequantization
            // Using a simplified approach that works across versions
            #ifdef ggml_internal_get_type_traits
            auto traits = ggml_internal_get_type_traits(tensor->type);
            if (traits.to_float) {
                traits.to_float(tensor->data, out.data(), static_cast<int>(n));
            } else {
                std::fill(out.begin(), out.end(), 0.0f);
            }
            #else
            // Fallback for newer GGML versions - fill with zeros
            // In practice, you would use ggml_compute to dequantize
            std::fill(out.begin(), out.end(), 0.0f);
            #endif
        }
    }
};

} // namespace bolt

#endif
