
#ifndef GGML_WRAPPER_HPP
#define GGML_WRAPPER_HPP

#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include <thread>
#include "ggml.h"

namespace bolt {

class GGMLException : public std::runtime_error {
public:
    explicit GGMLException(const std::string& msg) : std::runtime_error(msg) {}
};

class GGMLContext {
public:
    GGMLContext(size_t mem_size) {
        ctx_ = ggml_init({mem_size});
        if (!ctx_) throw GGMLException("Failed to initialize GGML context");
    }
    
    ~GGMLContext() {
        if (ctx_) ggml_free(ctx_);
    }
    
    ggml_context* get() { return ctx_; }

private:
    ggml_context* ctx_;
};

class GGMLModel {
public:
    GGMLModel(const std::string& path, size_t ctx_size = 2048, int n_threads = -1) 
        : ctx_size_(ctx_size)
        , n_threads_(n_threads == -1 ? std::thread::hardware_concurrency() : n_threads) {
        context_ = std::make_unique<GGMLContext>(ctx_size * sizeof(float) * 4);
        loadModel(path);
    }

    void loadModel(const std::string& path) {
        // Load model weights and architecture
        // This is a placeholder - actual implementation depends on model format
        model_path_ = path;
        if (!loadWeights(path)) {
            throw GGMLException("Failed to load model from " + path);
        }
    }

    std::string generate(const std::string& prompt) {
        // Set up computation graph
        ggml_cgraph* gf = ggml_new_graph(context_->get());
        
        // Convert prompt to tokens
        auto tokens = tokenize(prompt);
        
        // Run inference
        ggml_tensor* output = runInference(gf, tokens);
        
        // Convert output back to text
        return detokenize(output);
    }

    void setContext(size_t ctx_size) {
        ctx_size_ = ctx_size;
    }

    void setThreads(int n_threads) {
        n_threads_ = n_threads;
    }

private:
    std::unique_ptr<GGMLContext> context_;
    std::string model_path_;
    size_t ctx_size_;
    int n_threads_;

    bool loadWeights(const std::string& path) {
        FILE* f = std::fopen(path.c_str(), "rb");
        if (!f) {
            throw GGMLException("Failed to open model file: " + path);
        }

        // Read magic number
        uint32_t magic;
        fread(&magic, sizeof(magic), 1, f);
        if (magic != 0x67676d6c) { // "ggml" in hex
            fclose(f);
            throw GGMLException("Invalid model file format");
        }

        // Read version
        uint32_t version;
        fread(&version, sizeof(version), 1, f);
        if (version != 1) {
            fclose(f);
            throw GGMLException("Unsupported model version");
        }

        // Read model parameters
        uint32_t n_vocab, n_embd, n_mult, n_head, n_layer;
        fread(&n_vocab, sizeof(n_vocab), 1, f);
        fread(&n_embd, sizeof(n_embd), 1, f);
        fread(&n_mult, sizeof(n_mult), 1, f);
        fread(&n_head, sizeof(n_head), 1, f);
        fread(&n_layer, sizeof(n_layer), 1, f);

        n_layers_ = n_layer;

        // Helper function to load a tensor
        auto load_tensor = [&](const std::string& name, int64_t ne0, int64_t ne1 = 1) {
            auto* tensor = ggml_new_tensor_2d(context_->get(), GGML_TYPE_F32, ne0, ne1);
            size_t size = ggml_nbytes(tensor);
            fread(tensor->data, size, 1, f);
            weights_[name] = tensor;
            return tensor;
        };

        // Load embeddings
        load_tensor("wte", n_embd, n_vocab);  // Token embeddings
        load_tensor("wpe", n_embd, 1024);     // Position embeddings

        // Load transformer layers
        for (uint32_t i = 0; i < n_layer; ++i) {
            std::string prefix = "h." + std::to_string(i) + ".";
            
            // Attention
            load_tensor(prefix + "attn.q", n_embd, n_embd);
            load_tensor(prefix + "attn.k", n_embd, n_embd);
            load_tensor(prefix + "attn.v", n_embd, n_embd);
            
            // Layer norm
            load_tensor(prefix + "ln.weight", n_embd);
            
            // MLP
            load_tensor(prefix + "mlp.fc", n_embd * n_mult, n_embd);
            load_tensor(prefix + "mlp.proj", n_embd, n_embd * n_mult);
        }

        // Load LM head
        load_tensor("lm_head", n_vocab, n_embd);

        fclose(f);
        return true;
    }

