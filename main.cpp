#include <string>
#include <iostream>
#include <filesystem>
#include <windows.h>
#include <list>
#include <vector>

using namespace std;
namespace fs = std::filesystem;

struct pathVector {
    fs::path path;
    bool selected = false; // pentru a identifica vizual fisierele selectate: false - regular, true - chenar alb sau ceva
};

//std::vector<pathVector> currentDisplayedList; // unused
std::vector<pathVector> currentList; // the list that is currently accessed
std::vector<pathVector> leftList, rightList; // lists on the left/right
std::vector<pathVector> selectedList; // when selecting files, append them to this vector
std::vector<pathVector> copiedList; // when executing a copy command, append selectedList to this
fs::path currentPath; // the current path that is used
fs::path leftPath, rightPath; // path on the left tab, respectively on the right tab
//fs::path copyPath;

void _print_path(fs::path what) {
    std::cout << what << std::endl;
}
void _print_vector(std::vector<pathVector> what) {
    for (int i = 0; i < what.size(); i++) {
        std::cout << what[i].path << " " << what[i].selected;
        if (what[i].selected) std::cout << " <-- ";
        std::cout << std::endl;
    }
}

std::vector<pathVector> _update_list(fs::path newPath) { // remakes the visualized list
    std::vector<pathVector> newDisplayedList;
    std::cout<<newPath<<std::endl;

    int index = 0;

    for (const auto & entry : fs::directory_iterator(newPath)) {
        std::cout << "| " << entry.path() << " at position " << index << std::endl;
        index ++;
        pathVector constructPath;
        constructPath.path = entry.path();
        constructPath.selected = false;
        newDisplayedList.push_back(constructPath);
    }
    return newDisplayedList;
}

fs::path _get_file_path_at(int position, std::vector<pathVector> givenList) { // retrieves the file at the given position
    if (position > givenList.size()-1) std::cout << "Index above number of files included. Aborting search." << std::endl;
    else return givenList[position].path;
}

std::vector<pathVector> _deselect_all(std::vector<pathVector> givenList) {
    for (int i = 0; i < givenList.size(); i++) givenList[i].selected = false;
    return givenList;
}

std::vector<pathVector> _switch_current_list(std::string side) { // could be called whenever the active side is changed, but instead of identity it checks for cursor position
    if (side == "left" or side == "right") {
        std::cout << "Changed current path to ";
        if (side == "left") {
            rightPath = currentPath;
            currentPath = leftPath;
        }
        else if (side == "right") {
            leftPath = currentPath;
            currentPath = rightPath;
        }
        std::cout << currentPath << std::endl;

        std::cout << "The following files exist: " << std::endl;
    }
    else std::cout << "Invalid side, path remains the same: " << currentPath << std::endl;
    return _update_list(currentPath);
}

bool _check_validity_of_input(std::string input) { // Default check if the input that leads to a file index is a number
    bool ok = true;
    for (int i=0;i<input.length();i++)
        if (input[i] > '9' || input[i] < '0') {
            ok = false;
            break;
        }
    if (!ok)
        std::cout << "Given argument is not a number. Skipping" << std::endl;
    else if (stoi(input) >= currentList.size()) {
        std::cout << "Index above number of files included. Aborting search" << std::endl;
        ok = false;
    }
    return ok;
}

void _command_open_file(std::string input) {
    input.erase(0, 5);
    if (!_check_validity_of_input(input)) return;
    else {
        fs::path p = _get_file_path_at(std::stoi(input), currentList);

        if (fs::status(p).type() == fs::file_type::directory) { // Go further in the directory
            std::cout << "Opening directory: ";
            currentPath = p;
            currentList = _update_list(currentPath);
        }
        else { // Automatically find a matching app to open the given file
            ShellExecute(NULL, "open", p.string().c_str(), NULL, NULL, SW_SHOW);
            std::cout << "Opening file: " << p.filename() << " with extension: " << p.extension() << std::endl;
        }
    }
}

void _command_back(std::vector<pathVector> currentList) {
    if (currentPath != currentPath.root_path()) {
        std::cout << "Going back to previous directory." << std::endl;
        currentPath = currentPath.parent_path();
        currentList = _update_list(currentPath);
    }
    else std::cout << "Already reached the root folder." << std::endl;
}

