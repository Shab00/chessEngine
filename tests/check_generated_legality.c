#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "position.h"
#include "movegen.h"

/* Prototype for the attack test already in your codebase */
int is_square_attacked(const Position *pos, int sq, int by);

/*
  For each generated move in the search tree, make the move and assert that
  the side that moved does not have its king in check after the move.
  This detects moves that should be illegal (leave mover's king in check)
*/

#define MAX_PLY 64
static int path_from[MAX_PLY];
static int path_to[MAX_PLY];
static int path_prom[MAX_PLY];
static int cur_ply = 0;
static int found = 0;

static void print_path(int ply) {
    char a[8], b[8];
    for (int i = 0; i < ply; ++i) {
        position_square_to_coords(path_from[i], a, sizeof a);
        position_square_to_coords(path_to[i], b, sizeof b);
        if (path_prom[i] != 0) printf("%s%s=%d ", a, b, path_prom[i]);
        else printf("%s%s ", a, b);
    }
    printf("\n");
}

static int find_king_sq(const Position *pos, int color) {
    for (int i = 0; i < 64; ++i) {
        int8_t v = pos->board[i];
        if (color == COLOR_WHITE && v == PIECE_KING) return i;
        if (color == COLOR_BLACK && v == -PIECE_KING) return i;
    }
    return POS_NO_SQUARE;
}

static void check_tree(Position *pos, int depth) {
    if (depth == 0) return;
    int cap = 1024;
    int *from = malloc(sizeof(int)*cap);
    int *to   = malloc(sizeof(int)*cap);
    int *prom = malloc(sizeof(int)*cap);
    if (!from || !to || !prom) { free(from); free(to); free(prom); return; }

    int n = generate_legal_moves(pos, from, to, prom, cap);
    for (int i = 0; i < n; ++i) {
        Position before;
        memcpy(&before, pos, sizeof(Position));

        MoveUndo undo;
        make_move(pos, from[i], to[i], prom[i], &undo);

        /* The side that moved is the opposite of pos->side_to_move now */
        int moved_color = (pos->side_to_move == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
        int king_sq = find_king_sq(pos, moved_color);

        if (king_sq == POS_NO_SQUARE) {
            char fen_before[512], fen_after[512];
            position_to_fen(&before, fen_before, sizeof fen_before);
            position_to_fen(pos, fen_after, sizeof fen_after);
            printf("Missing king after move (path): ");
            path_from[cur_ply] = from[i]; path_to[cur_ply] = to[i]; path_prom[cur_ply] = prom[i];
            print_path(cur_ply+1);
            printf("Before: %s\nAfter:  %s\n", fen_before, fen_after);
            found++;
        } else if (is_square_attacked(pos, king_sq, (moved_color == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE)) {
            char fen_before[512], fen_after[512];
            position_to_fen(&before, fen_before, sizeof fen_before);
            position_to_fen(pos, fen_after, sizeof fen_after);
            printf("Illegal generated move (leaves mover in check) at path:\n");
            path_from[cur_ply] = from[i]; path_to[cur_ply] = to[i]; path_prom[cur_ply] = prom[i];
            print_path(cur_ply+1);
            printf("Before: %s\nAfter:  %s\n", fen_before, fen_after);
            found++;
        }

        /* Recurse */
        cur_ply++;
        check_tree(pos, depth - 1);
        cur_ply--;

        unmake_move(pos, &undo);
    }

    free(from); free(to); free(prom);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <FEN> <depth>\n", argv[0]);
        return 2;
    }
    const char *fen = argv[1];
    int depth = atoi(argv[2]);
    Position pos; char err[256];
    if (position_from_fen(&pos, fen, err, sizeof err) != POS_OK) {
        fprintf(stderr, "FEN parse failed: %s\n", err);
        return 3;
    }
    check_tree(&pos, depth);
    if (found == 0) printf("No illegal generated moves found up to depth %d\n", depth);
    else printf("Total illegal generated moves found: %d\n", found);
    return 0;
}