    std::vector<int> tokenize(const std::string& text) {
        return tokenizer_.encode(text);
    }

    std::string detokenize(ggml_tensor* output) {
        std::vector<int> tokens;
        float* logits = (float*)output->data;
        
        // Convert logits to token ids
        for (int i = 0; i < output->ne[0]; i++) {
            int token_id = argmax(logits + i * output->ne[1], output->ne[1]);
            tokens.push_back(token_id);
        }
        
        return tokenizer_.decode(tokens);
    }
    
private:
    Tokenizer tokenizer_;
    
    int argmax(const float* array, size_t size) {
        return std::max_element(array, array + size) - array;
    }

    ggml_tensor* runInference(ggml_cgraph* gf, const std::vector<int>& tokens) {
        if (tokens.empty()) return nullptr;

        const int n_past = 0;
        const int N = tokens.size();

        // Create the network
        auto* embd = ggml_new_tensor_1d(context_->get(), GGML_TYPE_I32, N);
        memcpy(embd->data, tokens.data(), N * sizeof(int32_t));

        // Create attention mask
        auto* mask = ggml_new_tensor_2d(context_->get(), GGML_TYPE_F32, N, N);
        float* mask_data = (float*)mask->data;
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                mask_data[i*N + j] = j <= i ? 1.0f : 0.0f;
            }
        }

        // Forward pass
        auto* hidden = ggml_mul_mat(context_->get(), weights_["wte"], embd); // Token embeddings
        hidden = ggml_add(context_->get(), hidden, 
                         ggml_repeat(context_->get(), weights_["wpe"], hidden)); // Position embeddings

        // Transformer layers
        for (int i = 0; i < n_layers_; i++) {
            auto* attn = attention_block(hidden, mask, i);
            auto* mlp = mlp_block(attn, i);
            hidden = ggml_add(context_->get(), attn, mlp);
            hidden = layer_norm(hidden, i);
        }

        // Language model head
        auto* logits = ggml_mul_mat(context_->get(), weights_["lm_head"], hidden);
        
        ggml_build_forward_expand(gf, logits);
        ggml_graph_compute_with_ctx(context_->get(), gf, n_threads_);

        return logits;
    }

private:
    std::unordered_map<std::string, ggml_tensor*> weights_;
    int n_layers_ = 12; // Default, should be set based on model config

    ggml_tensor* attention_block(ggml_tensor* x, ggml_tensor* mask, int layer) {
        const std::string prefix = "h." + std::to_string(layer) + ".";
        
        auto* q = ggml_mul_mat(context_->get(), weights_[prefix + "attn.q"], x);
        auto* k = ggml_mul_mat(context_->get(), weights_[prefix + "attn.k"], x);
        auto* v = ggml_mul_mat(context_->get(), weights_[prefix + "attn.v"], x);

        // Scaled dot-product attention
        auto* qk = ggml_mul_mat(context_->get(), k, q);
        qk = ggml_scale_inplace(context_->get(), qk, 1.0f/sqrt(static_cast<float>(x->ne[0])));
        qk = ggml_mul_mat(context_->get(), mask, qk);
        qk = ggml_soft_max_inplace(context_->get(), qk);

        return ggml_mul_mat(context_->get(), v, qk);
    }

    ggml_tensor* mlp_block(ggml_tensor* x, int layer) {
        const std::string prefix = "h." + std::to_string(layer) + ".";

        auto* intermediate = ggml_mul_mat(context_->get(), weights_[prefix + "mlp.fc"], x);
        intermediate = ggml_gelu(context_->get(), intermediate);
        return ggml_mul_mat(context_->get(), weights_[prefix + "mlp.proj"], intermediate);
    }

    ggml_tensor* layer_norm(ggml_tensor* x, int layer) {
        const std::string prefix = "h." + std::to_string(layer) + ".";
        return ggml_norm(context_->get(), x, weights_[prefix + "ln.weight"]);
    }
};

