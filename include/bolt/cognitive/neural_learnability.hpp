#pragma once

#include "tensor_embedding.hpp"
#include <vector>
#include <memory>
#include <functional>
#include <cmath>
#include <random>

namespace bolt {
namespace cognitive {

// Activation functions for neural processing
namespace activation {

inline float sigmoid(float x) {
    return 1.0f / (1.0f + std::exp(-x));
}

inline float sigmoidDerivative(float x) {
    float s = sigmoid(x);
    return s * (1.0f - s);
}

inline float relu(float x) {
    return std::max(0.0f, x);
}

inline float reluDerivative(float x) {
    return x > 0.0f ? 1.0f : 0.0f;
}

inline float tanh(float x) {
    return std::tanh(x);
}

inline float tanhDerivative(float x) {
    float t = std::tanh(x);
    return 1.0f - t * t;
}

} // namespace activation

// Simple neural layer for learnability
class NeuralLayer {
private:
    size_t input_size_;
    size_t output_size_;
    std::vector<std::vector<float>> weights_;  // [output_size][input_size]
    std::vector<float> biases_;
    std::function<float(float)> activation_;
    std::function<float(float)> activation_derivative_;
    
    // For backpropagation
    std::vector<float> last_input_;
    std::vector<float> last_output_;
    std::vector<float> last_preactivation_;
    
public:
    NeuralLayer(size_t input_size, size_t output_size,
                std::function<float(float)> activation = activation::relu,
                std::function<float(float)> activation_derivative = activation::reluDerivative)
        : input_size_(input_size)
        , output_size_(output_size)
        , activation_(activation)
        , activation_derivative_(activation_derivative)
        , weights_(output_size, std::vector<float>(input_size))
        , biases_(output_size, 0.0f)
    {
        initializeWeights();
    }
    
    void initializeWeights() {
        std::random_device rd;
        std::mt19937 gen(rd());
        
        // He initialization for ReLU
        float std_dev = std::sqrt(2.0f / input_size_);
        std::normal_distribution<float> dist(0.0f, std_dev);
        
        for (auto& weight_row : weights_) {
            for (auto& w : weight_row) {
                w = dist(gen);
            }
        }
        
        // Biases initialized to small positive values
        for (auto& b : biases_) {
            b = 0.01f;
        }
    }
    
    // Forward pass
    std::vector<float> forward(const std::vector<float>& input) {
        last_input_ = input;
        last_preactivation_.resize(output_size_);
        last_output_.resize(output_size_);
        
        for (size_t i = 0; i < output_size_; ++i) {
            float sum = biases_[i];
            for (size_t j = 0; j < input_size_; ++j) {
                sum += weights_[i][j] * input[j];
            }
            last_preactivation_[i] = sum;
            last_output_[i] = activation_(sum);
        }
        
        return last_output_;
    }
    
    // Backward pass (returns gradient w.r.t. input)
    std::vector<float> backward(const std::vector<float>& output_gradient, float learning_rate) {
        std::vector<float> input_gradient(input_size_, 0.0f);
        
        // Compute gradient w.r.t. pre-activation
        std::vector<float> preact_gradient(output_size_);
        for (size_t i = 0; i < output_size_; ++i) {
            preact_gradient[i] = output_gradient[i] * activation_derivative_(last_preactivation_[i]);
        }
        
        // Update weights and compute input gradient
        for (size_t i = 0; i < output_size_; ++i) {
            for (size_t j = 0; j < input_size_; ++j) {
                // Gradient w.r.t. input
                input_gradient[j] += preact_gradient[i] * weights_[i][j];
                
                // Update weight
                weights_[i][j] -= learning_rate * preact_gradient[i] * last_input_[j];
            }
            
            // Update bias
            biases_[i] -= learning_rate * preact_gradient[i];
        }
        
        return input_gradient;
    }
    
