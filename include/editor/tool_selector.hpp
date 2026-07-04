#pragma once
#include "GUI/container.hpp"
#include "GUI/button.hpp"
#include "control_state.hpp"
#include "simulation/simulation.hpp"
#include <future>


namespace edtr
{


    struct ToolOption : public GUI::Button
    {
        // MS02: ehemals "select_padding" (schrumpfte den Innenrahmen ein) - erzeugte mit
        // angle_radius=0 vier hässliche Punkte (negativer Radius). Jetzt 0..1 Auswahlstärke,
        // die einen animierten Rahmen einblendet statt den Button zu verformen
        trn::Transition<float> selection_amount = 0.0f;
        // MS02: manche Buttons (z.B. Speed) zeigen Auswahl nur über die Schriftfarbe an,
        // ohne den zusätzlichen weißen Auswahlrahmen
        bool show_selection_outline = true;

        ToolOption(const std::string& text, GUI::ButtonCallBack callback)
            : GUI::Button(text, callback)
        {
            selection_amount.setSpeed(3.0f);
        }

        void select()
        {
            selection_amount = 1.0f;
            label->setColor(sf::Color::White); // MS02: Ausnahme: weiße Schrift bei Auswahl
        }

        void reset()
        {
            selection_amount = 0.0f;
            label->setColor(sf::Color(140, 140, 140)); // MS02: zurück auf Standard-Grau (dunkler, siehe Button-Default)
        }

        // MS02: eigenes render() statt Button::render(), damit zusätzlich zum normalen
        // grauen Rahmen (transparente Füllung, siehe Button-Defaults) ein zweiter,
        // animierter Rahmen in Akzentfarbe bei Auswahl eingeblendet werden kann
        void render(sf::RenderTarget& target) override
        {
            sf::RectangleShape rect(size);
            rect.setPosition(position);
            rect.setFillColor(background_color);
            rect.setOutlineThickness(outline_thickness);
            rect.setOutlineColor(outline_color);
            draw(target, rect);

            if (show_selection_outline && selection_amount.as() > 0.0f) {
                sf::RectangleShape selection(size);
                selection.setPosition(position);
                selection.setFillColor(sf::Color::Transparent);
                selection.setOutlineThickness(-2.0f * selection_amount.as()); // MS02: dünner (vorher -3), gleiche Dicke wie der normale Button-Rahmen
                selection.setOutlineColor(sf::Color::White); // MS02: weiß statt orange - aktiver Button bleibt hell
                draw(target, selection);
            }
        }
    };


    struct ToolSelector : public GUI::NamedContainer
    {
        enum class Tool
        {
            None,         // MS02: kein Brush aktiv (Standard beim Start)
            BrushWall,
            BrushFood,
            BrushDelete,
            PlacePredator // MS02: Raeuber-Platzierungswerkzeug - Klick auf Karte spawnt einen Raeuber
        };

        Tool current_tool;

        SPtr<ToolOption> tool_wall;
        SPtr<ToolOption> tool_food;
        SPtr<ToolOption> tool_erase;
        SPtr<ToolOption> tool_predator; // MS02: Button fuer das Raeuber-Platzierungswerkzeug

        bool          edit_mode = false;
        Simulation& simulation;
        ControlState& control_state;

        int32_t brush_size = 3;

        explicit
            ToolSelector(ControlState& control_state_, Simulation& simulation_)
            : GUI::NamedContainer("Brushes", Container::Orientation::Horizontal)
            , current_tool(Tool::None) // MS02: kein Brush beim Start aktiv
            , control_state(control_state_)
            , simulation(simulation_)
        {
            padding = 5.0f;
            // MS02: etwas Abstand zwischen "Brushes"-Label und den Buttons in der Kopfzeile
            header->spacing = 10.0f; // MS02: etwas mehr Abstand zwischen "Brushes" und "Wall"

            tool_wall = create<ToolOption>("Wall", [this]() {
                selectTool(Tool::BrushWall, tool_wall);
                });
            tool_food = create<ToolOption>("Food", [this]() {
                selectTool(Tool::BrushFood, tool_food);
                });
            tool_erase = create<ToolOption>("Erase", [this]() {
                selectTool(Tool::BrushDelete, tool_erase);
                });
            // MS02: einheitlicher grauer Rahmen statt individueller Füllfarben (siehe Button-Defaults)
            // MS02: Breite passt sich automatisch dem Text an (siehe Button::fitWidth())
            // MS02: feste, kompakte Höhe passend zur Kopfzeile (statt der alten 30px-Root-Zeile)
            // MS02: Höhe um ca. 30% erhöht (20 -> 26)
            tool_wall->setHeight(26.0f);
            tool_food->setHeight(26.0f);
            tool_erase->setHeight(26.0f);
            // MS02: Breite jetzt Auto statt textgenau (Fixed) - die 3 Buttons teilen sich den
            // gesamten verbleibenden Platz in der Kopfzeile, damit "Erase" rechts genauso am
            // Sidebar-Rand endet wie die anderen Boxen, statt mit Leerraum dahinter
            tool_wall->setWidth(tool_wall->size.x, GUI::Size::Auto);
            tool_food->setWidth(tool_food->size.x, GUI::Size::Auto);
            tool_erase->setWidth(tool_erase->size.x, GUI::Size::Auto);
            // MS02: Buttons direkt in die Kopfzeile neben das "Brushes"-Label statt in die
            // eigene Zeile darunter - header zentriert seine Kinder vertikal automatisch,
            // dadurch sitzen Label und Buttons auf einer Höhe
            header->addItem(tool_wall);
            header->addItem(tool_food);
            header->addItem(tool_erase);
            // MS02: Predator ist kein Brush mehr - wird nicht mehr hier in "Brushes"
            // angezeigt, sondern in der separaten "Place"-Sektion (siehe EditorScene::initialize)
            // MS02: Raeuber-Button - bei Klick wird Tool::PlacePredator aktiviert
            tool_predator = create<ToolOption>("Predator", [this]() {
                selectTool(Tool::PlacePredator, tool_predator);
                });
            // MS02: keine Default-Auswahl mehr - der Spieler muss explizit einen Brush wählen
        }

