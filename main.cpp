#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <windows.h>
#include <conio.h>
#include <vector>
#include <algorithm>
#include <format>
#include <cmath>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <chrono>

namespace fs = std::filesystem;

// Definim aici constante, simplifica foarte mult modificarea acestora in mai multe parti ale codului, si sunt mai usor de gasit
#define POS_OFFSET         25
#define BOX_HEIGHT         20
#define PADDING_LEFT       10
#define PADDING_TOP        120
#define PADDING_TOP_SRC    60
#define PADDING_LEFT_SRC   130

#define PADDING_RIGHT_EXT  40
#define PADDING_RIGHT_SIZE 100
#define PADDING_RIGHT_DATE 120
#define PADDING_RIGHT_ATTR

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

enum fileAction{
    FILE_NEW_FILE,
    FILE_NEW_DIRECTORY,
    FILE_COPY,
    FILE_MOVE,
    FILE_DELETE,
    FILE_RENAME,
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
    bool additionalPostSearchCheck;
    std::string searchString;
} filewindow[2], historywindow[2];

struct scrollBar{
   sf::Rect<float> track;
   sf::Rect<float> thumb = {{0,0},{0,0}};
} scrollbar[2];

std::vector<sf::Rect<float>> menuButtons;
std::vector<sf::Rect<float>> attributeButtons[2], rootButtons[2];

sf::Font font_listFile("utfont.ttf");
sf::Texture texture_cmdIconPlaceholder;
sf::Texture texture_dirIcon;
sf::Texture texture_regularIcon;
sf::Texture texture_backIcon;

