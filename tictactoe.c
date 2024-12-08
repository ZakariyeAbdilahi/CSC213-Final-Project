#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

#define PLAYER1 'X'
#define PLAYER2 'O'
#define EMPTY ' '
#define MAX_GAMES 5

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

// Function to read a valid move from the player
void readMove(int *x, int *y, char player) {
    while (1) {
        if (player == PLAYER1) {
            printf("%s, enter your move (row 1-3 and column 1-3): ", player1Name);
        } else {
            printf("%s, enter your move (row 1-3 and column 1-3): ", player2Name);
        }

        if (scanf("%d %d", x, y) != 2) { // Validate the input
            while (getchar() != '\n'); // Clear invalid input from the buffer
            printf("Invalid input. Please enter numbers for row and column.\n");
            continue;
        }

        if (*x < 1 || *x > 3 || *y < 1 || *y > 3) {
            printf("Invalid move! Row and column must be between 1 and 3.\n");
        } else {
            *x -= 1;  // Adjust to 0-based index
            *y -= 1;  // Adjust to 0-based index
            if (board[*x][*y] == EMPTY) {
                break;
            } else {
                printf("Invalid move! That cell is already taken.\n");
            }
        }
    }
}

// Function to make a player's move
void playerMove(char player) {
    int x, y;
    readMove(&x, &y, player);
    pthread_mutex_lock(&board_mutex);
    board[x][y] = player;
    pthread_mutex_unlock(&board_mutex);
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

// Function to play a single game
void *playGame(void *arg) {
    resetBoard(); // Reset the board at the beginning of the game
    char winner = EMPTY;
    int turn = 0; // 0 for player1, 1 for player2

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

            pthread_mutex_lock(&scores.score_mutex);
            if (winner == PLAYER1) {
                scores.player1Wins++;
            } else if (winner == PLAYER2) {
                scores.player2Wins++;
            } else {
                scores.ties++;
            }
            pthread_mutex_unlock(&scores.score_mutex);
            convertToTxt(winner);
            return NULL;  // Exit the game thread once we have a winner
        }

        turn++;
    }

    printBoard();
    printf("It's a tie!\n");

    pthread_mutex_lock(&scores.score_mutex);
    scores.ties++;
    pthread_mutex_unlock(&scores.score_mutex);
    convertToTxt('T');
    return NULL;
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
