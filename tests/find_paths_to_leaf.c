#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "position.h"
#include "movegen.h"

#define MAX_PLY 64

static int path_from[MAX_PLY];
static int path_to[MAX_PLY];
static int path_prom[MAX_PLY];
static int cur_ply = 0;
static int found = 0;
static char target_fen[1024];

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

static void search_leaves(Position *pos, int depth) {
    if (depth == 0) {
        char fen[1024];
        position_to_fen(pos, fen, sizeof fen);
        if (strcmp(fen, target_fen) == 0) {
            found++;
            printf("Path #%d: ", found);
            print_path(cur_ply);
        }
        return;
    }
    int cap = 512;
    int *from = malloc(sizeof(int)*cap);
    int *to   = malloc(sizeof(int)*cap);
    int *prom = malloc(sizeof(int)*cap);
    if (!from || !to || !prom) { free(from); free(to); free(prom); return; }
    int n = generate_legal_moves(pos, from, to, prom, cap);
    for (int i = 0; i < n; ++i) {
        MoveUndo undo;
        make_move(pos, from[i], to[i], prom[i], &undo);
        path_from[cur_ply] = from[i];
        path_to[cur_ply] = to[i];
        path_prom[cur_ply] = prom[i];
        cur_ply++;
        search_leaves(pos, depth - 1);
        cur_ply--;
        unmake_move(pos, &undo);
    }
    free(from); free(to); free(prom);
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <root-FEN> <depth> <target-leaf-FEN>\n", argv[0]);
        return 2;
    }
    const char *root_fen = argv[1];
    int depth = atoi(argv[2]);
    strncpy(target_fen, argv[3], sizeof target_fen - 1);
    target_fen[sizeof target_fen - 1] = '\0';
    Position pos; char err[256];
    if (position_from_fen(&pos, root_fen, err, sizeof err) != POS_OK) {
        fprintf(stderr, "FEN parse failed: %s\n", err);
        return 3;
    }
    search_leaves(&pos, depth);
    printf("Total paths found: %d\n", found);
    return 0;
}
