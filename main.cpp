#include <string>
#include <iostream>
#include <filesystem>
#include <windows.h>
#include <vector>
#include <algorithm>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

namespace fs = std::filesystem;

// Definim aici constante, simplifica foarte mult modificarea acestora in mai multe parti ale codului, si sunt mai usor de gasit
#define POS_OFFSET         25
#define BOX_HEIGHT         20
#define PADDING_LEFT       10
#define PADDING_TOP        60
#define LINE_PADDING        5
#define SCROLLBAR_WIDTH    15
#define THUMB_MIN_HEIGHT   50
#define LINE_WIDTH          5
#define ICON_SIZE          16

// Mult mai ok decat un string, mai eficient in spatiu de memorie si in comparare (un simplu switch)
enum sortingType{
    SORT_NAME,
    SORT_DATE,
    SORT_TYPE,
    SORT_SIZE,
};

struct entry {
    fs::path path;
    bool selected;
    sf::RectangleShape box;
    sf::Rect<float> areaBox;
    std::string name;
};

int current;

struct fileWindow {
    fs::path currentPath;
    std::vector<entry> list;
    sortingType sort;
    int sortOrder; //ascending sau descending.
    float scrollAmount; // Offset-ul pozitiei butoanelor atunci cand dai scroll
} filewindow[2], historywindow[2];

struct scrollBar{
   sf::Rect<float> track;
   sf::Rect<float> thumb = {{0,0},{0,0}};
} scrollbar[2];

sf::Font font_listFile("utfont.ttf");
sf::Texture texture_cmdIconPlaceholder;
sf::Texture texture_dirIcon;
sf::Texture texture_regularIcon;
sf::Texture texture_backIcon;

void _draw_scrollbars(fileWindow filewindow[2], scrollBar scrollbar[2], sf::RenderWindow &window){
    sf::RectangleShape box;
    sf::Vector2f pos;
    sf::Vector2f size;
    // Track:
    for(int side=0;side<2;side++){
        pos = {(side*window.getSize().x/2)+window.getSize().x/2-SCROLLBAR_WIDTH-(float)(1-side)*LINE_WIDTH/2-side*2, PADDING_TOP};
        size = {SCROLLBAR_WIDTH, window.getSize().y-(float)PADDING_TOP};
        sf::Rect<float> areaBox(pos, size);
        box.setPosition(pos);
        box.setSize(size);
        scrollbar[side].track = areaBox;
        box.setFillColor(sf::Color(200, 200, 200));
        window.draw(box);
    }
    // Thumb:
    for(int side=0;side<2;side++){
        pos = {(side*window.getSize().x/2)+window.getSize().x/2-SCROLLBAR_WIDTH-(float)(1-side)*LINE_WIDTH/2-side*2, scrollbar[side].thumb.position.y};
        if(filewindow[side].list.size()*POS_OFFSET-scrollbar[side].track.size.y <= 0){
            size = {SCROLLBAR_WIDTH, scrollbar[side].track.size.y};
        }
        else size = {SCROLLBAR_WIDTH, THUMB_MIN_HEIGHT};
        sf::Rect<float> areaBox(pos, size);
        box.setPosition(pos);
        box.setSize(size);
        box.setFillColor(sf::Color(70, 70, 70));
        scrollbar[side].thumb = areaBox;
        window.draw(box);
    }
}

void _thumb_from_scroll(fileWindow filewindow, scrollBar &scrollbar){
    scrollbar.thumb.position.y = PADDING_TOP+(scrollbar.track.size.y - scrollbar.thumb.size.y)/(((filewindow.list.size()*POS_OFFSET-scrollbar.track.size.y)/POS_OFFSET))*((-filewindow.scrollAmount)/POS_OFFSET);
}
void _scroll_from_thumb(fileWindow &filewindow, scrollBar &scrollbar, sf::RenderWindow &window){
    if(sf::Mouse::getPosition(window).y > scrollbar.track.position.y + scrollbar.track.size.y - scrollbar.thumb.size.y){
        scrollbar.thumb.position.y = scrollbar.track.position.y + scrollbar.track.size.y - scrollbar.thumb.size.y;
    }
    else scrollbar.thumb.position.y = sf::Mouse::getPosition(window).y;
    if(filewindow.list.size()*POS_OFFSET-scrollbar.track.size.y > 0){
        filewindow.scrollAmount = -((scrollbar.thumb.position.y)*(((filewindow.list.size()*POS_OFFSET-scrollbar.track.size.y)/POS_OFFSET)/(scrollbar.track.size.y - scrollbar.thumb.size.y))*POS_OFFSET);
    }
}

