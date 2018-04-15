#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

int size = 80;
int maxSquares = 0;
int *** dead_end_boards;
int m,n,k;
int deadEndIndex = 0;

struct moveBoard
{
   int** board;
   int x;
   int y;
   int turn;
};

/* global mutex variable to synchronize the child threads */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* function executed by each thread */
void * thread_solve( void * arg );

// FINDING # OF VALID MOVES
int possibleMoves(struct moveBoard theBoard, int moveList[8]){
	int x = theBoard.x;
	int y = theBoard.y;
	int numMoves = 0;
	if (x+2 < m){
		if (y+1 < n && theBoard.board[x+2][y+1] == 0){
			numMoves++;
			moveList[0] = 1;
		}
		if (y-1 >= 0 && theBoard.board[x+2][y-1] == 0){
			numMoves++;
			moveList[1] = 1;
		}
	}
	if (x-2 >= 0){
		if (y+1 < n && theBoard.board[x-2][y+1] == 0){
			numMoves++;
			moveList[2] = 1;
		}
		if (y-1 >= 0 && theBoard.board[x-2][y-1] == 0){
			numMoves++;
			moveList[3] = 1;
		}
	}
	if (y+2 < n){
		if (x+1 < m && theBoard.board[x+1][y+2] == 0){
			numMoves++;
			moveList[4] = 1;
		}
		if (x-1 >= 0 && theBoard.board[x-1][y+2] == 0){
			numMoves++;
			moveList[5] = 1;
		}
	}
	if (y-2 >= 0){
		if (x+1 < m && theBoard.board[x+1][y-2] == 0){
			numMoves++;
			moveList[6] = 1;
		}
		if (x-1 >= 0 && theBoard.board[x-1][y-2] == 0){
			numMoves++;
			moveList[7] = 1;
		}
	}

	return numMoves;
}

// MAKE MOVE W/ MOVE CODE
void makeMove(struct moveBoard* theBoard, int moveCode){
	int x = theBoard->x;
	int y = theBoard->y;
	if (moveCode == 0){
		theBoard->board[x+2][y+1] = 1;
		theBoard->x += 2;
		theBoard->y += 1;
	}
	else if (moveCode == 1){
		theBoard->board[x+2][y-1] = 1;
		theBoard->x += 2;
		theBoard->y -= 1;
	}
	else if (moveCode == 2){
		theBoard->board[x-2][y+1] = 1;
		theBoard->x -= 2;
		theBoard->y += 1;
	}
	else if (moveCode == 3){
		theBoard->board[x-2][y-1] = 1;
		theBoard->x -= 2;
		theBoard->y -= 1;
	}
	else if (moveCode == 4){
		theBoard->board[x+1][y+2] = 1;
		theBoard->x += 1;
		theBoard->y += 2;
	}
	else if (moveCode == 5){
		theBoard->board[x-1][y+2] = 1;
		theBoard->x -= 1;
		theBoard->y += 2;
	}
	else if (moveCode == 6){
		theBoard->board[x+1][y-2] = 1;
		theBoard->x += 1;
		theBoard->y -= 2;
	}
	else if (moveCode == 7){
		theBoard->board[x-1][y-2] = 1;
		theBoard->x -= 1;
		theBoard->y -= 2;
	}
}

void addDeadEnd(int ** board, int squaresCovered){
	if (squaresCovered >= k) {
		if (deadEndIndex >= size) {
			size = size + 80;
			dead_end_boards = realloc(dead_end_boards, size * sizeof(int**));
		}
		dead_end_boards[deadEndIndex] = calloc(m, sizeof(int*));
		for (int i = 0; i < m; i++) {
			dead_end_boards[deadEndIndex][i] = calloc(n, sizeof(int));
			for (int j = 0; j < n; j++) {
				dead_end_boards[deadEndIndex][i][j] = board[i][j];
			}
		}
		deadEndIndex+=1;
	}
}

