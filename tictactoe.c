#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define PLAYER1 'X'
#define PLAYER2 'O'
#define EMPTY ' '
#define MAX_GAMES 7
#define LOG_DIR "game_logs"

char board[3][3]; // Declare board as a 3x3 array
pthread_mutex_t board_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex to protect the board state

// Player names
char player1Name[50];
char player2Name[50];

// Shared structure to track wins
typedef struct {
    int player1Wins;
    int player2Wins;
    int ties;
    pthread_mutex_t score_mutex;
} GameScores;

GameScores scores = {0, 0, 0, PTHREAD_MUTEX_INITIALIZER}; // Initialize the score structure

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

// Function to ensure the log directory exists
void ensureLogDirectory() {
    struct stat st;
    if (stat(LOG_DIR, &st) == -1) {
        if (mkdir(LOG_DIR, 0777) == -1) {
            perror("Error creating log directory");
            exit(EXIT_FAILURE);
        }
    }
}

// Function to record the result to a text file
void convertToTxt(char winner) {
    ensureLogDirectory();

    char filepath[100];
    snprintf(filepath, sizeof(filepath), "%s/Game-Records.txt", LOG_DIR);

    FILE *fp = fopen(filepath, "a"); // Open for appending
    if (fp == NULL) {
        perror("Error opening file");
        return;
    }

    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char timestamp[50];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(fp, "[%s] Game between %s and %s\n", timestamp, player1Name, player2Name);
    if (winner == 'T') {
        fprintf(fp, "Result: It's a tie!\n");
    } else {
        fprintf(fp, "Winner: %c\n", winner);
    }
    fprintf(fp, "Final Board State:\n");
    for (int i = 0; i < 3; i++) {
        fprintf(fp, " %c | %c | %c \n", board[i][0], board[i][1], board[i][2]);
        if (i < 2) fprintf(fp, "---|---|---\n");
    }
    fprintf(fp, "\n");

    fclose(fp);
}

int main() {
    srand(time(NULL)); // Seed for random number generation

    // Prompt for player names before starting the game
    printf("Enter player 1 name: ");
    fgets(player1Name, sizeof(player1Name), stdin);
    player1Name[strcspn(player1Name, "\n")] = '\0';  // Remove trailing newline

    printf("Enter player 2 name: ");
    fgets(player2Name, sizeof(player2Name), stdin);
    player2Name[strcspn(player2Name, "\n")] = '\0';  // Remove trailing newline

    pthread_t threads[MAX_GAMES]; // Array to store thread IDs
    int gameCount = 0;

    // Play games until one player wins 3 times
    while (scores.player1Wins < 3 && scores.player2Wins < 3 && gameCount < MAX_GAMES) {
        pthread_create(&threads[gameCount], NULL, playGame, NULL);
        pthread_join(threads[gameCount], NULL); // Wait for the game to finish
        gameCount++;

        printf("Current score: %s - %d, %s - %d\n", player1Name, scores.player1Wins, player2Name, scores.player2Wins);
    }

    // Declare final winner
    if (scores.player1Wins > scores.player2Wins) {
        printf("%s wins the best-of-5!\n", player1Name);
    } else {
        printf("%s wins the best-of-5!\n", player2Name);
    }

    return 0;
}
