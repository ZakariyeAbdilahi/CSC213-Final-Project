#pragma once
#include <stdbool.h>
#include <pthread.h>

// Constants for players and the board
#define PLAYER1 'X'
#define PLAYER2 'O'
#define EMPTY ' '

// Data structure to track game scores
typedef struct {
    int player1Wins;
    int player2Wins;
    int ties;
    pthread_mutex_t score_mutex;
} GameScores;

// Function declarations

// Resets the game board to an empty state
void resetBoard(void);

// Prints the current state of the board to the console
void printBoard(void);

// Returns the number of free spaces left on the board
int numFreeSpaces(void);

// Checks for a winner and returns the winning player's symbol (or EMPTY if no winner)
char checkWinner(void);

// Reads and validates a player's move, updating x and y with their input
void readMove(int *x, int *y, char player);

// Makes a move for the given player and updates the board
void playerMove(char player);

// Converts the game result to a text file for record-keeping
void convertToTxt(char winner);

// Plays a single game of Tic-Tac-Toe
void *playGame(void *arg);
