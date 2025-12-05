#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

#define DEFAULT_QUEUE_SIZE 100000

// eclipse didn't want to cooperate...
/*#define bool _Bool
#define false 0
#define true 1*/

typedef struct queue queue;
//typedef struct queueNode queueNode; // might be unnecessary to implement the queue as a linked list
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
	int size;
	int capacity; // not going to do any resizing, thi might be helpful for assessing the performance of hash function though
	int k; // might be helpful to keep around
	int k2;
	hashNode** hashTable;

	// insert function
	void (* insert)(hashTable* this, board* board_in);

	// hashing function
	int (* hash)(hashTable* this, int* board_in);

	// ismember function
	bool (* ismember)(hashTable* this, int* tiles);
};

struct queue
{
	board** seenBoards;
	int size;
	int head;
	int tail;

	// enqueue function
	void (* enqueue)(queue* this, board* board_in);
};

struct board
{
	int k; // width/height of board
	int k2; // k^2
	int emptyTileIdx;
	int move; // the piece that was swapped to create this board
	int* tiles; // one dimensional representation of the board's tiles

	board* parent; // useful for building back the array of moves
	board* up;
	board* down;
	board* left;
	board* right;

	bool (*isPossibleFunction)(board* this);
	void (*memberDataCleanup)(board* this);

	// 1D/2D converters
	void (*idx2RowCol)(board* this, int val, int* r, int* c);
	int  (*rowCol2Idx)(board* this, int r, int c);

	// set neighbors for a given row/col
	bool (*neighborPosition)(board* this, int r, int c, int* p_r, int* p_c);

	// check if position is valid
	bool (*isValidPosition)(board* this, int r, int c);