// Este mai bine path in loc de index. Asa putem folosi aceeasi functie si pentru filewindow, cat si pentru historywindow
// De obs totusi ca conteaza ordinea in care se apeleaza functia
fs::path _get_hover_path(fileWindow filewindow, sf::RenderWindow &window, int side) {
    for (int i = 0; i < filewindow.list.size(); i++) {
        if (filewindow.list[i].areaBox.contains(sf::Vector2<float>(sf::Mouse::getPosition(window))))
            return filewindow.list[i].path;
    }
    return filewindow.currentPath;
}

// Doar actualizam partial fiecare filewindow, nu e nevoie de variabile temporare, etc.
void _draw_lists(fileWindow filewindow[2], sf::RenderWindow &window){
    sf::Text text(font_listFile);
    sf::Vector2f pos;
    sf::Vector2f size;
    for(int side=0;side<2;side++){
        for(int i=0;i<filewindow[side].list.size();i++){
            pos = {(float)ICON_SIZE+PADDING_LEFT*2+side*((window.getSize().x)/2), (float)i*POS_OFFSET+filewindow[side].scrollAmount+PADDING_TOP+PADDING_LEFT};
            size = {(float)(window.getSize().x)/2 - SCROLLBAR_WIDTH-ICON_SIZE-PADDING_LEFT*3, BOX_HEIGHT};
            sf::RectangleShape box;
            box.setPosition(pos);
            box.setSize(size);
            box.setFillColor(sf::Color::Transparent);
            box.setOutlineThickness(2);
            sf::Rect<float> areaBox(pos, size);
            text.setPosition(pos);
            text.setFont(font_listFile);
            text.setCharacterSize(16);
            text.setFillColor(sf::Color::White);
            text.setString(filewindow[side].list[i].name);
            if (filewindow[side].list[i].selected == true) box.setOutlineColor(sf::Color(160, 160, 160));
            else {
                if (areaBox.contains(sf::Vector2<float>(sf::Mouse::getPosition(window)))) {
                    text.setPosition({pos.x+PADDING_LEFT, pos.y});
                    box.setOutlineColor(sf::Color(70, 70, 70));
                }
                else box.setOutlineColor(sf::Color::Transparent);
            }

            // Nu se deseneaza peste historywindow
            if(pos.y >= PADDING_TOP){
                window.draw(box);
                window.draw(text);
                if(i == 0 && filewindow[side].list[i].path == filewindow[side].currentPath.parent_path()){
                    sf::Sprite iconSprite(texture_backIcon);
                    iconSprite.setPosition({pos.x-ICON_SIZE-PADDING_LEFT, pos.y+3});
                    window.draw(iconSprite);
                }
                else if(is_directory(filewindow[side].list[i].path)){
                    sf::Sprite iconSprite(texture_dirIcon);
                    iconSprite.setPosition({pos.x-ICON_SIZE-PADDING_LEFT, pos.y+3});
                    window.draw(iconSprite);
                }
                else{
                    sf::Sprite iconSprite(texture_regularIcon);
                    iconSprite.setPosition({pos.x-ICON_SIZE-PADDING_LEFT, pos.y+3});
                    window.draw(iconSprite);
                }

                filewindow[side].list[i].box = box;
                filewindow[side].list[i].areaBox = areaBox;
            }
        }
    }
}