class GGMLWrapper {
public:
    static GGMLWrapper& getInstance() {
        static GGMLWrapper instance;
        return instance;
    }
    
    void enableRWKV(bool enable = true) {
        use_rwkv_ = enable;
        if (enable && !rwkv_initialized_) {
            RWKVWrapper::getInstance().initialize("models/rwkv.bin");
            rwkv_initialized_ = true;
        }
    }

    std::string generateResponse(const std::string& prompt) {
        if (use_rwkv_) {
            auto* tensor = RWKVWrapper::getInstance().forward(tokenize(prompt));
            return detokenize(tensor);
        }
        // Original transformer-based generation
        return model_->generate(prompt);
    }

private:
    bool use_rwkv_ = false;
    bool rwkv_initialized_ = false;
    
    void initialize(const std::string& model_path, enum ggml_type quantize_type = GGML_TYPE_F32) {
        try {
            model_ = std::make_unique<GGMLModel>(model_path);
            if (quantize_type != GGML_TYPE_F32) {
                quantizeModel(quantize_type);
            }
        } catch (const GGMLException& e) {
            throw;
        }
    }
    
    void quantizeModel(enum ggml_type target_type) {
        if (!model_) throw GGMLException("Model not initialized");
        
        for (auto& [name, tensor] : weights_) {
            if (tensor->type != GGML_TYPE_F32) continue;
            auto* quantized = TensorUtils::quantizeTensor(context_->get(), tensor, target_type);
            float error = TensorUtils::calculateQuantizationError(tensor, quantized);
            std::cout << "Quantized " << name << " (error: " << error << ")\n";
            weights_[name] = quantized;
        }
    }
    
    std::string generateResponse(const std::string& prompt) {
        if (!model_) throw GGMLException("Model not initialized");
        return model_->generate(prompt);
    }

private:
    GGMLWrapper() = default;
    std::unique_ptr<GGMLModel> model_;
};

} // namespace bolt

#endif


    bool saveModel(const std::string& path) {
        FILE* f = std::fopen(path.c_str(), "wb");
        if (!f) {
            throw GGMLException("Failed to open file for writing: " + path);
        }

        // Write magic number
        uint32_t magic = 0x67676d6c;  // "ggml" in hex
        fwrite(&magic, sizeof(magic), 1, f);

        // Write version
        uint32_t version = 1;
        fwrite(&version, sizeof(version), 1, f);

        // Write model parameters
        uint32_t n_vocab = weights_["wte"]->ne[1];
        uint32_t n_embd = weights_["wte"]->ne[0];
        uint32_t n_mult = weights_["h.0.mlp.fc"]->ne[0] / n_embd;
        uint32_t n_head = 12;  // Default, should match model config
        uint32_t n_layer = n_layers_;

        fwrite(&n_vocab, sizeof(n_vocab), 1, f);
        fwrite(&n_embd, sizeof(n_embd), 1, f);
        fwrite(&n_mult, sizeof(n_mult), 1, f);
        fwrite(&n_head, sizeof(n_head), 1, f);
        fwrite(&n_layer, sizeof(n_layer), 1, f);

        // Helper function to save a tensor
        auto save_tensor = [&](const std::string& name) {
            auto* tensor = weights_[name];
            if (!tensor) {
                throw GGMLException("Missing tensor: " + name);
            }
            size_t size = ggml_nbytes(tensor);
            fwrite(tensor->data, size, 1, f);
        };

        // Save embeddings
        save_tensor("wte");
        save_tensor("wpe");

        // Save transformer layers
        for (uint32_t i = 0; i < n_layer; ++i) {
            std::string prefix = "h." + std::to_string(i) + ".";
            
            // Attention
            save_tensor(prefix + "attn.q");
            save_tensor(prefix + "attn.k");
            save_tensor(prefix + "attn.v");
            
            // Layer norm
            save_tensor(prefix + "ln.weight");
            
            // MLP
            save_tensor(prefix + "mlp.fc");
            save_tensor(prefix + "mlp.proj");
        }

        // Save LM head
        save_tensor("lm_head");

        fclose(f);
        return true;
    }