int parent(struct moveBoard* oldBoard){
	int moves[8];
	for (int i = 0; i < 8; i++)
		moves[i] = 0;

	int numMoves = possibleMoves(*oldBoard,moves);
	while (numMoves == 1){
		int moveCode = 0;
		for(int j = 0; j < 8; j++){
			if (moves[j] == 1){
				moveCode = j;
				moves[j] = 0;
				break;
			}
		}
		makeMove(oldBoard, moveCode);
		oldBoard->turn+=1;
		numMoves = possibleMoves(*oldBoard,moves);
	}

	if (numMoves > 1) {

		printf("THREAD %u: %d moves possible after move #%d; creating threads\n", (unsigned int)pthread_self(), numMoves, oldBoard->turn);
		fflush(stdout);
		pthread_t tid[numMoves];
		int i, rc;
		for (i = 0; i < numMoves; i++){
		  	int moveCode = 0;
			for(int j = 0; j < 8; j++){
				if (moves[j] == 1){
					moveCode = j;
					moves[j] = 0;
					break;
				}
			}

			struct moveBoard* newBoard = calloc(1,sizeof(struct moveBoard));
			newBoard->x = oldBoard->x; newBoard->y = oldBoard->y; newBoard->turn = oldBoard->turn+1; 
			newBoard->board = calloc(m, sizeof(int*));

			for (int i = 0; i < m; i++) {
				newBoard->board[i] = calloc(n, sizeof(int));
				for (int j = 0; j < n; j++) {
					newBoard->board[i][j] = oldBoard->board[i][j];
				}
			}

			makeMove(newBoard, moveCode);

	   	 	rc = pthread_create( &tid[i], NULL, thread_solve, newBoard );

		    if ( rc != 0 ){
		      fprintf( stderr, "THREAD %u: Could not create thread (%d)\n", (unsigned int)pthread_self(), rc );
		      return EXIT_FAILURE;
		    }
		}
	
		/* wait for child threads to terminate */
		for ( i = 0 ; i < numMoves ; i++ ) {
			//unsigned int * h;
			rc = pthread_join( tid[i], NULL );   /* BLOCKING CALL */

			if ( rc != 0 )
			{
			  fprintf( stderr, "THREAD %u: Could not join thread (%d)\n", (unsigned int)pthread_self(), rc );
		      return EXIT_FAILURE;
			}
		}

	  	// printf( "THREAD %u: All threads terminated\n", (unsigned int)pthread_self() );	
	}
	else {
		printf("THREAD %u: Dead end after move #%d\n", (unsigned int)pthread_self(), oldBoard->turn);
		fflush(stdout);	

		pthread_mutex_lock(&mutex);
			addDeadEnd(oldBoard->board, oldBoard->turn);
		pthread_mutex_unlock(&mutex);

		if (oldBoard->turn > maxSquares)
			maxSquares = oldBoard->turn;
	}

	return EXIT_SUCCESS;
		
}

void * thread_solve( void * arg )
{
	struct moveBoard* theBoard = (struct moveBoard*)arg;
  	parent(theBoard);

  	for (int j = 0; j < m; j++)
    	free(theBoard->board[j]);
    free(theBoard->board);
    free(theBoard);

  	pthread_exit( 0 );
}

int main(int argc, char** argv){ 
	// Arguement handling
	if (argc == 3) {
		m = atoi(argv[1]); n = atoi(argv[2]); k = 1;
	}
	else if (argc == 4){
		m = atoi(argv[1]); n = atoi(argv[2]); k = atoi(argv[3]);
	}
	else {
		fprintf(stderr, "ERROR: Invalid argument(s)\nUSAGE: a.out <m> <n> [<k>]\n");
    	return EXIT_FAILURE;
	}
	if (n <= 2 || m <= 2 || k > (m*n) || k <= 0){
		fprintf(stderr, "ERROR: Invalid argument(s)\nUSAGE: a.out <m> <n> [<k>]\n");
    	return EXIT_FAILURE;
	}

	printf("THREAD %u: Solving the knight's tour problem for a %dx%d board\n", (unsigned int)pthread_self(), m, n);
	fflush(stdout);

	dead_end_boards = calloc(size, sizeof(int**));

	struct moveBoard* theBoard = calloc(1,sizeof(struct moveBoard));
	theBoard->x = 0; theBoard->y = 0; theBoard->turn = 1; 
	theBoard->board = calloc(m, sizeof(int*));
	for (int i = 0; i < m; i++) {
		theBoard->board[i] = calloc(n, sizeof(int));
		for (int j = 0; j < n; j++){
			if (i == 0 && j == 0) theBoard->board[i][j] = 1;
			else theBoard->board[i][j] = 0;
		}
	}

	parent(theBoard);

	//Free board
	for (int j = 0; j < m; j++)
    	free(theBoard->board[j]);
    free(theBoard->board);
    free(theBoard);

	printf("THREAD %u: Best solution found visits %d squares (out of %d)\n", (unsigned int)pthread_self(), maxSquares, m*n);

	for (int g = 0; g < deadEndIndex; g++){
		for (int i = 0; i < m; i++){
			if (i==0)
				printf("THREAD %u: > ", (unsigned int)pthread_self());
			else
				printf("THREAD %u:   ", (unsigned int)pthread_self());
			for (int j = 0; j < n; j++){
				if (dead_end_boards[g][i][j] != 0)
					printf("k");
				else
					printf(".");
			}
			printf("\n");
		}
		for (int j = 0; j < m; j++)
    		free(dead_end_boards[g][j]);
		free(dead_end_boards[g]);
	}
	free(dead_end_boards);

	return EXIT_SUCCESS;
}
