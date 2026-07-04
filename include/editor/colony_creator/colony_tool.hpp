#pragma once
#include "editor/GUI/container.hpp"
#include "editor/GUI/button.hpp"
#include "editor/GUI/rounded_rectangle.hpp"
#include "editor/GUI/empty_item.hpp"
#include "editor/control_state.hpp"
#include "common/color_utils.hpp"


// MS02: ColonyTool ist jetzt eine permanente Zeile pro Slot (existiert von Anfang an,
// unabhängig davon ob eine Kolonie aktiv ist). "colony" ist ein leerer (default-
// konstruierter) civ::Ref, solange der Slot leer ist - operator bool() davon sagt, ob
// eine Kolonie existiert. activate()/deactivate() schalten dazwischen um, statt die
// ganze Zeile dynamisch ein-/auszubauen.
struct ColonyTool : GUI::Container
{
    civ::Ref<Colony> colony; // ungültig (default), solange der Slot leer ist
    ControlState& control_state;
    SPtr<GUI::Container> top_zone;
    SPtr<GUI::Button> toggle_button; // ehemals Farb-Swatch, jetzt Add/Remove
    SPtr<GUI::Button> focus_button;
    SPtr<GUI::Button> reposition_button;
    SPtr<GUI::Button> show_markers_button;
    // MS02: Focus/Reposition leuchten weiß, solange sie "aktiv" sind (gegenseitig
    // exklusiv) - Show Markers folgt stattdessen direkt "selected" (siehe update())
    SPtr<GUI::Button> active_action_button = nullptr;
    sf::Color background_color = sf::Color::Black; // MS02: schwarz statt hellgrau

    bool selected = false;
    std::function<void(int8_t)> on_select = [](int8_t){};
    // MS02: von ColonyCreator gesetzt - erzeugt/entfernt die Kolonie für diesen Slot
    std::function<void()> on_toggle = nullptr;

    static void setButtonActive(const SPtr<GUI::Button>& button, bool active)
    {
        button->outline_color = active ? sf::Color::White : sf::Color(60, 60, 60); // MS02: inaktiv dunkelgrau wie der Sidebar-Rahmen
        button->label->setColor(active ? sf::Color::White : sf::Color(140, 140, 140));
    }

    void setActiveActionButton(const SPtr<GUI::Button>& button)
    {
        if (active_action_button) {
            setButtonActive(active_action_button, false);
        }
        active_action_button = button;
        if (active_action_button) {
            setButtonActive(active_action_button, true);
        }
    }

