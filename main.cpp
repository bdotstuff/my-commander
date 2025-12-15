#include <string>
#include <iostream>
#include <filesystem>
#include <windows.h>
#include <conio.h>
#include <vector>
#include <algorithm>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

namespace fs = std::filesystem;

sf::RenderWindow mainWindow;

// Definim aici constante, simplifica foarte mult modificarea acestora in mai multe parti ale codului, si sunt mai usor de gasit
#define POS_OFFSET         25
#define BOX_HEIGHT         20
#define PADDING_LEFT       10
#define PADDING_TOP        100
#define PADDING_TOP_SRC    60
#define PADDING_LEFT_SRC   90
#define LINE_PADDING        5
#define SCROLLBAR_WIDTH    15
#define THUMB_MIN_HEIGHT   50
#define LINE_WIDTH          5
#define ICON_SIZE          16
#define DOUBLE_CLICK_TIME 500

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
    bool isBackEntry;
    sf::RectangleShape box;
    sf::Rect<float> areaBox;
    std::string name;
};

int current;
bool capsOn = false;

struct fileWindow {
    fs::path currentPath;
    std::vector<entry> list;
    sortingType sort;
    int sortOrder; //ascending sau descending.
    float scrollAmount; // Offset-ul pozitiei butoanelor atunci cand dai scroll
    bool isSearching;
    bool midSearch;
    std::string searchString;
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
    scrollbar.thumb.position.y = PADDING_TOP+(scrollbar.track.size.y - scrollbar.thumb.size.y)/(((filewindow.list.size()*POS_OFFSET-scrollbar.track.size.y+10)/POS_OFFSET))*((-filewindow.scrollAmount)/POS_OFFSET);
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
fs::path _get_hover_path(fileWindow filewindow, sf::RenderWindow &window) {
    for (int i = 0; i < filewindow.list.size(); i++) {
        if (filewindow.list[i].areaBox.contains(sf::Vector2<float>(sf::Mouse::getPosition(window))))
            return filewindow.list[i].path;
    }
    return filewindow.currentPath;
}
int _get_hover_index(fileWindow filewindow, sf::RenderWindow &window) {
    for (int i = 0; i < filewindow.list.size(); i++) {
        if (filewindow.list[i].areaBox.contains(sf::Vector2<float>(sf::Mouse::getPosition(window))))
            return i;
    }
    return -1;
}

// Doar actualizam partial fiecare filewindow, nu e nevoie de variabile temporare, etc.
void _draw_lists(fileWindow filewindow[2], sf::RenderWindow &window){
    sf::Text text(font_listFile);
    sf::Vector2f pos;
    sf::Vector2f size;
    for(int side=0;side<2;side++){

        sf::Text searchText(font_listFile);
        searchText.setCharacterSize(16);
        searchText.setPosition(sf::Vector2f(PADDING_LEFT_SRC+side*window.getSize().x/2, PADDING_TOP_SRC));
        searchText.setString(filewindow[side].searchString);
        window.draw(searchText);

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
            if (filewindow[side].list[i].selected == true){
                box.setOutlineColor(sf::Color(160, 160, 160));
                text.setPosition({pos.x+PADDING_LEFT, pos.y});
            }
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
                if(i == 0 && (filewindow[side].list[i].path == filewindow[side].currentPath.parent_path() || filewindow[side].list[i].isBackEntry)){
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

// am adaugat isNewWindow pt ca sa putem avea anumite actualizari care sa nu refaca complet lista
// de exemplu, daca sortam o reface complet (verificat in keyispressed), dar nu are loc pentru orice tasta (daca apasam ctrl pentru multiselect trebuie sa retina selectia anterioara)
void _update_window(fileWindow &window, fs::path newPath, bool isNewWindow) {
    std::vector<entry> list;
    int index = 0;
    if (isNewWindow == false)
        list = window.list;
    window.list.clear();
    window.currentPath = newPath;

    // Primul element e Parent Directory
    if(newPath.root_path() != newPath){
        entry temp;
        temp.path = newPath.parent_path();
        if (isNewWindow == false) {
            std::cout << index << std::endl;
            temp.selected = list[index].selected;
            index ++;
        }
        else temp.selected = false;
        temp.name = "..";
        temp.isBackEntry = true;
        window.list.push_back(temp);
    }

    for (auto e : fs::directory_iterator(newPath))
        if(!(GetFileAttributes(e.path().string().c_str()) & FILE_ATTRIBUTE_HIDDEN)){
            entry temp;
            temp.path = e.path();
            if (isNewWindow == false) temp.selected = list[index].selected;
            temp.name = e.path().filename().string();
            window.list.push_back(temp);
        }

    // Sortare:
    auto first = window.list.begin();
    if(newPath.root_path() != newPath) first = window.list.begin()+1;
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

void _update_window_from_search(fileWindow &window) { // no newPath since it does not enter any directory, no isNewWindow since it always is a new window
    window.list.clear();
    // entry tempinit;
    // tempinit.path = window.currentPath; // to return back to pre-search
    // tempinit.selected = false;
    // tempinit.name = "..";
    // tempinit.isBackEntry = true;
    // window.list.push_back(tempinit); // actually this sucks to debug so out it goes
    for (auto e : fs::recursive_directory_iterator(window.currentPath)) {
        if(!(GetFileAttributes(e.path().string().c_str()) & FILE_ATTRIBUTE_HIDDEN) and e.path().stem().string().find(window.searchString) != std::string::npos) {
            std::cout << window.searchString << " " << e.path().string() << "\n";
            entry temp;
            temp.path = e.path();
            temp.selected = false;
            temp.name = e.path().filename().string();
            window.list.push_back(temp);
        }
    }
    auto first = window.list.begin()+1;
    auto last = window.list.end();
    switch(window.sort) {
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
    }
    // 0 - ascending
    // 1 - descending
    if(window.sortOrder == 1){
        std::reverse(first, last);
    }
}

// Acum deschidem cu un path. Acelasi motiv ca la _get_hover_path
void _command_open(fileWindow& window, fs::path p) {
    if (window.currentPath != p or window.midSearch == true) {
        std::cout << "entered open cmd" << std::endl;
        if (fs::is_directory(p) or window.midSearch == true) {

            window.scrollAmount = 0;
            if (window.midSearch) {
                std::cout << "opening attempt" << std::endl;
                _update_window_from_search(window);
            }
            else _update_window(window, p, true);
        }
        else { // Automatically find a matching app to open the given file
            ShellExecute(NULL, "open", p.string().c_str(), NULL, NULL, SW_SHOW);
        }
        window.midSearch = false;
    }
}

void _command_back(fileWindow& window) {
    _update_window(window, window.currentPath.parent_path(), true);
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
    _update_window(dest, dest.currentPath, true);
}

void _command_move(fileWindow& source, fileWindow& dest) {
    for (auto e : source.list) {
        if (e.selected == true) {
            rename(e.path, dest.currentPath / e.path.filename());
        }
    }
    _update_window(source, source.currentPath, true);
    _update_window(dest, dest.currentPath, true);
}
void _command_delete(fileWindow& window) {
    for (auto e : window.list) {
        if (e.selected == true) {
            remove(e.path);
        }
    }
    _update_window(window, window.currentPath, true);
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
    _update_window(window, window.currentPath, true);
}

void _draw_ui(sf::RenderWindow &window){
    sf::RectangleShape bgbar;
    bgbar.setPosition(sf::Vector2f(0, 0)); // border top
    bgbar.setFillColor(sf::Color::White);
    bgbar.setSize(sf::Vector2f(window.getSize().x, 90));
    window.draw(bgbar);

    bgbar.setPosition(sf::Vector2f(0, 30)); // history bar bg
    bgbar.setFillColor(sf::Color(58, 52, 74));
    bgbar.setSize(sf::Vector2f(window.getSize().x, 25));
    window.draw(bgbar);

    bgbar.setPosition(sf::Vector2f(0, 60)); // search bar thing
    bgbar.setFillColor(sf::Color(58, 52, 74));
    bgbar.setSize(sf::Vector2f(window.getSize().x, 25));
    window.draw(bgbar);

    bgbar.setPosition(sf::Vector2f(0, 60)); // extra white before search
    bgbar.setFillColor(sf::Color::White);
    bgbar.setSize(sf::Vector2f(80, 25));
    window.draw(bgbar);
    bgbar.setPosition(sf::Vector2f(window.getSize().x / 2, 60));
    bgbar.setFillColor(sf::Color::White);
    bgbar.setSize(sf::Vector2f(80, 25));
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
    bool scrollbarClicked = false;
    sf::Clock clock;
    if(!texture_cmdIconPlaceholder.loadFromFile("cmdIconPlaceholder.png") ||
       !texture_dirIcon.loadFromFile("dirIcon.png") ||
       !texture_regularIcon.loadFromFile("regularFile.png") ||
       !texture_backIcon.loadFromFile("backIcon.png")
       ) return -1;

    current = 0;
    _update_window(filewindow[0], fs::current_path(), true);
    _update_window(filewindow[1], "C:\\", true);


    mainWindow.create(sf::VideoMode({ 800, 600 }), "My window");

    _thumb_from_scroll(filewindow[0], scrollbar[0]);
    _thumb_from_scroll(filewindow[1], scrollbar[1]);
    _draw_lists(filewindow, mainWindow);
    while (mainWindow.isOpen())
    {
        mainWindow.clear(sf::Color(20, 23, 36));
        _draw_ui(mainWindow);

        historywindow[0] = _draw_list_history_button(sf::Vector2f(PADDING_LEFT, 30), filewindow[0], mainWindow);
        historywindow[1] = _draw_list_history_button(sf::Vector2f(mainWindow.getSize().x / 2 + PADDING_LEFT, 30), filewindow[1], mainWindow);

        if (sf::Mouse::getPosition(mainWindow).x > mainWindow.getSize().x / 2 + LINE_PADDING) current = 1;
        else current = 0;
        _draw_lists(filewindow, mainWindow);
        _draw_scrollbars(filewindow, scrollbar, mainWindow);

        while (std::optional event = mainWindow.pollEvent()) // event-uri/actiuni din sisteme periferice
        {
            if (event->is<sf::Event::Closed>())
                mainWindow.close();
            if (const auto* resized = event->getIf<sf::Event::Resized>()) // reajustarea marimii la resize
            {
                sf::FloatRect visibleArea({0.f, 0.f}, sf::Vector2f(resized->size));
                mainWindow.setView(sf::View(visibleArea));
            }
            if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && scrollbarClicked == true) {
                sf::Vector2<float> mousePos = (sf::Vector2<float>)sf::Mouse::getPosition(mainWindow);
                if(mousePos.y >= scrollbar[current].track.position.y && mousePos.y <= scrollbar[current].track.position.y+scrollbar[current].track.size.y){
                    scrollbar[current].thumb.position.y = scrollbar[current].track.position.y;
                    _scroll_from_thumb(filewindow[current], scrollbar[current], mainWindow);
                    _draw_scrollbars(filewindow, scrollbar, mainWindow);
                }
            }
            else scrollbarClicked = false;
            if (event->is<sf::Event::MouseButtonPressed>()) {
                if(scrollbar[current].track.contains((sf::Vector2<float>)sf::Mouse::getPosition(mainWindow))){
                    scrollbarClicked = true;
                }
                _command_open(filewindow[current], _get_hover_path(historywindow[current], mainWindow));
                if(_get_hover_index(filewindow[current], mainWindow) == -1){
                    for(auto &e : filewindow[current].list){
                        e.selected = false;
                    }
                }
                else if(filewindow[current].list[_get_hover_index(filewindow[current], mainWindow)].selected == false){
                    for(auto &e : filewindow[current].list){
                        e.selected = false;
                    }
                    _command_select(filewindow[current], _get_hover_index(filewindow[current], mainWindow));
                    clock.restart();
                }
                else if(clock.getElapsedTime().asMilliseconds() < DOUBLE_CLICK_TIME){
                    _command_open(filewindow[current], _get_hover_path(filewindow[current], mainWindow));
                    filewindow[current].scrollAmount = 0;
                    _thumb_from_scroll(filewindow[current], scrollbar[current]);
                    _draw_scrollbars(filewindow, scrollbar, mainWindow);
                    clock.restart();
                }
                clock.restart();
            }
            if(event->is<sf::Event::KeyPressed>()){
                if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::B)){
                    _command_back(filewindow[current]);
                    filewindow[current].scrollAmount = 0;
                    _thumb_from_scroll(filewindow[current], scrollbar[current]);
                    _draw_scrollbars(filewindow, scrollbar, mainWindow);
                }
            }
            // Testare sortari. In viitor le vom pune pe butoane
            if(event->getIf<sf::Event::KeyPressed>()){
                if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q)){
                    filewindow[current].sort = SORT_NAME;
                    _update_window(filewindow[current], filewindow[current].currentPath, true);
                    std::cout << "Sorted by name!\n";
                }
                if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)){
                    filewindow[current].sort = SORT_DATE;
                    _update_window(filewindow[current], filewindow[current].currentPath, true);
                    std::cout << "Sorted by date!\n";
                }
                if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E)){
                    filewindow[current].sort = SORT_TYPE;
                    _update_window(filewindow[current], filewindow[current].currentPath, true);
                    std::cout << "Sorted by type!\n";
                }
                if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R)){
                    filewindow[current].sort = SORT_SIZE;
                    _update_window(filewindow[current], filewindow[current].currentPath, true);
                    std::cout << "Sorted by size!\n";
                }

                if (filewindow[current].isSearching) {
                    // this will have to be a function
                    auto* keyPressed = event->getIf<sf::Event::KeyPressed>();
                    std::string keycode = sf::Keyboard::getDescription(keyPressed->scancode);
                    std::cout << keycode << "\n";
                    if (keycode == "Backspace") {
                        if (!filewindow[current].searchString.empty())
                            filewindow[current].searchString.pop_back();
                    }
                    else if (keycode == "Enter") {
                        if (!filewindow[current].searchString.empty()) {
                            filewindow[current].midSearch = true;
                            _command_open(filewindow[current], "placeholder_path");
                        }

                        filewindow[current].isSearching = false;
                        std::cout << "b";
                    }
                    else if (keycode == "Caps Lock")
                        capsOn = !capsOn;
                    else if (keycode != "Escape" and
                             keycode != "Ctrl" and
                             keycode != "Shift" and
                             keycode != "Alt" and
                             keycode != "Tab" and
                             keycode != "Windows" and
                             keycode != "Caps Lock") {
                        if ((!sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) and capsOn == false) || (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) and capsOn == true))
                            if (keycode[0] >= 'A' && keycode[0] <= 'Z') filewindow[current].searchString += keycode[0] + 32; // uncap letters
                            else filewindow[current].searchString += keycode[0];
                        if ((sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) and capsOn == false) || (!sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) and capsOn == true)) {
                            if (keycode[0] >= 65 && keycode[0] <= 90) filewindow[current].searchString += keycode[0]; // capitalized letters
                            else if (keycode[0] >= '0' && keycode[0] <= '9') {
                                if (keycode[0] == '0') filewindow[current].searchString += ')';
                                if (keycode[0] == '1') filewindow[current].searchString += '!';
                                if (keycode[0] == '2') filewindow[current].searchString += '@';
                                if (keycode[0] == '3') filewindow[current].searchString += '#';
                                if (keycode[0] == '4') filewindow[current].searchString += '$';
                                if (keycode[0] == '5') filewindow[current].searchString += '%';
                                if (keycode[0] == '6') filewindow[current].searchString += '^';
                                if (keycode[0] == '7') filewindow[current].searchString += '&';
                                if (keycode[0] == '8') filewindow[current].searchString += '*';
                                if (keycode[0] == '9') filewindow[current].searchString += '(';
                            }
                            else filewindow[current].searchString += keycode[0];
                        }
                    }
                    // need to figure out how to do shift + key = different input
                }
                if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Enter)) {
                    filewindow[current].isSearching = !filewindow[current].isSearching;
                    //std::cout << "b";
                }

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
                    if(filewindow[current].list.size()*POS_OFFSET + filewindow[current].scrollAmount < mainWindow.getSize().y-PADDING_TOP-10){
                        filewindow[current].scrollAmount = -((float)(filewindow[current].list.size())*POS_OFFSET)+(float)mainWindow.getSize().y-PADDING_TOP-10;
                    }
                    if(filewindow[current].scrollAmount > 0){
                        filewindow[current].scrollAmount = 0;
                    }
                    _thumb_from_scroll(filewindow[current], scrollbar[current]);
                }
            }
        }

        mainWindow.display();
    }
    return 0;
}
