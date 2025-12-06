#include <string>
#include <iostream>
#include <filesystem>
#include <windows.h>
#include <vector>
#include <algorithm> //pentru count_if si find_if in functia _command_rename()
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

namespace fs = std::filesystem;


// Nici nu ne trebuie index, putem sa facem window[current].list[index] (vezi putin mai jos)
struct entry {
    fs::path path;
    bool selected;
    // sfml properties
    sf::RectangleShape box;
    sf::Rect<float> areaBox;
    std::string name; // pentru ca back-ul sa fie [..]
};

int current;

// Pentru a copia/muta entry-urile selectate dintr-o lista in alta, trebuie sa stim path-ul directorului corespunzator listei destinatie. Asadar, am grupat intr-un struct lista impreuna cu aceasta informatie
// Asadar, variabila current va face alegerea intre window-uri, nu intre liste.
// !Ar trebui sa ne decidem la un nume mai bun pentru acest struct, libraria grafica va avea cel mai probabil si ea o variabila cu un nume window...

// alternativ o denumesti incepand cu litera mica, in general variabilele la librariile astea grafice au litera mare
// -- redenumit to fileWindow (self explanatory. fereastra in care afisezi fisiere)

struct fileWindow {
    fs::path currentPath;
    std::vector<entry> list;
    std::string orderType;
} filewindow[2];

sf::Font font_listFile("D:\\MyCommander\\cmake-build-debug\\bin\\assets\\utfont.ttf");

int _get_hovered_over_list_button(fileWindow file, sf::RenderWindow &window) {
    std::cout << "Checking hovering..." << std::endl;
    for (int i = 0; i < file.list.size(); i++) {
        if (file.list[i].areaBox.contains(sf::Vector2<float>(sf::Mouse::getPosition(window))))
            return i;
    }
    std::cout << "boowomp" << std::endl;
    return -1;
}