fileWindow _draw_list_history_button(sf::Vector2f pos, fileWindow file, sf::RenderWindow &window) {
    entry button;
    float additional_x_position;
    fileWindow history;
    history.currentPath = file.currentPath;
    while (file.currentPath.has_relative_path()) { // getting each directory
        entry temp;
        temp.path = file.currentPath;
        temp.selected = false;
        temp.name = file.currentPath.filename().string();
        history.list.push_back(temp);
        file.currentPath = file.currentPath.parent_path();
    }
    // getting the root drive
    entry temp;
    temp.path = file.currentPath.root_path();
    temp.selected = false;
    temp.name = file.currentPath.root_name().string();
    history.list.push_back(temp);

    while (history.list.size() > 0) { // calculating how many paths can be displayed (for auto-resize)
        additional_x_position = PADDING_LEFT;
        for (int j=0; j < history.list.size(); j++) {
            sf::Text text(font_listFile);
            text.setString(history.list[j].path.filename().string());
            text.setCharacterSize(20);
            additional_x_position += 6 + text.getGlobalBounds().size.x;
            text.setString(">");
            additional_x_position += text.getGlobalBounds().size.x;
        }
        if (additional_x_position >= window.getSize().x / 2) {
            history.list.pop_back();
        }
        else break;
    }

    additional_x_position = PADDING_LEFT;
    for (int i = history.list.size() - 1; i >= 0; i--) { // drawing the directories in their actual order (history contains them last to first)
        sf::Text text(font_listFile);
        text.setString(history.list[i].name);
        text.setCharacterSize(16);
        text.setPosition(pos + sf::Vector2f(additional_x_position, 0));

        sf::Rect<float> areaBox(pos + sf::Vector2<float>(additional_x_position, 4), text.getGlobalBounds().size);
        history.list[i].areaBox = areaBox;

        sf::RectangleShape box;
        box.setPosition(pos + sf::Vector2<float>(additional_x_position, 20));
        box.setSize(sf::Vector2f(text.getGlobalBounds().size.x, 0));
        box.setFillColor(sf::Color(0, 0, 0, 0));
        box.setOutlineThickness(1);

        if (history.list[i].selected == true) box.setOutlineColor(sf::Color(160, 160, 160));
        else {
            if (areaBox.contains(sf::Vector2<float>(sf::Mouse::getPosition(window)))) {
                box.setOutlineColor(sf::Color(255, 255, 255));
            }
            else box.setOutlineColor(sf::Color(0, 0, 0, 0));
        }

        history.list[i].box = box;
        window.draw(box);

        additional_x_position += 2;
        window.draw(text);
        additional_x_position += 2 + text.getGlobalBounds().size.x;

        if (i > 0) {
            text.setString(">");
            text.setPosition(pos + sf::Vector2f(additional_x_position, 0));
            text.setCharacterSize(16);
            window.draw(text);
            additional_x_position += 2 + text.getGlobalBounds().size.x;
        }
    }
    return history;
}

// Functii ajutatoare, nu le modific momentan
void _print_path(fs::path path) {
    std::cout << path << std::endl;
}
void _print_list(std::vector<entry> list) {
    int index = 0;
    for (auto e : list) {
        std::cout << e.path << " " << e.selected << " " << index << "\n";
        index++;
    }
}

// Este de asemenea void, deoarece actualizam, nu reinitializam
void _update_window(fileWindow &window, fs::path newPath) {
    window.list.clear();
    window.currentPath = newPath;

    // Primul element e Parent Directory
    if(newPath.root_path() != newPath){
        entry temp;
        temp.path = newPath.parent_path();
        temp.selected = false;
        temp.name = "..";
        window.list.push_back(temp);
    }

    for (auto e : fs::directory_iterator(newPath))
        if(!(GetFileAttributes(e.path().string().c_str()) & FILE_ATTRIBUTE_HIDDEN)){
            entry temp;
            temp.path = e.path();
            temp.selected = false;
            temp.name = e.path().filename().string();
            window.list.push_back(temp);
        }

    // Sortare:
    auto first = window.list.begin();
    auto last = window.list.end();
    switch(window.sort){
        case SORT_NAME:{
            // Sortarea asta e by default lol
            // Totusi, ar trebui ca SORT_DIR_FIRST sa fie implicit (zic asta raportandu-ma la Explorer, vedem)
            // Totusi nu implementez, in schimb implementez sortarile din Explorer.
            break;
        };
        case SORT_DATE:{
            std::sort(first, last, [](entry a, entry b){
                return fs::last_write_time(a.path) > last_write_time(b.path);
            });
            break;
        };
        case SORT_TYPE:{
            std::sort(first, last, [](entry a, entry b){
                return a.path.extension() > b.path.extension();
            });
            break;
        };
        case SORT_SIZE:{
            // Aici trebuie sa fac SORT_DIR_FIRST, vreau nu vreau. probabil ar trebui sa mut bucata asta de cod in afara switch-ului
            std::sort(first, last, [](entry a, entry b){
                return !fs::is_directory(a.path) && !fs::is_directory(b.path) && fs::file_size(a.path) > fs::file_size(b.path);
            });
            break;
        };

        // 0 - ascending
        // 1 - descending
        if(window.sortOrder == 1){
            std::reverse(first, last);
        }
    }
}

