#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "uci.h"
#include "position.h"
#include "movegen.h"
#include "search.h"

static Position g_position;
static int last_a1 = -999;

static int promo_from_char(char c) {
    switch (tolower((unsigned char)c)) {
        case 'q': return PIECE_QUEEN;
        case 'r': return PIECE_ROOK;
        case 'b': return PIECE_BISHOP;
        case 'n': return PIECE_KNIGHT;
        default:  return PIECE_EMPTY;
    }
}

static char promo_char(int promo) {
    switch (promo) {
        case PIECE_KNIGHT: return 'n';
        case PIECE_BISHOP: return 'b';
        case PIECE_ROOK:   return 'r';
        case PIECE_QUEEN:  return 'q';
        default:           return 0;
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
    if (from == POS_NO_SQUARE || to == POS_NO_SQUARE) {
        printf("ALERT: Could not parse move %s (from=%d to=%d)\n", move_str, from, to);
        fflush(stdout);
        return;
    }
    int promo = PIECE_EMPTY;
    if (strlen(move_str) >= 5) promo = promo_from_char(move_str[4]);
    // --------- DEBUG: show move string, from/to indices, promo ---------
//    printf("DEBUG: move_str=%s from=%d to=%d promo=%d\n", move_str, from, to, promo);
//    fflush(stdout);
    printf("DEBUG UCI: %s from=%d to=%d\n", move_str, from, to);
    fflush(stdout);
    // -------------------------------------------------------------------
    MoveUndo mv_undo;
    make_move(&g_position, from, to, promo, &mv_undo);

    // --------- BEGIN: SUPER DEBUGGING AFTER EACH MOVE ---------
    // Print board ASCII
    printf("info string BOARD after move %s:\n", move_str);
    position_print_ascii(&g_position, stdout);
    fflush(stdout);

    // Print FEN and a1/b1/c1
    char fen_str[128];
    position_to_fen(&g_position, fen_str, sizeof(fen_str));
    printf("info string FEN after move: %s\n", fen_str);
    int a1 = g_position.board[SQ_INDEX(0,0)];
    int b1 = g_position.board[SQ_INDEX(1,0)];
    int c1 = g_position.board[SQ_INDEX(2,0)];
    printf("info string a1=%d b1=%d c1=%d\n", a1, b1, c1);
    fflush(stdout);

    // Print alert for unexpected changes on a1
    if (a1 != last_a1) {
        printf("!!! ALERT a1 changed: old=%d new=%d\n", last_a1, a1);
        fflush(stdout);
        last_a1 = a1;
    }
    // --------- END: SUPER DEBUGGING AFTER EACH MOVE ---------
}

static void format_move(int from, int to, int promo, char *buf, int bufsz) {
    char fbuf[3], tbuf[3];
    position_square_to_coords(from, fbuf, sizeof(fbuf));
    position_square_to_coords(to,   tbuf, sizeof(tbuf));
    char pc = promo_char(promo);
    if (pc) {
        snprintf(buf, bufsz, "%s%s%c", fbuf, tbuf, pc);
    } else {
        snprintf(buf, bufsz, "%s%s", fbuf, tbuf);
    }
}

static int is_bare_promotion(int to, int promo) {
    int to_rank = to / 8;
    return (to_rank == 0 || to_rank == 7) && (promo == 0);
}

void uci_loop(void) {
    char buffer[256];

    while (fgets(buffer, sizeof(buffer), stdin)) {
        buffer[strcspn(buffer, "\r\n")] = 0;

        printf("info string received: %s\n", buffer);
        fflush(stdout);

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
                    // Accept only tokens of length 4 or 5 (promotion)
                    char move_str[8] = {0};
                    int n = 0;

                    // Copy chars until a space or end, up to 7 chars for safety
                    while (n < 7 && moves[n] && !isspace((unsigned char)moves[n])) n++;
                    if (n == 4 || n == 5) {
                        memcpy(move_str, moves, n);
                        move_str[n] = '\0';

                        printf("info string applying move: %s\n", move_str);
                        fflush(stdout);

                        make_move_from_uci(move_str);

                        // Print FEN & side after each move
                        char fenbuf[128];
                        position_to_fen(&g_position, fenbuf, sizeof(fenbuf));
                        printf("debug: after applying move %s -> FEN: %s\n", move_str, fenbuf);
                        printf("debug: after applying move %s -> side: %s\n",
                                move_str, g_position.side_to_move == COLOR_WHITE ? "w" : "b");
                        fflush(stdout);
                    } else if (n > 0) {
                        // If it's an unexpected length, print alert and skip
                        char bad_move[8] = {0};
                        memcpy(bad_move, moves, (n < 7 ? n : 7));
                        printf("ALERT: Ignoring invalid move token: '%s'\n", bad_move);
                        fflush(stdout);
                    }

                    moves += n;
                    while (*moves == ' ') moves++; // skip trailing spaces
                }
            }

        } else if (strncmp(buffer, "go", 2) == 0) {
            int movetime = 0, wtime = 0, btime = 0, winc = 0, binc = 0;
            char params[256];
            strncpy(params, buffer + 2, 255);
            params[255] = '\0';
            char* token = strtok(params, " ");
            while (token) {
                if (strcmp(token, "movetime") == 0) {
                    token = strtok(NULL, " ");
                    if (token) movetime = atoi(token);
                } else if (strcmp(token, "wtime") == 0) {
                    token = strtok(NULL, " ");
                    if (token) wtime = atoi(token);
                } else if (strcmp(token, "btime") == 0) {
                    token = strtok(NULL, " ");
                    if (token) btime = atoi(token);
                } else if (strcmp(token, "winc") == 0) {
                    token = strtok(NULL, " ");
                    if (token) winc = atoi(token);
                } else if (strcmp(token, "binc") == 0) {
                    token = strtok(NULL, " ");
                    if (token) binc = atoi(token);
                } else {
                    token = strtok(NULL, " ");
                }
            }
            printf("info string movetime=%d wtime=%d btime=%d winc=%d binc=%d\n",
                   movetime, wtime, btime, winc, binc);

            printf("info string squares: a1=%d b1=%d c1=%d\n",
                g_position.board[SQ_INDEX(0,0)],
                g_position.board[SQ_INDEX(1,0)],
                g_position.board[SQ_INDEX(2,0)]);
            fflush(stdout);

            int depth = 4;
            int best_from = -1, best_to = -1, best_promo = 0;

            if (search_root(&g_position, depth, &best_from, &best_to, &best_promo, NULL) > 0 && best_from >= 0) {
                char bestmove[8];
                format_move(best_from, best_to, best_promo, bestmove, sizeof(bestmove));
                // Also print what squares (from/to indices) and their algebraic names
                char frombuf[3], tobuf[3];
                position_square_to_coords(best_from, frombuf, 3);
                position_square_to_coords(best_to, tobuf, 3);
                printf("info string engine selects: %s (from %d [%s] to %d [%s] promo %d)\n",
                    bestmove, best_from, frombuf, best_to, tobuf, best_promo);
                // === more debug ===
                char fenbuf[128];
                position_to_fen(&g_position, fenbuf, sizeof(fenbuf));
                printf("info string ENGINE FEN before bestmove: %s\n", fenbuf);
                printf("info string ENGINE side to move: %s\n",
                        g_position.side_to_move == COLOR_WHITE ? "w" : "b");
                // ======================
                printf("bestmove %s\n", bestmove);
                fflush(stdout);
            } else {
                printf("bestmove 0000\n");
                fflush(stdout);
            }

        } else if (strncmp(buffer, "setoption ", 10) == 0) {
            printf("info string setoption received: %s\n", buffer + 10);
            fflush(stdout);

        } else if (strcmp(buffer, "quit") == 0) {
            break;
        }
    }
}
