#include <string>
#include <iostream>
#include <filesystem>
#include <windows.h>
#include <vector>
#include <algorithm> //pentru count_if si find_if in functia _command_rename()
#include <SFML/Window.hpp>

namespace fs = std::filesystem;

// Nici nu ne trebuie index, putem sa facem window[current].list[index] (vezi putin mai jos)
struct entry {
    fs::path path;
    bool selected;
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
} filewindow[2];

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
fileWindow _update_window(fs::path newPath) {
    fileWindow window;
    window.currentPath = newPath;
    for (auto& e : fs::directory_iterator(newPath)) {
        entry temp;
        temp.path = e.path();
        temp.selected = false;
        window.list.push_back(temp);
    }
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
    if (is_directory(window.list[index].path)) {
        //std::cout << window.list[index].path << is_directory(window.list[index].path);
        window = _update_window(window.list[index].path);
    }
    else { // Automatically find a matching app to open the given file
        ShellExecute(NULL, "open", window.list[index].path.string().c_str(), NULL, NULL, SW_SHOW);
        //std::cout << "Opening file: " << p.filename() << " with extension: " << p.extension() << std::endl;
    }
}

void _command_back(fileWindow& window) {
    window = _update_window(window.currentPath.parent_path());
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
    dest = _update_window(dest.currentPath); //dam un "refresh" window-urilor date prin referinta
}


// Move se implementeaza cu rename, asa scrie si in documentatie
void _command_move(fileWindow& source, fileWindow& dest) {
    for (auto e : source.list) {
        if (e.selected == true) {
            rename(e.path, dest.currentPath / e.path.filename()); // slash-ul e overload pentru concatenare de path-uri. vezi documentatia pentru clasa path si exemplele pentru rename
        }
    }
    source = _update_window(source.currentPath);
    dest = _update_window(dest.currentPath);
}
void _command_delete(fileWindow& window) {
    for (auto e : window.list) {
        if (e.selected == true) {
            remove(e.path);
        }
    }
    window = _update_window(window.currentPath);
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
    window = _update_window(window.currentPath);
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
    filewindow[0] = _update_window(fs::current_path());
    filewindow[1] = _update_window(fs::current_path());
    //_print_path(window[0].currentPath);
    //_print_list(window[0].list);
    //_print_path(window[0].list[1].path.parent_path());

    sf::Window window;
    window.create(sf::VideoMode({ 800, 600 }), "My window");

    while (true) {
        // Asta tine de prototipul command-line. Nu intru prea mult in detaliu
        //std::cout << "Enter the number of a line to return the path at said directory" << std::endl; //???
        std::cout << "Current path: " << filewindow[0].currentPath << "\n\n";
        _print_list(filewindow[0].list);
        std::cout << "Enter command: ";
        getline(std::cin, input);

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