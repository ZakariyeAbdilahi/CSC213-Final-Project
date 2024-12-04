#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

#define PLAYER1 'X'
#define PLAYER2 'O'
#define EMPTY ' '

#define NUM_GAMES 5 // The number of games to run concurrently

// Declare board as a 3x3 array
char board[3][3];

pthread_mutex_t board_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex to protect the board state

// Player names
char player1Name[50];
char player2Name[50];

// Function to reset the board
void resetBoard() {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            board[i][j] = EMPTY;
        }
    }
}

// Function to print the board
void printBoard() {
    pthread_mutex_lock(&board_mutex);
    printf(" %c | %c | %c \n", board[0][0], board[0][1], board[0][2]);
    printf("---|---|---\n");
    printf(" %c | %c | %c \n", board[1][0], board[1][1], board[1][2]);
    printf("---|---|---\n");
    printf(" %c | %c | %c \n", board[2][0], board[2][1], board[2][2]);
    pthread_mutex_unlock(&board_mutex);
}

// Function to count the number of free spaces on the board
int numFreeSpaces() {
    int freeSpaces = 0;
    pthread_mutex_lock(&board_mutex);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (board[i][j] == EMPTY) {
                freeSpaces++;
            }
        }
    }
    pthread_mutex_unlock(&board_mutex);
    return freeSpaces;
}

// Function to check for a winner
char checkWinner() {
    pthread_mutex_lock(&board_mutex);
    // Check rows
    for (int i = 0; i < 3; i++) {
        if (board[i][0] == board[i][1] && board[i][0] == board[i][2] && board[i][0] != EMPTY) {
            pthread_mutex_unlock(&board_mutex);
            return board[i][0];
        }
    }

    // Check columns
    for (int i = 0; i < 3; i++) {
        if (board[0][i] == board[1][i] && board[0][i] == board[2][i] && board[0][i] != EMPTY) {
            pthread_mutex_unlock(&board_mutex);
            return board[0][i];
        }
    }

    // Check diagonals
    if (board[0][0] == board[1][1] && board[0][0] == board[2][2] && board[0][0] != EMPTY) {
        pthread_mutex_unlock(&board_mutex);
        return board[0][0];
    }
    if (board[0][2] == board[1][1] && board[0][2] == board[2][0] && board[0][2] != EMPTY) {
        pthread_mutex_unlock(&board_mutex);
        return board[0][2];
    }

    pthread_mutex_unlock(&board_mutex);
    return EMPTY;
}

// Function to make a player's move
void playerMove(char player) {
    int x, y;
    do {
        if (player == PLAYER1) {
            printf("%s, enter your move (row 1-3 and column 1-3): ", player1Name);
        } else {
            printf("%s, enter your move (row 1-3 and column 1-3): ", player2Name);
        }

        scanf("%d %d", &x, &y);
        x--;  // Adjust to 0-based index
        y--;  // Adjust to 0-based index

        if (board[x][y] != EMPTY) {
            printf("Invalid move! Try again.\n");
        } else {
            pthread_mutex_lock(&board_mutex);
            board[x][y] = player;
            pthread_mutex_unlock(&board_mutex);
            break;
        }
    } while (board[x][y] != EMPTY);
}

// Function to record the result to a text file
void convertToTxt(char winner) {
    FILE *fp;
    char record[] = "Game-Records.txt";

    fp = fopen(record, "a"); // Open for appending
    if (fp == NULL) {
        perror("Error opening file\n");
        return;
    }

    char loser = (winner == PLAYER1) ? PLAYER2 : PLAYER1;
    fprintf(fp, "Game between %s and %s\n", player1Name, player2Name);
    fprintf(fp, "Winner: %c\n", winner);
    fprintf(fp, "Board State:\n");
    for (int i = 0; i < 3; i++) {
        fprintf(fp, " %c | %c | %c \n", board[i][0], board[i][1], board[i][2]);
        if (i < 2) fprintf(fp, "---|---\n");
    }
    fprintf(fp, "\n");

    fclose(fp);
}

// Game logic function that runs in a separate thread
void* runGame(void* arg) {
    char winner = EMPTY;
    int turn = 0; // 0 for player1, 1 for player2

    resetBoard(); // Reset the board at the beginning of each game

    while (numFreeSpaces() > 0) {
        printBoard();
        if (turn % 2 == 0) {
            playerMove(PLAYER1);
        } else {
            playerMove(PLAYER2);
        }

        winner = checkWinner();
        if (winner != EMPTY) {
            printBoard();
            printf("Winner: %c\n", winner);
            convertToTxt(winner);
            return NULL;  // Exit the game once we have a winner
        }

        turn++;
    }

    printBoard();
    printf("It's a tie!\n");
    convertToTxt('T'); // 'T' for tie
    return NULL;
}

int main() {
    srand(time(NULL)); // Seed for random number generation

    // Prompt for player names before running games
    printf("Enter player 1 name: ");
    fgets(player1Name, sizeof(player1Name), stdin);
    player1Name[strcspn(player1Name, "\n")] = '\0';  // Remove trailing newline

    printf("Enter player 2 name: ");
    fgets(player2Name, sizeof(player2Name), stdin);
    player2Name[strcspn(player2Name, "\n")] = '\0';  // Remove trailing newline

    pthread_t threads[NUM_GAMES]; // Array of threads for multiple games

    // Create threads to run multiple games concurrently
    for (int i = 0; i < NUM_GAMES; i++) {
        if (pthread_create(&threads[i], NULL, runGame, NULL) != 0) {
            perror("Error creating thread");
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < NUM_GAMES; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
