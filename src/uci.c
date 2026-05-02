#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "uci.h"
#include "position.h"
#include "movegen.h"
#include "search.h"

/* -----------------------------------------------------------------------
 * UCI input buffer size.
 *
 * A "position ... moves ..." line in a 400-move game is roughly
 *   len("position startpos moves ") + 400 * 5  ~= 2024 bytes.
 * Use 65536 (64 KB) to be completely safe for any realistic game length,
 * including FEN-based positions with very long move lists.
 * ----------------------------------------------------------------------- */
#define UCI_BUF_SIZE 65536

static Position g_position;
static int last_a1 = -999;

int compute_fun_depth(const Position *pos, int color) {
    int base_depth = 2;
    int knight_count = 0, bishop_count = 0, rook_count = 0, queen_count = 0;

    for (int i = 0; i < 64; ++i) {
        int p = pos->board[i];
        int abs_p = abs(p);
        if ((color == COLOR_WHITE && p > 0) || (color == COLOR_BLACK && p < 0)) {
            switch (abs_p) {
                case PIECE_KNIGHT: knight_count++; break;
                case PIECE_BISHOP: bishop_count++; break;
                case PIECE_ROOK:   rook_count++; break;
                case PIECE_QUEEN:  queen_count++; break;
            }
        }
    }

    int missing_knights  = 2 - knight_count;
    int missing_bishops  = 2 - bishop_count;
    int missing_rooks    = 2 - rook_count;
    int missing_queens   = 1 - queen_count;

    int depth = base_depth
        + (missing_knights  * 1)
        + (missing_bishops  * 1)
        + (missing_rooks    * 2)
        + (missing_queens   * 3);

    if (depth < 2) depth = 2;
    if (depth > 8) depth = 8;

    return depth;
}

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

    printf("DEBUG UCI: %s from=%d to=%d\n", move_str, from, to);
    fflush(stdout);

    MoveUndo mv_undo;
    make_move(&g_position, from, to, promo, &mv_undo);

    /* ---- post-move debug ---- */
    printf("info string BOARD after move %s:\n", move_str);
    position_print_ascii(&g_position, stdout);
    fflush(stdout);

    char fen_str[128];
    position_to_fen(&g_position, fen_str, sizeof(fen_str));
    printf("info string FEN after move: %s\n", fen_str);
    int a1 = g_position.board[SQ_INDEX(0,0)];
    int b1 = g_position.board[SQ_INDEX(1,0)];
    int c1 = g_position.board[SQ_INDEX(2,0)];
    printf("info string a1=%d b1=%d c1=%d\n", a1, b1, c1);
    fflush(stdout);

    if (a1 != last_a1) {
        printf("!!! ALERT a1 changed: old=%d new=%d\n", last_a1, a1);
        fflush(stdout);
        last_a1 = a1;
    }
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

static int validate_move_token(const char *s, int n) {
    if (n != 4 && n != 5) return 0;
    /* from-file, from-rank, to-file, to-rank */
    if (s[0] < 'a' || s[0] > 'h') return 0;
    if (s[1] < '1' || s[1] > '8') return 0;
    if (s[2] < 'a' || s[2] > 'h') return 0;
    if (s[3] < '1' || s[3] > '8') return 0;
    if (n == 5) {
        char p = (char)tolower((unsigned char)s[4]);
        if (p != 'q' && p != 'r' && p != 'b' && p != 'n') return 0;
    }
    return 1;
}

