#pragma once
#include <algorithm>
#include "GUI/scene.hpp"
#include "toolbox.hpp"
#include "editor/color_picker/color_picker.hpp"
#include "color_saver.hpp"
#include "GUI/utils.hpp"
#include "set_color_button.hpp"
#include "GUI/named_container.hpp"
#include "tool_selector.hpp"
#include "editor/colony_creator/colony_creator.hpp"
#include "simulation/world/world.hpp"
#include "render/renderer.hpp"
#include "simulation/config.hpp"
#include "editor/world_view.hpp"
#include "time_control/time_controller.hpp"
#include "display_options/display_options.hpp"
#include "statistics_panel.hpp"

namespace edtr
{

    struct EditorScene : public GUI::Scene
    {
        using Ptr = std::shared_ptr<EditorScene>;

        Simulation &simulation;
        ControlState control_state;

        SPtr<Toolbox> toolbox;
        SPtr<WorldView> renderer;
        SPtr<ToolSelector> tool_selector;
        SPtr<DisplayOption> display_controls;
        SPtr<ToolOption> quit_button;
        bool quit_button_visible = true; // siehe updateRendererLayout()
        SPtr<StatisticsPanel> statistics_panel;

        // MS02: gemeinsame Box-Maße von Sidebar+Spielfeld (für den grünen Rahmen in render())
        float combined_box_height = 0.0f;
        float combined_box_y      = 0.0f;

        explicit EditorScene(sf::RenderWindow &window, Simulation &sim)
            : GUI::Scene(window), control_state(sim), simulation(sim)
        {
            root.padding = 0.0f; // MS02: kein globales Padding mehr

            Conf::loadTextures();
            initialize();
        }

        ~EditorScene()
        {
            std::cout << "Exiting, clean resources" << std::endl;
            Conf::freeTextures();
        }