entry _draw_list_button(sf::Vector2f pos, sf::Vector2f size, entry file, sf::RenderWindow &window) {
    entry button;
    sf::RectangleShape box;
    box.setPosition(pos);
    box.setSize(size);
    sf::Rect<float> areaBox(pos, size);

    if (file.selected == true) box.setFillColor(sf::Color(160, 160, 160));
    else {
        if (areaBox.contains(sf::Vector2<float>(sf::Mouse::getPosition(window)))) {
            box.setFillColor(sf::Color(70, 70, 70));
        }
        else box.setFillColor(sf::Color(30, 30, 30));
    }

    sf::Text text(font_listFile);
    text.setPosition(pos);
    text.setFont(font_listFile);
    text.setCharacterSize(16);
    text.setFillColor(sf::Color::White);
    text.setString(file.name);

    button.box = box;
    button.areaBox = areaBox;
    button.selected = file.selected;
    button.path = file.path;

    window.draw(button.box);
    window.draw(text);

    return button;
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

// Am modificat numele si return type-ul functiei, pentru ca ne trebuie si currentPath
fileWindow _update_window(fs::path newPath, std::string sortingType) {
    fileWindow window;
    window.currentPath = newPath;
    window.orderType = sortingType;

    if (newPath.has_relative_path()) {
        entry backButton;
        backButton.path = newPath.parent_path();
        //std::cout << backButton.path << std::endl;
        backButton.selected = false;
        backButton.name = "[..]";
        window.list.push_back(backButton);
    }

    if (sortingType == "INCREASING") // de la a la z
    {
        for (auto& e : fs::directory_iterator(newPath)) {
            entry temp;
            temp.path = e.path();
            temp.selected = false;
            temp.name = e.path().filename().string();
            window.list.push_back(temp);
        }
    }
    if (sortingType == "DECREASING") // de la z la a
    {
        // ts so stupid
        fileWindow que;
        for (auto& e : fs::directory_iterator(newPath)) {
            entry temp;
            temp.path = e.path();
            temp.selected = false;
            temp.name = e.path().filename().string();
            que.list.push_back(temp);
        }
        while (que.list.size() > 0) {
            window.list.push_back(que.list.back());
            que.list.pop_back();
        }
    }
    if (sortingType == "DIR_FIRST") // directoare, apoi fisiere normale
    {
        std::cout << "sort by directory first" << std::endl;
        for (auto& e : fs::directory_iterator(newPath)) {
            if (std::filesystem::is_directory(e.path())) {
                entry temp;
                temp.path = e.path();
                temp.selected = false;
                temp.name = e.path().filename().string();
                window.list.push_back(temp);
            }

        }
        for (auto& e : fs::directory_iterator(newPath)) {
            if (!std::filesystem::is_directory(e.path())) {
                entry temp;
                temp.path = e.path();
                temp.selected = false;
                temp.name = e.path().filename().string();
                window.list.push_back(temp);
            }
        }
    }

    //mai trebuie adaugate in functie de data etc dar vedem mai tarziu dupa ce facem si afisarea mai ca lumea
    return window;
}

// Posibil sa nu trebuiasca: atunci cand se da click, ar veni o simpla atribuire pentru variabila current

// true that, doar ar trebui schimbat current si executate functii asupra window[current]
//
//std::vector<pathVector> _switch_current_list(std::string side) { // could be called whenever the active side is changed, but instead of identity it checks for cursor position
//    if (side == "left" or side == "right") {
//        std::cout << "Changed current path to ";
//        if (side == "left") {
//            rightPath = currentPath;
//            currentPath = leftPath;
//        }
//        else if (side == "right") {
//            leftPath = currentPath;
//            currentPath = rightPath;
//        }
//        std::cout << currentPath << std::endl;
//
//        std::cout << "The following files exist: " << std::endl;
//    }
//    else std::cout << "Invalid side, path remains the same: " << currentPath << std::endl;
//    return _update_list(currentPath);
//}
//

// Face parte din prototipul command-line. Nu modific
//bool _check_validity_of_input(std::string input) { // Default check if the input that leads to a file index is a number
//    bool ok = true;
//    for (int i=0;i<input.length();i++)
//        if (input[i] > '9' || input[i] < '0') {
//            ok = false;
//            break;
//        }
//    if (!ok)
//        std::cout << "Given argument is not a number. Skipping" << std::endl;
//    else if (stoi(input) >= currentList.size()) {
//        std::cout << "Index above number of files included. Aborting search" << std::endl;
//        ok = false;
//    }
//    return ok;
//}

// Am folosit is_directory si am redenumit functia, pentru ca deschide si directoare, nu numai fisiere
// De asemenea, am dat index ul ca parametru in loc de path, ramane sa discutam cum e mai bine

// am impresia ca prin click oricum va trebui sa iteram prin fiecare element pt a vedea care ii selectat... ambele merg, index pare mai intuitiv
void _command_open(fileWindow& window, int index) {
    std::cout << "Entered open command!" << std::endl;
    if (index >= 0) {
        std::cout << window.list[index].path << std::endl;
        if (is_directory(window.list[index].path)) {
            std::cout << "Opening " << window.list[index].path << std::endl;
            window = _update_window(window.list[index].path, window.orderType);
        }
        else { // Automatically find a matching app to open the given file
            ShellExecute(NULL, "open", window.list[index].path.string().c_str(), NULL, NULL, SW_SHOW);
        }
    }
}

void _command_back(fileWindow& window) {
    window = _update_window(window.currentPath.parent_path(), window.orderType);
}

// Algoritmi de selectare mai multe fisiere, deselectare cand selectezi numai 1, etc. vor fi scrisi in event loop-ul librariei grafice pe care o vom folosi.
// Ma gandesc ca functia asta e prea simpla, deci probabil in viitor o vom sterge, si vom creea functii mai complexe care contin instructiunea asta, pe care le vom apela in event loop

// corect, ar trebui sa fie parte din multi-select in care trimiti un array
void _command_select(fileWindow& window, int index) {
    window.list[index].selected = true;
}
// Am adaugat si functia asta de-un caz de ceva, dar conceptul ramane acelasi cu functia de mai sus
void _command_deselect(fileWindow& window, int index) {
    window.list[index].selected = false;
}

// Am folosit functia copy din <filesystem>
// Functiile de manipulare a fisierelor sunt asemanatoare
// Poate la rename e mai sofisticat, caci trebuie sa dam numele fisierului chiar in aplicatie, dar asta ramane sa facem in event loop
void _command_copy(fileWindow source, fileWindow& dest) {
    for (auto e : source.list) {
        if (e.selected == true) {
            copy(e.path, dest.currentPath);
        }
    }
    dest = _update_window(dest.currentPath, dest.orderType); //dam un "refresh" window-urilor date prin referinta
}


// Move se implementeaza cu rename, asa scrie si in documentatie
void _command_move(fileWindow& source, fileWindow& dest) {
    for (auto e : source.list) {
        if (e.selected == true) {
            rename(e.path, dest.currentPath / e.path.filename()); // slash-ul e overload pentru concatenare de path-uri. vezi documentatia pentru clasa path si exemplele pentru rename
        }
    }
    source = _update_window(source.currentPath, source.orderType);
    dest = _update_window(dest.currentPath, dest.orderType);
}
void _command_delete(fileWindow& window) {
    for (auto e : window.list) {
        if (e.selected == true) {
            remove(e.path);
        }
    }
    window = _update_window(window.currentPath, window.orderType);
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
        rename(selectedEntry->path, selectedEntry->path.parent_path() / newName); // merge si window.currentPath / newName
    }
    window = _update_window(window.currentPath, window.orderType);
}

