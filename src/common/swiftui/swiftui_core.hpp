#pragma once

#include "../stl.hpp"
#include "../ref.hpp"
#include "../uidefs.hpp"
#include <functional>
#include <memory>
#include <type_traits>
#include <any>

namespace iris {
namespace swiftui {

// Forward declarations
class AnyView;
class ViewModifier;
class EnvironmentValues;

// Core SwiftUI concepts
template<typename T>
concept ViewProtocol = requires(T t) {
    { t.body() } -> std::convertible_to<AnyView>;
};

// State management
template<typename T>
class State {
private:
    mutable T value_;
    mutable std::function<void()> onChange_;

public:
    State(T initial_value) : value_(std::move(initial_value)) {}
    
    const T& wrappedValue() const { return value_; }
    
    void set_wrappedValue(T new_value) {
        if (value_ != new_value) {
            value_ = std::move(new_value);
            if (onChange_) {
                onChange_();
            }
        }
    }
    
    void setOnChange(std::function<void()> callback) const {
        onChange_ = std::move(callback);
    }
    
    // Binding creation
    class Binding {
    private:
        State<T>* state_;
        
    public:
        Binding(State<T>* state) : state_(state) {}
        
        const T& wrappedValue() const { return state_->wrappedValue(); }
        void set_wrappedValue(T new_value) { state_->set_wrappedValue(std::move(new_value)); }
        
        operator const T&() const { return wrappedValue(); }
        Binding& operator=(const T& value) { set_wrappedValue(value); return *this; }
    };
    
    Binding projectedValue() { return Binding(this); }
};

template<typename T>
class Binding {
private:
    std::function<T()> getter_;
    std::function<void(T)> setter_;

public:
    Binding(std::function<T()> getter, std::function<void(T)> setter)
        : getter_(std::move(getter)), setter_(std::move(setter)) {}
    
    const T wrappedValue() const { return getter_(); }
    void set_wrappedValue(T new_value) { setter_(std::move(new_value)); }
    
    operator T() const { return wrappedValue(); }
    Binding& operator=(const T& value) { set_wrappedValue(value); return *this; }
};

// Environment system
class EnvironmentValues {
private:
    std::unordered_map<std::string, std::any> values_;

public:
    template<typename T>
    void set(const std::string& key, T value) {
        values_[key] = std::move(value);
    }
    
    template<typename T>
    T get(const std::string& key, T default_value = T{}) const {
        auto it = values_.find(key);
        if (it != values_.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (const std::bad_any_cast&) {
                return default_value;
            }
        }
        return default_value;
    }
};

// Color system
struct Color {
    float red, green, blue, alpha;
    
    Color(float r = 0, float g = 0, float b = 0, float a = 1.0f) 
        : red(r), green(g), blue(b), alpha(a) {}
    
    static Color black() { return Color(0, 0, 0); }
    static Color white() { return Color(1, 1, 1); }
    static Color red() { return Color(1, 0, 0); }
    static Color green() { return Color(0, 1, 0); }
    static Color blue() { return Color(0, 0, 1); }
    static Color clear() { return Color(0, 0, 0, 0); }
    static Color primary() { return Color(0, 0.48f, 1); }
    static Color secondary() { return Color(0.56f, 0.56f, 0.58f); }
};

// Font system
struct Font {
    string name;
    float size;
    bool bold;
    bool italic;
    
    Font(const string& font_name = "System", float font_size = 17.0f, bool is_bold = false, bool is_italic = false)
        : name(font_name), size(font_size), bold(is_bold), italic(is_italic) {}
    
    static Font system(float size = 17.0f) { return Font("System", size); }
    static Font title() { return Font("System", 34.0f, true); }
    static Font headline() { return Font("System", 17.0f, true); }
    static Font body() { return Font("System", 17.0f); }
    static Font caption() { return Font("System", 12.0f); }
    
    Font bold() const { return Font(name, size, true, italic); }
    Font italic() const { return Font(name, size, bold, true); }
};

// Edge insets for padding
struct EdgeInsets {
    float top, leading, bottom, trailing;
    
    EdgeInsets(float all = 0) : top(all), leading(all), bottom(all), trailing(all) {}
    EdgeInsets(float vertical, float horizontal) 
        : top(vertical), leading(horizontal), bottom(vertical), trailing(horizontal) {}
    EdgeInsets(float t, float l, float b, float tr) 
        : top(t), leading(l), bottom(b), trailing(tr) {}
    
