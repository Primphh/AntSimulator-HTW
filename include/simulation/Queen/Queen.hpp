#pragma once
#include <SFML/Graphics.hpp>

// MS02: The Queen sits in the colony's nest.
// While she is alive, the colony can produce new ants.
// If a predator gets too close, she loses health and dies.
struct Queen
{
    // Fixed parameters
    static constexpr float max_health = 100.0f; // full life
    static constexpr float damage_radius = 25.0f;  // how close a predator must be to hurt her
    static constexpr float display_size = 22.0f;  // size of the image on screen (tune this)

    // State
    sf::Vector2f position;
    float        health = max_health;
    bool         alive = true;

    Queen() = default;

    // Load the queen image only once and share it for all queens
    static const sf::Texture& getTexture()
    {
        static sf::Texture texture;
        static bool loaded = false;
        if (!loaded) {
            texture.loadFromFile("res/queen.png");
            texture.setSmooth(true);
            loaded = true;
        }
        return texture;
    }

    // Queen loses some health. She dies when health reaches 0.
    void takeDamage(float amount)
    {
        if (!alive) { return; }
        health -= amount;
        if (health <= 0.0f) {
            health = 0.0f;
            alive = false;
        }
    }

    bool isAlive() const
    {
        return alive;
    }

    // Draw the queen image. Grey when dead so you can see it died.
    void render(sf::RenderTarget& target, const sf::RenderStates& states) const
    {
        const sf::Texture& tex = getTexture();
        const sf::Vector2u   size = tex.getSize();

        // Fallback: if queen.png is missing, draw a simple circle instead
        if (size.x == 0 || size.y == 0) {
            sf::CircleShape body(8.0f);
            body.setOrigin(8.0f, 8.0f);
            body.setPosition(position);
            body.setFillColor(alive ? sf::Color(255, 215, 0) : sf::Color(90, 90, 90));
            target.draw(body, states);
            return;
        }

        sf::Sprite sprite(tex);
        sprite.setOrigin(size.x / 2.0f, size.y / 2.0f);  // center the image
        sprite.setPosition(position);

        // Scale the image to the wanted size, no matter how big the png is
        const float scale = display_size / static_cast<float>(size.x);
        sprite.setScale(scale, scale);

        if (!alive) {
            sprite.setColor(sf::Color(120, 120, 120));  // grey = dead
        }
        target.draw(sprite, states);
    }
};