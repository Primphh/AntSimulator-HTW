#pragma once
#include <SFML/Graphics.hpp>
#include "simulation/config.hpp"

struct ViewportHandler
{
    struct State
    {
        sf::Vector2f center;
        sf::Vector2f offset;
        float zoom;
        bool clicking;
        sf::Vector2f mouse_position;
        sf::Vector2f mouse_world_position;
        sf::RenderStates state;

        State(sf::Vector2f render_size, const float base_zoom = 5.0f)
            : center(render_size.x * 0.5f, render_size.y * 0.5f), offset(center / base_zoom), zoom(base_zoom), clicking(false), mouse_position(), mouse_world_position(), state()
        {
        }

        void updateState()
        {
            const float z = zoom;
            const float inv_z = 1.0f / z;
            state = sf::RenderStates();
            state.transform.translate(center);
            state.transform.scale(z, z);
            state.transform.translate(-offset);
        }

        void updateMousePosition(sf::Vector2f new_position)
        {
            mouse_position = new_position;
            const sf::Vector2f pos(static_cast<float>(new_position.x), static_cast<float>(new_position.y));
            mouse_world_position = offset + (pos - center) / zoom;
        }
    };

    State state;
    // MS02: untere Zoom-Grenze ist jetzt dynamisch (= aktueller "contain"-Fit-Zoom aus
    // EditorScene::updateRendererLayout), nicht mehr fix 1.0 - sonst klemmt der Zoom fest,
    // wenn der Fit-Zoom selbst schon < 1.0 ist (z.B. Fenster kleiner als 1920x1080)
    float min_zoom = 1.0f;

    void setMinZoom(float new_min_zoom)
    {
        min_zoom = new_min_zoom;
    }

    ViewportHandler(sf::Vector2f size)
        : state(size)
    {
        state.updateState();
    }

    // MS02: Offset auf die ECHTE Weltgröße begrenzt (Conf::WORLD_WIDTH/HEIGHT), nicht mehr auf
    // center*2 - center ist seit dem "contain"-Fit nur noch halb so groß wie die gefittete Box,
    // nicht mehr die (ungefähre) Weltgröße. Wird bei JEDER Zoom-/Pan-Änderung aufgerufen, sonst
    // bleibt ein bei hohem Zoom gültiger Offset beim Rauszoomen ungültig (Bild "rutscht" weg).
    void clampOffset()
    {
        const float world_w = to<float>(Conf::WORLD_WIDTH);
        const float world_h = to<float>(Conf::WORLD_HEIGHT);
        const float half_x = state.center.x / state.zoom;
        const float half_y = state.center.y / state.zoom;
        state.offset.x = std::max(half_x, std::min(state.offset.x, world_w - half_x));
        state.offset.y = std::max(half_y, std::min(state.offset.y, world_h - half_y));
    }

    void addOffset(sf::Vector2f v)
    {
        state.offset += v / state.zoom;
        clampOffset();
        state.updateState();
    }

    void zoom(float f)
    {
        // MS02: zoomt zum aktuellen Mauszeiger hin statt immer zur View-Mitte - der Weltpunkt
        // unter dem Cursor (state.mouse_world_position, mit dem ALTEN offset/zoom berechnet)
        // bleibt dabei exakt unter dem Cursor stehen. clampOffset() danach sorgt weiterhin
        // dafür, dass die Kamera nicht über den Weltrand hinaus geschoben werden kann.
        const sf::Vector2f mouse_world = state.mouse_world_position;
        state.zoom *= f;
        state.offset = mouse_world - (state.mouse_position - state.center) / state.zoom;
        clampOffset();
        state.updateState();
    }

    void wheelZoom(float w)
    {
        // MS02: Zoom-Bereich begrenzt - kein Rauszoomen unter den aktuellen Fit-Zoom (min_zoom)
        const float zoom_max = 15.0f;
        if (w)
        {
            const float zoom_amount = 1.2f;
            const float delta = w > 0 ? zoom_amount : 1.0f / zoom_amount;
            const float new_zoom = state.zoom * delta;
            if (new_zoom >= min_zoom && new_zoom <= zoom_max)
            {
                zoom(delta);
            }
            else if (delta < 1.0f && state.zoom > min_zoom)
            {
                // MS02: letzter Zoom-Out-Schritt würde unter min_zoom rutschen - stattdessen
                // exakt auf min_zoom springen statt stehenzubleiben
                state.zoom = min_zoom;
                clampOffset();
                state.updateState();
            }
        }
        else
        {
            zoom(1.0f);
        }
    }

    void reset()
    {
        state.zoom = 1.0f;
        setFocus(state.center);
    }

    [[nodiscard]]
    const sf::RenderStates &getRenderState() const
    {
        return state.state;
    }

    void click(sf::Vector2f relative_click_position)
    {
        state.mouse_position = relative_click_position;
        state.clicking = true;
    }

    void click()
    {
        state.clicking = true;
    }

    void unclick()
    {
        state.clicking = false;
    }

    void setMousePosition(sf::Vector2f new_mouse_position)
    {
        if (state.clicking)
        {
            addOffset(state.mouse_position - new_mouse_position);
        }
        state.updateMousePosition(new_mouse_position);
    }

    void setFocus(sf::Vector2f focus_position)
    {
        state.offset = focus_position;
        // MS02: ohne Clamping konnte die Kamera über den Weltrand hinaus zentrieren (z.B.
        // beim "Focus"-Button auf ein Nest nahe am Rand) und damit Leerraum außerhalb der
        // Welt zeigen, statt den Rand sichtbar zu lassen
        clampOffset();
        state.updateState();
    }

    void setZoom(float zoom)
    {
        state.zoom = zoom;
        clampOffset(); // MS02: siehe setFocus() - Offset kann beim neuen Zoom ungültig werden
        state.updateState();
    }

    sf::Vector2f getMouseWorldPosition() const
    {
        return state.mouse_world_position;
    }

    sf::Vector2f getScreenCoords(sf::Vector2f world_pos) const
    {
        return state.state.transform.transformPoint(world_pos);
    }
};
