#include "Board.h"

std::string Board::get_save_path() {
    char* path = SDL_GetPrefPath(COMP_NAME, GAME_NAME);
    std::string complete_path {path};

    complete_path = complete_path + "save_game";
    return complete_path;
}

bool Board::save(Options &opts) {
    std::string complete_path = get_save_path();

    std::ofstream fptr;
    fptr.open(complete_path);

    if (!fptr.is_open()) return false;

    for (int l = 0; l < 9; l++) {
        for (int c = 0; c < 9; c++) {
            int val = tiles[l][c];
            if (!original[l][c]) val += 10;
            fptr << val << std::endl;
        }
    }
    fptr << opts.num_remove << std::endl;

    fptr.close();

    return true;
}

void Board::load(Options &opts) {
    std::string complete_path = get_save_path();
    std::string buffer;
    std::ifstream fptr(complete_path);

    if (!fptr.is_open()) return;

    int l = 0, c = 0;
    while (l < 9) {
        getline (fptr, buffer);
        int value = std::stoi(buffer);

        tiles[l][c] = value % 10;
        original[l][c] = value < 10;

        c++;
        if (c == 9) c = 0, l++;
    }

    if (l != 9) {
        fptr.close();
        return;
    }

    getline (fptr, buffer);
    opts.num_remove = std::stoi(buffer);

    fptr.close();
}

bool Board::is_original(int l, int c) {
    return original[l][c];
}

int Board::get_tile(int l, int c) {
    return tiles[l][c];
}

void Board::set_tile(int val, int l, int c) {
    tiles[l][c] = val;
}

void Board::clear() {
    for (int l = 0; l < 9; l++) {
        for (int c = 0; c < 9; c++) {
            tiles[l][c] = 0;
            original[l][c] = false;
        }
    }
}

void Board::consolidate() {
    for (int l = 0; l < 9; l++) {
        for (int c = 0; c < 9; c++) {
            if (tiles[l][c] != 0) {
                original[l][c] = true;
            }
        }
    }
}

void Board::clear_user() {
    for (int l = 0; l < 9; l++)
        for (int c = 0; c < 9; c++)
            if (!is_original(l, c))
                tiles[l][c] = 0;
}

bool Board::remove(int num_remove) {
    int tries = 0;

    while (num_remove > 0 && tries < MAX_TRIES) {
        int l = rand() % 9;
        int c = rand() % 9;

        if (tiles[l][c] == 0) continue;

        int backup = tiles[l][c];
        tiles[l][c] = 0;

        if (is_unique_solvable()) {
            num_remove--;
        } else {
            tiles[l][c] = backup;
            tries++;
        }
    }

    for (int l = 0; l < 9; l++)
        for (int c = 0; c < 9; c++)
            this->original[l][c] = tiles[l][c] != 0;

    return tries < MAX_TRIES;
}

bool Board::fill(bool random) {
    Coords xy = next_empty();
    if (xy.first == -1) return true;

    int offset = 0;
    if (random) offset = rand() % 9;

    for (int val = 0; val < 9; val++) {
        int maybe = 1 + ((val + offset) % 9);

        if (!is_allowed(maybe, xy.first, xy.second)) continue;

        tiles[xy.first][xy.second] = maybe;
        if (!fill(random)) tiles[xy.first][xy.second] = 0;
        else return true;
    }

    return false;
}

bool Board::unique_rec(int &numSols) {
    Coords xy = next_empty();
    if (xy.first == -1) {
        numSols++;
        return true;
    }

    for (int val = 1; val <= 9; val++) {
        if (!is_allowed(val, xy.first, xy.second)) continue;

        tiles[xy.first][xy.second] = val;
        if (!unique_rec(numSols)) {
            tiles[xy.first][xy.second] = 0;
        } else {
            if (numSols < 2) tiles[xy.first][xy.second] = 0;
            else return true;
        }
    }

    return false;
}

bool Board::is_unique_solvable() {
    int backup[9][9];
    std::copy(&tiles[0][0], &tiles[0][0] + 81, &backup[0][0]);

    int sols = 0;
    unique_rec(sols);

    std::copy(&backup[0][0], &backup[0][0] + 81, &tiles[0][0]);

    return sols == 1;
}

bool Board::lin_has_val(int val, int lin) {
    for (int k = 0; k < 9; k++)
        if (tiles[lin][k] == val)
            return true;
    return false;
}

bool Board::col_has_val(int val, int col) {
    for (int k = 0; k < 9; k++)
        if (tiles[k][col] == val)
            return true;
    return false;
}

bool Board::block_has_val(int val, int lin, int col) {
    lin = (lin / 3) * 3;
    col = (col / 3) * 3;

    for (int l = lin; l < lin + 3; l++)
        for (int c = col; c < col + 3; c++)
            if (tiles[l][c] == val)
                return true;
    return false;
}

bool Board::is_allowed(int val, int lin, int col) {
    if (tiles[lin][col] != 0)
        return false;
    if (lin_has_val(val, lin))
        return false;
    if (col_has_val(val, col))
        return false;
    if (block_has_val(val, lin, col))
        return false;
    return true;
}

Coords Board::next_empty() {
    for (int l = 0; l < 9; l++)
        for (int c = 0; c < 9; c++)
            if (tiles[l][c] == 0)
                return std::make_pair(l, c);
    return std::make_pair(-1, -1);
}