	// function to generate neighboring nodes and assign their pointers back to current
	void (*generateNeighbors)(board* this);
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
bool up(board* this, int r, int c, int* p_r, int* p_c) { *p_r = r - 1; *p_c = c; return(isValidPosition(this, *p_r, *p_c)); }
bool down(board* this, int r, int c, int* p_r, int* p_c) { *p_r = r + 1; *p_c = c; return(isValidPosition(this, *p_r, *p_c)); }
bool left(board* this, int r, int c, int* p_r, int* p_c) { *p_r = r; *p_c = c - 1; return(isValidPosition(this, *p_r, *p_c)); }
bool right(board* this, int r, int c, int* p_r, int* p_c) { *p_r = r; *p_c = c + 1; return(isValidPosition(this, *p_r, *p_c)); }

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
board* initBoard(int k);

// generate SINGLE neighbor
board* generateNeighbor(board* orig, hashTable* table, int emptyIdxRow, int emptyIdxCol, int newIdxRow, int newIdxCol)
{
	board* newBoard = NULL;

	// create a temporary board first and check to see if it is already a member in the map, if it is, return nothing
	int temporaryBoard[orig->k2];
	memcpy(temporaryBoard, orig->tiles, sizeof(temporaryBoard[0]) * orig->k2);
	int emptySpaceIdx = orig->rowCol2Idx(orig, emptyIdxRow, emptyIdxCol);
	int newEmptySpaceIdx = orig->rowCol2Idx(orig, newIdxRow, newIdxCol);

	// now want to swap the empty position with the new one
	swap(temporaryBoard, emptySpaceIdx, newEmptySpaceIdx);

	if(table->ismember(table, temporaryBoard))
	{
		return(newBoard);
	}
	else
	{
		board* newBoard = generateNewBoard(orig);
		memcpy(newBoard->tiles, temporaryBoard, sizeof(temporaryBoard[0] * orig->k2));
	}
	return(newBoard);
}

// neighbor generation function
/*void generateNeighbors(board* this)
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
		this->idx2RowCol(this, this->emptyTileIdx, &emptyRowIdx, &emptyColIdx);

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
}*/

/*int hashValue(hashTable* this, board* b)
{
	int key = 0;
	for(int ii = 0; ii < b->k2; ii++)
	{
		key += (ii*b->tiles[ii]);
	}
	key -= this->capacity;
	return(key);
}*/

//int hashValue(hashTable* this, board* b)
int hashValue(hashTable* this, int* tiles)
{
	unsigned long total = 0;

	for(int ii = 0; ii < this->k2; ii++)
	{
		total += tiles[ii];
		total *= 101;
	}

	return(total % this->capacity);
}

bool ismember(hashTable* this, int* tiles)
{
	int position = this->hash(this, tiles);
	hashNode* slot = this->hashTable[position];

	while(slot != NULL)
	{
		// check to see if the board in the hashed position
		if(!memcmp(slot->board->tiles, tiles, sizeof(tiles[0])*this->k2))
		{
			return(true);
		}
		else
		{
			slot = slot->next;
		}
	}

	return(false);
}

// Hash table slot initializer
hashNode* initializeHashNode(board* this)
{
	hashNode* nodeOut = malloc(sizeof(hashNode));
	nodeOut->board = generateNewBoard(this); // this might be slow
	return(nodeOut);
}

// Hash table insertion
void insertToHashTable(hashTable* this, board* board_in)
{
	int position = this->hash(this, board_in->tiles);
	hashNode* slot = this->hashTable[position];
	hashNode* newNode;

	while(slot != NULL)
	{
		// check to see if the board in the hashed position
		if(!memcmp(slot->board->tiles, board_in->tiles, sizeof(board_in->tiles[0])*board_in->k2))
		{
			return;
		}
		else
		{
			slot = slot->next;
		}
	}

	newNode = initializeHashNode(board_in);
	newNode->next = this->hashTable[position];
	this->hashTable[position] = newNode;
	this->size++;
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
	//boardOut->generateNeighbors = generateNeighbors;

	boardOut->parent = NULL;
	boardOut->up = NULL;
	boardOut->down = NULL;
	boardOut->left = NULL;
	boardOut->right = NULL;
	return(boardOut);
}

// this function should take in the existing board, clone it, reassign
board* generateNewBoard(board* orig)
{
	board* newBoard = initBoard(orig->k);
	memcpy(newBoard->tiles, orig->tiles, sizeof(orig->tiles[0]) * orig->k2);
	newBoard->parent = orig;
	return(newBoard);
}

void enqueue(queue* this, board* board_in)
{
	if(board_in == NULL)
	{
		return;
	}

	this->seenBoards[this->tail] = board_in;
	this->tail++;
	this->size++;
}

// this needs to be revisited
queue* initializeQueue(board* first)
{
	queue* queueOut = malloc(sizeof(queue));
	queueOut->seenBoards = malloc(sizeof(board* ) * DEFAULT_QUEUE_SIZE); // may need to increase as the program runs - or upgrade to linked list
	queueOut->seenBoards[0] = first;
	queueOut->head = 0;
	queueOut->tail = 1;
	queueOut->size = 1;

	// set functions
	queueOut->enqueue = enqueue;

	return(queueOut);
}

hashTable* initializeHashTable(board* first)
{
	hashTable* tableOut = malloc(sizeof(hashTable));

	// set functions
	tableOut->insert = insertToHashTable;
	tableOut->hash = hashValue;
	tableOut->ismember = ismember;

	// compute size of the table based off of k
	int tableSize = 1;
	for(int ii = 0; ii < (first->k << 1) + 7; ii++)
	{
		tableSize = tableSize << 1;
	}

	tableOut->hashTable = malloc(sizeof(hashNode* ) * tableSize);
	if(tableOut->hashTable == NULL)
	{
		fprintf(stderr, "ERROR: failed to allocate %d elements for hash table.\n", tableSize);
		exit(EXIT_FAILURE);
	}

	for(int ii = 0; ii < tableSize; ii++)
	{
		tableOut->hashTable[ii] = NULL;
	}

	tableOut->k = first->k;
	tableOut->k2 = first->k2;
	tableOut->capacity = tableSize;
	tableOut->insert(tableOut, first);

	return(tableOut);
}

/*hashTable* initializeHashTable(board* first)
{
	hashTable* tableOut = malloc(sizeof(hashTable));

	// set functions
	tableOut->insert = insertToHashTable;
	tableOut->hash = hashValue;

	// maximum value we can store in the hash table
	int MaxTableSize = 0;
	for(int ii = 0; ii < first->k2; ii++)
	{
		MaxTableSize += ii*ii;
	}

	// minimum value we can store in the hash table
	int MinTableSize = 0;
	int jj = first->k2;
	for(int ii = 0; ii < first->k2; ii++)
	{
		--jj;
		MinTableSize += ii*jj;
	}

	int tableSize = (MaxTableSize - MinTableSize) + 1;
	tableOut->hashTable = malloc(sizeof(hashNode* ) * tableSize);
	if(tableOut->hashTable == NULL)
	{
		fprintf(stderr, "ERROR: failed to allocate %d elements for hash table.\n", tableSize);
		exit(EXIT_FAILURE);
	}

	for(int ii = 0; ii < tableSize; ii++)
	{
		tableOut->hashTable[ii] = NULL;
	}

	tableOut->capacity = tableSize;
	tableOut->insert(tableOut, first);

	return(tableOut);
}*/

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
	int k;
	int numberOfMoves = INT_MAX; // something to keep track of when number of moves was never set due to not finding a solution