// Acum deschidem cu un path. Acelasi motiv ca la _get_hover_path
void _command_open(fileWindow& window, fs::path p) {
    if (window.currentPath != p) {
        if (fs::is_directory(p)) {
            window.scrollAmount = 0;
            _update_window(window, p);
        }
        else { // Automatically find a matching app to open the given file

            ShellExecute(NULL, "open", p.string().c_str(), NULL, NULL, SW_SHOW);
        }
    }
}

void _command_back(fileWindow& window) {
    _update_window(window, window.currentPath.parent_path());
}

void _command_select(fileWindow& window, int index) {
    window.list[index].selected = true;
}
void _command_deselect(fileWindow& window, int index) {
    window.list[index].selected = false;
}

void _command_copy(fileWindow source, fileWindow& dest) {
    for (auto e : source.list) {
        if (e.selected == true) {
            copy(e.path, dest.currentPath);
        }
    }
    _update_window(dest, dest.currentPath);
}

void _command_move(fileWindow& source, fileWindow& dest) {
    for (auto e : source.list) {
        if (e.selected == true) {
            rename(e.path, dest.currentPath / e.path.filename());
        }
    }
    _update_window(source, source.currentPath);
    _update_window(dest, dest.currentPath);
}
void _command_delete(fileWindow& window) {
    for (auto e : window.list) {
        if (e.selected == true) {
            remove(e.path);
        }
    }
    _update_window(window, window.currentPath);
}

// Momentan, redenumirea se face doar asupra unui singur fisier. Altfel, trebuie sa adaugam (nr) la finalul numelor fisierelor, deci verific daca doar un singur fisier e selectat.
void _command_rename(fileWindow& window, fs::path newName) {
    auto first = window.list.begin();
    auto last = window.list.end();
    auto is_selected = [](entry e) {return e.selected == true; }; // o functie care este apelata de count_if pentru fiecare element din lista
    if (count_if(first, last, is_selected) == 1) {
        std::cout << "Exactly one file must be selected for renaming";
    }
    else {
        auto selectedEntry = find_if(first, last, is_selected); // entry-ul care are .selected = true
        rename(selectedEntry->path, selectedEntry->path.parent_path() / newName);
    }
    _update_window(window, window.currentPath);
}

void _draw_ui(sf::RenderWindow &window){
    sf::RectangleShape bgbar;
    bgbar.setPosition(sf::Vector2f(0, 0)); // border top
    bgbar.setFillColor(sf::Color::White);
    bgbar.setSize(sf::Vector2f(window.getSize().x, 60));
    window.draw(bgbar);

    bgbar.setPosition(sf::Vector2f(0, 30)); // history bar bg
    bgbar.setFillColor(sf::Color(58, 52, 74));
    bgbar.setSize(sf::Vector2f(window.getSize().x, 25));
    window.draw(bgbar);

    bgbar.setPosition(sf::Vector2f(0, 0)); // border left
    bgbar.setFillColor(sf::Color::White);
    bgbar.setSize(sf::Vector2f(2, window.getSize().y));
    window.draw(bgbar);

    bgbar.setPosition(sf::Vector2f(window.getSize().x - 2, 0)); // border right
    bgbar.setFillColor(sf::Color::White);
    bgbar.setSize(sf::Vector2f(2, window.getSize().y));
    window.draw(bgbar);

    bgbar.setSize(sf::Vector2f(5, window.getSize().y)); // middle split
    bgbar.setPosition(sf::Vector2f(window.getSize().x / 2 - 2.5, 0));
    bgbar.setFillColor(sf::Color::White);
    window.draw(bgbar);

    bgbar.setPosition(sf::Vector2f(0, 0)); // top split (icons/shortcuts, whatever was there)
    bgbar.setFillColor(sf::Color(200, 200, 200));
    bgbar.setSize(sf::Vector2f(window.getSize().x, 25));
    window.draw(bgbar);
}