    static EdgeInsets all(float value) { return EdgeInsets(value); }
};

// Alignment system
enum class HorizontalAlignment {
    leading, center, trailing
};

enum class VerticalAlignment {
    top, center, bottom, firstTextBaseline, lastTextBaseline
};

struct Alignment {
    HorizontalAlignment horizontal;
    VerticalAlignment vertical;
    
    Alignment(HorizontalAlignment h = HorizontalAlignment::center, 
              VerticalAlignment v = VerticalAlignment::center)
        : horizontal(h), vertical(v) {}
    
    static Alignment topLeading() { return Alignment(HorizontalAlignment::leading, VerticalAlignment::top); }
    static Alignment top() { return Alignment(HorizontalAlignment::center, VerticalAlignment::top); }
    static Alignment topTrailing() { return Alignment(HorizontalAlignment::trailing, VerticalAlignment::top); }
    static Alignment leading() { return Alignment(HorizontalAlignment::leading, VerticalAlignment::center); }
    static Alignment center() { return Alignment(HorizontalAlignment::center, VerticalAlignment::center); }
    static Alignment trailing() { return Alignment(HorizontalAlignment::trailing, VerticalAlignment::center); }
    static Alignment bottomLeading() { return Alignment(HorizontalAlignment::leading, VerticalAlignment::bottom); }
    static Alignment bottom() { return Alignment(HorizontalAlignment::center, VerticalAlignment::bottom); }
    static Alignment bottomTrailing() { return Alignment(HorizontalAlignment::trailing, VerticalAlignment::bottom); }
};

// Base view renderer interface
class ViewRenderer : public ref_counted {
public:
    virtual ~ViewRenderer() = default;
    virtual void render(const ui::rect& frame, ui::view* parent_view) = 0;
    virtual ui::frame_size intrinsicSize(const ui::frame_size& available_size) const = 0;
    virtual void setEnvironment(const EnvironmentValues& env) {}
};

// Type-erased view container
class AnyView {
private:
    ref_ptr<ViewRenderer> renderer_;

public:
    AnyView() = default;
    
    template<typename V>
    AnyView(V&& view) {
        if constexpr (ViewProtocol<V>) {
            renderer_ = new ViewRendererImpl<V>(std::forward<V>(view));
        } else {
            static_assert(ViewProtocol<V>, "Type must conform to ViewProtocol");
        }
    }
    
    AnyView(ref_ptr<ViewRenderer> renderer) : renderer_(std::move(renderer)) {}
    
    void render(const ui::rect& frame, ui::view* parent_view) {
        if (renderer_) {
            renderer_->render(frame, parent_view);
        }
    }
    
    ui::frame_size intrinsicSize(const ui::frame_size& available_size) const {
        if (renderer_) {
            return renderer_->intrinsicSize(available_size);
        }
        return {0, 0};
    }
    
    void setEnvironment(const EnvironmentValues& env) {
        if (renderer_) {
            renderer_->setEnvironment(env);
        }
    }
    
    bool isEmpty() const { return !renderer_; }

private:
    template<typename V>
    class ViewRendererImpl : public ViewRenderer {
    private:
        V view_;
        EnvironmentValues environment_;
        
    public:
        ViewRendererImpl(V view) : view_(std::move(view)) {}
        
        void render(const ui::rect& frame, ui::view* parent_view) override {
            if constexpr (ViewProtocol<V>) {
                AnyView body = view_.body();
                body.setEnvironment(environment_);
                body.render(frame, parent_view);
            }
        }
        
        ui::frame_size intrinsicSize(const ui::frame_size& available_size) const override {
            if constexpr (ViewProtocol<V>) {
                AnyView body = view_.body();
                return body.intrinsicSize(available_size);
            }
            return {0, 0};
        }
        
        void setEnvironment(const EnvironmentValues& env) override {
            environment_ = env;
        }
    };
};

// Empty view
class EmptyView {
public:
    AnyView body() const {
        return AnyView(ref_ptr<ViewRenderer>(new EmptyViewRenderer()));
    }

private:
    class EmptyViewRenderer : public ViewRenderer {
    public:
        void render(const ui::rect& frame, ui::view* parent_view) override {}
        ui::frame_size intrinsicSize(const ui::frame_size& available_size) const override {
            return {0, 0};
        }
    };
};

} // namespace swiftui
} // namespace iris


