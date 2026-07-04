#include <SFML/Graphics.hpp>
#include <list>
#include <fstream>
#include "simulation/config.hpp"
#include "simulation/world/distance_field_builder.hpp"
#include "simulation/simulation.hpp"
#include "editor/editor_scene.hpp"

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

int main()
{
#ifdef _WIN32
    // MS02: ohne explizite DPI-Awareness virtualisiert Windows alle Koordinaten
    // (Bildschirmauflösung, Arbeitsfläche, Fensterposition) auf 96 DPI und skaliert das
    // Fenster danach selbst auf die echte Anzeige - das ergibt genau den "Fenster sitzt
    // verschoben/nicht zentriert" Effekt, weil unsere Berechnung unten in den virtuellen
    // Koordinaten passiert, das tatsächliche Fenster aber in echten Pixeln landet
    SetProcessDPIAware();
#endif

    // Load configuration
    if (Conf::loadUserConf())
    {
        std::cout << "Configuration file loaded." << std::endl;
    }
    else
    {
        std::cout << "Configuration file couldn't be found." << std::endl;
    }

    RNGf::initialize();

    sf::ContextSettings settings;
    settings.antialiasingLevel = 4;
    
    // MS02: Fenstergröße an Bildschirmauflösung anpassen statt fester 1920x1080
    // MS02: Spielfeld bleibt fest 1920x1080 (Conf-Defaults) - nur das Fenster
    // füllt den Bildschirm, das Spielfeld wird in updateRendererLayout() passend skaliert
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    Conf::WIN_WIDTH = desktop.width;
    Conf::WIN_HEIGHT = desktop.height;

#ifdef _WIN32
    // MS02: unter Windows die Arbeitsfläche (ohne Taskleiste) statt der vollen
    // Bildschirmauflösung verwenden - sonst ragt das Fenster unter die Taskleiste
    RECT work_area;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &work_area, 0);

    // MS02: sf::VideoMode-Größe ist nur die Client-Area, Titelleiste/Rahmen kommen vom
    // Betriebssystem noch oben drauf. Ohne das hier rauszurechnen sitzt das Fenster im
    // normalen Fenstermodus (mit Titelleiste) verschoben/zu groß, weil das Außenmaß dann
    // über die Arbeitsfläche hinausragt statt exakt mit ihr übereinzustimmen.
    RECT decoration = {0, 0, 0, 0};
    AdjustWindowRectEx(&decoration, WS_OVERLAPPEDWINDOW, FALSE, 0);

    Conf::WIN_WIDTH  = (work_area.right - work_area.left) - (decoration.right - decoration.left);
    Conf::WIN_HEIGHT = (work_area.bottom - work_area.top) - (decoration.bottom - decoration.top);
#endif

    sf::RenderWindow window(sf::VideoMode(Conf::WIN_WIDTH, Conf::WIN_HEIGHT), "AntSim", sf::Style::Default, settings);
#ifdef _WIN32
    // MS02: setPosition() setzt unter Windows die Position des AUSSEN-Rahmens
    // (SetWindowPos arbeitet auf dem Window-Rect, nicht dem Client-Rect) - die Größe
    // oben ist bereits auf die Client-Area runtergerechnet, die Position braucht daher
    // keinen zusätzlichen Decoration-Abzug. Mit dem Abzug landete das Fenster um
    // Rahmenbreite/Titelleistenhöhe nach rechts/unten verschoben (das "verschobene
    // Fenster"-Problem).
    window.setPosition(sf::Vector2i(work_area.left, work_area.top));
#endif
    window.setFramerateLimit(60);

    // Initialize simulation
    Simulation simulation(window);
    // Create editor scene around it
    GUI::Scene::Ptr scene = create<edtr::EditorScene>(window, simulation);
    scene->resize();
    // Main loop
    while (window.isOpen())
    {
        // Update
        scene->update();
        // Render
        window.clear(sf::Color::Black);
            scene->render();
        window.display();
    }
    return 0;
}
