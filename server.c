#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>


#include "message.h"
#include "socket.h"

#define NUM_CLIENTS 2 // Number of clients to connect sequentially

int main() {
    // Open a server socket
    unsigned short port = 0;
    int server_socket_fd = server_socket_open(&port);
    if (server_socket_fd == -1) {
        perror("Server socket was not opened");
        exit(EXIT_FAILURE);
    }

    // Start listening for connections
    if (listen(server_socket_fd, NUM_CLIENTS)) { // Allow up to NUM_CLIENTS connections in the queue
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %u\n", port);

    int client_sockets[NUM_CLIENTS] = {0}; // Array to store client sockets
    char *player_welcome_messages[NUM_CLIENTS] = {
        "Welcome Player 1\nPlease wait for your turn.\n",
        "Welcome Player 2\nPlease wait for your turn.\n"
    };

    // Accept connections for NUM_CLIENTS clients
    for (int i = 0; i < NUM_CLIENTS; i++) {
        client_sockets[i] = server_socket_accept(server_socket_fd);
        if (client_sockets[i] == -1) {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }
        printf("Client %d connected as Player %d!\n", i + 1, i + 1);

        // Send a welcome message to the client
        if (send_message(client_sockets[i], player_welcome_messages[i]) == -1) {
            perror("Failed to send welcome message");
            exit(EXIT_FAILURE);
        }
    }

    // Sequential communication loop
    char *messages[NUM_CLIENTS] = {"Your Turn\n", "Wait for Your Turn\n"};
    int active_player = 0; // Keep track of whose turn it is

    while (1) {
        // Notify all clients whose turn it is
        for (int i = 0; i < NUM_CLIENTS; i++) {
            if (send_message(client_sockets[i], messages[i == active_player ? 0 : 1]) == -1) {
                perror("Failed to send turn message");
                exit(EXIT_FAILURE);
            }
        }

        // Receive a message from the active player
        char *message = receive_message(client_sockets[active_player]);
        if (message == NULL) {
            printf("Player %d disconnected.\n", active_player + 1);
            break;
        }
        printf("Player %d: %s", active_player + 1, message);

        // Echo the message back in uppercase to the active player
        for (int j = 0; j < strlen(message); j++) {
            message[j] = toupper(message[j]);
        }
        if (send_message(client_sockets[active_player], message) == -1) {
            perror("Failed to send message to client");
            free(message);
            break;
        }
        free(message);

        // Switch to the next player
        active_player = (active_player + 1) % NUM_CLIENTS;
    }

    // Close all client connections
    for (int i = 0; i < NUM_CLIENTS; i++) {
        close(client_sockets[i]);
    }
    close(server_socket_fd);

    return 0;
}