        void reset() const
        {
            tool_wall->reset();
            tool_food->reset();
            tool_erase->reset();
            tool_predator->reset(); // MS02: Raeuber-Button zuruecksetzen
        }

        void select(const SPtr<ToolOption>& option)
        {
            reset();
            if (option) {
                option->select();
                setCallback();
            }
            notifyChanged();
        }

        // MS02: Klick auf das bereits aktive Tool wählt es ab (zurück auf Tool::None), statt
        // dass immer irgendein Brush/Predator aktiv bleiben muss
        void selectTool(Tool tool, const SPtr<ToolOption>& option)
        {
            if (current_tool == tool) {
                current_tool = Tool::None;
                reset();
                resetCallback();
                notifyChanged();
            }
            else {
                current_tool = tool;
                select(option);
            }
        }

        template<typename TCallback>
        void applyBrush(sf::Vector2f mouse_position, TCallback&& callback)
        {
            const auto x = to<int32_t>(mouse_position.x) / simulation.world.map.cell_size;
            const auto y = to<int32_t>(mouse_position.y) / simulation.world.map.cell_size;

            const int32_t min_x = std::max(1, x - brush_size);
            const int32_t max_x = std::min(to<int32_t>(simulation.world.map.width - 1), x + brush_size + 1);
            const int32_t min_y = std::max(1, y - brush_size);
            const int32_t max_y = std::min(to<int32_t>(simulation.world.map.height - 1), y + brush_size + 1);

            for (int32_t px(min_x); px < max_x; ++px) {
                for (int32_t py(min_y); py < max_y; ++py) {
                    callback(px, py);
                }
            }
        }

        void setCallback()
        {
            // MS02: ohne ausgewählten Brush keine Aktion - sonst würde wegen current_tool's
            // Default beim Start trotzdem die Wall-Brush-Logik aktiv sein
            if (current_tool == Tool::None) {
                resetCallback();
                return;
            }
            if (edit_mode) {
                // Edit callbacks
                switch (current_tool) {
                case Tool::None:
                    break;
                case Tool::BrushWall:
                    control_state.view_action = [this](sf::Vector2f mouse_position) {
                        applyBrush(mouse_position, [this](int32_t x, int32_t y) {
                            simulation.world.addWall(sf::Vector2i{ x, y });
                            });
                        };
                    control_state.view_action_end = [this]() {
                        simulation.distance_field_builder.requestUpdate();
                        };
                    break;
                case Tool::BrushFood:
                    control_state.view_action = [this](sf::Vector2f mouse_position) {
                        applyBrush(mouse_position, [this](int32_t x, int32_t y) {
                            simulation.world.addFoodAt(sf::Vector2i{ x, y }, 2);
                            });
                        };
                    break;
                case Tool::BrushDelete:
                    control_state.view_action = [this](sf::Vector2f mouse_position) {
                        applyBrush(mouse_position, [this](int32_t x, int32_t y) {
                            simulation.world.map.clearCell(sf::Vector2i{ x, y });
                            });
                        simulation.distance_field_builder.requestUpdate();
                        };
                    control_state.view_action_end = [this]() {
                        simulation.distance_field_builder.requestUpdate();
                        };
                    break;
                    // MS02: Raeuber-Platzierung - Rechtsklick auf Karte erzeugt einen neuen
                    // Raeuber an der Mausposition in Weltkoordinaten. Mehrere Raeuber moeglich.
                case Tool::PlacePredator:
                    control_state.view_action = [this](sf::Vector2f mouse_position) {
                        simulation.predators.emplace_back(
                            mouse_position.x,
                            mouse_position.y
                        );
                        };
                    break;
                }
                // Preview callbacks
                control_state.draw_action = [this](sf::RenderTarget& target, const ViewportHandler& vp_handler) {
                    const int32_t cell_size = simulation.world.map.cell_size;
                    const float side_size = (2.0f * control_state.brush_radius + 1) * to<float>(cell_size);
                    sf::RectangleShape brush_preview({ side_size, side_size });
                    brush_preview.setFillColor(sf::Color(100, 100, 100, 100));
                    brush_preview.setOrigin(side_size * 0.5f, side_size * 0.5f);
                    const sf::Vector2f current_position = vp_handler.getMouseWorldPosition();
                    brush_preview.setPosition(to<float>(int(current_position.x / to<float>(cell_size)) * cell_size) + 2.0f,
                        to<float>(int(current_position.y / to<float>(cell_size)) * cell_size) + 2.0f);
                    target.draw(brush_preview, vp_handler.getRenderState());
                    };
            }
        }

        void setEditMode(bool b)
        {
            edit_mode = b;
            if (b) {
                setCallback();
            }
            else {
                resetCallback();
            }
        }

        void resetCallback()
        {
            control_state.view_action = nullptr;
            control_state.view_action_end = nullptr;
            control_state.draw_action = nullptr;
        }
    };

}