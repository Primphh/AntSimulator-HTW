#pragma once
#include "editor/tool_selector.hpp"
#include "editor/GUI/toggle.hpp"


namespace edtr
{

// MS02: ersetzt den alten "Full Speed"-Toggle (nur an/aus) durch vier auswählbare
// Geschwindigkeitsstufen, ähnlich der Brush-Auswahl in ToolSelector
struct SpeedSelector : public GUI::Container
{
    SPtr<ToolOption> speed_05x;
    SPtr<ToolOption> speed_1x;
    SPtr<ToolOption> speed_3x;
    SPtr<ToolOption> speed_5x;
    float current_speed = 1.0f;

    SpeedSelector()
        : GUI::Container(Container::Orientation::Horizontal)
    {
        padding = 6.0f;
        spacing = 4.0f;
        // MS02: "Speed" + Stufen jetzt eine eigene Box statt nur lose Buttons - Box
        // passt sich exakt ihrem Inhalt an statt die restliche Zeile zu füllen
        size_type = {GUI::Size::FitContent, GUI::Size::FitContent};

        auto label = create<GUI::TextLabel>("Speed", 12);
        label->setColor(sf::Color(200, 200, 200)); // MS02: grau statt weiß
        label->setAlignment(GUI::Alignment::Left);
        Container::addItem(label);

        speed_05x = create<ToolOption>("0.5x", [this](){ select(0.5f); });
        speed_1x  = create<ToolOption>("1x",   [this](){ select(1.0f); });
        speed_3x  = create<ToolOption>("5x",   [this](){ select(5.0f); });
        speed_5x  = create<ToolOption>("10x",  [this](){ select(10.0f); });
        for (const auto& button : {speed_05x, speed_1x, speed_3x, speed_5x}) {
            // MS02: Breite passt sich automatisch dem Text an (siehe Button::fitWidth())
            button->setHeight(26.0f); // MS02: Höhe um ca. 30% erhöht (20 -> 26)
            button->outline_thickness = 0.0f; // MS02: kein eigener Rahmen mehr - die Box drumherum reicht
            button->show_selection_outline = false; // MS02: kein weißer Rahmen bei Auswahl - nur die Schriftfarbe zeigt's an
            button->label->setCharacterSize(12); // MS02: 1px kleiner (13 -> 12)
            button->fitWidth(); // MS02: Breite an die neue Schriftgröße anpassen
            // MS02: Text nach dem endgültigen Sizing nochmal zentrieren - setCharacterSize()
            // setzt die Label-Höhe zwischendurch auf reine Textgröße zurück (siehe TextLabel::
            // updateSize()), wodurch sich der Text sonst nicht auf der finalen Button-Höhe zentriert
            button->label->setAlignment(GUI::Alignment::Center);
            Container::addItem(button);
        }

        select(1.0f); // MS02: 1x ist die Standardgeschwindigkeit
    }

    // MS02: grauer Rahmen um die ganze "Speed"-Box, gleicher Look wie die Size-Box
    void render(sf::RenderTarget& target) override
    {
        sf::RectangleShape background(size);
        background.setPosition(position);
        background.setFillColor(sf::Color::Transparent);
        background.setOutlineThickness(-2.0f);
        background.setOutlineColor(sf::Color(60, 60, 60)); // MS02: gleiches Grau wie der Sidebar-Rahmen
        GUI::Item::draw(target, background);
    }

    void reset()
    {
        speed_05x->reset();
        speed_1x->reset();
        speed_3x->reset();
        speed_5x->reset();
    }

    void select(float speed)
    {
        reset();
        current_speed = speed;
        if (speed == 0.5f)       { speed_05x->select(); }
        else if (speed == 5.0f)  { speed_3x->select(); }
        else if (speed == 10.0f) { speed_5x->select(); }
        else                     { speed_1x->select(); }
        notifyChanged();
    }
};

struct TimeController : public GUI::NamedContainer
{
    enum class State
    {
        Play,
        Pause,
    };

    State current_state = State::Pause;

    SPtr<ToolOption> tool_play_pause;
    SPtr<SpeedSelector> speed_selector;

    TimeController()
            : GUI::NamedContainer("Time Control", Container::Orientation::Horizontal)
            , current_state(State::Pause)
    {
        // MS02: x bleibt Auto (Standard) statt FitContent, damit die Box wie die anderen
        // Sidebar-Abschnitte über die volle Breite geht und linksbündig sitzt, statt
        // mittig im Toolbox-Inhalt zu schweben
        size_type.y       = GUI::Size::FitContent;
        root->size_type.y = GUI::Size::FitContent;

        // MS02: Play/Pause zu einem einzigen Button zusammengelegt - Klick schaltet
        // zwischen beiden Zuständen um statt zwei getrennte Buttons anzuzeigen
        tool_play_pause = create<ToolOption>("START", [this](){
            select(current_state == State::Pause ? State::Play : State::Pause);
        });
        // MS02: Breite passt sich automatisch dem Text an (siehe Button::fitWidth())

        speed_selector = create<SpeedSelector>();
        watch(speed_selector, [this]{
            notifyChanged();
        });

        // MS02: Höhe explizit fixieren statt Auto/Stretch zu überlassen - sonst übernimmt der
        // Button beim allerersten Layout-Durchlauf eine zu kleine Zwischenhöhe (siehe SpeedSelector
        // Box, die ihre FitContent-Höhe erst NACH diesem Button in der Reihe berechnet) und der
        // "START"-Text zentriert sich dadurch auf eine falsche (zu kleine) Boxhöhe
        tool_play_pause->setHeight(speed_selector->size.y);

        // Add items
        addItem(tool_play_pause);
        // MS02: Auto-breiter Spacer schiebt die Speed-Box rechtsbündig, damit der Abstand
        // zum rechten Sidebar-Rand dem Abstand der Überschriften zum linken Rand entspricht
        addItem(create<GUI::EmptyItem>());
        addItem(speed_selector);
        // Default selection
        select(State::Pause);
    }

    void select(State option)
    {
        current_state = option;
        switch (option) {
            case State::Pause:
                tool_play_pause->label->setText("START");
                tool_play_pause->fitWidth(); // MS02: Breite an neuen Text anpassen
                tool_play_pause->label->setColor(sf::Color::Green); // MS02: Ausnahme: grüne Schrift
                break;
            case State::Play:
                tool_play_pause->label->setText("PAUSE");
                tool_play_pause->fitWidth(); // MS02: Breite an neuen Text anpassen
                tool_play_pause->label->setColor(sf::Color::Red); // MS02: Ausnahme: rote Schrift
                break;
        }
        // MS02: siehe SpeedSelector - setText() setzt die Label-Höhe zwischendurch auf reine
        // Textgröße zurück. Normalerweise stellt der Container das über eine Resize-Kaskade
        // wieder her, sobald sich die Button-Breite ändert - bleibt der Text aber gleich (z.B.
        // beim allerersten Aufruf hier mit "START"), ändert sich die Breite nicht und die
        // Kaskade bleibt aus. Deshalb Label-Größe hier explizit zurücksetzen, bevor zentriert wird.
        tool_play_pause->label->setSize(tool_play_pause->size);
        tool_play_pause->label->setAlignment(GUI::Alignment::Center);
        notifyChanged();
    }
};

}
