
#pragma once
#include "editor/GUI/item.hpp"
#include "rounded_rectangle.hpp"
#include "editor/transition.hpp"
#include "text_label.hpp"


namespace GUI
{

using ButtonCallBack = std::function<void()>;

struct DefaultButton : public GUI::Item
{
    ButtonCallBack click_callback = [](){};
    trn::Transition<sf::Vector3f> color;
    
    DefaultButton(sf::Vector2f size_ = {}, sf::Vector2f position_ = {})
        : GUI::Item(size_, position_)
    {}
    
    DefaultButton(ButtonCallBack callback, sf::Vector2f size_ = {}, sf::Vector2f position_ = {})
        : GUI::Item(size_, position_)
        , click_callback(callback)
    {}

    void onClick(sf::Vector2f, sf::Mouse::Button) override
    {
        click_callback();
    }
};

struct Button : public DefaultButton
{
    SPtr<TextLabel> label;
    // MS02: einheitlicher Look für alle Buttons - transparente Füllung, grauer Rahmen,
    // hellgraue Schrift. Einzelne Buttons überschreiben das gezielt (z.B. Start/Pause-
    // Schriftfarbe, Colony-Slot-Rahmenfarbe)
    sf::Color background_color = sf::Color::Transparent;
    float outline_thickness = -2.0f;
    sf::Color outline_color = sf::Color(60, 60, 60); // MS02: inaktiv dunkelgrau wie der Sidebar-Rahmen
    float horizontal_padding = 8.0f; // MS02: etwas Luft links/rechts vom Text statt textgenauer Breite

    Button(const std::string& text, ButtonCallBack callback)
        : DefaultButton(callback)
    {
        label = create<TextLabel>(text, 11); // MS02: Schriftgröße auf 11px reduziert
        label->setColor(sf::Color(140, 140, 140)); // MS02: dunkler als vorher (200) - wirkte zu blass/ausgewaschen
        label->catch_event = false;
        addItem(label);
        fitWidth(); // MS02: Breite passt sich exakt dem Text an, nicht mehr/weniger
    }

    // MS02: Breite exakt an den aktuellen Label-Text anpassen - direkt nach dem
    // Konstruktor aufgerufen, und erneut nötig, wenn sich der Text später ändert
    // (z.B. Add/Remove-Toggle), da sich die Textbreite dann ändert
    void fitWidth()
    {
        setWidth(label->size.x + 2.0f * horizontal_padding);
    }

    void onSizeChange() override
    {
        label->setSize(size);
    }

    void onPositionChange() override
    {
        label->setPosition({});
    }

    void render(sf::RenderTarget& target) override
    {
        sf::RectangleShape background(size);
        background.setPosition(position);
        background.setFillColor(background_color);
        background.setOutlineThickness(outline_thickness);
        background.setOutlineColor(outline_color);
        GUI::Item::draw(target, background);
    }
};

}
