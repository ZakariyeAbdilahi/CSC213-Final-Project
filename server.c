#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "message.h"

#define PLAYER1 'X'
#define PLAYER2 'O'
#define EMPTY ' '

#define NUM_GAMES 5 // Number of games to run concurrently

#define PORT 8080  // Port for the server to listen on

// Structure to hold game data
typedef struct {
    char board[3][3];
    int player1_fd; // File descriptor for Player 1
    int player2_fd; // File descriptor for Player 2
    pthread_mutex_t game_mutex; // Mutex for synchronizing game access
    int gameNumber; // Unique game number for identifying output
} GameData;

// Function to reset the board
void resetBoard(char board[3][3]) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            board[i][j] = EMPTY;
        }
    }
}

// Function to print the board
void printBoard(char board[3][3]) {
    printf(" %c | %c | %c \n", board[0][0], board[0][1], board[0][2]);
    printf("---|---|---\n");
    printf(" %c | %c | %c \n", board[1][0], board[1][1], board[1][2]);
    printf("---|---|---\n");
    printf(" %c | %c | %c \n", board[2][0], board[2][1], board[2][2]);
}

// Function to check for a winner
char checkWinner(char board[3][3]) {
    // Check rows
    for (int i = 0; i < 3; i++) {
        if (board[i][0] == board[i][1] && board[i][0] == board[i][2] && board[i][0] != EMPTY) {
            return board[i][0];
        }
    }

    // Check columns
    for (int i = 0; i < 3; i++) {
        if (board[0][i] == board[1][i] && board[0][i] == board[2][i] && board[0][i] != EMPTY) {
            return board[0][i];
        }
    }

    // Check diagonals
    if (board[0][0] == board[1][1] && board[0][0] == board[2][2] && board[0][0] != EMPTY) {
        return board[0][0];
    }
    if (board[0][2] == board[1][1] && board[0][2] == board[2][0] && board[0][2] != EMPTY) {
        return board[0][2];
    }

    return EMPTY;
}

// Function to send the current board to the players
void sendBoardToPlayers(GameData* game) {
    char board_message[256];
    snprintf(board_message, sizeof(board_message), "Current Board:\n");
    for (int i = 0; i < 3; i++) {
        snprintf(board_message + strlen(board_message), sizeof(board_message) - strlen(board_message), " %c | %c | %c \n", game->board[i][0], game->board[i][1], game->board[i][2]);
        if (i < 2) strcat(board_message, "---|---|---\n");
    }
    send_message(game->player1_fd, board_message);
    send_message(game->player2_fd, board_message);
}

// Function to make a player's move
int playerMove(GameData* game, char player, int x, int y) {
    if (x < 0 || x >= 3 || y < 0 || y >= 3 || game->board[x][y] != EMPTY) {
        return -1; // Invalid move
    }
    game->board[x][y] = player;
    return 0;
}

// Function to accept a client connection
int accept_connection(int server_fd) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        perror("Error accepting connection");
        exit(EXIT_FAILURE);
    }
    return client_fd;
}

// Game logic function that runs in a separate thread
void* runGame(void* arg) {
    GameData* game = (GameData*)arg;
    char winner = EMPTY;
    int turn = 0;
    int x, y;

    resetBoard(game->board);

    // Print the server (game) number
    printf("Game %d started!\n", game->gameNumber);

    // Game loop
    while (1) {
        pthread_mutex_lock(&game->game_mutex); // Lock the game data

        // Send current board to players
        sendBoardToPlayers(game);

        // Alternate turns between players
        if (turn % 2 == 0) {
            send_message(game->player1_fd, "Your Turn\n");
            send_message(game->player2_fd, "Please wait for Player 1's move.\n");
            // Get Player 1's move
            if (receive_message(game->player1_fd)) {
                if (sscanf(receive_message(game->player1_fd), "%d %d", &x, &y) == 2) {
                    if (playerMove(game, PLAYER1, x-1, y-1) == -1) {
                        send_message(game->player1_fd, "Invalid move. Try again.\n");
                        continue;
                    }
                }
            }
        } else {
            send_message(game->player2_fd, "Your Turn\n");
            send_message(game->player1_fd, "Please wait for Player 2's move.\n");
            // Get Player 2's move
            if (receive_message(game->player2_fd)) {
                if (sscanf(receive_message(game->player2_fd), "%d %d", &x, &y) == 2) {
                    if (playerMove(game, PLAYER2, x-1, y-1) == -1) {
                        send_message(game->player2_fd, "Invalid move. Try again.\n");
                        continue;
                    }
                }
            }
        }

        // Check for winner
        winner = checkWinner(game->board);
        if (winner != EMPTY) {
            if (winner == PLAYER1) {
                send_message(game->player1_fd, "You Win!\n");
                send_message(game->player2_fd, "Player 1 Wins!\n");
            } else if (winner == PLAYER2) {
                send_message(game->player2_fd, "You Win!\n");
                send_message(game->player1_fd, "Player 2 Wins!\n");
            } else {
                send_message(game->player1_fd, "It's a tie!\n");
                send_message(game->player2_fd, "It's a tie!\n");
            }
            break;
        }

        turn++;

        pthread_mutex_unlock(&game->game_mutex); // Unlock the game data
        sleep(1); // To prevent too fast turns, you can adjust this
    }

    free(game); // Free allocated memory for the game
    return NULL;
}

int main() {
    pthread_t threads[NUM_GAMES];
    int server_fd;
    struct sockaddr_in server_addr;

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, NUM_GAMES) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < NUM_GAMES; i++) {
        GameData* game = malloc(sizeof(GameData));
        if (game == NULL) {
            perror("Failed to allocate memory");
            exit(EXIT_FAILURE);
        }

        printf("Waiting for two players for Game %d...\n", i + 1);

        // Accept two players
        game->player1_fd = accept_connection(server_fd);
        game->player2_fd = accept_connection(server_fd);

        printf("Player 1 and Player 2 connected to Game %d\n", i + 1);

        game->gameNumber = i + 1;
        pthread_mutex_init(&game->game_mutex, NULL);

        // Start game thread
        pthread_create(&threads[i], NULL, runGame, (void*)game);
    }

    for (int i = 0; i < NUM_GAMES; i++) {
        pthread_join(threads[i], NULL);
    }

    close(server_fd);
    return 0;
}
