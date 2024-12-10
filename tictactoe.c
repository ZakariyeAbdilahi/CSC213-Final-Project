#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>

#define PLAYER1 'X'
#define PLAYER2 'O'
#define EMPTY ' '

#define NUM_GAMES 5 // Number of games to run concurrently

// Structure to hold game data
typedef struct {
    char board[3][3];
    char player1Name[50];
    char player2Name[50];
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

// Function to count the number of free spaces on the board
int numFreeSpaces(char board[3][3]) {
    int freeSpaces = 0;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (board[i][j] == EMPTY) {
                freeSpaces++;
            }
        }
    }
    return freeSpaces;
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

// Helper function to clear the input buffer
void clearInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {
        // Discard characters until newline or EOF
    }
}

// Function to make a player's move
void playerMove(char board[3][3], char player, const char* playerName) {
    int x, y;
    do {
        printf("%s, enter your move (row 1-3 and column 1-3): ", playerName);
        if (scanf("%d %d", &x, &y) != 2) {
            printf("Invalid input! Please enter two numbers.\n");
            clearInputBuffer(); // Flush invalid input
            continue;
        }
        clearInputBuffer(); // Flush the newline after valid input

        x--; // Adjust to 0-based index
        y--; // Adjust to 0-based index

        if (x < 0 || x >= 3 || y < 0 || y >= 3 || board[x][y] != EMPTY) {
            printf("Invalid move! Try again.\n");
        } else {
            board[x][y] = player;
            break;
        }
    } while (true);
}

// Function to record the result to a text file
void convertToTxt(char board[3][3], const char* player1Name, const char* player2Name, char winner, int gameNumber) {
    FILE *fp = fopen("Game-Records.txt", "a");
    if (fp == NULL) {
        perror("Error opening file");
        return;
    }

    fprintf(fp, "Game %d between %s and %s\n", gameNumber, player1Name, player2Name);
    fprintf(fp, "Winner: %c\n", winner);
    fprintf(fp, "Board State:\n");
    for (int i = 0; i < 3; i++) {
        fprintf(fp, " %c | %c | %c \n", board[i][0], board[i][1], board[i][2]);
        if (i < 2) fprintf(fp, "---|---|---\n");
    }
    fprintf(fp, "\n");

    fclose(fp);
}

// Game logic function that runs in a separate thread
void* runGame(void* arg) {
    GameData* data = (GameData*)arg;
    char (*board)[3] = data->board;
    const char* player1Name = data->player1Name;
    const char* player2Name = data->player2Name;
    int gameNumber = data->gameNumber;

    char winner = EMPTY;
    int turn = 0;

    printf("Starting Game %d...\n", gameNumber);
    printf("Player 1: %s (X)\n", player1Name);
    printf("Player 2: %s (O)\n\n", player2Name);

    resetBoard(board);

    while (numFreeSpaces(board) > 0) {
        printBoard(board);
        if (turn % 2 == 0) {
            playerMove(board, PLAYER1, player1Name);
        } else {
            playerMove(board, PLAYER2, player2Name);
        }

        winner = checkWinner(board);
        if (winner != EMPTY) {
            printf("\nWinner: %c\n", winner);
            printBoard(board);
            convertToTxt(board, player1Name, player2Name, winner, gameNumber);
            free(data); // Free allocated memory
            return NULL;
        }

        turn++;
    }

    printf("\nIt's a tie!\n");
    printBoard(board);
    convertToTxt(board, player1Name, player2Name, 'T', gameNumber);
    free(data); // Free allocated memory
    return NULL;
}

int main() {
    pthread_t threads[NUM_GAMES];

    for (int i = 0; i < NUM_GAMES; i++) {
        GameData* data = malloc(sizeof(GameData));
        if (data == NULL) {
            perror("Failed to allocate memory");
            exit(EXIT_FAILURE);
        }

        printf("Enter player 1 name for Game %d: ", i + 1);
        if (fgets(data->player1Name, sizeof(data->player1Name), stdin) == NULL) {
            perror("Error reading input");
            free(data);
            continue;
        }
        data->player1Name[strcspn(data->player1Name, "\n")] = '\0'; // Remove trailing newline

        printf("Enter player 2 name for Game %d: ", i + 1);
        if (fgets(data->player2Name, sizeof(data->player2Name), stdin) == NULL) {
            perror("Error reading input");
            free(data);
            continue;
        }
        data->player2Name[strcspn(data->player2Name, "\n")] = '\0'; // Remove trailing newline

        data->gameNumber = i + 1;
        resetBoard(data->board);

        // Create thread for the game
        if (pthread_create(&threads[i], NULL, runGame, data) != 0) {
            perror("Error creating thread");
            free(data);
        }
    }

    for (int i = 0; i < NUM_GAMES; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
