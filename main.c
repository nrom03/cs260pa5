#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

typedef struct queue queue;
typedef struct queueNode queueNode; // might be unnecessary to implement the queue as a linked list
typedef struct hashTable hashTable;
typedef struct hashNode hashNode;
typedef struct graphNode graphNode;
typedef struct board board;

struct hashNode
{
	board* board;
	hashNode* next;
};

struct hashTable
{
	hashNode* hashTable;
};

struct queueNode
{
	board* board;
};

struct queue
{
	queueNode* head;
	queueNode* tail;
	int size;
};

struct board
{
	int k; // width/height of board
	int k2; // k^2
	int emptyTileIdx;
	int* tiles; // one dimensional representation of the board's tiles

	board* parent; // useful for building back the array of moves
	bool (*isPossibleFunction)(board* this);
	void (*memberDataCleanup)(board* this);

	// 1D/2D converters
	void (*idx2RowCol)(board* this, int val, int* r, int* c);
	int  (*rowCol2Idx)(board* this, int r, int c);

	// set neighbors for a given row/col
	// "virtual" bool function that will use some pseudo runtime polymorphism to set the position
	// if this is running too slow might want to just remove this, it is more fun this way though
	bool (*neighborPosition)(board* this, int r, int c, int* p_r, int* p_c);

	// check if position is valid
	bool (*isValidPosition)(board* this, int r, int c);

	// function to generate neighboring nodes and assign their pointers back to current
	void (*generateNeighbors)(board* this);

	// reference back to the hashTable
	hashTable* mainHashTable;

	// reference back to the queue
	queue* mainQueue;

};

