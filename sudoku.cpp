#include <cstdio>
#include <optional>
#include <string>
#include <iostream>
#include <fstream>

#include <getopt.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#define MAX_TRIES 80
#define COMP_NAME "myGames"
#define GAME_NAME "sudoku"

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

class Board {
private:
    int tiles[9][9] = {};
    bool original[9][9] = {};
private:
    std::string get_save_path() {
        char* path = SDL_GetPrefPath(COMP_NAME, GAME_NAME);
        std::string complete_path {path};

        complete_path = complete_path + "save_game";
        return complete_path;
    }
public:

    bool save(Options& opts) {
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

    void load(Options& opts) {
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

    bool is_original(int l, int c) {
        return original[l][c];
    }

    int get_tile(int l, int c) {
        return tiles[l][c];
    }

    void set_tile(int val, int l, int c) {
        tiles[l][c] = val;
    }

    void clear() {
        for (int l = 0; l < 9; l++) {
            for (int c = 0; c < 9; c++) {
                tiles[l][c] = 0;
                original[l][c] = false;
            }
        }
    }

    void consolidate() {
        for (int l = 0; l < 9; l++) {
            for (int c = 0; c < 9; c++) {
                if (tiles[l][c] != 0) {
                    original[l][c] = true;
                }
            }
        }
    }

    void clear_user() {
        for (int l = 0; l < 9; l++)
            for (int c = 0; c < 9; c++)
                if (!is_original(l, c))
                    tiles[l][c] = 0;
    }

    Board() = default;
    Board(int tiles[9][9]) {
        for (int l = 0; l < 9; l++)
            for (int c = 0; c < 9; c++)
                this->tiles[l][c] = tiles[l][c];
    }

    bool remove(int num_remove) {
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

    bool fill(bool random) {
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

    bool unique_rec(int& numSols) {
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

    bool is_unique_solvable() {
        int backup[9][9];
        std::copy(&tiles[0][0], &tiles[0][0] + 81, &backup[0][0]);

        int sols = 0;
        unique_rec(sols);

        std::copy(&backup[0][0], &backup[0][0] + 81, &tiles[0][0]);

        return sols == 1;
    }

    bool lin_has_val(int val, int lin) {
        for (int k = 0; k < 9; k++)
            if (tiles[lin][k] == val)
                return true;
        return false;
    }

    bool col_has_val(int val, int col) {
        for (int k = 0; k < 9; k++)
            if (tiles[k][col] == val)
                return true;
        return false;
    }

    bool block_has_val(int val, int lin, int col) {
        lin = (lin / 3) * 3;
        col = (col / 3) * 3;

        for (int l = lin; l < lin + 3; l++)
            for (int c = col; c < col + 3; c++)
                if (tiles[l][c] == val)
                    return true;
        return false;
    }

    bool is_allowed(int val, int lin, int col) {
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

    Coords next_empty() {
        for (int l = 0; l < 9; l++)
            for (int c = 0; c < 9; c++)
                if (tiles[l][c] == 0)
                    return std::make_pair(l, c);
        return std::make_pair(-1, -1);
    }

    void print() {
        for (int y = 0; y < 9; y++) {
            printf("%d %d %d", tiles[y][0], tiles[y][1], tiles[y][2]);
            printf("|");
            printf("%d %d %d", tiles[y][3], tiles[y][4], tiles[y][5]);
            printf("|");
            printf("%d %d %d\n", tiles[y][6], tiles[y][7], tiles[y][8]);
            if (y == 2 || y == 5)
                printf("=================\n");
        }
        printf("\n");
    }
};

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

void exit_sdl_error(std::string msg) {
    fprintf(stderr, "%s: %s\n", msg.c_str(), SDL_GetError());
    exit(EXIT_FAILURE);
}

Graphics init_graphics() {
    Graphics graphics;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) exit_sdl_error("Impossível iniciar o vídeo.");

    graphics.win = SDL_CreateWindow(
            WIN_TITLE,
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            WIN_WIDTH,
            WIN_HEIGHT,
            SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL
    );
    if (graphics.win == NULL) exit_sdl_error("Impossível criar a janela");

    graphics.ren = SDL_CreateRenderer(graphics.win, -1, SDL_RENDERER_ACCELERATED);
    if (graphics.ren == NULL) exit_sdl_error("Impossível criar o renderer");

    if (TTF_Init() < 0) exit_sdl_error("Impossível inicializar TTF.");

    graphics.font = TTF_OpenFont("/usr/local/games/sudokudata/monogram.ttf", 54);
    if (graphics.font == NULL) exit_sdl_error("Impossível carregar a fonte.");

    graphics.small = TTF_OpenFont("/usr/local/games/sudokudata/monogram.ttf", 16);
    if (graphics.small == NULL) exit_sdl_error("Impossível carregar a fonte.");

    return graphics;
}

void end_graphics(Graphics& gpx) {
    TTF_CloseFont(gpx.font);
    TTF_CloseFont(gpx.small);
    SDL_DestroyRenderer(gpx.ren);
    SDL_DestroyWindow(gpx.win);
    SDL_Quit();
    TTF_Quit();
}

int get_win_x(int x) {
    int n_thick = 1 + x / 3;
    int n_thin = x - n_thick + 1;
    return n_thick * THICK_PAD + n_thin * THIN_PAD + CELL_WIDTH * x;
}

int get_win_y(int y) {
    int n_thick = 2 + y / 3;
    int n_thin = y - n_thick + 1;
    return n_thick * THICK_PAD + n_thin * THIN_PAD + CELL_WIDTH * y + CELL_WIDTH;
}

void draw_number(Graphics& gpx, int num, int x_win, int y_win, SDL_Color color, bool small = false) {
    if (num == 0) return;

    char buffer[10];
    snprintf(buffer, 10, "%d", num);
    SDL_Surface* textSurface = TTF_RenderText_Solid( small ? gpx.small : gpx.font, buffer, color );
    SDL_Texture* Message = SDL_CreateTextureFromSurface(gpx.ren, textSurface);

    SDL_Rect Message_rect; //create a rect
    Message_rect.x = x_win;  //controls the rect's x coordinate
    Message_rect.y = y_win; // controls the rect's y coordinte
    Message_rect.w = textSurface->w; // controls the width of the rect
    Message_rect.h = textSurface->h; // controls the height of the rect

    SDL_RenderCopy(gpx.ren, Message, NULL, &Message_rect);

    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(Message);
}

Coords win_to_grid(int x, int y) {
    Coords par;
    for (int k = 0; k < 0; k++) {
        int win_x = get_win_x(k);
        if (x > win_x && x < win_x + CELL_WIDTH) {
            par.first = k;
            break;
        }
    }

    for (int k = 0; k < 0; k++) {
        int win_y = get_win_y(k);
        if (y > win_y && x < win_y + CELL_WIDTH) {
            par.second = k;
            break;
        }
    }

    return par;
}

void draw_header(Graphics& gpx, Board& board, State& stat, Options& opts) {
    // Possíveis números
    for (int k = 1; k <= 9; k++) {
        if (opts.hints && board.is_allowed(k, stat.y, stat.x))
            circleRGBA(gpx.ren, get_win_x(k - 1) + CELL_WIDTH / 2, THICK_PAD + CELL_WIDTH / 2, CELL_WIDTH / 2, 255, 160, 0, 255);

        SDL_Color color = {0, 150, 255, 255};

        draw_number(gpx, k, get_win_x(k - 1) + 16, THICK_PAD, color);
    }
}

void draw_board(Graphics& gpx, Board& board, State& stat, Options& opts) {
    for (int l = 0; l < 9; l++) {
        for (int c = 0; c < 9; c++) {
            int x_win = get_win_x(c);
            int y_win = get_win_y(l);

            int r = 255;
            int g = 255;
            int b = 255;

            if (opts.hints && stat.highlight != 0) {
                if  (!board.is_allowed(stat.highlight, l, c)) {
                    r = 100;
                    g = 100;
                    b = 100;
                } else {
                    r = 255;
                    g = 0xFF;
                    b = 0;
                }
            }

            roundedRectangleRGBA(
                    gpx.ren,
                    x_win,
                    y_win,
                    x_win + CELL_WIDTH,
                    y_win + CELL_WIDTH,
                    8,
                    r, g, b, 255
                    );

            SDL_Color color;
            if (board.is_original(l,c)) color = {255, 255, 160, SDL_ALPHA_OPAQUE};
            else color = {105, 255, 155, SDL_ALPHA_OPAQUE};
            draw_number(gpx, board.get_tile(l, c), x_win + 16, y_win, color);

            if (board.get_tile(l, c) != 0 || !opts.annotations) continue;

            for (int ll = 0; ll < 3; ll++) {
                for (int cc = 0; cc < 3; cc++) {
                    int value = ll * 3 + cc + 1;
                    if (board.is_allowed(value, l, c)) {
                        draw_number(gpx, value, x_win + 8 + 14 * cc, y_win + 3 + 14 * ll, color, true);
                    }
                }
            }
        }
    }
}

void draw_selection(Graphics& gpx, State& stat) {
    /*SDL_SetRenderDrawColor(gpx.ren, 255, 160, 0x00, 0xFF);
    SDL_Rect rect = {get_win_x(stat.x), get_win_y(stat.y), CELL_WIDTH, CELL_WIDTH};
    SDL_RenderDrawRect(gpx.ren, &rect);*/

    int x = get_win_x(stat.x);
    int y = get_win_y(stat.y);
    roundedRectangleRGBA(
            gpx.ren,
            x,
            y,
            x + CELL_WIDTH,
            y + CELL_WIDTH,
            8,
            255, 160, 0, 255
    );
}

void draw(Graphics& gpx, State& stat, Board& board, Options& opts) {
    SDL_SetRenderDrawColor(gpx.ren, 40, 40, 40, 255);
    SDL_RenderClear(gpx.ren);

    draw_board(gpx, board, stat, opts);

    if (stat.selected) {
        draw_selection(gpx, stat);
        draw_header(gpx, board, stat, opts);
    }

    char buffer[50];
    snprintf(buffer, 50, "Sudoku - Nível: %.0lf %%", (double) opts.num_remove / 58.0 * 100.0);

    SDL_SetWindowTitle(gpx.win, buffer);
    SDL_RenderPresent(gpx.ren);
}

int grid_coords_y(int win_y) {
    for (int l = 0; l < 9; l++) {
        int aux = get_win_y(l);
        if (aux < win_y && aux + CELL_WIDTH > win_y) return l;
    }
    return -1;
}

int grid_coords_x(int win_x) {
    for (int c = 0; c < 9; c++) {
        int aux = get_win_x(c);
        if (aux < win_x && aux + CELL_WIDTH > win_x) return c;
    }
    return -1;
}

std::optional<Coords> grid_coords(int win_x, int win_y) {
    int x_grid = grid_coords_x(win_x);
    if (x_grid == -1) return {};

    int y_grid = grid_coords_y(win_y);
    if (y_grid == -1) return {};

    return std::make_pair(y_grid, x_grid);
}

void reset_board(Board& board, Options& opts) {
    board.clear();
    board.fill(true);
    while (!board.remove(opts.num_remove)) {
        board.clear();
        board.fill(true);
    }
}

void handle_event(SDL_Event& event, Board& board, State& stat, Options& opts) {
    if (event.type == SDL_QUIT) stat.quit = true;
    if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
            case SDLK_1: case SDLK_2: case SDLK_3:
            case SDLK_4: case SDLK_5: case SDLK_6:
            case SDLK_7: case SDLK_8: case SDLK_9: {
                int val = event.key.keysym.sym - '0';
                if (stat.selected && board.is_allowed(val, stat.y, stat.x)) {
                    board.set_tile(val, stat.y, stat.x);
                }
                break;
            }
            case SDLK_w:
                board.save(opts);
                break;
            case SDLK_l:
                board.load(opts);
                break;
            case SDLK_DELETE:
                if (stat.selected && !board.is_original(stat.y, stat.x)) board.set_tile(0, stat.y, stat.x);
                break;
            case SDLK_q:
                stat.quit = true;
                break;
            case SDLK_a:
                opts.annotations = !opts.annotations;
                break;
            case SDLK_RIGHTBRACKET:
                opts.num_remove--;
                if (opts.num_remove == 0) opts.num_remove = 1;
                reset_board(board, opts);
                break;
            case SDLK_LEFTBRACKET:
                opts.num_remove++;
                if (opts.num_remove == 59) opts.num_remove = 58;
                reset_board(board, opts);
                break;
            case SDLK_e:
                board.clear();
                break;
            case SDLK_c:
                board.consolidate();
                break;
            case SDLK_n:
                reset_board(board, opts);
                break;
            case SDLK_s:
                board.fill(false);
                break;
            case SDLK_h:
                opts.hints = !opts.hints;
                break;
            case SDLK_r:
                board.clear_user();
                break;
            case SDLK_UP:
                if (stat.selected && stat.y > 0) stat.y--;
                break;
            case SDLK_DOWN:
                if (stat.selected && stat.y < 8) stat.y++;
                break;
            case SDLK_LEFT:
                if (stat.selected && stat.x > 0) stat.x--;
                break;
            case SDLK_RIGHT:
                if (stat.selected && stat.x < 8) stat.x++;
                break;
            case SDLK_SPACE:
                if(stat.selected && !board.is_original(stat.y, stat.x)) board.set_tile(0, stat.y, stat.x);
                break;
        }
    }
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        int mouse_x, mouse_y;
        SDL_GetMouseState(&mouse_x, &mouse_y);
        auto coords = grid_coords(mouse_x, mouse_y);
        if (event.button.button == SDL_BUTTON_LEFT) {
            if (mouse_y > CELL_WIDTH + THICK_PAD) {
                stat.selected = false;
                stat.highlight = 0;

                if (coords && !board.is_original(coords->first, coords->second)) {
                    stat.selected = true;
                    stat.y = coords->first;
                    stat.x = coords->second;
                }

                if (coords && board.get_tile(coords->first, coords->second) != 0) {
                    stat.highlight = board.get_tile(coords->first, coords->second);
                }
            } else {
                int grid_x = grid_coords_x(mouse_x);
                if (stat.selected && grid_x != -1 && board.is_allowed(grid_x + 1, stat.y, stat.x))
                    board.set_tile(grid_x + 1, stat.y, stat.x);
            }
        } else {
            stat.selected = true;
            stat.y = coords->first;
            stat.x = coords->second;
            if (!board.is_original(coords->first, coords->second))
                board.set_tile(0, coords->first, coords->second);
        }
    }
}

void verify_game_over(Board& board, Options& opts) {
    if (board.next_empty().first == -1) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Parabéns!", "Tabuleiro completo!", NULL);
        reset_board(board, opts);
    }
}

void main_game(Graphics& gpx, Options& opts) {
    SDL_Event event;

    if (opts.seed != 0)
        srand(opts.seed);
    else
        srand(time(NULL));

    Board board;
    reset_board(board, opts);

    State stat;

    while(!stat.quit) {
        while(SDL_PollEvent(&event)) {
            handle_event(event, board, stat, opts);
        }
        draw(gpx, stat,board, opts);
        SDL_Delay(100);
        verify_game_over(board, opts);
    }
}

void print_help() {
    printf("sudoku [opções]\n");
    printf("Opções:\n");
    printf("\t-s: Semente do gerador de números pseudo-aleatórios.\n");
    printf("\t-a: Ativa as dicas.\n");
    printf("\t-r: Número de células em branco.\n");
    printf("\n\nTeclas de atalho:\n");
    printf("\ts: Soluciona o tabuleiro.\n");
    printf("\tr: Apaga todos os números do usuário.\n");
    printf("\te: Apaga todas as células.\n");
    printf("\tc: Torna as células fornecidas permanentes.\n");
    printf("\tn: Novo jogo.\n");
}

Options parse_options(int argc, char** argv) {
    Options opts;

    int c;
    while ((c = getopt(argc, argv, "s:h")) != -1) {
        switch(c) {
            case 's':
                opts.seed = atoi(optarg);
                break;
            case 'h':
                print_help();
                exit(EXIT_SUCCESS);
                break;
            case '?':
                fprintf(stderr, "A opção -%c necessita de um argumento.\n", optopt);
            default:
                exit(EXIT_FAILURE);
        }
    }

    if (optind < argc) {
        fprintf(stderr, "Esse programa não aceita argumentos extras.\n");
        exit(1);
    }

    return opts;
}

int main(int argc, char** argv) {
    Options opts = parse_options(argc, argv);
    Graphics gpx = init_graphics();

    main_game(gpx, opts);

    end_graphics(gpx);
    return EXIT_SUCCESS;
}
