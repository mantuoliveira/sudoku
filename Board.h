#ifndef SUDOKU_BOARD_H
#define SUDOKU_BOARD_H

#include <string>
#include <fstream>

#include <SDL2/SDL.h>

#include "sudoku.h"

#define MAX_TRIES 80
#define COMP_NAME "myGames"
#define GAME_NAME "sudoku"

class Board {
private:
    int tiles[9][9] = {};
    bool original[9][9] = {};
private:
    std::string get_save_path();
public:

    Board() = default;

    bool save(Options& opts);

    void load(Options& opts);

    bool is_original(int l, int c);

    int get_tile(int l, int c);

    void set_tile(int val, int l, int c);

    void clear();

    void consolidate();

    void clear_user();

    bool remove(int num_remove);

    bool fill(bool random);

    bool unique_rec(int& numSols);

    bool is_unique_solvable();

    bool lin_has_val(int val, int lin);

    bool col_has_val(int val, int col);

    bool block_has_val(int val, int lin, int col);

    bool is_allowed(int val, int lin, int col);

    Coords next_empty();
};

#endif //SUDOKU_BOARD_H