// setter for row column positions given the value in the array
// want this to take in the value or the index?
void setRowCol(board* this, int val, int* r, int* c)
{
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

void setRowColIdx(board* this, int idx, int* r, int* c)
{
	// only update row and col if the value was found
	if(idx != this->k2)
	{
		*r = idx/this->k;
		*c = idx % this->k;
	}
}


// getter for the index of a specified row and column
int getIdx(board* this, int r, int c)
{
	int idx = (this->k * r) + c;
	return(idx);
}

// function to check if the position is valid
bool isValidPosition(board* this, int r, int c)
{
	return(r >= 0 && c >= 0 && r < this->k && c < this->k);
}

// family of setters for positions
bool up(board* this, int r, int c, int* p_r, int* p_c){ *p_r = r - 1; *p_c = c; return(isValidPosition(this, *p_r, *p_c)); }
bool down(board* this, int r, int c, int* p_r, int* p_c){ *p_r = r + 1; *p_c = c; return(isValidPosition(this, *p_r, *p_c)); }
bool left(board* this, int r, int c, int* p_r, int* p_c){ *p_r = r; *p_c = c - 1; return(isValidPosition(this, *p_r, *p_c)); }
bool right(board* this, int r, int c, int* p_r, int* p_c){ *p_r = r; *p_c = c + 1; return(isValidPosition(this, *p_r, *p_c)); }

// faster solution available using a merge-sort style of splitting where the counts are a function of the sort
// this should be a fine algorithm for this portion though
int computeNumInversions(board* this)
{
	int inversionCount = 0;
	int* tiles = this->tiles; // save a derefence operation per iteration
	int size = this->k2;

	for(int ii = 0; ii < size; ii++)
	{
		for(int jj = ii + 1; jj < size; jj++)
		{
			if(ii < jj && tiles[ii] > tiles[jj] && tiles[ii] != 0 && tiles[jj] != 0)
			{
				++inversionCount;
			}
		}
	}
	return(inversionCount);
}

// if compiler doesn't like you I will switch you back to standard bool
//inline bool isOddFast(unsigned int num){ return(num & 1); }
bool isOdd(int num){ return(num % 2); }

// function to see if a board is solvable at all
bool isSolvable(board* this)
{
	// board is unsolvable if k is odd and number of inversions is odd
	// board is unsolvable if k is even and the sum of row index of empty space + number of inversions is also even
	int numInversions = computeNumInversions(this);
	int emptySpaceRowIndex = this->k2;
	int emptySpaceColIndex = this->k2;
	this->idx2RowCol(this, this->emptyTileIdx, &emptySpaceRowIndex, &emptySpaceColIndex);
	if((isOdd(this->k) && isOdd(numInversions)) || (!isOdd(this->k) && !isOdd(emptySpaceRowIndex + numInversions)))
	{
		return(false);
	}
	return(true);

}

// clear tiles
void clearTiles(board* this)
{
	if(this->tiles != NULL)
	{
		free(this->tiles);
	}
}

void swap(int* arr, int from, int to)
{
	int temp = arr[from];
	arr[from] = arr[to];
	arr[to] = temp;
}

// this is my fault for creating a circular dependency
board* generateNewBoard(board* orig);

// neighbor generation function
void generateNeighbors(board* this)
{
	// for the current board, we want to know the up, down, left, and right nodes relative to the empty node
	// max number of neighbors = 4
	int emptyRowIdx;
	int emptyColIdx;
	int newEmptyRowIdx;
	int newEmptyColIdx;

	bool validNeighbor;
	board* newBoard = NULL;

	for(int ii = 0; ii < 4; ii++)
	{
		// reassign function at runtime
		switch(ii)
		{
		case 0:
			this->neighborPosition = up;
			break;
		case 1:
			this->neighborPosition = down;
			break;
		case 2:
			this->neighborPosition = left;
			break;
		case 3:
			this->neighborPosition = right;
			break;
		}
		this->idx2RowCol(this, this->emptyTileIdx, &emptyRowIdx, &emptyColIdx);
		validNeighbor = this->neighborPosition(this, emptyRowIdx, emptyColIdx, &newEmptyRowIdx, &newEmptyColIdx);
		if(validNeighbor)
		{
			newBoard = generateNewBoard(this);

			// now want to swap the empty position with the new one
			swap(newBoard->tiles, newBoard->rowCol2Idx(this, emptyRowIdx, emptyColIdx), newBoard->rowCol2Idx(this, newEmptyRowIdx, newEmptyColIdx));

			// this might be a good spot to check if it already exists in the hashmap
			// if it does exist in the hasmap, add it to the linked list (open hash map)
			// if it does not exist, add it to the hash map and enqueue it
		}
	}
}

// board initializer
board* initBoard(int k)
{
	board* boardOut = malloc(sizeof(board)); // needs a free later
	boardOut->k = k;
	boardOut->k2 = k*k;
	boardOut->tiles = malloc(sizeof(int) * (boardOut->k2)); // needs a free later
	boardOut->isPossibleFunction = isSolvable;
	boardOut->memberDataCleanup = clearTiles;
	boardOut->rowCol2Idx = getIdx;
	boardOut->idx2RowCol = setRowColIdx;
	boardOut->neighborPosition = NULL;
	boardOut->isValidPosition = isValidPosition;
	boardOut->generateNeighbors = generateNeighbors;
	boardOut->parent = NULL;
	return(boardOut);
}

// this function should take in the existing board, clone it, reassign
board* generateNewBoard(board* orig)
{
	board* newBoard = initBoard(orig->k);
	memcpy(newBoard->tiles, orig->tiles, sizeof(orig->tiles[0]) * orig->k2);
	newBoard->parent = orig;
	newBoard->mainQueue = orig->mainQueue;
	newBoard->mainHashTable = orig->mainHashTable;
	return(newBoard);
}

// this needs to be revisited
queue* initializeQueue(board* first)
{
	queue* queueOut = malloc(sizeof(queue));
	queueOut->head->board = first;
	queueOut->tail->board = first;
	queueOut->size = 1;
	return(queueOut);
}

int main(int argc, char **argv)
{
	FILE* fp_in;
	FILE* fp_out;
	
	fp_in = fopen(argv[1], "r");

	if (fp_in == NULL)
	{
		printf("Could not open input file.\n");
		return -1;
	}
	
	fp_out = fopen(argv[2], "w");

	if (fp_out == NULL)
	{
		printf("Could not open output file.\n");
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

	int k2 = k * k;
	int move[k2];

	// initialize hashtable and queue structures
	hashTable* table = NULL;
	queue* q = NULL;

	// initialize the board structure;
	board* gameBoard = initBoard(k);
	gameBoard->mainHashTable = table;
	gameBoard->mainQueue = q;
	gameBoard->emptyTileIdx = k2;

	for(int ii = 0; ii < k*k; ii++)
	{
		fscanf(fp_in, "%d ", &gameBoard->tiles[ii]);
		if(gameBoard->tiles[ii] == 0)
		{
			gameBoard->emptyTileIdx = ii;
		}
		move[ii] = -1;
	}

	if(gameBoard->emptyTileIdx == k2)
	{
		printf("Error: Board contains no empty spaces!\n");
		return(-1);
	}

	//printBoard(initial_board, k); //Assuming that I have a function to print the board, print it here to make sure I read the input board properly for DEBUG purposes
	fclose(fp_in);

	if(gameBoard->isPossibleFunction(gameBoard))
	{
		//probably within a loop, or however you stored proper moves, print them one by one by leaving a space between moves, as below
		fprintf(fp_out, "#moves\n");
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

	// final cleanup
	gameBoard->memberDataCleanup(gameBoard);
	free(gameBoard);
	free(line);
	return 0;

}