        void initialize()
        {
            const sf::Vector2u window_size = window.getSize();

            // MS02: Sidebar zuerst erstellen - ihre (variable, ein-/ausklappbare) Breite
            // bestimmt danach die Größe/Position des Spielfelds daneben
            toolbox = create<Toolbox>(sf::Vector2f{Toolbox::opened_width, to<float>(window_size.y)}, sf::Vector2f{0.0f, 0.0f});

            const sf::Vector2f renderer_size = {to<float>(window_size.x) - toolbox->size.x, to<float>(window_size.y)};
            renderer = create<WorldView>(renderer_size, simulation, control_state);

            // MS02: mehr Abstand zwischen den Top-Level-Kategorien (Time Control, Display,
            // Edit Map, Colonies) - vorher Container-Standard von 8px
            toolbox->root->spacing = 28.0f;
            // MS02: gleicher Abstand zwischen "AntSimulator v2.0" und "Time Control"
            // (Lücke zwischen Toolbox-Header und Toolbox-Root, vorher NamedContainer-
            // Standard von 5px)
            toolbox->spacing = 28.0f;

            // MS02: Schriftgröße der verschachtelten Brush/Size/Place-Überschriften
            const uint32_t section_header_char_size = 11;
            // MS02: Schriftgröße der Top-Level-Sidebar-Abschnitte (Time Control, Display,
            // Edit Map, Colonies) - etwas kleiner als der Sidebar-Titel ("AntSimulator
            // v2.0", bleibt bei 14)
            const uint32_t top_level_header_char_size = 13;
            // MS02: #FBE3D6 für die Top-Level-Überschriften (Time Control, Display,
            // Edit Map, Colonies) und den Sidebar-Titel
            const sf::Color top_level_header_color(251, 227, 214);

            // MS02: Time Control lebt jetzt oben in der Sidebar (direkt unter dem Titel)
            // statt als eigenes Overlay oben rechts über dem Spielfeld
            auto time_controls = create<TimeController>();
            time_controls->label->setCharacterSize(top_level_header_char_size);
            time_controls->label->setColor(top_level_header_color);
            // MS02: padding=0 statt NamedContainer-Standard (7), damit die Überschrift
            // linksbündig auf gleicher Höhe wie "AntSimulator v2.0" sitzt
            time_controls->padding = 0.0f;
            time_controls->spacing = 10.0f; // MS02: mehr Abstand zwischen "Time Control" und dem Start-Button
            watch(time_controls, [this, time_controls]()
                  {
            this->renderer->current_time_state = time_controls->current_state;
            this->control_state.updating = time_controls->current_state == TimeController::State::Play;
            this->control_state.simulation_speed = time_controls->speed_selector->current_speed; });
            toolbox->addItem(time_controls);

            // Add display options
            display_controls = create<DisplayOption>(control_state);
            display_controls->label->setCharacterSize(top_level_header_char_size);
            display_controls->label->setColor(top_level_header_color);
            display_controls->padding = 0.0f; // MS02: siehe Time Control
            watch(display_controls, [this]()
                  { updateRenderOptions(); });
            toolbox->addItem(display_controls);

            // Add map edition tools
            auto tools = create<GUI::NamedContainer>("Edit Map", GUI::Container::Orientation::Vertical);
            tools->label->setCharacterSize(top_level_header_char_size);
            tools->label->setColor(top_level_header_color);
            tools->padding = 0.0f; // MS02: siehe Time Control
            tools->root->spacing = 8.0f; // MS02: etwas Abstand zwischen Brushes/Size/Place
            // MS02: Edit-Map-Toggle entfernt - war ohnehin ohne Wirkung (Inhalt immer
            // sichtbar, Brushes unten immer aktiv, siehe setEditMode(true))
            toolbox->addItem(tools);

            tool_selector = create<ToolSelector>(control_state, simulation);
            tool_selector->padding = 0.0f; // MS02: siehe Time Control - linksbündig wie die anderen Überschriften
            tool_selector->label->setCharacterSize(12); // MS02: so groß wie "Draw Ants" etc. (NamedToggle)
            tool_selector->setEditMode(true); // MS02: Brushes ohne Edit-Map-Toggle nutzbar
            tools->addItem(tool_selector);
            // MS02: "Size" jetzt direkt als Text in der Slider-Box selbst statt als eigene
            // Sektionsüberschrift davor - kein separater NamedContainer mehr nötig
            auto slider = create<SliderLabel>("Size", 10.0f);
            slider->name_label->setCharacterSize(section_header_char_size);
            watch(slider, [this, slider]()
                  { setBrushSize(slider->getValue()); });
            setBrushSize(slider->getValue());
            // MS02: Slider-Box soll links auf gleicher Höhe wie der "Wall"-Button beginnen
            // statt am Sidebar-Rand - dazu ein fester Abstandshalter (Breite = Label "Brushes"
            // + Kopfzeilen-Abstand) vor die (weiterhin Auto-breite, bis zum rechten Rand
            // reichende) Slider-Box, beides in eine gemeinsame Zeile verpackt
            auto slider_row = create<GUI::Container>(GUI::Container::Orientation::Horizontal);
            slider_row->padding = 0.0f;
            slider_row->spacing = 0.0f;
            slider_row->size_type.y = GUI::Size::FitContent;
            auto slider_indent = create<GUI::EmptyItem>();
            slider_indent->setWidth(tool_selector->label->size.x + tool_selector->header->spacing);
            slider_row->addItem(slider_indent);
            slider_row->addItem(slider);
            tools->addItem(slider_row);
            // MS02: Predator ist kein Brush - eigene "Place"-Sektion unter Brushes/Size
            auto place = create<GUI::NamedContainer>("Place", GUI::Container::Orientation::Horizontal);
            place->padding = 0.0f; // MS02: siehe Time Control - linksbündig wie die anderen Überschriften
            place->label->setCharacterSize(12); // MS02: so groß wie "Draw Ants" etc. (NamedToggle)
            // MS02: "Place"-Label auf die gleiche Breite wie "Brushes" fixieren (Text bleibt
            // linksbündig stehen) - dadurch beginnt der Predator-Button an der gleichen Stelle
            // wie "Wall", statt durch den kürzeren Text "Place" weiter links zu starten
            place->label->setAlignment(GUI::Alignment::Left);
            place->label->auto_size_update = false;
            place->label->setWidth(tool_selector->label->size.x);
            // MS02: etwas Abstand zwischen "Place"-Label und dem Button in der Kopfzeile
            place->header->spacing = 10.0f; // MS02: etwas mehr Abstand zwischen "Place" und "Predator"
            tool_selector->tool_predator->setHeight(26.0f); // MS02: Höhe um ca. 30% erhöht (20 -> 26)
            // MS02: Breite wieder textgenau (Fixed via fitWidth) statt Auto - Predator-Button
            // muss nicht bis zum rechten Sidebar-Rand reichen wie Wall/Food/Erase
            tool_selector->tool_predator->fitWidth();
            // MS02: Button direkt in die Kopfzeile neben das "Place"-Label statt in die
            // eigene Zeile darunter (wie schon bei "Brushes")
            place->header->addItem(tool_selector->tool_predator);
            tools->addItem(place);
            // Add colonies edition tools
            auto colonies = create<ColonyCreator>(simulation, control_state);
            colonies->label->setCharacterSize(top_level_header_char_size);
            colonies->label->setColor(top_level_header_color);
            colonies->padding = 0.0f; // MS02: siehe Time Control
            colonies->spacing = 10.0f; // MS02: mehr Abstand zwischen "Colonies" und dem ersten Add-Button
            toolbox->addItem(colonies);

            // MS02: Quit-Button - sitzt außerhalb von toolbox->root (das sich an seinem
            // Inhalt orientiert, siehe FitContent), damit er unabhängig davon fest am
            // unteren Sidebar-Rand bleibt (Position/Breite werden in updateRendererLayout()
            // bei jedem Resize/Ein-Ausklappen neu berechnet)
            quit_button = create<ToolOption>("QUIT", [this](){ window.close(); });
            quit_button->label->setColor(sf::Color::Red);
            quit_button->label->setCharacterSize(10);
            quit_button->setHeight(30.0f);

            // MS02: Statistik-Panel - eigenständiges Overlay oben rechts über dem
            // Spielfeld, unabhängig von der Sidebar (siehe statistics_panel.hpp)
            statistics_panel = create<StatisticsPanel>(simulation, control_state);

            addItem(renderer);
            addItem(toolbox, "Toolbox");
            addItem(quit_button);
            addItem(statistics_panel);

            // MS02: Spielfeld-Layout neu berechnen, sobald die Sidebar ein-/ausgeklappt wird
            toolbox->on_layout_changed = [this]() { updateRendererLayout(); };
            updateRendererLayout();
        }

