#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "message.h"
#include "socket.h"

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server name> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Read command line arguments
    char *server_name = argv[1];
    unsigned short port = atoi(argv[2]);

    // Connect to the server
    int socket_fd = socket_connect(server_name, port);
    if (socket_fd == -1) {
        perror("Failed to connect");
        exit(EXIT_FAILURE);
    }

    // Receive and print the welcome message from the server
    char *welcome_message = receive_message(socket_fd);
    if (welcome_message) {
        printf("Server: %s", welcome_message);
        free(welcome_message);
    }

    char *line = NULL;
    size_t len = 0;

    // Communication loop
    while (1) {
        // Receive and display the board
        char *board = receive_message(socket_fd);
        if (board == NULL) {
            printf("Disconnected from server.\n");
            break;
        }
        printf("Current Board:\n%s", board);
        free(board);

        // Wait for the player's turn
        char *server_message = receive_message(socket_fd);
        if (server_message == NULL) {
            printf("Disconnected from server.\n");
            break;
        }
        printf("Server: %s", server_message);
        free(server_message);

        if (strcmp(server_message, "Your Turn\n") == 0) {
            // Player's turn to make a move
            int x, y;
            printf("Enter your move (x y): ");
            while (1) {
                getline(&line, &len, stdin);
                if (sscanf(line, "%d %d", &x, &y) == 2 && x >= 0 && x < 3 && y >= 0 && y < 3) {
                    break;
                } else {
                    printf("Invalid move. Please enter valid coordinates (x y): ");
                }
            }

            if (send_message(socket_fd, line) == -1) {
                perror("Failed to send move to server");
                free(line);
                break;
            }
        }
    }

    free(line);
    close(socket_fd);

    return 0;
}
