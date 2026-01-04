#ifndef BOLT_AI_GGML_HPP
#define BOLT_AI_GGML_HPP

// Include the real GGML library
#include <ggml.h>
#include <stdexcept>

// Add any bolt-specific extensions or utilities here
namespace bolt {

// Utility functions for working with GGML
class GGMLContext {
public:
    explicit GGMLContext(size_t mem_size) {
        ggml_init_params params = {
            .mem_size = mem_size,
            .mem_buffer = nullptr,
            .no_alloc = false,
        };
        ctx_ = ggml_init(params);
        if (!ctx_) {
            throw std::runtime_error("Failed to initialize GGML context");
        }
    }
    
    ~GGMLContext() {
        if (ctx_) {
            ggml_free(ctx_);
        }
    }
    
    ggml_context* get() { return ctx_; }
    
    // Non-copyable
    GGMLContext(const GGMLContext&) = delete;
    GGMLContext& operator=(const GGMLContext&) = delete;
    
    // Movable
    GGMLContext(GGMLContext&& other) noexcept : ctx_(other.ctx_) {
        other.ctx_ = nullptr;
    }
    
    GGMLContext& operator=(GGMLContext&& other) noexcept {
        if (this != &other) {
            if (ctx_) {
                ggml_free(ctx_);
            }
            ctx_ = other.ctx_;
            other.ctx_ = nullptr;
        }
        return *this;
    }

private:
    ggml_context* ctx_ = nullptr;
};

} // namespace bolt

#endif // BOLT_AI_GGML_HPP