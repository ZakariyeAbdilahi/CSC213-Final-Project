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

    // Receive welcome message from the server
    char *welcome_message = receive_message(socket_fd);
    if (welcome_message) {
        printf("Server: %s", welcome_message);
        free(welcome_message);
    }

    char *line = NULL;
    size_t len = 0;

    // Communication loop
    while (1) {
        // Wait for a message from the server
        char *server_message = receive_message(socket_fd);
        if (server_message == NULL) {
            printf("Disconnected from server.\n");
            break;
        }
        printf("Server: %s", server_message);

        // If it's the client's turn, allow them to send a message
        if (strcmp(server_message, "Your Turn\n") == 0) {
            printf("Enter a message (type 'quit' to exit): ");
            getline(&line, &len, stdin);

            if (strcmp(line, "quit\n") == 0) {
                free(server_message);
                break;
            }

            if (send_message(socket_fd, line) == -1) {
                perror("Failed to send message to server");
                free(server_message);
                break;
            }
        }
        free(server_message);
    }

    free(line);
    close(socket_fd);

    return 0;
}