    ColonyTool(sf::Color slot_color, ControlState& control_state_)
        : GUI::Container(Container::Orientation::Vertical)
        , control_state(control_state_)
    {
        // Container configuration
        // MS02: padding 5->0 - sonst bekommt top_zone (über das Cross-Axis-Stretching) ein
        // zusätzliches 5px-Inset links/rechts, wodurch Markers nicht ganz mit den anderen
        // Elementen am rechten Rand bündig abschließt. Vertikaler Abstand zu colony_stats
        // kommt weiterhin über "spacing" (8, Container-Standard, unverändert).
        padding = 0.0f;
        size_type.y = GUI::Size::FitContent;
        // Add the buttons (for add/remove, position, focus and remove)
        top_zone = create<GUI::Container>(GUI::Container::Orientation::Horizontal);
        top_zone->size_type.y = GUI::Size::FitContent;
        top_zone->padding = 0.0f;
        // MS02: etwas mehr Abstand zwischen Focus/Reposition/Markers (vorher 4)
        top_zone->spacing = 8.0f;
        this->addItem(top_zone);

        // MS02: ersetzt den alten Farb-Swatch - gleiche Größe/Position, aber jetzt mit
        // Add/Remove-Text und in der festen Slot-Farbe statt frei wählbar per Color-Picker.
        // Transparenter Hintergrund, nur ein farbiger Rahmen statt voller Füllung
        toggle_button = create<GUI::Button>("Add", [this](){
            if (on_toggle) { on_toggle(); }
        });
        toggle_button->setHeight(26.0f); // MS02: gleiche Höhe wie Focus/Reposition/Remove
        // MS02: Breite passt sich automatisch dem Text an (siehe Button::fitWidth())
        toggle_button->background_color = sf::Color::Transparent;
        toggle_button->outline_color = slot_color;
        toggle_button->outline_thickness = -2.0f;
        toggle_button->label->setColor(sf::Color::White); // MS02: Ausnahme: weiße Schrift
        top_zone->addItem(toggle_button, "toggle");

        // MS02: Auto-breiter Spacer schiebt Focus/Reposition/Markers rechtsbündig, damit
        // der Abstand zum rechten Sidebar-Rand dem Abstand der anderen Elemente entspricht
        top_zone->addItem(create<GUI::EmptyItem>());

        focus_button = create<GUI::Button>("Focus", [this](){
            if (!colony) { return; }
            if (active_action_button == focus_button) {
                setActiveActionButton(nullptr); // zweiter Klick: Focus wieder deselektieren
                return;
            }
            // MS02: Bugfix - falls vorher "Reposition" aktiv war, hängen in control_state
            // sonst noch dessen view_action/draw_action (referenzieren "colony" per Lambda-
            // Capture). Bleiben die nach einem späteren "Remove" aktiv, crasht es beim
            // nächsten Klick/Render auf der jetzt leeren civ::Ref - siehe ColonyTool::deactivate()
            control_state.resetCallbacks();
            setActiveActionButton(focus_button); // MS02: sichtbar machen, dass aktiviert
            control_state.requestFocus(colony->base.position, 2.0f);
        });
        focus_button->setHeight(26.0f); // MS02: Höhe um ca. 30% erhöht (20 -> 26)
        // MS02: Breite passt sich automatisch dem Text an (siehe Button::fitWidth())
        top_zone->addItem(focus_button);

        // MS02: "Set Position" -> "Reposition" umbenannt
        reposition_button = create<GUI::Button>("Reposition", [this](){
            if (!colony) { return; }
            setActiveActionButton(reposition_button); // MS02: sichtbar machen, dass aktiviert
            control_state.requestEditModeOff();
            // Preview callbacks
            control_state.draw_action = [this](sf::RenderTarget& target, const ViewportHandler& vp_handler) {
                sf::CircleShape c(colony->base.radius);
                c.setOrigin(c.getRadius(), c.getRadius());
                c.setPosition(vp_handler.getMouseWorldPosition());
                const sf::Color colony_color = colony->ants_color;
                c.setFillColor({colony_color.r, colony_color.g, colony_color.b, 100});
                target.draw(c, vp_handler.getRenderState());
            };
            control_state.view_action = [this](sf::Vector2f world_position) {
                const sf::FloatRect bounds = {{0.0f, 0.0f}, control_state.simulation.world.size};
                if (bounds.contains(world_position)) {
                    colony->setPosition(world_position);
                    control_state.draw_action = nullptr;
                    control_state.view_action = nullptr;
                    setActiveActionButton(nullptr); // MS02: Reposition-Modus beendet - Highlight aus
                } else {
                    std::cout << "Invalid colony position: outside world" << std::endl;
                }
            };
        });
        reposition_button->setHeight(26.0f); // MS02: Höhe um ca. 30% erhöht (20 -> 26)
        // MS02: Breite passt sich automatisch dem Text an (siehe Button::fitWidth())
        top_zone->addItem(reposition_button);

        // MS02: ersetzt den alten, redundanten "Remove"-Button (Remove macht jetzt
        // schon toggle_button) - wählt die Kolonie aus, damit ihre Marker sichtbar
        // werden (Renderer zeigt Marker nur für die "selected_colony", siehe
        // world_renderer.hpp - "Draw Markers"-Toggle allein reicht dafür nicht)
        show_markers_button = create<GUI::Button>("Markers", [this](){
            if (colony) { on_select(colony->id); }
        });
        show_markers_button->setHeight(26.0f); // MS02: Höhe um ca. 30% erhöht (20 -> 26)
        // MS02: Breite passt sich automatisch dem Text an (siehe Button::fitWidth())
        // MS02: Highlight folgt "selected" (siehe update()), nicht active_action_button
        top_zone->addItem(show_markers_button, "show_markers");
    }

    // MS02: wird von ColonyCreator aufgerufen, nachdem die Kolonie für diesen Slot erstellt wurde
    void activate(civ::Ref<Colony> colony_)
    {
        colony = colony_;
        toggle_button->label->setText("Remove");
        toggle_button->fitWidth(); // MS02: Breite an neuen Text anpassen
    }

    // MS02: wird von ColonyCreator aufgerufen, nachdem die Kolonie entfernt wurde
    void deactivate()
    {
        // MS02: Bugfix - falls "Reposition" für diese Kolonie aktiv war, hängen in
        // control_state.view_action/draw_action noch Lambdas, die "this->colony" beim
        // nächsten Klick/Render dereferenzieren. Ohne dieses Reset crasht das, weil colony
        // unten auf eine leere civ::Ref gesetzt wird, auf die diese Lambdas dann zugreifen.
        if (active_action_button == reposition_button) {
            control_state.resetCallbacks();
        }
        // MS02: Highlight (Focus oder Reposition) immer zurücksetzen, sonst bleibt z.B.
        // "Focus" optisch aktiv, obwohl die Kolonie schon entfernt wurde
        setActiveActionButton(nullptr);
        colony = civ::Ref<Colony>{};
        toggle_button->label->setText("Add");
        toggle_button->fitWidth(); // MS02: Breite an neuen Text anpassen
    }

    void update() override
    {
        // MS02: Show-Markers-Button spiegelt direkt den Auswahlstatus wider, unabhängig
        // davon, ob Focus/Reposition gerade aktiv sind
        setButtonActive(show_markers_button, selected);
    }

    void onClick(sf::Vector2f, sf::Mouse::Button) override
    {
        if (colony) {
            on_select(colony->id);
        }
    }

    void render(sf::RenderTarget& target) override
    {
        auto background = GUI::RoundedRectangle(size, position, 5.0f);
        // MS02: aufhellen statt abdunkeln, damit die Auswahl-Hervorhebung auf dem
        // schwarzen Hintergrund nicht durch uint8_t-Unterlauf falsch wird
        const uint8_t select = selected ? 30 : 0;
        background.setFillColor(sf::Color(background_color.r + select, background_color.g + select, background_color.b + select));
        GUI::Item::draw(target, background);
    }
};
