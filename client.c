#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "message.h"
#include "socket.h"

int main(int argc, char **argv) {
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

    printf("Connected to server\n");

    // Receive welcome message
    char *welcome_message = receive_message(socket_fd);
    if (welcome_message != NULL) {
        printf("%s\n", welcome_message);
        free(welcome_message);
    }

    char *line = NULL;
    size_t len = 0;

    // Game loop: send moves and receive responses
    while (1) {
        // Wait for the server's prompt
        char *server_message = receive_message(socket_fd);
        if (server_message == NULL) {
            perror("Failed to read message from server");
            break;
        }

        printf("Server: %s\n", server_message);
        free(server_message);

        // Input move
        printf("Enter your move: ");
        getline(&line, &len, stdin);

        // Send the move to the server
        int rc = send_message(socket_fd, line);
        if (rc == -1) {
            perror("Failed to send message to server");
            break;
        }
    }

    free(line);
    close(socket_fd);

    return 0;
}
