#pragma once
#include "editor/GUI/container.hpp"
#include "editor/GUI/button.hpp"
#include "colony_tool.hpp"
#include "simulation/simulation.hpp"


namespace edtr
{

struct ColonyCreator : public GUI::NamedContainer
{
    // MS02: ein ColonyTool pro fester Slot-Farbe (Conf::COLONY_COLORS), von Anfang an
    // sichtbar (Focus/Set Position/Remove bleiben an Ort und Stelle) - nur dessen
    // toggle_button + colony wechseln zwischen leer ("Add") und aktiv ("Remove")
    SPtr<ColonyTool> slots[Conf::MAX_COLONIES_COUNT];

    Simulation&   simulation;
    ControlState& control_state;
    int32_t       last_selected = -1;

    explicit
    ColonyCreator(Simulation& sim, ControlState& control_state_)
        : GUI::NamedContainer("Colonies", Container::Orientation::Vertical)
        , simulation(sim)
        , control_state(control_state_)
    {
        root->size_type.y = GUI::Size::FitContent;
        header->addItem(create<GUI::EmptyItem>());

        // MS02: Kolonien spawnen nicht mehr automatisch - jeder Slot startet leer ("Add")
        for (uint32_t i = 0; i < Conf::MAX_COLONIES_COUNT; ++i) {
            auto tool = create<ColonyTool>(Conf::COLONY_COLORS[i], control_state);
            tool->on_select = [this](int8_t id){
                select(id);
            };
            tool->on_toggle = [this, i](){
                toggleSlot(i);
            };
            this->addItem(tool);
            slots[i] = tool;
        }
    }

    // MS02: jeder Slot spawnt in einer anderen Ecke der Welt statt alle oben links -
    // sonst landen alle Kolonien direkt übereinander und interferieren sofort miteinander
    static sf::Vector2f getSpawnPosition(uint32_t index)
    {
        const float margin = 100.0f;
        const float world_w = to<float>(Conf::WORLD_WIDTH);
        const float world_h = to<float>(Conf::WORLD_HEIGHT);
        switch (index % 4) {
            case 0:  return {margin, margin};                     // oben links
            case 1:  return {world_w - margin, world_h - margin};  // unten rechts
            case 2:  return {margin, world_h - margin};            // unten links (statt oben rechts,
                                                                     // sonst vom Statistics-Panel verdeckt)
            default: return {world_w - margin, margin};            // oben rechts
        }
    }

    void toggleSlot(uint32_t index)
    {
        SPtr<ColonyTool>& tool = slots[index];
        if (tool->colony) {
            // Kolonie entfernen, Slot bleibt als Zeile bestehen, fällt nur auf "Add" zurück
            simulation.removeColony(tool->colony->id);
            tool->deactivate();
        } else {
            // Kolonie in diesem Slot erstellen, feste Slot-Farbe übernehmen
            const sf::Vector2f spawn_position = getSpawnPosition(index);
            auto new_colony = simulation.createColony(spawn_position.x, spawn_position.y);
            new_colony->setColor(Conf::COLONY_COLORS[index]);
            tool->activate(new_colony);
        }
    }

    void select(int32_t id)
    {
        int32_t selected = -1;
        if (id != last_selected) {
            selected = id;
        }
        last_selected = selected;
        this->simulation.world.renderer.selected_colony = selected;

        for (const auto& tool : slots) {
            tool->selected = tool->colony && tool->colony->id == selected;
        }
    }
};

}