        void updateRenderOptions() const
        {
            renderer->simulation.renderer.render_ants = display_controls->draw_ants;
            renderer->simulation.world.renderer.draw_markers = display_controls->draw_markers;
            renderer->simulation.world.renderer.draw_density = display_controls->draw_density;
        }

        void setBrushSize(float size)
        {
            const auto brush_size = to<int32_t>(size);
            tool_selector->brush_size = brush_size;
            control_state.brush_radius = to<float>(brush_size);
        }

        void onSizeChange() override
        {
            updateRendererLayout();
        }

        // MS02: Einzige Stelle, die Größe/Position/Zoom des Spielfelds anhand der
        // aktuellen Sidebar-Breite berechnet (Fenster-Resize UND Sidebar Ein-/Ausklappen).
        // Die Welt (WorldGrid) hat eine feste Größe (Conf::WORLD_WIDTH/HEIGHT = 1920x1080,
        // beim Start angelegt) - sie kann nicht mitwachsen/schrumpfen, also wird stattdessen
        // die Kamera so gezoomt, dass die feste Welt vollständig (per "contain") in die
        // verfügbare Fläche neben der Sidebar passt - auf kleinen Bildschirmen entsprechend
        // verkleinert, auf großen Bildschirmen vergrößert, statt immer 1:1 in Pixeln.
        void updateRendererLayout()
        {
            const float sidebar_width = toolbox->size.x;
            const sf::Vector2f available_size = {root.size.x - sidebar_width, root.size.y};

            ViewportHandler &vp_handler = renderer->simulation.renderer.vp_handler;
            // MS02: Spielfeld (fest Conf::WORLD_WIDTH/HEIGHT) per "contain"-Skalierung in die
            // verfügbare Fläche einpassen, statt nur auf die Breite zu zoomen - sonst würde das
            // Spielfeld auf sehr breiten Bildschirmen (z.B. Ultrawide) über oben/unten hinausragen
            const float zoom = std::min(available_size.x / to<float>(Conf::WORLD_WIDTH),
                                         available_size.y / to<float>(Conf::WORLD_HEIGHT));
            // MS02: der Fit-Zoom ist jetzt auch die untere Zoom-Grenze fürs Scrollen (siehe
            // ViewportHandler::wheelZoom) - vorher war die Grenze fix 1.0, wodurch Reinzoomen
            // nie funktionierte, sobald der Fit-Zoom selbst schon unter 1.0 lag
            vp_handler.setMinZoom(zoom);
            const float playfield_width  = to<float>(Conf::WORLD_WIDTH)  * zoom;
            const float playfield_height = to<float>(Conf::WORLD_HEIGHT) * zoom;
            const float playfield_y_offset = (root.size.y - playfield_height) * 0.5f;

            // MS02: Renderer-Box exakt auf den gefitteten Bereich begrenzen (nicht auf den ganzen
            // verfügbaren Platz) - das ist jetzt gleichzeitig die Clip-Grenze fürs Reinzoomen
            // (siehe WorldView::render, das per setViewport() genau auf position/size clippt)
            renderer->size = {playfield_width, playfield_height};
            renderer->setPosition({sidebar_width, playfield_y_offset});

            vp_handler.state.center = sf::Vector2f{playfield_width, playfield_height} * 0.5f * Conf::GUI_SCALE;
            vp_handler.setZoom(zoom);
            // MS02: auf die Mitte der Welt fokussieren, nicht die Mitte des Bildschirms,
            // sonst verschiebt sich das gerenderte Spielfeld relativ zu seiner GUI-Position
            vp_handler.setFocus({to<float>(Conf::WORLD_WIDTH) * 0.5f, to<float>(Conf::WORLD_HEIGHT) * 0.5f});

            // MS02: Sidebar (egal ob minimiert oder maximiert) immer exakt so hoch wie das
            // Spielfeld und auf gleicher Höhe ausgerichtet - kein Sonderfall mehr für "voller
            // Bildschirm" beim Einklappen, sonst wäre die Sidebar dort höher als das Spielfeld
            toolbox->setHeight(playfield_height);
            toolbox->setPosition({0.0f, playfield_y_offset});
            combined_box_height = playfield_height;
            combined_box_y      = playfield_y_offset;

            // MS02: Quit-Button fest am unteren Sidebar-Rand, gleicher Randabstand wie die
            // Überschriften links/rechts (toolbox->padding) - liegt außerhalb von toolbox->root
            // (das FitContent ist), deshalb hier separat neu positioniert statt im normalen Layout-Fluss
            if (toolbox->collapsed) {
                // MS02: eingeklappt - kein Platz/Text mehr, Button unsichtbar & nicht klickbar
                // (auch Höhe auf 0, sonst bleibt vom Rahmen ein dünner grauer Streifen sichtbar)
                // - setText nur bei tatsächlichem Wechsel aufrufen, nicht bei jedem Resize, da
                // TextLabel::setText() die Label-Größe sonst unnötig auf Textgröße zurücksetzt.
                // WICHTIG: TextLabel::label wird von setText() nie aktualisiert (nur vom
                // Konstruktor), taugt also nicht als "aktueller Text"-Check - daher eigenes Flag.
                if (quit_button_visible) {
                    quit_button->label->setText("");
                    quit_button_visible = false;
                }
                quit_button->setWidth(0.0f);
                quit_button->setHeight(0.0f);
                quit_button->catch_event = false;
            } else {
                if (!quit_button_visible) {
                    quit_button->label->setText("QUIT");
                    quit_button_visible = true;
                }
                quit_button->catch_event = true;
                quit_button->setHeight(30.0f);
                const float quit_margin = toolbox->padding;
                quit_button->setWidth(std::max(0.0f, toolbox->size.x - 2.0f * quit_margin));
                quit_button->setPosition({toolbox->position.x + quit_margin,
                                           toolbox->position.y + toolbox->size.y - quit_margin - quit_button->size.y});
                // MS02: Text nach dem endgültigen Sizing nochmal zentrieren - verhindert, dass
                // die Zentrierung beim allerersten Layout-Durchlauf einen falschen (zu kleinen)
                // Zwischenstand der Label-Größe verwendet und der Text links hängen bleibt
                quit_button->label->setAlignment(GUI::Alignment::Center);
            }

            // MS02: oben rechts IM SPIELFELD (nicht im Fenster) - bleibt also in dessen
            // Ecke, egal ob das Spielfeld durch Sidebar-Ein-/Ausklappen breiter/schmaler wird
            const float stats_margin = 10.0f;
            const float playfield_right = sidebar_width + playfield_width;
            statistics_panel->setPosition({playfield_right - statistics_panel->size.x - stats_margin,
                                            playfield_y_offset + stats_margin});

            // This is to update mouse_position
            simulation.renderer.vp_handler.wheelZoom(0);
        }