void _draw_lists(fileWindow filewindow[2], sf::RenderWindow &window){
    sf::Text text(font_listFile);
    sf::Vector2f pos;
    sf::Vector2f size;

    sf::Vector2 wsize = window.getSize();
    for(int side=0;side<2;side++){

        sf::Text searchText(font_listFile);
        searchText.setCharacterSize(16);
        searchText.setPosition(sf::Vector2f(PADDING_LEFT_SRC+side*wsize.x/2, PADDING_TOP_SRC));
        searchText.setString(filewindow[side].searchString);
        window.draw(searchText);

        for(int i=-(filewindow[side].scrollAmount/25);i<filewindow[side].list.size();i++){
            pos = {(float)ICON_SIZE+PADDING_LEFT*2+side*((wsize.x)/2), (float)i*POS_OFFSET+filewindow[side].scrollAmount+PADDING_TOP};
            size = {(float)(window.getSize().x)/2 - SCROLLBAR_WIDTH-ICON_SIZE-PADDING_LEFT*3, BOX_HEIGHT};

            if (pos.y > window.getSize().y) break;
                // creating the highlight box
            sf::RectangleShape box;
            box.setPosition(pos);
            box.setSize(size);
            box.setFillColor(sf::Color::Transparent);
            box.setOutlineThickness(2);

                // writing file text
            sf::Rect<float> areaBox(pos, size);
            text.setPosition(pos);
            text.setFont(font_listFile);
            text.setCharacterSize(16);
            text.setFillColor(sf::Color::White);
            text.setString(filewindow[side].list[i].name);
            while (text.getLocalBounds().size.x > window.getSize().x/2 - SCROLLBAR_WIDTH-ICON_SIZE-PADDING_LEFT-PADDING_RIGHT_EXT-PADDING_RIGHT_DATE-PADDING_RIGHT_SIZE-40) {
                std::string ph = text.getString();
                ph.erase(ph.size()-5); ph.append("[..]");
                text.setString(ph);
            }
            if (filewindow[side].list[i].selected == true){
                box.setOutlineColor(sf::Color(160, 160, 160));
                box.setFillColor(sf::Color(255, 255, 255, 20));
                text.setPosition({pos.x+PADDING_LEFT, pos.y});
            }
            else {
                box.setFillColor(sf::Color::Transparent);
                if (areaBox.contains(sf::Vector2<float>(sf::Mouse::getPosition(window)))) {
                    text.setPosition({pos.x+PADDING_LEFT, pos.y});
                    box.setOutlineColor(sf::Color(70, 70, 70));
                }
                else box.setOutlineColor(sf::Color::Transparent);
            }

                // writing the extension
            sf::Text ext(font_listFile);
            ext.setPosition(sf::Vector2f(window.getSize().x - ((1-side)*window.getSize().x/2)-SCROLLBAR_WIDTH-PADDING_RIGHT_EXT, pos.y));
            ext.setCharacterSize(16);
            ext.setFillColor(sf::Color::White);
            ext.setString(sf::String(filewindow[side].list[i].path.extension()));
            if (fs::is_directory(filewindow[side].list[i].path)) ext.setString("<DIR>");

                // writing the file size
            sf::Text fsize(font_listFile);
            fsize.setPosition(sf::Vector2f(window.getSize().x - ((1-side)*window.getSize().x/2)-SCROLLBAR_WIDTH-PADDING_RIGHT_EXT-PADDING_RIGHT_SIZE, pos.y));
            fsize.setCharacterSize(16);
            fsize.setFillColor(sf::Color::White);
            if (fs::is_directory(filewindow[side].list[i].path)) fsize.setString("<DIR>");
            else fsize.setString(sf::String(std::to_string(fs::file_size(filewindow[side].list[i].path))));

            sf::Text date(font_listFile);
            date.setPosition(sf::Vector2f(window.getSize().x - ((1-side)*window.getSize().x/2)-SCROLLBAR_WIDTH-PADDING_RIGHT_EXT-PADDING_RIGHT_SIZE-PADDING_RIGHT_DATE, pos.y));
            date.setCharacterSize(16);
            date.setFillColor(sf::Color::White);
            auto filetime = fs::last_write_time(filewindow[side].list[i].path);
            date.setString(sf::String(std::format("{0:%R} {0:%F}", filetime)));

            if(pos.y >= PADDING_TOP and pos.y <= window.getSize().y - 20){
                window.draw(box);
                window.draw(text);
                window.draw(ext);
                window.draw(fsize);
                window.draw(date);
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

void _draw_underline(sf::RenderWindow &window, sf::Rect<float> box, sf::Text text) {
    if(box.contains(sf::Vector2<float>(sf::Mouse::getPosition(window)))){
        sf::RectangleShape underline;
        underline.setPosition({box.position.x, box.position.y+20});
        underline.setSize({text.getGlobalBounds().size.x, 0});
        underline.setOutlineThickness(1);
        underline.setOutlineColor(sf::Color::Black);
        window.draw(underline);
    }
}

void _draw_menu_buttons(sf::RenderWindow &window){
    menuButtons.clear();
    sf::Rect<float> box;
    sf::Text text(font_listFile);
    float additional_x_position = PADDING_LEFT;
    for(auto e : {"New File", "New Directory", "Copy File(s)", "Move File(s)", "Delete File(s)", "Rename File"}){
        box.position = {additional_x_position, 0};
        text.setPosition(box.position);
        text.setCharacterSize(16);
        text.setFillColor(sf::Color::Black);
        text.setString(e);
        box.size = {text.getGlobalBounds().size.x + PADDING_LEFT, 25};
        additional_x_position += box.size.x + PADDING_LEFT;
        _draw_underline(window, box, text);
        window.draw(text);
        menuButtons.push_back(box);
    }
}

void _draw_root_change_buttons(sf::RenderWindow &window) {
    for (int side = 0; side < 2; side++) {
        sf::Text attr(font_listFile);
        attr.setCharacterSize(16);
        attr.setFillColor(sf::Color::Black);
        attr.setPosition(sf::Vector2f(PADDING_LEFT+8+window.getSize().x/2*side, 60));
        attr.setString("Change Root");
        sf::Rect<float> box;
        box.position = {PADDING_LEFT+8+float(window.getSize().x/2*side), 60};
        box.size = {attr.getGlobalBounds().size.x + PADDING_LEFT, 20};
        _draw_underline(window, box, attr);
        window.draw(attr);
        rootButtons[side].push_back(box);
    }
}

void _draw_attribute_buttons(sf::RenderWindow &window, int side) {
    for (int side = 0; side < 2; side++) {
        sf::Text attr(font_listFile);
        attr.setCharacterSize(16);
        attr.setFillColor(sf::Color::Black);
        attr.setPosition(sf::Vector2f(PADDING_LEFT+ICON_SIZE+6+window.getSize().x/2*side, 90));
        attr.setString("Name");
        sf::Rect<float> box;
        box.position = {PADDING_LEFT+ICON_SIZE+6+float(window.getSize().x/2*side), 90};
        box.size = {attr.getGlobalBounds().size.x + PADDING_LEFT, 20};
        _draw_underline(window, box, attr);
        window.draw(attr);
        attributeButtons[side].push_back(box);

        attr.setPosition(sf::Vector2f(window.getSize().x - ((1-side)*window.getSize().x/2)-SCROLLBAR_WIDTH-PADDING_RIGHT_EXT-PADDING_RIGHT_SIZE-PADDING_RIGHT_DATE, 90));
        attr.setString("Date (H Y/D/M)");
        box.position = {window.getSize().x - float(((1-side)*window.getSize().x/2))-SCROLLBAR_WIDTH-PADDING_RIGHT_EXT-PADDING_RIGHT_SIZE-PADDING_RIGHT_DATE, 90};
        box.size = {attr.getGlobalBounds().size.x + PADDING_LEFT, 20};
        _draw_underline(window, box, attr);
        window.draw(attr);
        attributeButtons[side].push_back(box);

        attr.setPosition(sf::Vector2f(window.getSize().x - ((1-side)*window.getSize().x/2)-SCROLLBAR_WIDTH-PADDING_RIGHT_EXT-PADDING_RIGHT_SIZE, 90));
        attr.setString("Size (Bytes)");
        box.position = {window.getSize().x - float(((1-side)*window.getSize().x/2))-SCROLLBAR_WIDTH-PADDING_RIGHT_EXT-PADDING_RIGHT_SIZE, 90};
        box.size = {attr.getGlobalBounds().size.x + PADDING_LEFT, 20};
        _draw_underline(window, box, attr);
        window.draw(attr);
        attributeButtons[side].push_back(box);

        attr.setPosition(sf::Vector2f(window.getSize().x - ((1-side)*window.getSize().x/2)-SCROLLBAR_WIDTH-PADDING_RIGHT_EXT, 90));
        attr.setString("Ext");
        box.position = {window.getSize().x - float(((1-side)*window.getSize().x/2))-SCROLLBAR_WIDTH-PADDING_RIGHT_EXT, 90};
        box.size = {attr.getGlobalBounds().size.x + PADDING_LEFT, 20};
        _draw_underline(window, box, attr);
        window.draw(attr);
        attributeButtons[side].push_back(box);
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

int _get_menu_index(sf::RenderWindow &window){
    int i = 0;
    for(auto e : menuButtons){
        if(e.contains(sf::Vector2<float>(sf::Mouse::getPosition(window)))){
            return i;
        }
        i++;
    }
    return -1;
}
int _get_rootchange_index(sf::RenderWindow &window, int side) {
    int i = 0;
    for(auto e : rootButtons[side]){
        if(e.contains(sf::Vector2<float>(sf::Mouse::getPosition(window)))){
            return i;
        }
        i++;
    }
    return -1;
}

fs::path _get_filename_from_prompt(){
    sf::RenderWindow window;
    sf::Text text(font_listFile);
    text.setFillColor(sf::Color::White);
    text.setCharacterSize(16);
    text.setPosition({0,0});
    std::string filename = "";
    window.create(sf::VideoMode({ 300, 100 }), "Enter filename:");
    window.clear(sf::Color(20, 23, 36));
    while(window.isOpen()){
        window.clear(sf::Color(20, 23, 36));
        while (std::optional event = window.pollEvent()){
            if(event->is<sf::Event::TextEntered>()){
                auto key = event->getIf<sf::Event::TextEntered>()->unicode;
                switch(key){
                    // \ / : * ? " < > |
                    case 92:
                    case 47:
                    case 58:
                    case 42:
                    case 63:
                    case 34:
                    case 60:
                    case 62:
                    case 124:
                        break;

                    // ENTER
                    case 10:
                    case 13:{
                        window.close();
                        return filename;
                    }

                    case 8:{
                        if(!filename.empty()) filename.pop_back();
                        break;
                    }

                    default:
                        filename += key;
                        break;
                }
            }
        }
        text.setString(filename);
        window.draw(text);
        window.display();
    }
}

int _get_attribute_index(sf::RenderWindow &window, int side) {
    int i = 0;
    for(auto e : attributeButtons[side]){
        if(e.contains(sf::Vector2<float>(sf::Mouse::getPosition(window)))){
            return i;
        }
        i++;
    }
    return -1;
}

bool _clicked_over_history_button(fileWindow historywindow, sf::RenderWindow &window) {
    for (int i = 0; i < historywindow.list.size(); i++) {
        if (historywindow.list[i].areaBox.contains(sf::Vector2f(sf::Mouse::getPosition(window)))) {
            return true;
        }
    }
    return false;
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
            temp.selected = list[index].selected;
            index ++;
        }
        else temp.selected = false;
        temp.name = "..";
        temp.isBackEntry = true;
        window.list.push_back(temp);
    }

    for (auto e : fs::directory_iterator(newPath))
    {
        if(!(GetFileAttributes(e.path().string().c_str()) & FILE_ATTRIBUTE_HIDDEN)){
            entry temp;
            temp.path = e.path();
            if (isNewWindow == false) temp.selected = list[index].selected;
            temp.name = e.path().stem().string();
            window.list.push_back(temp);
        }
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
    }
    // 0 - ascending
    // 1 - descending
    if(window.sortOrder == 1){
        std::reverse(first, last);
    }
}

void _update_window_from_search(fileWindow &window) { // no newPath since it does not enter any directory, no isNewWindow since it always is a new window
    window.list.clear();
    for(auto e = fs::recursive_directory_iterator(window.currentPath, fs::directory_options::skip_permission_denied);
        e != fs::recursive_directory_iterator(); e++) {
        //std::cout << e->path() << std::endl;
        DWORD attrs = GetFileAttributes(e->path().string().c_str());
        if(attrs & FILE_ATTRIBUTE_HIDDEN) {
            e.disable_recursion_pending();
        }
        if(!(attrs & FILE_ATTRIBUTE_HIDDEN) and e->path().stem().string().find(window.searchString) != std::string::npos) {
            window.list.push_back({.path = e->path(), .selected = false, .name = e->path().filename().string()});
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

void _command_open(fileWindow& window, fs::path p) {
    if (window.currentPath != p or window.midSearch == true or window.additionalPostSearchCheck == true) {
        //std::cout << "entered open cmd" << std::endl;

        if (fs::is_directory(p) or window.midSearch == true or window.additionalPostSearchCheck == true) {

            window.scrollAmount = 0;
            window.additionalPostSearchCheck = false;
            if (window.midSearch) {
                //std::cout << "opening attempt" << std::endl;
                _update_window_from_search(window);
            }
            else _update_window(window, p, true);
        }
        else { // Automatically find a matching app to open the given file
            ShellExecute(NULL, "open", p.string().c_str(), NULL, NULL, SW_SHOW);
        }
        if (window.midSearch == true) window.additionalPostSearchCheck = true;
        window.midSearch = false;

    }
}

void _command_back(fileWindow& window) {
    if (window.currentPath.relative_path() != window.currentPath.root_path())
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
            fs::remove(dest.currentPath / e.path.filename());
            fs::copy(e.path, dest.currentPath / e.path.filename(), fs::copy_options::update_existing);
        }
    }
    _update_window(dest, dest.currentPath, true);
}

void _command_move(fileWindow& source, fileWindow& dest) {
    for (auto e : source.list) {
        if (e.selected == true) {
            fs::remove(dest.currentPath / e.path.filename());
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
    if (count_if(first, last, is_selected) != 1) {
        std::cout << "Exactly one file must be selected for renaming";
    }
    else {
        auto selectedEntry = find_if(first, last, is_selected); // entry-ul care are .selected = true
        rename(selectedEntry->path, selectedEntry->path.parent_path() / newName);
    }
    _update_window(window, window.currentPath, true);
}

void _command_new_file(fileWindow filewindow, fs::path filename) {
    for(auto e : filewindow.list){
        if(e.path == filename){
            std::cout << "already exists, NOT creating\n";
            return;
        }
    }
    std::ofstream ofs(filename);
    ofs.close();
    _update_window(filewindow, filewindow.currentPath, true);
}

void _command_new_directory(fileWindow filewindow, fs::path filename) {
    for(auto e : filewindow.list){
        if(e.path == filename){
            std::cout << "already exists, NOT creating\n";
            return;
        }
    }
    fs::create_directory(filewindow.currentPath / filename);
    _update_window(filewindow, filewindow.currentPath, true);
};

void _draw_ui(sf::RenderWindow &window, fileWindow filewindow[2], float &timer){
    timer += 0.0025;
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
    for (int i = 0; i < 2; i++) {
        if (filewindow[i].isSearching) {
            bgbar.setPosition(sf::Vector2f(i*window.getSize().x/2, 60));
            bgbar.setFillColor(sf::Color(128, 122, 144));
            bgbar.setSize(sf::Vector2f(window.getSize().x/2, 25));
            window.draw(bgbar);

            sf::Text typingbar(font_listFile);
            typingbar.setCharacterSize(16);
            typingbar.setString("|");
            if (timer < 1)
                typingbar.setFillColor(sf::Color::White);
            else if (timer < 2)
                typingbar.setFillColor(sf::Color(0, 0, 0, 0));
            else timer = 0;
            sf::Text refstring(font_listFile);
            refstring.setCharacterSize(16);
            refstring.setString(filewindow[i].searchString);
            refstring.setPosition(sf::Vector2f(PADDING_LEFT_SRC+i*window.getSize().x/2, PADDING_TOP_SRC));
            typingbar.setPosition(refstring.getPosition() + sf::Vector2f(5 + refstring.getGlobalBounds().size.x, 0));
            window.draw(typingbar);
        }
    }

    bgbar.setPosition(sf::Vector2f(0, 60)); // extra white before search
    bgbar.setFillColor(sf::Color::White);
    bgbar.setSize(sf::Vector2f(120, 25));
    window.draw(bgbar);
    bgbar.setPosition(sf::Vector2f(window.getSize().x / 2, 60));
    bgbar.setFillColor(sf::Color::White);
    bgbar.setSize(sf::Vector2f(120, 25));
    window.draw(bgbar);

    bgbar.setPosition(sf::Vector2f(0, 0)); // border left
    bgbar.setFillColor(sf::Color::White);
    bgbar.setSize(sf::Vector2f(2, window.getSize().y));
    window.draw(bgbar);

    bgbar.setPosition(sf::Vector2f(window.getSize().x - 2, 0)); // border right
    bgbar.setFillColor(sf::Color::White);
    bgbar.setSize(sf::Vector2f(2, window.getSize().y));
    window.draw(bgbar);

    bgbar.setPosition(sf::Vector2f(0, 90)); // another gray for section names
    bgbar.setFillColor(sf::Color(200, 200, 200));
    bgbar.setSize(sf::Vector2f(window.getSize().x, 25));
    window.draw(bgbar);

    bgbar.setPosition(sf::Vector2f(0, 115));
    bgbar.setFillColor(sf::Color::White);
    bgbar.setSize(sf::Vector2f(window.getSize().x, 5));
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
    filewindow[0].sortOrder = 0;
    filewindow[1].sortOrder = 0;
    int strongCurrent;
    std::string str;
    sf::RenderWindow mainWindow;

    bool scrollbarClicked = false;
    sf::Clock clock;
    if(!texture_cmdIconPlaceholder.loadFromFile("spritesheet.png", false, sf::IntRect({0, 0}, {16, 16})) ||
       !texture_dirIcon.loadFromFile("spritesheet.png", false, sf::IntRect({16, 0}, {16, 16})) ||
       !texture_regularIcon.loadFromFile("spritesheet.png", false, sf::IntRect({32, 0}, {16, 16})) ||
       !texture_backIcon.loadFromFile("spritesheet.png", false, sf::IntRect({48, 0}, {16, 16}))
       ) return -1;

    current = 0;
    _update_window(filewindow[0], fs::current_path(), true);
    _update_window(filewindow[1], "C:\\", true);

    mainWindow.create(sf::VideoMode({ 1200, 600 }), "MyCommander");
    auto cmdIcon = sf::Image{};
    cmdIcon.loadFromFile("cmdIconPlaceholder.png");
    mainWindow.setIcon(cmdIcon.getSize(), cmdIcon.getPixelsPtr());

    _thumb_from_scroll(filewindow[0], scrollbar[0]);
    _thumb_from_scroll(filewindow[1], scrollbar[1]);
    _draw_lists(filewindow, mainWindow);

    float timer;

    while (mainWindow.isOpen())
    {
        mainWindow.clear(sf::Color(20, 23, 36));
        _draw_ui(mainWindow, filewindow, timer);
        _draw_menu_buttons(mainWindow);
        _draw_attribute_buttons(mainWindow, 0);
        _draw_attribute_buttons(mainWindow, 1);
        _draw_root_change_buttons(mainWindow);

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
                switch(_get_menu_index(mainWindow)){
                    case FILE_NEW_FILE:{
                        fs::path filename = _get_filename_from_prompt();
                        _command_new_file(filewindow[strongCurrent], filename);
                        _update_window(filewindow[current], filewindow[current].currentPath, true);
                        break;
                    }
                    case FILE_NEW_DIRECTORY:{
                        fs::path filename = _get_filename_from_prompt();
                        _command_new_directory(filewindow[strongCurrent], filename);
                        _update_window(filewindow[current], filewindow[current].currentPath, true);
                        break;
                    }
                    case FILE_COPY:{
                        _command_copy(filewindow[strongCurrent], filewindow[1-strongCurrent]);
                        break;
                    }
                    case FILE_MOVE:{
                        _command_move(filewindow[strongCurrent], filewindow[1-strongCurrent]);
                        break;
                    }
                    case FILE_DELETE:{
                        _command_delete(filewindow[strongCurrent]);
                        break;
                    }
                    case FILE_RENAME:{
                        fs::path filename = _get_filename_from_prompt();
                        _command_rename(filewindow[strongCurrent], filename);
                        _update_window(filewindow[current], filewindow[current].currentPath, true);
                        break;
                    }
                }
                switch(_get_attribute_index(mainWindow, current)){
                    case 0:{
                        filewindow[current].sort = SORT_NAME;
                        _update_window(filewindow[current], filewindow[current].currentPath, true);
                        break;
                    }
                    case 1:{
                        filewindow[current].sort = SORT_DATE;
                        _update_window(filewindow[current], filewindow[current].currentPath, true);
                        break;
                    }
                    case 2:{
                        filewindow[current].sort = SORT_SIZE;
                        _update_window(filewindow[current], filewindow[current].currentPath, true);
                        break;
                    }
                    case 3:{
                        filewindow[current].sort = SORT_TYPE;
                        _update_window(filewindow[current], filewindow[current].currentPath, true);
                        break;
                    }
                }
                if (_get_rootchange_index(mainWindow, current) != -1) {
                    std::string fname = filewindow[current].currentPath.root_name().string();
                    fname[0] = char(int(fname[0]) + 1);
                    fname.append("\\");
                    fs::path fpath(fname);
                    fs::path cpath("C:\\");
                    if (!is_directory(fpath))
                        filewindow[current].currentPath = cpath;
                    else filewindow[current].currentPath = fpath;
                    //std::cout << fname << std::endl;
                    filewindow[current].scrollAmount = 0;
                    _update_window(filewindow[current], filewindow[current].currentPath, true);
                    _thumb_from_scroll(filewindow[current], scrollbar[current]);
                    _draw_scrollbars(filewindow, scrollbar, mainWindow);
                }
                strongCurrent = current;
                if(scrollbar[current].track.contains((sf::Vector2<float>)sf::Mouse::getPosition(mainWindow))){
                    scrollbarClicked = true;
                }
                if (_clicked_over_history_button(historywindow[current], mainWindow))
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
                if(event->getIf<sf::Event::KeyPressed>()->scancode == sf::Keyboard::Scan::Enter){
                    if(filewindow[current].isSearching == true){
                        if (!filewindow[current].searchString.empty()) {
                            filewindow[current].midSearch = true;
                            _command_open(filewindow[current], "placeholder_path");
                            filewindow[current].isSearching = false;
                        }
                    }
                    else filewindow[current].isSearching = true;
                }
            }
            if(filewindow[current].isSearching){
                if(event->is<sf::Event::TextEntered>()){
                    auto key = event->getIf<sf::Event::TextEntered>()->unicode;
                    if(key == 10 || key == 13){ //ENTER
                    }
                    else if(key == 27){ //ESCAPE
                        filewindow[current].isSearching = false;
                    }
                    else if(key == 8){ //BACKSPACE
                        if(!filewindow[current].searchString.empty()) filewindow[current].searchString.pop_back();
                    }
                    else filewindow[current].searchString += key;
                }
            }
            if(!filewindow[current].isSearching){
                if(event->is<sf::Event::KeyPressed>()){
                    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)){
                        filewindow[current].sortOrder = 1 - filewindow[current].sortOrder;
                        _update_window(filewindow[current], filewindow[current].currentPath, true);
                        //_draw_lists(filewindow, mainWindow);
                    }
                    // if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q)){
                    //     filewindow[current].sort = SORT_NAME;
                    //     _update_window(filewindow[current], filewindow[current].currentPath, true);
                    //     std::cout << "Sorted by name!\n";
                    // }
                    // if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)){
                    //     filewindow[current].sort = SORT_DATE;
                    //     _update_window(filewindow[current], filewindow[current].currentPath, true);
                    //     std::cout << "Sorted by date!\n";
                    // }
                    // if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E)){
                    //     filewindow[current].sort = SORT_TYPE;
                    //     _update_window(filewindow[current], filewindow[current].currentPath, true);
                    //     std::cout << "Sorted by type!\n";
                    // }
                    // if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R)){
                    //     filewindow[current].sort = SORT_SIZE;
                    //     _update_window(filewindow[current], filewindow[current].currentPath, true);
                    //     std::cout << "Sorted by size!\n";
                    // }
                }
            }
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