	getline(&line, &lineBuffSize, fp_in); //ignore the first line in file, which is a comment
	fscanf(fp_in, "%d\n", &k); //read size of the board
	getline(&line, &lineBuffSize, fp_in); //ignore the second line in file, which is a comment

	int k2 = k * k;
	int move[k2];

	// initialize the board structure;
	board* gameBoard = initBoard(k);
	gameBoard->parent = NULL;
	gameBoard->move = -1;
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

	// initialize hashtable and queue structures
	// inserts to hashTable by default
	hashTable* table = initializeHashTable(gameBoard);
	queue* q = initializeQueue(gameBoard);

	//printBoard(initial_board, k); //Assuming that I have a function to print the board, print it here to make sure I read the input board properly for DEBUG purposes
	fclose(fp_in);

	if(gameBoard->isPossibleFunction(gameBoard))
	{
		// BFS approach to find the solution
		while (q->head < q->tail)
		{
			int neighborNewRow;
			int neighborNewCol;
			int emptySpaceRow;
			int emptySpaceCol;

			board* currentBoard = q->seenBoards[q->head];
			board* neighborBoard;

			currentBoard->idx2RowCol(currentBoard, currentBoard->emptyTileIdx, &emptySpaceRow, &emptySpaceCol);
			/*int* tempBoard = malloc(sizeof(currentBoard->tiles[0]) * currentBoard->k2);
			memcpy(tempBoard, currentBoard->tiles, sizeof(currentBoard->tiles[0]) * currentBoard->k2);*/
			q->head++;

			// definitely an opportunity here to put these into a function
			// check if there are any valid neighbors UP from current empty
			currentBoard->neighborPosition = up;
			if(currentBoard->neighborPosition(currentBoard, emptySpaceRow, emptySpaceCol, &neighborNewRow, &neighborNewCol))
			{
				neighborBoard = generateNeighbor(currentBoard, table, emptySpaceRow, emptySpaceCol, neighborNewRow, neighborNewCol);
				currentBoard->up = neighborBoard;
				q->enqueue(neighborBoard);
			}

			// check if there are any valid neighbors DOWN from current empty
			currentBoard->neighborPosition = down;
			if(currentBoard->neighborPosition(currentBoard, emptySpaceRow, emptySpaceCol, &neighborNewRow, &neighborNewCol))
			{
				neighborBoard = generateNeighbor(currentBoard, table, emptySpaceRow, emptySpaceCol, neighborNewRow, neighborNewCol);
				currentBoard->down = neighborBoard;
				q->enqueue(neighborBoard);
			}

			// check if there are any valid neighbors LEFT from current empty
			currentBoard->neighborPosition = left;
			if(currentBoard->neighborPosition(currentBoard, emptySpaceRow, emptySpaceCol, &neighborNewRow, &neighborNewCol))
			{
				neighborBoard = generateNeighbor(currentBoard, table, emptySpaceRow, emptySpaceCol, neighborNewRow, neighborNewCol);
				currentBoard->left = neighborBoard;
				q->enqueue(neighborBoard);
			}

			// check if there are any valid neighbors RIGHT from current empty
			currentBoard->neighborPosition = right;
			if(currentBoard->neighborPosition(currentBoard, emptySpaceRow, emptySpaceCol, &neighborNewRow, &neighborNewCol))
			{
				neighborBoard = generateNeighbor(currentBoard, table, emptySpaceRow, emptySpaceCol, neighborNewRow, neighborNewCol);
				currentBoard->right = neighborBoard;
				q->enqueue(neighborBoard);
			}
		}

		// now want to traverse through the parents from the final node back to the parent
		// to build up the path of moves needed to solve the puzzle

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
