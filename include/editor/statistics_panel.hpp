#pragma once
#include "editor/GUI/named_container.hpp"
#include "editor/GUI/toggle.hpp"
#include "editor/control_state.hpp"
#include "editor/colony_creator/colony_stats.hpp"
#include "simulation/simulation.hpp"


namespace edtr
{

// MS02: ein Block pro aktiver Kolonie - Population, Workers/Soldiers-Aufteilung,
// ob die Queen noch lebt, und ein Populations-Verlauf als Liniendiagramm (ColonyChart
// wiederverwendet, siehe colony_stats.hpp), jeweils live aus der Colony gelesen
struct StatisticsColonyBlock : public GUI::Container
{
    civ::Ref<Colony> colony;
    SPtr<GUI::TextLabel> population_value;
    SPtr<GUI::TextLabel> workers_soldiers_value;
    SPtr<GUI::TextLabel> queen_value;
    SPtr<ColonyChart> chart;

    StatisticsColonyBlock(civ::Ref<Colony> colony_, ControlState& control_state)
        : GUI::Container(GUI::Container::Orientation::Vertical)
        , colony(colony_)
    {
        padding = 0.0f;
        spacing = 2.0f;
        size_type.y = GUI::Size::FitContent;

        population_value       = addStatRow("Population:");
        workers_soldiers_value  = addStatRow("Workers / Soldiers:");
        queen_value             = addStatRow("Queen alive:");

        // MS02: zusätzlicher Abstandshalter vor dem Chart - mehr Lücke zur letzten
        // Textzeile als der normale "spacing" zwischen den Textzeilen selbst
        auto chart_spacer = create<GUI::EmptyItem>();
        chart_spacer->setHeight(8.0f);
        addItem(chart_spacer);

        chart = create<ColonyChart>(colony, control_state);
        chart->setHeight(40.0f); // MS02: feste Höhe statt Auto/FitContent (siehe Container::updateItems)
        addItem(chart);
    }

    // MS02: eine Zeile "Label:" ... (Auto-Spacer) ... "Wert" - schiebt den Wert wie
    // mit einem Tab nach rechts, statt direkt nach dem Label zu stehen (gleiche Technik
    // wie NamedToggle/TimeController: EmptyItem als Auto-breiter Spacer)
    SPtr<GUI::TextLabel> addStatRow(const std::string& label_text)
    {
        auto row = create<GUI::Container>(GUI::Container::Orientation::Horizontal);
        row->padding = 0.0f;
        row->spacing = 0.0f;
        row->size_type.y = GUI::Size::FitContent;

        auto label_part = create<GUI::TextLabel>(label_text, 12);
        label_part->setColor(sf::Color::White);
        label_part->setAlignment(GUI::Alignment::Left);
        row->addItem(label_part);

        row->addItem(create<GUI::EmptyItem>());

        auto value_part = create<GUI::TextLabel>("", 12);
        value_part->setColor(sf::Color::White);
        value_part->setAlignment(GUI::Alignment::Left);
        row->addItem(value_part);

        addItem(row);
        return value_part;
    }

    // MS02: grauer Rahmen um den Chart, etwas heller als der Sidebar-Rahmen.
    // WICHTIG: chart->position ist relativ zu DIESEM Block (seinem direkten Elternteil),
    // draw() addiert aber nur das eigene "offset" - die eigene "position" muss hier
    // manuell mit addiert werden, sonst landet der Rahmen bei jedem Block außer dem
    // ersten (dessen eigene Position ~0 ist) an der falschen Stelle
    void render(sf::RenderTarget& target) override
    {
        sf::RectangleShape border(chart->size);
        border.setPosition(position + chart->position);
        border.setFillColor(sf::Color::Transparent);
        border.setOutlineColor(sf::Color(110, 110, 110));
        border.setOutlineThickness(-1.0f);
        GUI::Item::draw(target, border);
    }

    void update() override
    {
        if (!colony) { return; }
        const uint32_t total    = to<uint32_t>(colony->ants.size());
        const uint32_t soldiers = colony->soldiersCount();
        const uint32_t workers  = total - soldiers;
        population_value->setText(toStr(total));
        workers_soldiers_value->setText(toStr(workers) + " / " + toStr(soldiers));
        queen_value->setText(colony->queen.isAlive() ? "Yes" : "No");
    }
};

// MS02: Statistik-Übersicht oben rechts über dem Spielfeld - eingeklappt nur eine
// schmale Kopfzeile ("Statistics" + Toggle), aufgeklappt wächst sie nach unten genau
// so weit wie der tatsächliche Inhalt (siehe NamedContainer: root->size_type.y =
// FitContent), also nur so viele Blöcke wie aktuell aktive Kolonien existieren
struct StatisticsPanel : public GUI::NamedContainer
{
    static constexpr float width = 220.0f;

    Simulation& simulation;
    ControlState& control_state;
    std::vector<SPtr<StatisticsColonyBlock>> colony_blocks;
    std::vector<uint8_t> displayed_colony_ids;
    SPtr<GUI::Container> predator_kills_row;
    SPtr<GUI::TextLabel> predator_kills_value;
    SPtr<GUI::EmptyItem> predator_kills_spacer;
    bool predator_kills_shown = false;

