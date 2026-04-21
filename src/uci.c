#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "uci.h"
#include "position.h"
#include "movegen.h"

static Position g_position;

static int promo_from_char(char c) {
    switch (tolower((unsigned char)c)) {
        case 'q': return PIECE_QUEEN;
        case 'r': return PIECE_ROOK;
        case 'b': return PIECE_BISHOP;
        case 'n': return PIECE_KNIGHT;
        default: return PIECE_EMPTY;
    }
}

void load_startpos(void) {
    char err[256];
    pos_error_t r = position_from_fen(&g_position,
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        err, sizeof err);
    if (r != POS_OK) fprintf(stderr, "position_from_fen failed: %s\n", err);
}

void load_fen(const char* fen) {
    char err[256];
    pos_error_t r = position_from_fen(&g_position, fen, err, sizeof err);
    if (r != POS_OK) fprintf(stderr, "position_from_fen failed: %s\n", err);
}

void make_move_from_uci(const char* move_str) {
    if (!move_str || strlen(move_str) < 4) return;
    int from = position_square_from_coords(move_str[0], move_str[1]);
    int to   = position_square_from_coords(move_str[2], move_str[3]);
    int promo = PIECE_EMPTY;
    if (strlen(move_str) >= 5) promo = promo_from_char(move_str[4]);
    MoveUndo mv_undo;
    make_move(&g_position, from, to, promo, &mv_undo);
}

void search_for_bestmove(char* move_out) {
    int moves_from[256], moves_to[256], promotions[256];
    int num = generate_legal_moves(&g_position, moves_from, moves_to, promotions, 256);

    if (num > 0) {
        int from = moves_from[0], to = moves_to[0], promo = promotions[0];
        char fbuf[3], tbuf[3];
        position_square_to_coords(from, fbuf, sizeof(fbuf));
        position_square_to_coords(to, tbuf, sizeof(tbuf));
        if (promo != PIECE_EMPTY) {
            snprintf(move_out, 8, "%s%s%c", fbuf, tbuf, " nbrq"[promo]);
        } else {
            snprintf(move_out, 8, "%s%s", fbuf, tbuf);
        }
    } else {
        strcpy(move_out, "0000");
    }
}

void uci_loop(void) {
    char buffer[256];

    while (fgets(buffer, sizeof(buffer), stdin)) {
        buffer[strcspn(buffer, "\r\n")] = 0;

        if (strcmp(buffer, "uci") == 0) {
            printf("id name c-chess-engine\n");
            printf("id author YourName\n");
            printf("uciok\n");
            fflush(stdout);
        } else if (strcmp(buffer, "isready") == 0) {
            printf("readyok\n");
            fflush(stdout);
        } else if (strcmp(buffer, "ucinewgame") == 0) {
        } else if (strncmp(buffer, "position ", 9) == 0) {
            char *ptr = buffer + 9;
            if (strncmp(ptr, "startpos", 8) == 0) {
                load_startpos();
                ptr += 8;
            } else if (strncmp(ptr, "fen ", 4) == 0) {
                ptr += 4;
                char fen[128] = {0};
                const char* moves_kw = strstr(ptr, " moves");
                if (moves_kw) {
                    strncpy(fen, ptr, moves_kw - ptr);
                    fen[moves_kw - ptr] = '\0';
                } else {
                    strncpy(fen, ptr, 127);
                    fen[127] = '\0';
                }
                load_fen(fen);
                ptr = (char*)moves_kw;
            }

            char *moves = strstr(ptr, "moves");
            if (moves) {
                moves += 5;
                while (*moves == ' ') moves++;
                while (*moves) {
                    char move_str[8];
                    if (sscanf(moves, "%7s", move_str) == 1) {
                        make_move_from_uci(move_str);
                    }
                    moves = strchr(moves, ' ');
                    if (!moves) break;
                    while (moves && *moves == ' ') moves++;
                }
            }

        } else if (strncmp(buffer, "go", 2) == 0) {
            char bestmove[8] = "0000";
            search_for_bestmove(bestmove);
            printf("bestmove %s\n", bestmove);
            fflush(stdout);
        } else if (strcmp(buffer, "quit") == 0) {
            break;
        }
    }
}
