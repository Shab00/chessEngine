#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "position.h"
#include "movegen.h"

static void sq_to_coord(int sq, char *buf) {
    if (sq < 0 || sq >= 64) { buf[0]='\0'; return; }
    position_square_to_coords(sq, buf, 3);
}

static char promo_letter(int p) {
    switch (p) {
    case PIECE_QUEEN:  return 'q';
    case PIECE_ROOK:   return 'r';
    case PIECE_BISHOP: return 'b';
    case PIECE_KNIGHT: return 'n';
    default:           return '\0';
    }
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <FEN> <depth>\n", argv[0]);
        return 2;
    }
    const char *fen = argv[1];
    int depth = atoi(argv[2]);

    Position pos;
    char err[256];
    if (position_from_fen(&pos, fen, err, sizeof err) != POS_OK) {
        fprintf(stderr, "position_from_fen failed: %s\n", err);
        return 3;
    }

    position_print_ascii(&pos, stdout);

    int max_moves = 512;
    int *from = malloc(sizeof(int) * max_moves);
    int *to   = malloc(sizeof(int) * max_moves);
    int *prom = malloc(sizeof(int) * max_moves);
    if (!from || !to || !prom) { fprintf(stderr, "alloc failed\n"); return 4; }

    int n = generate_legal_moves(&pos, from, to, prom, max_moves);

    printf("Legal root moves: %d\n", n);
    for (int i = 0; i < n; ++i) {
        char a[4], b[4];
        sq_to_coord(from[i], a);
        sq_to_coord(to[i], b);
        int moved = pos.board[from[i]];
        char pc = '?';
        switch (abs(moved)) {
          case PIECE_PAWN:   pc = 'P'; break;
          case PIECE_KNIGHT: pc = 'N'; break;
          case PIECE_BISHOP: pc = 'B'; break;
          case PIECE_ROOK:   pc = 'R'; break;
          case PIECE_QUEEN:  pc = 'Q'; break;
          case PIECE_KING:   pc = 'K'; break;
        }
        if (moved < 0) pc += 'a' - 'A';
        printf("%2d: %s%s  %c  prom:%d\n", i+1, a, b, pc, prom[i]);
    }

    printf("\nPerft divide for depth=%d:\n", depth);
    uint64_t total = 0;

    for (int i = 0; i < n; ++i) {
        MoveUndo undo;
        make_move(&pos, from[i], to[i], prom[i], &undo);
        uint64_t nodes = perft(&pos, depth - 1);
        unmake_move(&pos, &undo);

        char a[4], b[4];
        sq_to_coord(from[i], a);
        sq_to_coord(to[i], b);
        if (prom[i] != 0) {
            printf("%s%s%c: %llu\n", a, b, promo_letter(prom[i]), (unsigned long long)nodes);
        } else {
            printf("%s%s: %llu\n", a, b, (unsigned long long)nodes);
        }
        total += nodes;
    }

    printf("Total nodes: %llu\n", (unsigned long long)total);
    printf("Root moves output: %d\n", n);

    free(from); free(to); free(prom);
    return 0;
}