void uci_loop(void) {
    char buffer[UCI_BUF_SIZE];

    while (fgets(buffer, sizeof(buffer), stdin)) {

        size_t len = strlen(buffer);
        if (len == sizeof(buffer) - 1 && buffer[len - 1] != '\n') {
            fprintf(stderr,
                "info string WARNING: UCI input line exceeded %d bytes — "
                "draining and skipping.\n", UCI_BUF_SIZE - 1);
            fflush(stderr);
            int c;
            while ((c = fgetc(stdin)) != '\n' && c != EOF) { /* drain */ }
            continue;
        }

        buffer[strcspn(buffer, "\r\n")] = '\0';

        printf("info string received: %s\n", buffer);
        fflush(stdout);

        /* ================================================================
         * Command dispatch
         * ================================================================ */

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

            /* --- Load base position --- */
            if (strncmp(ptr, "startpos", 8) == 0) {
                load_startpos();
                ptr += 8;
            } else if (strncmp(ptr, "fen ", 4) == 0) {
                ptr += 4;
                char fen[128] = {0};
                const char *moves_kw = strstr(ptr, " moves");
                if (moves_kw) {
                    size_t flen = (size_t)(moves_kw - ptr);
                    if (flen >= sizeof(fen)) flen = sizeof(fen) - 1;
                    strncpy(fen, ptr, flen);
                    fen[flen] = '\0';
                } else {
                    strncpy(fen, ptr, sizeof(fen) - 1);
                    fen[sizeof(fen) - 1] = '\0';
                }
                load_fen(fen);
                ptr = (char *)moves_kw;
            }

            if (ptr == NULL) goto position_done; 

            char *moves = strstr(ptr, "moves");
            if (moves) {
                moves += 5;   /* skip "moves" */
                while (*moves == ' ') moves++;

                while (*moves != '\0') {
                    int n = 0;
                    while (n < 6 && moves[n] != '\0' && !isspace((unsigned char)moves[n]))
                        n++;

                    if (validate_move_token(moves, n)) {
                        char move_str[8] = {0};
                        memcpy(move_str, moves, n);
                        move_str[n] = '\0';

                        printf("info string applying move: %s\n", move_str);
                        fflush(stdout);

                        make_move_from_uci(move_str);

                        char fenbuf[128];
                        position_to_fen(&g_position, fenbuf, sizeof(fenbuf));
                        printf("debug: after applying move %s -> FEN: %s\n", move_str, fenbuf);
                        printf("debug: after applying move %s -> side: %s\n",
                               move_str,
                               g_position.side_to_move == COLOR_WHITE ? "w" : "b");
                        fflush(stdout);

                    } else if (n > 0) {
                        /* Bad token — could be a partial token caused by a
                         * truncated line that somehow slipped through, or
                         * just garbage from the GUI.  Stop processing moves
                         * for this command; do NOT print a noisy ALERT
                         * because partial tokens from truncated lines would
                         * spam the log.  We already guard against truncation
                         * above, so reaching here is genuinely unexpected —
                         * log it quietly to stderr only. */
                        char bad[8] = {0};
                        memcpy(bad, moves, (n < 7 ? n : 7));
                        fprintf(stderr,
                            "info string WARNING: unexpected move token '%s' "
                            "(len=%d) — stopping move replay.\n", bad, n);
                        fflush(stderr);
                        break;
                    }

                    moves += n;
                    while (*moves == ' ') moves++;
                }
            }
            position_done:;

        } else if (strncmp(buffer, "go", 2) == 0) {
            int movetime = 0, wtime = 0, btime = 0, winc = 0, binc = 0;

            char params[UCI_BUF_SIZE];
            strncpy(params, buffer + 2, sizeof(params) - 1);
            params[sizeof(params) - 1] = '\0';

            char *token = strtok(params, " ");
            while (token) {
                if      (strcmp(token, "movetime") == 0) { token = strtok(NULL, " "); if (token) movetime = atoi(token); }
                else if (strcmp(token, "wtime")    == 0) { token = strtok(NULL, " "); if (token) wtime    = atoi(token); }
                else if (strcmp(token, "btime")    == 0) { token = strtok(NULL, " "); if (token) btime    = atoi(token); }
                else if (strcmp(token, "winc")     == 0) { token = strtok(NULL, " "); if (token) winc     = atoi(token); }
                else if (strcmp(token, "binc")     == 0) { token = strtok(NULL, " "); if (token) binc     = atoi(token); }
                else { token = strtok(NULL, " "); }
            }

            printf("info string movetime=%d wtime=%d btime=%d winc=%d binc=%d\n",
                   movetime, wtime, btime, winc, binc);
            printf("info string squares: a1=%d b1=%d c1=%d\n",
                   g_position.board[SQ_INDEX(0,0)],
                   g_position.board[SQ_INDEX(1,0)],
                   g_position.board[SQ_INDEX(2,0)]);
            fflush(stdout);

            int engine_color = g_position.side_to_move;
            
            int depth = compute_fun_depth(&g_position, engine_color);
            printf("info string adaptive depth set to %d\n", depth);
            int knight_count = 0, bishop_count = 0, rook_count = 0, queen_count = 0;
            for (int i = 0; i < 64; ++i) {
                int p = g_position.board[i];
                int abs_p = abs(p);
                if ((engine_color == COLOR_WHITE && p > 0) || (engine_color == COLOR_BLACK && p < 0)) {
                    switch (abs_p) {
                        case PIECE_KNIGHT: knight_count++; break;
                        case PIECE_BISHOP: bishop_count++; break;
                        case PIECE_ROOK:   rook_count++; break;
                        case PIECE_QUEEN:  queen_count++; break;
                    }
                }
            }
            printf("info string pieces left: N=%d B=%d R=%d Q=%d | depth=%d\n",
                    knight_count, bishop_count, rook_count, queen_count, depth);

            int best_from = -1, best_to = -1, best_promo = 0;

            if (search_root(&g_position, depth, &best_from, &best_to, &best_promo, NULL) > 0
                    && best_from >= 0) {
                char bestmove[8];
                format_move(best_from, best_to, best_promo, bestmove, sizeof(bestmove));

                char frombuf[3], tobuf[3];
                position_square_to_coords(best_from, frombuf, 3);
                position_square_to_coords(best_to,   tobuf,   3);
                printf("info string engine selects: %s "
                       "(from %d [%s] to %d [%s] promo %d)\n",
                       bestmove, best_from, frombuf, best_to, tobuf, best_promo);

                char fenbuf[128];
                position_to_fen(&g_position, fenbuf, sizeof(fenbuf));
                printf("info string ENGINE FEN before bestmove: %s\n", fenbuf);
                printf("info string ENGINE side to move: %s\n",
                       g_position.side_to_move == COLOR_WHITE ? "w" : "b");

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