int main()
{
    if(!texture_cmdIconPlaceholder.loadFromFile("cmdIconPlaceholder.png") ||
       !texture_dirIcon.loadFromFile("dirIcon.png") ||
       !texture_regularIcon.loadFromFile("regularFile.png") ||
       !texture_backIcon.loadFromFile("backIcon.png")
       ) return -1;

    current = 0;
    _update_window(filewindow[0], fs::current_path());
    _update_window(filewindow[1], "C:\\");

    sf::RenderWindow window;
    window.create(sf::VideoMode({ 800, 600 }), "My window");

    _thumb_from_scroll(filewindow[0], scrollbar[0]);
    _thumb_from_scroll(filewindow[1], scrollbar[1]);
        _draw_lists(filewindow, window);
    while (window.isOpen())
    {
        window.clear(sf::Color(20, 23, 36));
        _draw_ui(window);

        historywindow[0] = _draw_list_history_button(sf::Vector2f(PADDING_LEFT, 30), filewindow[0], window);
        historywindow[1] = _draw_list_history_button(sf::Vector2f(window.getSize().x / 2 + PADDING_LEFT, 30), filewindow[1], window);

        if (sf::Mouse::getPosition(window).x > window.getSize().x / 2 + LINE_PADDING) current = 1;
        else current = 0;
        _draw_lists(filewindow, window);
        _draw_scrollbars(filewindow, scrollbar, window);

        while (std::optional event = window.pollEvent()) // event-uri/actiuni din sisteme periferice
        {
            if (event->is<sf::Event::Closed>())
                window.close();
            if (const auto* resized = event->getIf<sf::Event::Resized>()) // reajustarea marimii la resize
            {
                sf::FloatRect visibleArea({0.f, 0.f}, sf::Vector2f(resized->size));
                window.setView(sf::View(visibleArea));
            }
            if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
                sf::Vector2<float> mousePos = (sf::Vector2<float>)sf::Mouse::getPosition(window);
                if(mousePos.y >= scrollbar[current].track.position.y && mousePos.y <= scrollbar[current].track.position.y+scrollbar[current].track.size.y){
                    scrollbar[current].thumb.position.y = scrollbar[current].track.position.y;
                    _scroll_from_thumb(filewindow[current], scrollbar[current], window);
                    _draw_scrollbars(filewindow, scrollbar, window);
                }
            }
            if (event->is<sf::Event::MouseButtonPressed>()) {
                    {
                        _command_open(filewindow[current], _get_hover_path(historywindow[current], window, current));
                        _command_open(filewindow[current], _get_hover_path(filewindow[current], window, current));

                        filewindow[current].scrollAmount = 0;
                        _thumb_from_scroll(filewindow[current], scrollbar[current]);
                        _draw_scrollbars(filewindow, scrollbar, window);
                    }
            }
            if(event->is<sf::Event::KeyPressed>()){
                if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::B)){
                    _command_back(filewindow[current]);
                    filewindow[current].scrollAmount = 0;
                    _thumb_from_scroll(filewindow[current], scrollbar[current]);
                    _draw_scrollbars(filewindow, scrollbar, window);
                }
            }
            // Testare sortari. In viitor le vom pune pe butoane
            if(event->is<sf::Event::KeyPressed>()){
                if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q)){
                    filewindow[current].sort = SORT_NAME;
                    std::cout << "Sorted by name!\n";
                }
                if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)){
                    filewindow[current].sort = SORT_DATE;
                    std::cout << "Sorted by date!\n";
                }
                if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E)){
                    filewindow[current].sort = SORT_TYPE;
                    std::cout << "Sorted by type!\n";
                }
                if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R)){
                    filewindow[current].sort = SORT_SIZE;
                    std::cout << "Sorted by size!\n";
                }
                _update_window(filewindow[current], filewindow[current].currentPath);
            }
            // I hate sfml3 events AAAAHHH
            if (auto *mousewheelscrolled = event->getIf<sf::Event::MouseWheelScrolled>()) {
                if (mousewheelscrolled->wheel == sf::Mouse::Wheel::Vertical) {
                    if(mousewheelscrolled->delta > 0){
                        filewindow[current].scrollAmount += POS_OFFSET;
                    }
                    if(mousewheelscrolled->delta < 0){
                        filewindow[current].scrollAmount -= POS_OFFSET;
                    }

                    // Verifica sa nu mearga in afara
                    if(filewindow[current].list.size()*POS_OFFSET + filewindow[current].scrollAmount < window.getSize().y-PADDING_TOP){
                        filewindow[current].scrollAmount = -((float)(filewindow[current].list.size())*POS_OFFSET)+(float)window.getSize().y-PADDING_TOP;
                    }
                    if(filewindow[current].scrollAmount > 0){
                        filewindow[current].scrollAmount = 0;
                    }
                    _thumb_from_scroll(filewindow[current], scrollbar[current]);
                }
            }
        }

        window.display();
    }
    return 0;
}
