#pragma once
#include "button.hpp"
#include "rounded_rectangle.hpp"
#include "../transition.hpp"
#include "container.hpp"
#include "empty_item.hpp"


namespace GUI
{

struct Toggle : public Button
{
    bool state = false;
    trn::Transition<sf::Vector2f> toggle_position;
    trn::Transition<sf::Vector3f> toggle_color;
    sf::Vector3f color_on  = {255.0f, 255.0f, 255.0f};
    sf::Vector3f color_off = {255.0f, 255.0f, 255.0f}; // MS02: Aus-Zustand jetzt auch weißer Kreis statt grau

    std::function<void(bool)> on_state_changed = nullptr;

    float padding = 3.0f;

    Toggle()
        : Button("", nullptr)
    {
        click_callback = [this](){
            setState(!state);
        };

        setWidth(40.0f);
        setHeight(24.0f);

        toggle_position.setSpeed(5.0f);
        toggle_color.setSpeed(5.0f);

        updateColor(true);
    }

    void setState(bool s)
    {
        if (s == state) {
            return;
        }

        state = s;
        updateTogglePosition();
        updateColor();
        notifyChanged();
        if (on_state_changed) {
            on_state_changed(s);
        }
    }

    void onPositionChange() override
    {
        updateTogglePosition(true);
    }

    void updateTogglePosition(bool instant = false)
    {
        const float radius  = 0.5f * size.y - padding;
        const sf::Vector2f target_position = position + (state ? sf::Vector2f{size.x - radius - padding, size.y * 0.5f} : sf::Vector2f{radius + padding, size.y * 0.5f});
        if (instant) {
            toggle_position.setValueInstant(target_position);
        } else {
            toggle_position = target_position;
        }
    }

    void updateColor(bool instant = false)
    {
        if (instant) {
            toggle_color.setValueInstant(state ? color_on : color_off);
        } else {
            toggle_color = state ? color_on : color_off;
        }
    }

    void render(sf::RenderTarget& target) override
    {
        RoundedRectangle back(size, position, size.y);
        back.setFillColor(sf::Color(100, 100, 100)); // MS02: dunkler als vorher (200)
        GUI::Item::draw(target, back);

        const float radius  = 0.5f * size.y - padding;
        sf::CircleShape toggle(radius);
        toggle.setOrigin(radius, radius);
        toggle.setPosition(toggle_position);
        toggle.setFillColor(vec3ToColor(toggle_color.as()));
        GUI::Item::draw(target, toggle);
    }

    template<typename TCallback>
    void onStateChange(const TCallback&& callback)
    {
        on_state_changed = callback;
    }
};

// Toggle, das statt des Schalter-Kreises ein Icon zeigt (z.B. Sidebar ein-/ausklappen)
struct IconToggle : public Button
{
    bool state = false;
    std::shared_ptr<sf::Texture> texture_on;
    std::shared_ptr<sf::Texture> texture_off;

    std::function<void(bool)> on_state_changed = nullptr;

    IconToggle(const std::string& texture_on_path, const std::string& texture_off_path)
        : Button("", nullptr)
    {
        texture_on = std::make_shared<sf::Texture>();
        texture_on->loadFromFile(texture_on_path);
        texture_on->setSmooth(true);
        texture_off = std::make_shared<sf::Texture>();
        texture_off->loadFromFile(texture_off_path);
        texture_off->setSmooth(true);

        background_color  = sf::Color::Transparent;
        outline_thickness = 0.0f;

        click_callback = [this](){
            setState(!state);
        };
    }

    void setState(bool s)
    {
        if (s == state) {
            return;
        }

        state = s;
        notifyChanged();
        if (on_state_changed) {
            on_state_changed(s);
        }
    }

    void render(sf::RenderTarget& target) override
    {
        sf::Sprite sprite(state ? *texture_on : *texture_off);
        const sf::Vector2u texture_size = sprite.getTexture()->getSize();
        sprite.setScale(size.x / to<float>(texture_size.x), size.y / to<float>(texture_size.y));
        sprite.setPosition(position);
        draw(target, sprite);
    }

    template<typename TCallback>
    void onStateChange(const TCallback&& callback)
    {
        on_state_changed = callback;
    }
};

struct NamedToggle : public Container
{
    SPtr<Toggle> toggle;

    explicit
    NamedToggle(const std::string& name)
        // MS02: Horizontal statt Vertikal - Name links, Toggle rechts in derselben Zeile
        : Container(Container::Orientation::Horizontal)
    {
        padding = 0.0f;
        spacing = 8.0f;
        size_type.y = Size::FitContent;
        auto name_label = create<TextLabel>(name, 12);
        name_label->setColor(sf::Color::White); // MS02: heller Text für schwarzen Sidebar-Hintergrund
        name_label->setAlignment(Alignment::Left);
        // MS02: feste Breite, damit die Toggles in einer Spalte ausgerichtet sind,
        // egal wie lang der jeweilige Name ist
        name_label->setWidth(100.0f);
        name_label->auto_size_update = false;
        Container::addItem(name_label);

        // MS02: Auto-breiter Spacer schiebt den Toggle rechtsbündig, damit der Abstand
        // zum rechten Sidebar-Rand dem Abstand der Überschriften zum linken Rand entspricht
        // (gleiche Technik wie beim Sidebar-Klapp-Icon im Toolbox-Header)
        Container::addItem(create<EmptyItem>());

        toggle = create<Toggle>();
        Container::addItem(toggle);

        watch(toggle, [this](){
            notifyChanged();
        });
    }

    [[nodiscard]]
    bool getState() const
    {
        return toggle->state;
    }

    void setState(bool state) const
    {
        toggle->setState(state);
    }

    template<typename TCallback>
    void onStateChange(const TCallback&& callback)
    {
        toggle->on_state_changed = callback;
    }
};

}