// Ideea de baza la Total Commander este ca fisierele se copie/muta instant dintr-o parte in alta, nu e ca in Explorer, unde trebuie sa spui ce fisiere vrei sa copiezi/muti, apoi sa te duci in alt director si sa dai paste acolo. In schimb, exista pur si simplu un buton care, cand il apesi, face copierea/mutarea din prima.
// Asadar, nu modific functia asta
//
// de sters eventual atunci, vedem
//void _command_paste(std::string input) {
//    input.erase(0, 6);
//    if (input == "COPY" || input == "PASTE") {
//        bool isEmpty = false;
//        for (int i=0;i<copiedList.size();i++) {
//            if (copiedList[i].path == "") {
//                isEmpty = true;
//                break;
//            }
//        }
//        if (!isEmpty) {
//            std::cout << "Copied the copiedList (of size " << copiedList.size() << ") to " << currentPath << std::endl;
//            for (int i=0;i<copiedList.size();i++) {
//                fs::path p = currentPath;
//                p += "\\";
//                p += copiedList[i].path.filename();
//                std::cout << p << std::endl;
//                fs::copy(copiedList[i].path, p, fs::copy_options::overwrite_existing);
//            }
//            currentList = _update_list(currentPath);
//        }
//        else {
//            std::cout << "Copy path is empty." << std::endl;
//        }
//    }
//    else std::cout << "Mode not specified. Skipping";
//}

int main()
{
    std::string input;

    current = 0;
    filewindow[0] = _update_window("C:\\", "INCREASING");
    filewindow[1] = _update_window("C:\\", "DECREASING");

    filewindow[0].orderType = "DIR_FIRST";
    filewindow[1].orderType = "DECREASING";

    //SYSTEMTIME t = _get_creation_time(filewindow[0].currentPath);
    //_print_path(window[0].currentPath);
    //_print_list(window[0].list);
    //_print_path(window[0].list[1].path.parent_path());

    sf::RenderWindow window;
    window.create(sf::VideoMode({ 800, 600 }), "My window");

    while (window.isOpen())
    {
        window.clear();

        filewindow[0] = _update_window(filewindow[0].currentPath, filewindow[0].orderType);
        filewindow[1] = _update_window(filewindow[1].currentPath, filewindow[1].orderType);

        if (sf::Mouse::getPosition(window).x > window.getSize().x / 2 + 10) current = 1;
        else current = 0;

        sf::RectangleShape line;
        line.setSize(sf::Vector2f(5, window.getSize().y));
        line.setPosition(sf::Vector2f(window.getSize().x / 2, 0));
        line.setFillColor(sf::Color::White);

        window.draw(line);

        for (int i=0;i<filewindow[0].list.size();i++)
            filewindow[0].list[i] = _draw_list_button(sf::Vector2f(0, i*25), sf::Vector2f(150, 20), filewindow[0].list[i], window);
        for (int i=0;i<filewindow[1].list.size();i++)
            filewindow[1].list[i] = _draw_list_button(sf::Vector2f(window.getSize().x / 2 + 10, i*25), sf::Vector2f(150, 20), filewindow[1].list[i], window);

        while (const std::optional event = window.pollEvent()) // event-uri/actiuni din sisteme periferice
        {
            if (event->is<sf::Event::Closed>())
                window.close();
            if (const auto* resized = event->getIf<sf::Event::Resized>()) // reajustarea marimii la resize
            {
                sf::FloatRect visibleArea({0.f, 0.f}, sf::Vector2f(resized->size));
                window.setView(sf::View(visibleArea));
            }
            if (event->is<sf::Event::MouseButtonPressed>()) {
                if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
                    _print_list(filewindow[0].list);
                    _command_open(filewindow[current], _get_hovered_over_list_button(filewindow[current], window));
                }
            }
        }

        window.display();
    }


    while (true) {
        // Asta tine de prototipul command-line. Nu intru prea mult in detaliu
        //std::cout << "Enter the number of a line to return the path at said directory" << std::endl; //???
        std::cout << "Current path: " << filewindow[0].currentPath << "\n\n";
        _print_list(filewindow[0].list);
        std::cout << "Enter command: ";
        getline(std::cin, input);

        //SYSTEMTIME t = _get_creation_time(filewindow[0].currentPath);

        if (input.substr(0, 4) == "open" && input[4] == ' ' && input.size() > 4) { // Opens a file
            input.erase(0, 5);
            _command_open(filewindow[current], stoi(input));
        }

        else if (input == "back") { // Move one step back in the directory, until Root
            _command_back(filewindow[current]);
        }

        //else if (input.substr(0, 6) == "select" and input[6] == ' ' and input.size() > 6) // Apply a selection tag to the given element + adds it to a selection list
        //    _command_select(input);

        //else if (input == "copy") // Copies the selected list
        //    _command_copy();

        //else if (input.substr(0, 5) == "paste" and input[5] == ' ' and input.size() > 5) // Pastes the currently copied path of a file in the current directory
        //    _command_paste(input);

        //else if (input.substr(0, 6) == "delete" and input[6] == ' ' and input.size() > 6)
        //    _command_delete(input);

        //else if (input == "print")
        //    _print_vector(currentList);

        //else if (input.substr(0, 9) == "switch to" and input[9] == ' ') { // Moves between the Left and Right tabs (since they show different paths)
        //    input.erase(0, 10);
        //    currentList = _switch_current_list(input);
        //}

        //else if(input.substr(0,6) == "rename" and input[6] == ' '){
        //    _command_rename(input);
        //}

        else if (input == "exit") {
            std::cout << "Exiting..." << std::endl;
            break;
        }

        //else std::cout << "Likely invalid command." << std::endl;
    }
    return 0;
}