#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <math.h>

// FINDING # OF VALID MOVES
int possibleMoves(int** board, int moveList[8], int x, int y, int m, int n){
	int numMoves = 0;
	if (x+2 < m){
		if (y+1 < n && board[x+2][y+1] == 0){
			numMoves++;
			moveList[0] = 1;
		}
		if (y-1 >= 0 && board[x+2][y-1] == 0){
			numMoves++;
			moveList[1] = 1;
		}
	}
	if (x-2 >= 0){
		if (y+1 < n && board[x-2][y+1] == 0){
			numMoves++;
			moveList[2] = 1;
		}
		if (y-1 >= 0 && board[x-2][y-1] == 0){
			numMoves++;
			moveList[3] = 1;
		}
	}
	if (y+2 < n){
		if (x+1 < m && board[x+1][y+2] == 0){
			numMoves++;
			moveList[4] = 1;
		}
		if (x-1 >= 0 && board[x-1][y+2] == 0){
			numMoves++;
			moveList[5] = 1;
		}
	}
	if (y-2 >= 0){
		if (x+1 < m && board[x+1][y-2] == 0){
			numMoves++;
			moveList[6] = 1;
		}
		if (x-1 >= 0 && board[x-1][y-2] == 0){
			numMoves++;
			moveList[7] = 1;
		}
	}

	return numMoves;
}

// MAKE MOVE W/ MOVE CODE
void makeMove(int** board, int* x, int* y, int moveCode, int* round){
	if (moveCode == 0){
		board[*x+2][*y+1] = *round;
		*x += 2;
		*y += 1;
	}
	else if (moveCode == 1){
		board[*x+2][*y-1] = *round;
		*x += 2;
		*y -= 1;
	}
	else if (moveCode == 2){
		board[*x-2][*y+1] = *round;
		*x -= 2;
		*y += 1;
	}
	else if (moveCode == 3){
		board[*x-2][*y-1] = *round;
		*x -= 2;
		*y -= 1;
	}
	else if (moveCode == 4){
		board[*x+1][*y+2] = *round;
		*x += 1;
		*y += 2;
	}
	else if (moveCode == 5){
		board[*x-1][*y+2] = *round;
		*x -= 1;
		*y += 2;
	}
	else if (moveCode == 6){
		board[*x+1][*y-2] = *round;
		*x += 1;
		*y -= 2;
	}
	else if (moveCode == 7){
		board[*x-1][*y-2] = *round;
		*x -= 1;
		*y -= 2;
	}
}

int intermediate(int** board, int m, int n, int x, int y, int turn, int* send_back){
	int moves[8];
	for (int i = 0; i < 8; i++)
		moves[i] = 0;

	int numMoves = possibleMoves(board, moves, x, y, m, n);
	while (numMoves == 1){
		int moveCode = 0;
		for(int j = 0; j < 8; j++){
			if (moves[j] == 1){
				moveCode = j;
				moves[j] = 0;
				break;
			}
		}
		makeMove(board, &x, &y, moveCode, &turn);
		turn++;
		numMoves = possibleMoves(board, moves, x, y, m, n);
	}

	if (numMoves > 1){
		int p[2], rc;
		rc = pipe(p);
		if ( rc == -1 ) {
			perror( "pipe() failed" );
			return EXIT_FAILURE;
		}
		printf("PID %d: %d moves possible after move #%d\n", getpid(), numMoves, turn);
		fflush(stdout);

#ifdef DISPLAY_BOARD
for (int i = 0; i < m; i++) {
printf("PID %d:   ", getpid());
for (int j = 0; j < n; j++){
if (board[i][j] != 0)
printf("k");
else
printf(".");
}
printf("\n");
}
fflush(stdout);
#endif

		for (int i = 0; i < numMoves; i++){
			pid_t pid_rc = fork();
			if ( pid_rc == -1 ) {
		   		perror( "fork() failed" );
		    	return EXIT_FAILURE;
		  	}

#ifdef NO_PARALLEL
wait( NULL );
#endif

		  	int moveCode = 0;
			for(int j = 0; j < 8; j++){
				if (moves[j] == 1){
					moveCode = j;
					moves[j] = 0;
					break;
				}
			}

			// CHILD
		  	if ( pid_rc == 0 ) {
		  		makeMove(board, &x, &y, moveCode, &turn);
		  		turn++;
		   		int exitCode = intermediate(board, m, n, x, y, turn, p);

		   		int readValue;
		   	    read( p[0], &readValue, sizeof(readValue) );

		   		write( p[1], &readValue, sizeof(readValue) ); // ?

		   		// Free board
				for (int j = 0; j < m; j++)
    				free(board[j]);	
    			free(board);
    		
		   		exit(exitCode);	
			}
		}

  		int status, readValue, max = 0;
  		pid_t wpid;
  		while ((wpid = wait(&status)) > 0);

  		for (int j = 0; j < numMoves; j++){
  			read( p[0], &readValue, sizeof(readValue) );
   			printf("PID %d: Received %d from child\n", getpid(), readValue);
   			fflush(stdout);
   			if (max < readValue)
   				max = readValue;
   		}
   		printf("PID %d: All child processes terminated; sent %d on pipe to parent\n", getpid(), max);
   		fflush(stdout);

   		write( send_back[1], &max, sizeof(max) );

   		// Free board
   		for (int j = 0; j < m; j++)
			free(board[j]);	
		free(board);

   		exit(EXIT_SUCCESS);

	}
	else {
		printf("PID %d: Dead end after move #%d\n", getpid(), turn);
		fflush(stdout);
		write( send_back[1], &turn, sizeof(turn) );		
		printf("PID %d: Sent %d on pipe to parent\n", getpid(), turn); 

#ifdef DISPLAY_BOARD
for (int i = 0; i < m; i++) {
printf("PID %d:   ", getpid());
for (int j = 0; j < n; j++){
if (board[i][j] != 0)
printf("k");
else
printf(".");
}
printf("\n");
}
fflush(stdout);
#endif 
	}

	return EXIT_SUCCESS;
}



