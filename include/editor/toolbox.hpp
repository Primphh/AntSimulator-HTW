#pragma once
#include "GUI/button.hpp"
#include "editor/color_picker/color_picker.hpp"
#include "editor/GUI/named_container.hpp"
#include "editor/GUI/toggle.hpp"
#include "slider.hpp"
#include "common/dynamic_blur.hpp"


namespace edtr
{

struct Toolbox : public GUI::NamedContainer
{
    // MS02: Breite offen/eingeklappt
    static constexpr float opened_width = 300.0f;
    static constexpr float closed_width = 50.0f;
    bool collapsed = false;
    // MS02: wird nach jedem Ein-/Ausklappen aufgerufen, damit EditorScene das
    // Spielfeld-Layout (und die Sidebar-Höhe) neu berechnen kann
    std::function<void()> on_layout_changed = nullptr;
    // MS02: Titeltext merken, um ihn beim Ausklappen wiederherzustellen (siehe setCollapsed)
    // - static statt normales Member, da sonst beim Konstruieren vor der Basisklasse
    // (NamedContainer-Konstruktor unten) noch uninitialisiert verwendet würde
    static constexpr const char* title = "AntSimulator v2.0";

    explicit
    Toolbox(sf::Vector2f size_, sf::Vector2f position_ = {})
        : GUI::NamedContainer(title, GUI::Container::Orientation::Vertical) // MS02: Sidebar-Titel umbenannt
    {
        padding = 14.0f; // mehr Abstand zwischen Sidebar-Inhalt und Sidebar-Rand (vorher 7)
        label->setCharacterSize(15); // MS02: 1px kleiner als vorher (16 -> 15)
        label->setColor(sf::Color(251, 227, 214)); // MS02: #FBE3D6
        header->addItem(create<GUI::EmptyItem>());
        auto tools_toggle = create<GUI::IconToggle>("res/close-sidebar.png", "res/open-sidebar.png");
        tools_toggle->setWidth(20.0f);
        tools_toggle->setHeight(20.0f);
        tools_toggle->setState(true);
        // MS02: Einziger noch funktionaler Toggle - klappt die Sidebar nach links ein (50px)/aus (300px)
        watch(tools_toggle, [this, tools_toggle]() {
            setCollapsed(!tools_toggle->state);
        });
        header->addItem(tools_toggle);

        setPosition(position_);
        background_intensity = 0; // MS02: schwarz statt hellgrau
        setWidth(size_.x);
        setHeight(size_.y); // MS02: Sidebar feste Höhe (volle Fensterhöhe), auch wenn eingeklappt
    }

    // MS02: Sidebar ein-/ausklappen
    void setCollapsed(bool should_collapse)
    {
        collapsed = should_collapse;
        if (collapsed) {
            hideRoot();
            label->setText(""); // MS02: Titel ausblenden - kein Platz mehr neben dem Klapp-Icon
        } else {
            showRoot();
            label->setText(title);
        }
        setWidth(collapsed ? closed_width : opened_width);
        if (on_layout_changed) {
            on_layout_changed();
        }
    }

//    void render(sf::RenderTarget& target) override
//    {
//        sf::RectangleShape background(size);
//        background.setPosition(position);
//        background.setFillColor(sf::Color(100, 100, 100, 200));
//        GUI::Item::draw(target, background);
//    }
};

}