    StatisticsPanel(Simulation& simulation_, ControlState& control_state_)
        : GUI::NamedContainer("Statistics", GUI::Container::Orientation::Vertical)
        , simulation(simulation_)
        , control_state(control_state_)
    {
        padding = 8.0f;
        label->setCharacterSize(15); // MS02: +2px (vorher 13)
        spacing = 14.0f; // MS02: mehr Abstand zwischen "Statistics"-Kopfzeile und erstem Block (vorher 5)
        root->spacing = 16.0f; // MS02: mehr Abstand zwischen den Kolonie-Blöcken (vorher 10)

        header->addItem(create<GUI::EmptyItem>());
        auto stats_toggle = create<GUI::Toggle>();
        stats_toggle->setWidth(34.0f);
        stats_toggle->setHeight(20.0f);
        stats_toggle->setState(false); // MS02: Standard eingeklappt
        watch(stats_toggle, [this, stats_toggle]() {
            if (stats_toggle->state) { showRoot(); } else { hideRoot(); }
        });
        header->addItem(stats_toggle);
        hideRoot(); // MS02: passend zum Toggle-Standardzustand (aus)

        // MS02: nicht pro Kolonie, sondern eine einzige Zeile über alle Predators
        // summiert - ganz unten im Panel. Nur sichtbar, solange mindestens ein Predator
        // aktiv ist (siehe update()), deshalb hier NICHT direkt hinzugefügt.
        // Gleiches Label/Wert-Zeilen-Schema wie bei den Kolonie-Blöcken (siehe addStatRow).
        predator_kills_row = create<GUI::Container>(GUI::Container::Orientation::Horizontal);
        predator_kills_row->padding = 0.0f;
        predator_kills_row->spacing = 0.0f;
        predator_kills_row->size_type.y = GUI::Size::FitContent;
        auto predator_kills_label = create<GUI::TextLabel>("Predator Kills:", 12);
        predator_kills_label->setColor(sf::Color::Red);
        predator_kills_label->setAlignment(GUI::Alignment::Left);
        predator_kills_row->addItem(predator_kills_label);
        predator_kills_row->addItem(create<GUI::EmptyItem>());
        predator_kills_value = create<GUI::TextLabel>("0", 12);
        predator_kills_value->setColor(sf::Color::Red);
        predator_kills_value->setAlignment(GUI::Alignment::Left);
        predator_kills_row->addItem(predator_kills_value);

        // MS02: zusätzlicher Abstandshalter vor "Predator Kills" - mehr Lücke zur
        // letzten Kolonie als der normale root->spacing zwischen den Kolonien selbst
        predator_kills_spacer = create<GUI::EmptyItem>();
        predator_kills_spacer->setHeight(10.0f);

        setWidth(width);
    }

    // MS02: schwarzer Hintergrund + grauer Rahmen, gleicher Look wie die Sidebar
    // (sf::Color(60,60,60), -2px) statt des hellgrauen NamedContainer-Standards
    void render(sf::RenderTarget& target) override
    {
        sf::RectangleShape background(size);
        background.setPosition(position);
        background.setFillColor(sf::Color::Black);
        background.setOutlineColor(sf::Color(60, 60, 60));
        background.setOutlineThickness(-2.0f);
        GUI::Item::draw(target, background);
    }

    void update() override
    {
        // MS02: Predator-Kills-Zeile nur ein-/ausblenden, wenn sich ihre Sichtbarkeit
        // tatsächlich ändert (kein Predator aktiv -> komplett ausgeblendet)
        const bool has_predators = !simulation.predators.empty();
        if (has_predators != predator_kills_shown) {
            if (has_predators) {
                addItem(predator_kills_spacer);
                addItem(predator_kills_row);
            } else {
                removeItem(predator_kills_row);
                removeItem(predator_kills_spacer);
            }
            predator_kills_shown = has_predators;
        }
        if (has_predators) {
            uint32_t total_kills = 0;
            for (const Predator& p : simulation.predators) {
                total_kills += p.kills;
            }
            predator_kills_value->setText(toStr(total_kills));
        }

        // MS02: Kolonie-Blöcke nur neu aufbauen, wenn sich die Menge der aktiven
        // Kolonien tatsächlich geändert hat (Add/Remove) - sonst würde z.B. der
        // Populations-Graph bei jedem Frame seine Historie verlieren
        std::vector<uint8_t> current_ids;
        for (Colony& c : simulation.colonies) {
            current_ids.push_back(c.id);
        }
        if (current_ids == displayed_colony_ids) {
            return;
        }
        for (const SPtr<StatisticsColonyBlock>& block : colony_blocks) {
            removeItem(block);
        }
        colony_blocks.clear();
        for (Colony& c : simulation.colonies) {
            auto block = create<StatisticsColonyBlock>(simulation.colonies.getRef(c.id), control_state);
            addItem(block);
            colony_blocks.push_back(block);
        }
        // MS02: Predator-Kills-Zeile (falls sichtbar) ans Ende schieben, damit sie
        // unter den (neu hinzugefügten) Kolonie-Blöcken bleibt statt darüber
        if (predator_kills_shown) {
            removeItem(predator_kills_row);
            removeItem(predator_kills_spacer);
            addItem(predator_kills_spacer);
            addItem(predator_kills_row);
        }
        displayed_colony_ids = current_ids;
    }
};

}