int parent(int** board, int m, int n, int x, int y, int turn){
	int max = 0, moves[8];
	for (int i = 0; i < 8; i++)
		moves[i] = 0;

	int numMoves = possibleMoves(board, moves, x, y, m, n);

	int p[2], rc;
	rc = pipe(p);
	if ( rc == -1 ) {
		perror( "pipe() failed" );
		return EXIT_FAILURE;
	}
	printf("PID %d: %d moves possible after move #%d\n", getpid(), numMoves, turn);
	fflush(stdout);

#ifdef DISPLAY_BOARD
for (int i = 0; i < m; i++) {
printf("PID %d:   ", getpid());
for (int j = 0; j < n; j++){
if (board[i][j] != 0)
printf("k");
else
printf(".");
}
printf("\n");
}
fflush(stdout);
#endif

	for (int i = 0; i < numMoves; i++){
		pid_t pid_rc = fork();
		if ( pid_rc == -1 ) {
	   		perror( "fork() failed" );
	    	return EXIT_FAILURE;
	  	}

#ifdef NO_PARALLEL
wait( NULL );
#endif


	  	int moveCode = 0;
		for(int j = 0; j < 8; j++){
			if (moves[j] == 1){
				moveCode = j;
				moves[j] = 0;
				break;
			}
		}

		// CHILD
	  	if ( pid_rc == 0 ) {
	  		makeMove(board, &x, &y, moveCode, &turn);
	  		turn++;
	   		int oc = intermediate(board, m, n, x, y, turn, p);

	   		int readValue;
		   	read( p[0], &readValue, sizeof(readValue) );

	   		write( p[1], &readValue, sizeof(readValue) );

			for (int j = 0; j < m; j++)
				free(board[j]);
			free(board);
		
	   		exit(oc);	
		}
	}
	int status, oc;
	pid_t wpid;
	while ((wpid = wait(&status)) > 0);

	for (int j = 0; j < numMoves; j++){
		read( p[0], &oc, sizeof(oc) );
		printf("PID %d: Received %d from child\n", getpid(), oc);
		fflush(stdout);
		if (max < oc)
			max = oc;
	}	  	
		
	return max;
}


int main(int argc, char** argv){ 
	int m, n;
	int turn = 1;

	// Arguement handling
	if (argc == 3) {
		m = atoi(argv[1]); n = atoi(argv[2]);
	}
	else {
		fprintf(stderr, "ERROR: Invalid argument(s)\nUSAGE: a.out <m> <n>\n");
    	return EXIT_FAILURE;
	}
	if (n <= 2 || m <= 2){
		fprintf(stderr, "ERROR: Invalid argument(s)\nUSAGE: a.out <m> <n>\n");
    	return EXIT_FAILURE;
	}

	printf("PID %d: Solving the knight's tour problem for a %dx%d board\n", getpid(), m, n);
	fflush(stdout);

	// Allocate board
	int** board = calloc(m, sizeof(int*));
	for (int i = 0; i < m; i++) {
		board[i] = calloc(n, sizeof(int));
		for (int j = 0; j < n; j++){
			if (i == 0 && j == 0) board[i][j] = 1;
			else board[i][j] = 0;
		}
	}

	int oc = parent(board, m, n, 0, 0, turn);

	// Free board
	for (int j = 0; j < m; j++)
    	free(board[j]);
    free(board);

	printf("PID %d: Best solution found visits %d squares (out of %d)", getpid(), oc, m*n);
	fflush(stdout);

	return EXIT_SUCCESS;
}

