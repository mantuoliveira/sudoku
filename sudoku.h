#ifndef SUDOKU_SUDOKU_H
#define SUDOKU_SUDOKU_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

struct Options {
    int num_remove = 50;
    bool hints = false;
    int seed = 0;
    bool annotations = false;
};

struct Graphics {
    SDL_Window* win;
    SDL_Renderer* ren;
    TTF_Font* font;
    TTF_Font* small;
};

typedef std::pair<int, int> Coords;

#define CELL_WIDTH 48
#define THIN_PAD 8
#define THICK_PAD 18
#define WIN_TITLE "Sudoku"
#define WIN_WIDTH (CELL_WIDTH * 9 + THIN_PAD * 6 + THICK_PAD * 4)
#define WIN_HEIGHT (WIN_WIDTH + CELL_WIDTH + THICK_PAD - THIN_PAD)

struct State {
    bool quit = false;
    bool selected = false;
    int x;
    int y;
    int highlight = 0;
};

#endif //SUDOKU_SUDOKU_H