        // MS02: grüner Rahmen außen um Sidebar+Spielfeld zusammen, damit beide wie eine
        // gemeinsame Box aussehen. GUI::Scene::render() ist deshalb virtual geworden.
        void render() override
        {
            GUI::Scene::render();
            sf::RectangleShape border({root.size.x * Conf::GUI_SCALE, combined_box_height * Conf::GUI_SCALE});
            border.setPosition(0.0f, combined_box_y * Conf::GUI_SCALE);
            border.setFillColor(sf::Color::Transparent);
            border.setOutlineColor(sf::Color(0, 255, 0, 0)); // MS02: Rahmen komplett unsichtbar
            border.setOutlineThickness(3.0f);
            window.draw(border);

            // MS02: Sidebar und Spielfeld bekommen exakt den gleichen Rahmen-Look (gleiche
            // Farbe/Dicke/Konstruktion) - beide nutzen bewusst combined_box_y/-height (statt
            // z.B. toolbox->size.y oder renderer->size), damit oben/unten garantiert
            // übereinstimmen. Die Wand-Zellen der Welt selbst werden nicht mehr sichtbar
            // gerendert (siehe world_renderer.hpp), damit ihre mit dem Zoom mitskalierende
            // Dicke nicht mehr gegen diesen fixen UI-Rahmen konkurriert.
            const sf::Color border_color(60, 60, 60);
            const float border_thickness = -2.0f * Conf::GUI_SCALE;

            sf::RectangleShape sidebar_border({toolbox->size.x * Conf::GUI_SCALE, combined_box_height * Conf::GUI_SCALE});
            sidebar_border.setPosition(0.0f, combined_box_y * Conf::GUI_SCALE);
            sidebar_border.setFillColor(sf::Color::Transparent);
            sidebar_border.setOutlineColor(border_color);
            sidebar_border.setOutlineThickness(border_thickness);
            window.draw(sidebar_border);

            // MS02: kein eigenes Outline-Rechteck für das Spielfeld - eine zweite Kante direkt
            // neben der Sidebar-Kante (beide an x=toolbox->size.x) würde sich mit deren rechter
            // Kante zu einer doppelt dicken Naht aufsummieren. Stattdessen nur oben/unten/rechts
            // einzeln zeichnen, die linke Kante liefert bereits sidebar_border mit.
            const float playfield_thickness = std::abs(border_thickness);
            const float playfield_left      = toolbox->size.x * Conf::GUI_SCALE;
            const float playfield_right     = root.size.x * Conf::GUI_SCALE;
            const float playfield_top       = combined_box_y * Conf::GUI_SCALE;
            const float playfield_bottom    = (combined_box_y + combined_box_height) * Conf::GUI_SCALE;

            sf::RectangleShape playfield_top_line({playfield_right - playfield_left, playfield_thickness});
            playfield_top_line.setPosition(playfield_left, playfield_top);
            playfield_top_line.setFillColor(border_color);
            window.draw(playfield_top_line);

            sf::RectangleShape playfield_bottom_line({playfield_right - playfield_left, playfield_thickness});
            playfield_bottom_line.setPosition(playfield_left, playfield_bottom - playfield_thickness);
            playfield_bottom_line.setFillColor(border_color);
            window.draw(playfield_bottom_line);

            sf::RectangleShape playfield_right_line({playfield_thickness, playfield_bottom - playfield_top});
            playfield_right_line.setPosition(playfield_right - playfield_thickness, playfield_top);
            playfield_right_line.setFillColor(border_color);
            window.draw(playfield_right_line);

            renderCategorySeparators();
        }

