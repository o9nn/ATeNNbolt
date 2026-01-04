
#ifndef BOLT_GUI_COMPONENTS_HPP
#define BOLT_GUI_COMPONENTS_HPP

#include <string>
#include <vector>
#include <memory>

namespace bolt {

struct BracketMatch {
    size_t openPosition;
    size_t closePosition;
    char bracket;
};

class Workbench {
public:
    Workbench() = default;
    virtual ~Workbench() = default;
    
    void setCurrentProject(const std::string& path) { currentProject_ = path; }
    std::string getCurrentProject() const { return currentProject_; }
    
private:
    std::string currentProject_;
};

class Widget {
public:
    virtual ~Widget() = default;
    virtual void render() = 0;
};

class Button : public Widget {
private:
    std::string label_;
    bool enabled_{true};

public:
    explicit Button(const std::string& label) : label_(label) {}
    void render() override;
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
};

class Panel : public Widget {
private:
    std::vector<std::shared_ptr<Widget>> children_;

public:
    void addWidget(std::shared_ptr<Widget> widget);
    void render() override;
};

class TextInput : public Widget {
private:
    std::string text_;
    std::string placeholder_;

public:
    explicit TextInput(const std::string& placeholder = "") : placeholder_(placeholder) {}
    void render() override;
    void setText(const std::string& text) { text_ = text; }
    std::string getText() const { return text_; }
};

} // namespace bolt

#endif // BOLT_GUI_COMPONENTS_HPP
