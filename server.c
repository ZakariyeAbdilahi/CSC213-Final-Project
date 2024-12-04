#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "message.h"
#include "socket.h"

#define MAX_CLIENTS 2 // Limit for simplicity; adjust as needed for the game.

int main() {
    unsigned short port = 0;
    int server_socket_fd = server_socket_open(&port);
    if (server_socket_fd == -1) {
        perror("Server socket was not opened");
        exit(EXIT_FAILURE);
    }

    // Start listening for connections
    if (listen(server_socket_fd, MAX_CLIENTS)) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %u\n", port);

    // Accept multiple clients (up to MAX_CLIENTS)
    int client_sockets[MAX_CLIENTS];
    int connected_clients = 0;

    while (connected_clients < MAX_CLIENTS) {
        int client_socket_fd = server_socket_accept(server_socket_fd);
        if (client_socket_fd == -1) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        printf("Client %d connected!\n", connected_clients + 1);
        client_sockets[connected_clients] = client_socket_fd;
        connected_clients++;

        // Send a welcome message
        char welcome_message[50];
        snprintf(welcome_message, sizeof(welcome_message), "Welcome Player %d!", connected_clients);
        send_message(client_socket_fd, welcome_message);
    }

    printf("All players connected. Starting game...\n");

    // Simple game loop to echo moves
    char *message = NULL;
    while (1) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            send_message(client_sockets[i], "Your move:");
            message = receive_message(client_sockets[i]);
            if (message == NULL) {
                perror("Failed to read message from client");
                exit(EXIT_FAILURE);
            }

            printf("Player %d sent: %s\n", i + 1, message);

            // Echo the move to the other player
            for (int j = 0; j < MAX_CLIENTS; j++) {
                if (i != j) {
                    send_message(client_sockets[j], message);
                }
            }

            free(message);
        }
    }

    // Clean up
    for (int i = 0; i < MAX_CLIENTS; i++) {
        close(client_sockets[i]);
    }
    close(server_socket_fd);

    return 0;
}
