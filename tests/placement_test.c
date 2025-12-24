#include <stdio.h>
#include <stdlib.h>
#include "position.h"

static const char *default_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR";

static void print_signed_board(const Position *pos)
{
    for (int r = 7; r >= 0; --r) {
        for (int f = 0; f < 8; ++f) {
            int idx = r * 8 + f;
            printf("%3d", (int)pos->board[idx]);
        }
        printf("\n");
    }
}

static char piece_char(int8_t v)
{
    if (v == 0) return '.';
    const char *order = "PNBRQK";
    int a = piece_abs(v);
    char base = (a >= 1 && a <= 6) ? order[a-1] : '?';
    return (v > 0) ? base : (char)(base - 'A' + 'a');
}

static void print_ascii_board(const Position *pos)
{
    for (int r = 7; r >= 0; --r) {
        printf("%d  ", r+1);
        for (int f = 0; f < 8; ++f) {
            int idx = r * 8 + f;
            printf("%c ", piece_char(pos->board[idx]));
        }
        printf("\n");
    }
    printf("\n   a b c d e f g h\n");
}

int main(int argc, char **argv)
{
    const char *fen = (argc > 1) ? argv[1] : default_fen;
    Position pos;
    char err[128];

    pos_error_t res = position_from_fen(&pos, fen, err, sizeof err);
    if (res != POS_OK) {
        fprintf(stderr, "position_from_fen failed: %s\n", err);
        return 1;
    }

    printf("Parsed board (signed ints):\n");
    print_signed_board(&pos);
    printf("\nASCII board:\n");
    print_ascii_board(&pos);

    return 0;
}