        // MS02: graue Trennlinien zwischen den Sidebar-Kategorien (Time Control, Display,
        // Edit Map, Colonies), mittig in der Lücke zwischen je zwei Abschnitten.
        // WICHTIG: "position" ist nur lokal relativ zum direkten Elternelement - für die
        // tatsächliche Bildschirmposition muss man "offset" (kumulierter Eltern-Versatz,
        // siehe Item::updatePosition()) hinzurechnen, sonst landen die Linien an der
        // falschen Stelle (z.B. über dem Hintergrund statt in der Sidebar).
        static float absoluteY(const GUI::ItemPtr& item)
        {
            return item->offset.y + item->position.y;
        }

        void renderCategorySeparators()
        {
            if (!toolbox->show_root) {
                return; // Sidebar eingeklappt - keine Kategorien sichtbar
            }
            // MS02: keine Trennlinie mehr zwischen "AntSimulator v2.0" und "Time Control" -
            // nur noch zwischen den Kategorien selbst (Time Control/Display/Edit Map/Colonies)
            for (size_t i = 0; i + 1 < toolbox->root->sub_items.size(); ++i) {
                const float bottom = absoluteY(toolbox->root->sub_items[i]) + toolbox->root->sub_items[i]->size.y;
                const float top    = absoluteY(toolbox->root->sub_items[i + 1]);
                drawCategorySeparator((bottom + top) * 0.5f);
            }
        }

        void drawCategorySeparator(float y_logical)
        {
            const float thickness = 2.0f;
            const float x = (toolbox->offset.x + toolbox->position.x) * Conf::GUI_SCALE;
            const float y = y_logical * Conf::GUI_SCALE - (thickness * Conf::GUI_SCALE) * 0.5f;
            sf::RectangleShape line({toolbox->size.x * Conf::GUI_SCALE, thickness * Conf::GUI_SCALE});
            line.setPosition(x, y);
            line.setFillColor(sf::Color(60, 60, 60));
            window.draw(line);
        }
    };

}
