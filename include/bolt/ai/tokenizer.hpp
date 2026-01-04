
#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "bolt/core/thread_safety.hpp"

namespace bolt {

class Tokenizer {
private:
    ThreadSafe<std::unordered_map<std::string, int>> vocab_;
    ThreadSafe<std::unordered_map<int, std::string>> reverse_vocab_;
    int next_token_id_;

    Tokenizer() : next_token_id_(0) {
        // Initialize with common tokens
        addToken("<s>", 0);
        addToken("</s>", 1);
        addToken("<pad>", 2);
        addToken("<unk>", 3);
    }

public:
    static Tokenizer& getInstance() {
        static Tokenizer instance;
        return instance;
    }

    void addToken(const std::string& token, int id) {
        vocab_.write([&](auto& vocab) {
            vocab[token] = id;
        });
        reverse_vocab_.write([&](auto& rev_vocab) {
            rev_vocab[id] = token;
        });
        next_token_id_ = std::max(next_token_id_, id + 1);
    }

    int addNewToken(const std::string& token) {
        int id = next_token_id_++;
        addToken(token, id);
        return id;
    }

    std::vector<int> encode(const std::string& text) {
        std::vector<int> tokens;
        std::string current_token;
        
        for (size_t i = 0; i < text.length(); i++) {
            current_token += text[i];
            bool found = vocab_.read([&](const auto& vocab) {
                return vocab.find(current_token) != vocab.end();
            });

            if (!found) {
                if (current_token.length() > 1) {
                    std::string prev_token = current_token.substr(0, current_token.length() - 1);
                    int token_id = vocab_.read([&](const auto& vocab) {
                        return vocab.at(prev_token);
                    });
                    tokens.push_back(token_id);
                    current_token = text[i];
                }
            } else if (i == text.length() - 1) {
                int token_id = vocab_.read([&](const auto& vocab) {
                    return vocab.at(current_token);
                });
                tokens.push_back(token_id);
            }
        }
        
        return tokens;
    }

    std::string decode(const std::vector<int>& tokens) {
        std::string text;
        for (int token : tokens) {
            reverse_vocab_.read([&](const auto& rev_vocab) {
                if (rev_vocab.find(token) != rev_vocab.end()) {
                    text += rev_vocab.at(token);
                }
            });
        }
        return text;
    }
};

} // namespace bolt

#endif
