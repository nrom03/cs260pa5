#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

typedef struct board board;
struct board
{
	int k; // width/height of board
	int k2; // k^2
	int* tiles; // one dimensional representation of the board's tiles

	bool (*isPossibleFunction)(board* this);
	void (*memberDataCleanup)(board* this);

	// 1D/2D converters
	void (*val2RowCol)(board* this, int val, int* r, int* c);
	int  (*rowCol2Idx)(board* this, int r, int c);

	// set neighbors for a given index

};

// setter for row column positions given the value in the array
// want this to take in the value or the index?
void setRowCol(board* this, int val, int* r, int* c)
{
	r = NULL;
	c = NULL;
	int oneDimArrayPos = this->k2; // invalid position used to check if the value could be found
	for(int ii = 0; ii < this->k2; ii++)
	{
		if(this->tiles[ii] == val)
		{
			oneDimArrayPos = ii;
			break;
		}
	}

	// only update row and col if the value was found
	if(oneDimArrayPos != this->k2)
	{
		*r = oneDimArrayPos/this->k;
		*c = oneDimArrayPos % this->k;
	}
}

// getter for the index of a specified row and column
int getIdx(board* this, int r, int c)
{
	int idx = (this->k * r) + c;
	return(idx);
}

// function to see if a board is solvable at all
bool isSolvable(board* this)
{

}

// clear tiles
void clearTiles(board* this)
{
	if(this->tiles != NULL)
	{
		free(this->tiles);
	}
}

// board initializer
board* initBoard(int k)
{
	board* boardOut = malloc(sizeof(board));
	boardOut->k = k;
	boardOut->k2 = k*k;
	boardOut->tiles = malloc(sizeof(int) * (boardOut->k2));
	boardOut->isPossibleFunction = isSolvable;
	boardOut->memberDataCleanup = clearTiles;
	boardOut->rowCol2Idx =
	boardOut->val2RowCol = setRowCol;
	return(boardOut);
}

int main(int argc, char **argv)
{
	FILE* fp_in;
	FILE* fp_out;
	
	fp_in = fopen(argv[1], "r");

	if (fp_in == NULL)
	{
		printf("Could not open a file.\n");
		return -1;
	}
	
	fp_out = fopen(argv[2], "w");

	if (fp_out == NULL)
	{
		printf("Could not open a file.\n");
		return -1;
	}

	char* line = NULL;
	size_t lineBuffSize = 0;
	ssize_t lineSize;
	int k;
	int numberOfMoves = INT_MAX; // something to keep track of when number of moves was never set due to not finding a solution


	getline(&line, &lineBuffSize, fp_in); //ignore the first line in file, which is a comment
	fscanf(fp_in, "%d\n", &k); //read size of the board
	//printf("k = %d\n", k); //make sure you read k properly for DEBUG purposes
	getline(&line, &lineBuffSize, fp_in); //ignore the second line in file, which is a comment

	int initial_board[k*k]; //get kxk memory to hold the initial board
	int move[k*k];

	for(int ii = 0; ii < k*k; ii++)
	{
		fscanf(fp_in, "%d ", &initial_board[ii]);
		move[ii] = -1;
	}

	//printBoard(initial_board, k); //Assuming that I have a function to print the board, print it here to make sure I read the input board properly for DEBUG purposes
	fclose(fp_in);

	// 0 is the empty square

	////////////////////
	// do the rest to solve the puzzle
	////////////////////

	// initialize the board structure;
	board* gameBoard = initBoard(k);



	//once you are done, you can use the code similar to the one below to print the output into file
	
	//if it is solvable, then use something as follows:
	fprintf(fp_out, "#moves\n");

	//probably within a loop, or however you stored proper moves, print them one by one by leaving a space between moves, as below
	if(numberOfMoves != INT_MAX)
	{
		for(int i=0;i<numberOfMoves;i++)
		{
			fprintf(fp_out, "%d ", move[i]);
		}
	}
	else
	{
		//if the puzzle is NOT solvable use something as follows
		fprintf(fp_out, "#moves\n");
		fprintf(fp_out, "no solution\n");
	}
	fclose(fp_out);

	return 0;

}
