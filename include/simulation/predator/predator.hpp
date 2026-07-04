#pragma once
#include <SFML/Graphics.hpp>
#include "simulation/world/world.hpp"
#include "simulation/colony/colony.hpp"
#include "common/number_generator.hpp"
#include "common/direction.hpp"
#include "common/utils.hpp"

// MS02: Raeuber-Struct - ein autonomer Agent, der sich selbststaendig auf der Karte
// bewegt, Ameisen aller Kolonien jagt und toetet, sowie die Koenigin beschaedigen kann.
// Daten und Verhalten sind hier kombiniert (anders als bei Ant + WorkerUpdater/SoldierUpdater),
// da die Logik des Raeubers deutlich einfacher ist und keine Pheromone benoetigt.
struct Predator
{
    // Feste Parameter - constexpr fuer Null-Overhead zur Laufzeit
    static constexpr float move_speed = 60.0f;      // Bewegungsgeschwindigkeit (50% schneller als Ameisen mit 40.0f)
    static constexpr float hunt_radius = 80.0f;      // Erkennungsradius fuer Beute in Pixeln
    static constexpr float kill_radius = 6.0f;       // Kontaktradius zum Toeten einer Ameise
    static constexpr float direction_noise = PI * 0.03f; // Zufaelliges Richtungsrauschen beim Wandern (+-3.6 Grad pro Tick)

    // Zustandsvariablen
    sf::Vector2f position;                 // Aktuelle Weltposition des Raeubers
    Direction    direction;                // Bewegungsrichtung (nutzt bestehende Direction-Klasse fuer weiche Rotation)
    bool         hunting = false;          // true, solange eine Beute im Erkennungsradius ist
    uint32_t     kills = 0;             // MS02: Anzahl getoeteter Ameisen fuer die Statistikanzeige
    std::shared_ptr<sf::Texture> texture; // Spinnen-Textur (predator.png), shared_ptr fuer sichere Vektorkopien

    // Konstruktor - setzt Startposition, zufaellige Anfangsrichtung und laedt Textur
    Predator(float x, float y)
        : position(x, y)
        , direction(RNGf::getUnder(2.0f * PI))
        , hunting(false)
        , texture(std::make_shared<sf::Texture>())
    {
        texture->loadFromFile("res/predator.png");
        texture->setSmooth(true);
    }

    // Sucht die naechste lebende Ameise innerhalb von hunt_radius.
    // Anders als Ameisen (32 Zufallspunkte im 90-Grad-Kegel) prueft der Raeuber
    // alle Ameisen aller Kolonien erschoepfend und waehlt die naechste aus.
    // Gibt true zurueck, wenn eine Beute gefunden wurde.
    bool huntAnts(civ::Vector<Colony>& colonies, World& world)
    {
        float closest_dist = hunt_radius;
        Ant* target = nullptr;

        // Alle Kolonien und deren Ameisen durchsuchen
        for (Colony& colony : colonies) {
            for (Ant& ant : colony.ants) {
                if (ant.isDead()) { continue; }
                const float dist = getLength(ant.position - position);
                if (dist < closest_dist) {
                    closest_dist = dist;
                    target = &ant;
                }
            }
        }

        if (target) {
            hunting = true;
            // Richtung direkt zur Beute ausrichten (Zufallsrauschen wird in update() unterdrueckt)
            const sf::Vector2f to_target = getNormalized(target->position - position);
            direction = getAngle(to_target);
            // terminate() statt kill() - damit laeuft die Ameise sauber durch
            // killWeakAnts() -> removeDeadAnts(), ohne den Renderer-Zaehler zu umgehen.
            // Direktes kill() wuerde zu "vector subscript out of range" beim Renderer fuehren.
            if (closest_dist < kill_radius) {
                target->terminate();
                ++kills;
                hunting = false;
            }
            return true;
        }

        hunting = false;
        return false;
    }

    // Positionsaktualisierung - nutzt getFirstHit() Raycasting aus WorldGrid,
    // genau wie Ameisen. Bei Wandkontakt wird die Richtung an der Kollisionsnormale gespiegelt
    // (Reflexion), sodass der Raeuber von Waenden abprallt.
    void updatePosition(World& world, float dt)
    {
        sf::Vector2f v = direction.getVec();
        const HitPoint hit = world.map.getFirstHit(position, v, dt * move_speed);
        if (hit.cell) {
            // Richtungsvektor an Kollisionsnormale spiegeln
            v.x *= hit.normal.x != 0.0f ? -1.0f : 1.0f;
            v.y *= hit.normal.y != 0.0f ? -1.0f : 1.0f;
            direction.setDirectionNow(v);
        }
        else {
            position += (dt * move_speed) * v;
        }
    }

    // Hauptupdate - wird jeden Tick von Simulation::update() aufgerufen.
    // Reihenfolge: Richtung glaetten -> Beute suchen -> Rauschen (nur bei Wandern) -> Bewegen
    void update(civ::Vector<Colony>& colonies, World& world, float dt)
    {
        direction.update(dt);
        huntAnts(colonies, world);
        // Zufaelliges Richtungsrauschen nur wenn kein aktives Ziel vorhanden (Wanderverhalten)
        if (!hunting) {
            direction += RNGf::getFullRange(direction_noise);
        }
        updatePosition(world, dt);
    }

    // Zeichnet den Raeuber als Spinnen-Sprite sowie einen halbtransparenten
    // roten Erkennungsradius. sf::RenderStates wird uebergeben, damit Zoom und Pan
    // des ViewportHandlers korrekt angewendet werden (Weltkoordinaten statt Bildschirmkoordinaten).
    void render(sf::RenderTarget& target, const sf::RenderStates& states) const
    {
        if (!texture) { return; }

        // Spinnen-Sprite zentrieren, skalieren und in Bewegungsrichtung drehen
        sf::Sprite sprite(*texture);
        const sf::FloatRect bounds = sprite.getLocalBounds();
        sprite.setOrigin(bounds.width * 0.5f, bounds.height * 0.5f);
        sprite.setPosition(position);
        sprite.setScale(0.05f, 0.05f);                              // Skalierungsfaktor: an Bildschirmgroesse anpassen
        sprite.setColor(sf::Color(220, 50, 50));                    // Rote Faerbung zur visuellen Kennzeichnung
        sprite.setRotation(direction.getCurrentAngle() * 180.0f / PI);
        target.draw(sprite, states);

        // Halbtransparenter roter Erkennungskreis fuer die visuelle Darstellung des hunt_radius
        sf::CircleShape detection(hunt_radius);
        detection.setFillColor(sf::Color(220, 30, 30, 15));
        detection.setOutlineColor(sf::Color(220, 30, 30, 60));
        detection.setOutlineThickness(1.0f);
        detection.setOrigin(hunt_radius, hunt_radius);
        detection.setPosition(position);
        target.draw(detection, states);
    }
};