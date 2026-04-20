#include <stdio.h>
#include <string.h>
#include "uci.h"

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
            // TODO: Add game reset logic if needed in future
        } else if (strcmp(buffer, "quit") == 0) {
            break;
        }
    }
}
