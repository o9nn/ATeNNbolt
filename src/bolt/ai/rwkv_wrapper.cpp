#include "bolt/ai/rwkv_wrapper.hpp"
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <algorithm>
#include <cstring>

namespace bolt {

void RWKVWrapper::initialize(const std::string& model_path, int ctx_size) {
    model_path_ = model_path;
    
    std::cout << "Initializing RWKV model from: " << model_path << std::endl;
    
    // Create GGUF loader
    gguf_loader_ = std::make_unique<GGUFLoader>(model_path);
    
    // Load the GGUF file
    if (!gguf_loader_->load()) {
        throw std::runtime_error("Failed to load GGUF file: " + gguf_loader_->getError());
    }
    
    std::cout << "GGUF file loaded successfully" << std::endl;
    std::cout << "  Version: " << gguf_loader_->getVersion() << std::endl;
    std::cout << "  Architecture: " << gguf_loader_->getArchitecture() << std::endl;
    
    // Extract model parameters
    n_layers_ = gguf_loader_->getNumLayers();
    n_embd_ = gguf_loader_->getEmbedDim();
    n_vocab_ = gguf_loader_->getVocabSize();
    
    if (n_layers_ == 0 || n_embd_ == 0 || n_vocab_ == 0) {
        throw std::runtime_error("Invalid model parameters: n_layers=" + std::to_string(n_layers_) +
                               ", n_embd=" + std::to_string(n_embd_) +
                               ", n_vocab=" + std::to_string(n_vocab_));
    }
    
    std::cout << "Model parameters:" << std::endl;
    std::cout << "  Layers: " << n_layers_ << std::endl;
    std::cout << "  Embedding dim: " << n_embd_ << std::endl;
    std::cout << "  Vocabulary size: " << n_vocab_ << std::endl;
    
    // Initialize tokenizer
    tokenizer_ = std::make_unique<BPETokenizer>();
    auto tokens = gguf_loader_->getMetadataStringArray("tokenizer.ggml.tokens");
    auto scores = gguf_loader_->getMetadataFloatArray("tokenizer.ggml.scores");
    
    if (tokens.empty()) {
        throw std::runtime_error("No tokenizer vocabulary found in model file");
    }
    
    if (!tokenizer_->loadVocabulary(tokens, scores)) {
        throw std::runtime_error("Failed to load tokenizer vocabulary");
    }
    
    // Calculate required memory
    size_t mem_size = ctx_size * 1024 * 1024; // Convert MB to bytes
    
    // Create GGML context
    context_ = std::make_unique<GGMLContext>(mem_size);
    
    // Load model weights
    loadModel(model_path);
    
    // Initialize state
    state_ = std::make_unique<RWKVState>(context_->get(), n_layers_, n_embd_);
    
    // Initialize random number generator
    std::random_device rd;
    rng_ = std::mt19937(rd());
    
    model_loaded_ = true;
    std::cout << "RWKV model initialized successfully" << std::endl;
}

void RWKVWrapper::loadModel(const std::string& path) {
    std::cout << "Loading model weights..." << std::endl;
    
    // Get list of all tensors
    auto tensor_names = gguf_loader_->getTensorNames();
    std::cout << "Found " << tensor_names.size() << " tensors in model file" << std::endl;
    
    // Load each tensor
    int loaded_count = 0;
    for (const auto& name : tensor_names) {
        ggml_tensor* tensor = gguf_loader_->loadTensor(context_->get(), name);
        if (tensor) {
            weights_[name] = tensor;
            loaded_count++;
        } else {
            std::cerr << "Warning: Failed to load tensor: " << name << std::endl;
        }
    }
    
    std::cout << "Loaded " << loaded_count << " / " << tensor_names.size() << " tensors" << std::endl;
    
    // Verify essential weights are present
    std::vector<std::string> essential_weights = {
        "token_embd.weight",
        "output.weight"
    };
    
    for (const auto& weight_name : essential_weights) {
        if (weights_.find(weight_name) == weights_.end()) {
            // Try alternative names
            if (weight_name == "token_embd.weight" && weights_.find("emb.weight") != weights_.end()) {
                weights_["token_embd.weight"] = weights_["emb.weight"];
            } else if (weight_name == "output.weight" && weights_.find("head.weight") != weights_.end()) {
                weights_["output.weight"] = weights_["head.weight"];
            } else {
                std::cerr << "Warning: Essential weight not found: " << weight_name << std::endl;
            }
        }
    }
}

std::vector<int> RWKVWrapper::tokenize(const std::string& text) const {
    if (tokenizer_ && tokenizer_->isLoaded()) {
        return tokenizer_->encode(text);
    }
    
    // Fallback to character-level tokenization
    std::vector<int> tokens;
    for (char c : text) {
        tokens.push_back(static_cast<int>(static_cast<unsigned char>(c)));
    }
    return tokens;
}

std::string RWKVWrapper::detokenize(const std::vector<int>& tokens) const {
    if (tokenizer_ && tokenizer_->isLoaded()) {
        return tokenizer_->decode(tokens);
    }
    
    // Fallback to character-level detokenization
    std::string text;
    for (int token : tokens) {
        if (token >= 0 && token < 256) {
            text += static_cast<char>(token);
        }
    }
    return text;
}

std::string RWKVWrapper::generate(const std::string& prompt, size_t max_tokens) {
    return generateImproved(prompt, max_tokens, 1.0f, 0.95f);
}

std::string RWKVWrapper::generateImproved(const std::string& prompt, size_t max_tokens,
                                         float temperature, float top_p) {
    if (!model_loaded_) {
        throw std::runtime_error("RWKV model not initialized");
    }
    
    // Tokenize the prompt
    auto prompt_tokens = tokenize(prompt);
    
    if (prompt_tokens.empty()) {
        throw std::runtime_error("Empty prompt after tokenization");
    }
    
    std::cout << "Generating " << max_tokens << " tokens (prompt: " 
              << prompt_tokens.size() << " tokens)" << std::endl;
    
    std::vector<int> generated_tokens = prompt_tokens;
    
    // Generation loop
    for (size_t i = 0; i < max_tokens; ++i) {
        // Create input tensor from last token
        int last_token = generated_tokens.back();
        auto* input = ggml_new_tensor_1d(context_->get(), GGML_TYPE_I32, 1);
        memcpy(input->data, &last_token, sizeof(int));
        
        // Forward pass through the model
        auto* logits = forwardWithLogits(input);
        
        if (!logits) {
            std::cerr << "Forward pass failed at token " << i << std::endl;
            break;
        }
        
        // Sample next token
        int next_token = sampleToken(logits, temperature, top_p);
        
        // Check for end of sequence
        if (tokenizer_ && (next_token == tokenizer_->getEOSToken() || 
                          next_token == tokenizer_->getPADToken())) {
            std::cout << "EOS token generated at position " << i << std::endl;
            break;
        }
        
        generated_tokens.push_back(next_token);
        
        // Progress indicator
        if ((i + 1) % 10 == 0) {
            std::cout << "." << std::flush;
        }
    }
    
    std::cout << std::endl;
    
    // Remove prompt tokens and return only generated text
    std::vector<int> output_tokens(generated_tokens.begin() + prompt_tokens.size(), 
                                   generated_tokens.end());
    return detokenize(output_tokens);
}

ggml_tensor* RWKVWrapper::forwardWithLogits(ggml_tensor* input) {
    if (!model_loaded_) {
        throw std::runtime_error("RWKV model not initialized");
    }
    
    // Get input embeddings
    auto* current = getEmbeddings(input);
    
    if (!current) {
        std::cerr << "Failed to get embeddings" << std::endl;
        return nullptr;
    }
    
    // Process through RWKV layers
    for (int i = 0; i < n_layers_; i++) {
        current = rwkvLayer(current, i);
        if (!current) {
            std::cerr << "Failed at layer " << i << std::endl;
            return nullptr;
        }
    }
    
    // Final layer norm
    current = layerNorm(current, "ln_out.weight", "ln_out.bias");
    
    // Project to vocabulary logits
    auto* logits = projectToVocab(current);
    
    return logits;
}

ggml_tensor* RWKVWrapper::getEmbeddings(ggml_tensor* input) {
    // Look for embedding weight
    ggml_tensor* emb_weight = nullptr;
    
    if (weights_.find("token_embd.weight") != weights_.end()) {
        emb_weight = weights_["token_embd.weight"];
    } else if (weights_.find("emb.weight") != weights_.end()) {
        emb_weight = weights_["emb.weight"];
    }
    
    if (!emb_weight) {
        std::cerr << "Embedding weight not found" << std::endl;
        return nullptr;
    }
    
    // Use GGML's get_rows operation to extract embeddings
    return ggml_get_rows(context_->get(), emb_weight, input);
}

ggml_tensor* RWKVWrapper::projectToVocab(ggml_tensor* hidden) {
    // Look for output weight
    ggml_tensor* output_weight = nullptr;
    
    if (weights_.find("output.weight") != weights_.end()) {
        output_weight = weights_["output.weight"];
    } else if (weights_.find("head.weight") != weights_.end()) {
        output_weight = weights_["head.weight"];
    } else if (weights_.find("token_embd.weight") != weights_.end()) {
        // Weight tying: use embedding weights
        output_weight = weights_["token_embd.weight"];
    }
    
    if (!output_weight) {
        std::cerr << "Output weight not found" << std::endl;
        return nullptr;
    }
    
    // Matrix multiplication: hidden @ output_weight^T
    return ggml_mul_mat(context_->get(), output_weight, hidden);
}

ggml_tensor* RWKVWrapper::rwkvLayer(ggml_tensor* x, int layer_idx) {
    // Time-mixing (attention-like mechanism)
    std::string ln1_weight = "blk." + std::to_string(layer_idx) + ".ln1.weight";
    std::string ln1_bias = "blk." + std::to_string(layer_idx) + ".ln1.bias";
    
    auto* x_norm = layerNorm(x, ln1_weight, ln1_bias);
    auto* tm_out = timeMixing(x_norm, layer_idx);
    
    // Residual connection
    auto* x1 = ggml_add(context_->get(), x, tm_out);
    
    // Channel-mixing (FFN-like mechanism)
    std::string ln2_weight = "blk." + std::to_string(layer_idx) + ".ln2.weight";
    std::string ln2_bias = "blk." + std::to_string(layer_idx) + ".ln2.bias";
    
    auto* x1_norm = layerNorm(x1, ln2_weight, ln2_bias);
    auto* cm_out = channelMixing(x1_norm, layer_idx);
    
    // Residual connection
    auto* output = ggml_add(context_->get(), x1, cm_out);
    
    return output;
}

ggml_tensor* RWKVWrapper::timeMixing(ggml_tensor* x, int layer_idx) {
    // RWKV time-mixing with WKV (Weighted Key-Value) operation
    std::string prefix = "blk." + std::to_string(layer_idx) + ".att.";
    
    // Get time-mixing weights
    ggml_tensor* time_mix_k = weights_.find(prefix + "time_mix_k") != weights_.end() ? 
                              weights_[prefix + "time_mix_k"] : nullptr;
    ggml_tensor* time_mix_v = weights_.find(prefix + "time_mix_v") != weights_.end() ? 
                              weights_[prefix + "time_mix_v"] : nullptr;
    ggml_tensor* time_mix_r = weights_.find(prefix + "time_mix_r") != weights_.end() ? 
                              weights_[prefix + "time_mix_r"] : nullptr;
    
    // Get weight matrices
    ggml_tensor* Wk = weights_.find(prefix + "key.weight") != weights_.end() ? 
                      weights_[prefix + "key.weight"] : nullptr;
    ggml_tensor* Wv = weights_.find(prefix + "value.weight") != weights_.end() ? 
                      weights_[prefix + "value.weight"] : nullptr;
    ggml_tensor* Wr = weights_.find(prefix + "receptance.weight") != weights_.end() ? 
                      weights_[prefix + "receptance.weight"] : nullptr;
    ggml_tensor* Wout = weights_.find(prefix + "output.weight") != weights_.end() ? 
                        weights_[prefix + "output.weight"] : nullptr;
    
    // Get decay and bonus parameters
    ggml_tensor* time_decay = weights_.find(prefix + "time_decay") != weights_.end() ? 
                              weights_[prefix + "time_decay"] : nullptr;
    ggml_tensor* time_first = weights_.find(prefix + "time_first") != weights_.end() ? 
                              weights_[prefix + "time_first"] : nullptr;
    
    // If weights not found, return input unchanged
    if (!time_mix_k || !time_mix_v || !time_mix_r || !Wk || !Wv || !Wr || !Wout) {
        return x;
    }
    
    // Get previous token state (state_pp_)
    ggml_tensor* last_x = state_ && layer_idx < state_->state_pp_.size() ? 
                          state_->state_pp_[layer_idx] : nullptr;
    
    if (!last_x) {
        // First token, use zeros
        last_x = ggml_new_tensor_1d(context_->get(), GGML_TYPE_F32, n_embd_);
        ggml_set_zero(last_x);
    }
    
    // Interpolate current and previous token: x * mix + last_x * (1 - mix)
    // Rewritten as: last_x + (x - last_x) * mix
    auto* x_diff_k = ggml_sub(context_->get(), x, last_x);
    auto* x_diff_k_scaled = ggml_mul(context_->get(), x_diff_k, time_mix_k);
    auto* xk_interp = ggml_add(context_->get(), last_x, x_diff_k_scaled);
    
    auto* x_diff_v = ggml_sub(context_->get(), x, last_x);
    auto* x_diff_v_scaled = ggml_mul(context_->get(), x_diff_v, time_mix_v);
    auto* xv_interp = ggml_add(context_->get(), last_x, x_diff_v_scaled);
    
    auto* x_diff_r = ggml_sub(context_->get(), x, last_x);
    auto* x_diff_r_scaled = ggml_mul(context_->get(), x_diff_r, time_mix_r);
    auto* xr_interp = ggml_add(context_->get(), last_x, x_diff_r_scaled);
    
    // Compute k, v, r vectors
    auto* k = ggml_mul_mat(context_->get(), Wk, xk_interp);
    auto* v = ggml_mul_mat(context_->get(), Wv, xv_interp);
    auto* r = ggml_mul_mat(context_->get(), Wr, xr_interp);
    
    // Get state variables (last_num, last_den)
    ggml_tensor* last_num = state_ && layer_idx < state_->state_aa_.size() ? 
                            state_->state_aa_[layer_idx] : nullptr;
    ggml_tensor* last_den = state_ && layer_idx < state_->state_bb_.size() ? 
                            state_->state_bb_[layer_idx] : nullptr;
    
    if (!last_num || !last_den) {
        // Initialize state
        last_num = ggml_new_tensor_1d(context_->get(), GGML_TYPE_F32, n_embd_);
        last_den = ggml_new_tensor_1d(context_->get(), GGML_TYPE_F32, n_embd_);
        ggml_set_zero(last_num);
        ggml_set_zero(last_den);
    }
    
    // WKV operation: wkv = (last_num + exp(bonus + k) * v) / (last_den + exp(bonus + k))
    auto* bonus_k = time_first ? ggml_add(context_->get(), time_first, k) : k;
    auto* exp_bonus_k = ggml_exp(context_->get(), bonus_k);
    auto* exp_bonus_k_v = ggml_mul(context_->get(), exp_bonus_k, v);
    
    auto* wkv_num = ggml_add(context_->get(), last_num, exp_bonus_k_v);
    auto* wkv_den = ggml_add(context_->get(), last_den, exp_bonus_k);
    auto* wkv = ggml_div(context_->get(), wkv_num, wkv_den);
    
    // Apply receptance gating: rwkv = sigmoid(r) * wkv
    auto* r_sigmoid = ggml_sigmoid(context_->get(), r);
    auto* rwkv = ggml_mul(context_->get(), r_sigmoid, wkv);
    
    // Output projection
    auto* output = ggml_mul_mat(context_->get(), Wout, rwkv);
    
    // Update state for next token
    if (time_decay) {
        auto* exp_neg_exp_decay = ggml_exp(context_->get(), 
                                           ggml_neg(context_->get(), 
                                                   ggml_exp(context_->get(), time_decay)));
        auto* exp_k = ggml_exp(context_->get(), k);
        auto* exp_k_v = ggml_mul(context_->get(), exp_k, v);
        
        // num = exp(-exp(decay)) * last_num + exp(k) * v
        auto* new_num = ggml_add(context_->get(),
                                 ggml_mul(context_->get(), exp_neg_exp_decay, last_num),
                                 exp_k_v);
        
        // den = exp(-exp(decay)) * last_den + exp(k)
        auto* new_den = ggml_add(context_->get(),
                                 ggml_mul(context_->get(), exp_neg_exp_decay, last_den),
                                 exp_k);
        
        // Update state
        if (state_ && layer_idx < state_->state_aa_.size()) {
            state_->state_aa_[layer_idx] = new_num;
            state_->state_bb_[layer_idx] = new_den;
            state_->state_pp_[layer_idx] = x;
        }
    }
    
    return output;
}

ggml_tensor* RWKVWrapper::channelMixing(ggml_tensor* x, int layer_idx) {
    // RWKV channel-mixing (Feed-Forward with memory)
    std::string prefix = "blk." + std::to_string(layer_idx) + ".ffn.";
    
    // Get time-mixing weights for FFN
    ggml_tensor* time_mix_k = weights_.find(prefix + "time_mix_k") != weights_.end() ? 
                              weights_[prefix + "time_mix_k"] : nullptr;
    ggml_tensor* time_mix_r = weights_.find(prefix + "time_mix_r") != weights_.end() ? 
                              weights_[prefix + "time_mix_r"] : nullptr;
    
    // Get weight matrices
    ggml_tensor* Wk = weights_.find(prefix + "key.weight") != weights_.end() ? 
                      weights_[prefix + "key.weight"] : nullptr;
    ggml_tensor* Wr = weights_.find(prefix + "receptance.weight") != weights_.end() ? 
                      weights_[prefix + "receptance.weight"] : nullptr;
    ggml_tensor* Wv = weights_.find(prefix + "value.weight") != weights_.end() ? 
                      weights_[prefix + "value.weight"] : nullptr;
    
    // If weights not found, return input unchanged
    if (!time_mix_k || !time_mix_r || !Wk || !Wr || !Wv) {
        return x;
    }
    
    // Get previous token state
    ggml_tensor* last_x = state_ && layer_idx < state_->state_pp_.size() ? 
                          state_->state_pp_[layer_idx] : nullptr;
    
    if (!last_x) {
        // First token, use zeros
        last_x = ggml_new_tensor_1d(context_->get(), GGML_TYPE_F32, n_embd_);
        ggml_set_zero(last_x);
    }
    
    // Interpolate current and previous token: x * mix + last_x * (1 - mix)
    // Rewritten as: last_x + (x - last_x) * mix
    auto* x_diff_k = ggml_sub(context_->get(), x, last_x);
    auto* x_diff_k_scaled = ggml_mul(context_->get(), x_diff_k, time_mix_k);
    auto* xk_interp = ggml_add(context_->get(), last_x, x_diff_k_scaled);
    
    auto* x_diff_r = ggml_sub(context_->get(), x, last_x);
    auto* x_diff_r_scaled = ggml_mul(context_->get(), x_diff_r, time_mix_r);
    auto* xr_interp = ggml_add(context_->get(), last_x, x_diff_r_scaled);
    
    // Compute k and r vectors
    auto* k = ggml_mul_mat(context_->get(), Wk, xk_interp);
    auto* r = ggml_mul_mat(context_->get(), Wr, xr_interp);
    
    // Apply squared ReLU: max(k, 0)^2
    auto* k_relu = ggml_relu(context_->get(), k);
    auto* k_squared = ggml_sqr(context_->get(), k_relu);
    
    // Apply value transformation: vk = Wv @ k_squared
    auto* vk = ggml_mul_mat(context_->get(), Wv, k_squared);
    
    // Apply receptance gating: output = sigmoid(r) * vk
    auto* r_sigmoid = ggml_sigmoid(context_->get(), r);
    auto* output = ggml_mul(context_->get(), r_sigmoid, vk);
    
    // Update state (store current x for next token)
    if (state_ && layer_idx < state_->state_pp_.size()) {
        state_->state_pp_[layer_idx] = x;
    }
    
    return output;
}

ggml_tensor* RWKVWrapper::layerNorm(ggml_tensor* x, const std::string& weight_name, 
                                    const std::string& bias_name) {
    ggml_tensor* weight = weights_.find(weight_name) != weights_.end() ? 
                         weights_[weight_name] : nullptr;
    ggml_tensor* bias = weights_.find(bias_name) != weights_.end() ? 
                       weights_[bias_name] : nullptr;
    
    if (!weight) {
        // If no weight, just return input
        return x;
    }
    
    // Use GGML's layer norm operation
    auto* normed = ggml_norm(context_->get(), x, 1e-5f);
    
    // Scale by weight
    normed = ggml_mul(context_->get(), normed, weight);
    
    // Add bias if present
    if (bias) {
        normed = ggml_add(context_->get(), normed, bias);
    }
    
    return normed;
}

int RWKVWrapper::sampleToken(ggml_tensor* logits, float temperature, float top_p) {
    // Get logits data
    float* logits_data = ggml_get_data_f32(logits);
    size_t n_logits = ggml_nelements(logits);
    
    if (n_logits != static_cast<size_t>(n_vocab_)) {
        std::cerr << "Logits size mismatch: " << n_logits << " vs " << n_vocab_ << std::endl;
        return 0;
    }
    
    // Apply temperature
    if (temperature != 1.0f && temperature > 0.0f) {
        for (size_t i = 0; i < n_logits; i++) {
            logits_data[i] /= temperature;
        }
    }
    
    // Compute softmax
    float max_logit = *std::max_element(logits_data, logits_data + n_logits);
    float sum = 0.0f;
    std::vector<float> probs(n_logits);
    
    for (size_t i = 0; i < n_logits; i++) {
        probs[i] = std::exp(logits_data[i] - max_logit);
        sum += probs[i];
    }
    
    for (size_t i = 0; i < n_logits; i++) {
        probs[i] /= sum;
    }
    
    // Top-p sampling
    if (top_p < 1.0f) {
        // Create index-probability pairs
        std::vector<std::pair<float, int>> prob_idx;
        for (size_t i = 0; i < n_logits; i++) {
            prob_idx.push_back({probs[i], static_cast<int>(i)});
        }
        
        // Sort by probability (descending)
        std::sort(prob_idx.begin(), prob_idx.end(), 
                 [](const auto& a, const auto& b) { return a.first > b.first; });
        
        // Compute cumulative probability and find cutoff
        float cumsum = 0.0f;
        size_t cutoff = 0;
        for (size_t i = 0; i < prob_idx.size(); i++) {
            cumsum += prob_idx[i].first;
            cutoff = i + 1;
            if (cumsum >= top_p) {
                break;
            }
        }
        
        // Zero out probabilities below cutoff
        std::fill(probs.begin(), probs.end(), 0.0f);
        for (size_t i = 0; i < cutoff; i++) {
            probs[prob_idx[i].second] = prob_idx[i].first;
        }
        
        // Renormalize
        sum = 0.0f;
        for (float p : probs) sum += p;
        for (float& p : probs) p /= sum;
    }
    
    // Sample from distribution
    std::discrete_distribution<int> dist(probs.begin(), probs.end());
    return dist(rng_);
}

ggml_tensor* RWKVWrapper::forward(ggml_tensor* input) {
    return forwardWithLogits(input);
}

} // namespace bolt