void _command_select(std::string input) {
    input.erase(0, 7);
    if (!_check_validity_of_input(input)) return;
    else {
        int alreadySelected = -1;
        for (int i=0; i<selectedList.size(); i++) {
            if (selectedList[i].path == currentList[stoi(input)].path) {
                alreadySelected = i;
                break;
            }
        }
        if (alreadySelected != -1) {
            std::cout << "Unselected path " << selectedList[alreadySelected].path << std::endl;
            selectedList.erase(selectedList.begin() + alreadySelected);
            currentList[stoi(input)].selected = false;
        }
        else {
            pathVector p;
            p.path = _get_file_path_at(std::stoi(input), currentList);
            p.selected = true;
            selectedList.push_back(p);
            currentList[stoi(input)].selected = true;
            std::cout << "Selected path " << selectedList.back().path << std::endl;
        }
    }
}

void _command_copy() {
    copiedList = selectedList;
    std::cout << "Copied list in selectedList to copiedList: [";
    for (int i=0;i<copiedList.size();i++)
        std::cout << copiedList[i].path << ", ";
    std::cout << "]" << std::endl;
}

void _command_paste(std::string input) {
    input.erase(0, 6);
    if (input == "COPY" || input == "PASTE") {
        bool isEmpty = false;
        for (int i=0;i<copiedList.size();i++) {
            if (copiedList[i].path == "") {
                isEmpty = true;
                break;
            }
        }
        if (!isEmpty) {
            std::cout << "Copied the copiedList (of size " << copiedList.size() << ") to " << currentPath << std::endl;
            for (int i=0;i<copiedList.size();i++) {
                fs::path p = currentPath;
                p += "\\";
                p += copiedList[i].path.filename();
                std::cout << p << std::endl;
                fs::copy(copiedList[i].path, p, fs::copy_options::overwrite_existing);
            }
            currentList = _update_list(currentPath);
        }
        else {
            std::cout << "Copy path is empty." << std::endl;
        }
    }
    else std::cout << "Mode not specified. Skipping";
}

void _command_delete(std::string input) {
    input.erase(0, 7);
    if (!_check_validity_of_input(input)) return;
    else {
        fs::path p = _get_file_path_at(std::stoi(input), currentList);
        std::cout << "Deleting path: " << p << std::endl;
        fs::remove(p);
    }
}

int main()
{
    leftPath = "C:\\"; // At least on CLion, C: redirects to the current code of the .cpp file. Use C:\\ for the actual C root drive
    rightPath = "D:\\";
    currentPath = leftPath;

    std::string input;

    currentList = _update_list(currentPath);
    leftList = currentList;
    rightList = _update_list(rightPath);

    while (true) {
        std::cout << "Enter the number of a line to return the path at said directory" << std::endl;
        getline(std::cin, input);

        if (input.substr(0, 4) == "open" and input[4] == ' ' and input.size() > 4) { // Opens a file
            _command_open_file(input);
        }

        else if (input == "back") { // Move one step back in the directory, until Root
            _command_back(currentList);
        }

        else if (input.substr(0, 6) == "select" and input[6] == ' ' and input.size() > 6) // Apply a selection tag to the given element + adds it to a selection list
            _command_select(input);

        else if (input == "copy") // Copies the selected list
            _command_copy();

        else if (input.substr(0, 5) == "paste" and input[5] == ' ' and input.size() > 5) // Pastes the currently copied path of a file in the current directory
            _command_paste(input);

        else if (input.substr(0, 6) == "delete" and input[6] == ' ' and input.size() > 6)
            _command_delete(input);

        else if (input == "print")
            _print_vector(currentList);

        else if (input.substr(0, 9) == "switch to" and input[9] == ' ') { // Moves between the Left and Right tabs (since they show different paths)
            input.erase(0, 10);
            currentList = _switch_current_list(input);
        }

        else if (input == "exit") {
            std::cout << "Exiting..." << std::endl;
            break;
        }

        else std::cout << "Likely invalid command." << std::endl;
    }
    return 0;
}