    size_t getInputSize() const { return input_size_; }
    size_t getOutputSize() const { return output_size_; }
};

// Neural network for learnability
class LearnableNetwork {
private:
    std::vector<NeuralLayer> layers_;
    float learning_rate_;
    
public:
    explicit LearnableNetwork(float learning_rate = 0.001f)
        : learning_rate_(learning_rate) {}
    
    // Add a layer to the network
    void addLayer(size_t input_size, size_t output_size,
                  std::function<float(float)> activation = activation::relu,
                  std::function<float(float)> activation_derivative = activation::reluDerivative) {
        layers_.emplace_back(input_size, output_size, activation, activation_derivative);
    }
    
    // Forward pass through all layers
    std::vector<float> forward(const std::vector<float>& input) {
        std::vector<float> current = input;
        for (auto& layer : layers_) {
            current = layer.forward(current);
        }
        return current;
    }
    
    // Backward pass and update weights
    void backward(const std::vector<float>& output_gradient) {
        std::vector<float> current_gradient = output_gradient;
        
        // Backpropagate through layers in reverse order
        for (auto it = layers_.rbegin(); it != layers_.rend(); ++it) {
            current_gradient = it->backward(current_gradient, learning_rate_);
        }
    }
    
    // Train on a single example
    float train(const std::vector<float>& input, const std::vector<float>& target) {
        // Forward pass
        std::vector<float> output = forward(input);
        
        // Compute loss (MSE) and gradient
        std::vector<float> gradient(output.size());
        float loss = 0.0f;
        for (size_t i = 0; i < output.size(); ++i) {
            float error = output[i] - target[i];
            gradient[i] = 2.0f * error / output.size();
            loss += error * error;
        }
        loss /= output.size();
        
        // Backward pass
        backward(gradient);
        
        return loss;
    }
    
    void setLearningRate(float rate) { learning_rate_ = rate; }
    float getLearningRate() const { return learning_rate_; }
    
    size_t getLayerCount() const { return layers_.size(); }
};

// Embedding learner for adaptive embeddings
class EmbeddingLearner {
private:
    size_t embedding_dim_;
    std::unique_ptr<LearnableNetwork> network_;
    
public:
    explicit EmbeddingLearner(size_t embedding_dim, float learning_rate = 0.001f)
        : embedding_dim_(embedding_dim)
        , network_(std::make_unique<LearnableNetwork>(learning_rate))
    {
        // Simple transformation network
        network_->addLayer(embedding_dim, embedding_dim * 2);
        network_->addLayer(embedding_dim * 2, embedding_dim);
    }
    
    // Transform an embedding
    TensorEmbedding transform(const TensorEmbedding& input) {
        if (input.dimensions() != embedding_dim_) {
            throw std::runtime_error("Input dimension mismatch");
        }
        
        auto output = network_->forward(input.data());
        return TensorEmbedding(output);
    }
    
    // Learn from similarity feedback
    float learnFromSimilarity(
        const TensorEmbedding& embedding1,
        const TensorEmbedding& embedding2,
        float target_similarity
    ) {
        // Transform embeddings
        auto transformed1 = transform(embedding1);
        auto transformed2 = transform(embedding2);
        
        // Compute current similarity
        float current_similarity = transformed1.cosineSimilarity(transformed2);
        
        // Compute loss
        float loss = std::pow(current_similarity - target_similarity, 2);
        
        // Create target output (push towards target similarity)
        std::vector<float> target1 = embedding1.data();
        std::vector<float> target2 = embedding2.data();
        
        // Simple gradient approximation
        float adjustment = (target_similarity - current_similarity) * 0.1f;
        for (size_t i = 0; i < embedding_dim_; ++i) {
            target1[i] += adjustment * (transformed2[i] - transformed1[i]);
        }
        
        // Train the network
        network_->train(embedding1.data(), target1);
        
        return loss;
    }
    
    void setLearningRate(float rate) { network_->setLearningRate(rate); }
};

} // namespace cognitive
} // namespace bolt
