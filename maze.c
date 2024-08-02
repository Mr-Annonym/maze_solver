/*
maze_solver
Author: Martin Mendl x247581@stud.fit.vut.cz
Date: 15.11.2023
Note: 
This program solves a maze, that is formated in a specific way. 
It can solve the maze using the right or left hand rule, find the shortest path in the maze and also test the maze for validity.
Shortest Path is found using the a* algorithem.
For the a* algorithem to work properrly, I needed to first simplify the maze, into a graph, that has only decision points as nodes.
The a* algorithem is run between the start and end point, that are connected to the graph.
The a* algorithem is run for every entry, exit point combination, and than the shortest path is chosen.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

///////////////////////////////////////////////////////////////////////////////////////////////
// ENUMS
///////////////////////////////////////////////////////////////////////////////////////////////

enum Sides { // declare the directions
    DIAGONAL_LEFT,
    DIAGONAL_RIGHT,
    STRAIGHT
};

void fns(enum Sides *currentDir, int direction) {
    // Calculate the new direction based on left or right.
    if (direction == 0) 
        *currentDir= (*currentDir +2) % 3;
    else 
        *currentDir = (*currentDir + 1) % 3;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// STRUCTURES
///////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
    int row;
    int col;
} Point;

typedef struct {
  int rows;
  int cols;
  unsigned char *cells;
} Map;

typedef struct {
    int row;
    int col;
    enum Sides entrySide;
} EntryPoint;

typedef struct {
    EntryPoint *entryPoints;
    int entryPointsCount;
} EntryPointsArray;

typedef struct {
    Point startPoint;
    Point endPoint;
    enum Sides currentDir;
    int distance;
    enum Sides *moveThrowFaces;
    bool finished;
} MazePoint;

typedef struct {
    MazePoint *mazePoints;
    int mazePointsCount;
} MazePointsArray;

typedef struct {
    Point* points;
    int pointsCount;
} Path;

typedef struct {
    int* pathIdxes;
    int pathIdxesCount;
}ResultPathArray;

///////////////////////////////////////////////////////////////////////////////////////////////
// GENERAL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////

/**
* @brief - coverst strings form 0 to 9 to int
* @return - returns an int
*/
int convertCharToInt(char c) {
    return -1*(c < '0' || c > '9') + (c - '0');
}

/** 
* @brief - check weather the triangle cell is pointing up
* @return - bool
*/
bool cellPointingUp(int row, int col) {
    if (row%2 != col%2)
        return true;
    return false;
}

/** 
* @brief - calculates the square root of a number
* @param - int number
* @return - double square root
*/
double calcSquareRoot(int number) {
    double root = number / 3;
    double lastRoot = 0;
    while (root != lastRoot) {
        lastRoot = root;
        root = (number / root + root) / 2;
    }
    return root;
} 

/**
 * @brief - calculates the euclidian distance between two points
 * @param - int row1
 * @param - int col1
 * @param - int row2
 * @param - int col2
 * @return - double distance
 */
double calculateEuklidianDistance(int row1, int col1, int row2, int col2) {
    double d1 = row1 - row2;
    double d2 = col1 - col2;
    return calcSquareRoot(d1*d1 + d2*d2);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// MEMORY ALOCATION FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief - adds an entry point to the array
 * @param - EntryPointsArray struct
 * @param - EntryPoint element
 * @return - void
 * @note - reallocates the memory if needed
 */
void addEntryPointToArray(EntryPointsArray* array, EntryPoint element) {
    if (array->entryPointsCount % 10 == 0) {
        array->entryPoints = realloc(array->entryPoints, (array->entryPointsCount + 10) * sizeof(EntryPoint));
        if (array->entryPoints == NULL) {
            fprintf(stderr, "Memory reallocation error.\n");
            exit(1);
        }
    }
    array->entryPoints[array->entryPointsCount++] = element;
}

/**
 * @brief - frees the entry points array
 * @param - EntryPointsArray struct
 * @return - void
 */
void freeEntryPointsArray(EntryPointsArray* arrayStruct) {
    free(arrayStruct->entryPoints);
}

/**
 * @brief - adds a maze point to the array
 * @param - MazePointsArray struct
 * @param - MazePoint element
 * @return - void
 */
void addMazePointToArray(MazePointsArray* array, MazePoint element) {
    if (array->mazePointsCount % 10 == 0) {
        array->mazePoints = realloc(array->mazePoints, (array->mazePointsCount + 10) * sizeof(MazePoint));        
        if (array->mazePoints == NULL) {
            fprintf(stderr, "Memory reallocation error.\n");
            exit(1);
        }
    }
    array->mazePoints[array->mazePointsCount++] = element;
}

/**
 * @brief - removes a maze point from the array
 * @param - MazePointsArray struct
 * @param - int index
 * @return - void
 * @note - reallocates the memory if needed
 */
void removeMazePoint(MazePointsArray *array, int index) {

    if (index < 0 || index >= array->mazePointsCount) {
        fprintf(stderr, "Index out of bounds.\n");
        return;
    }
    MazePoint* elementToRemove = array->mazePoints + index;
    MazePoint* nextElement = elementToRemove + 1;
    size_t bytesToMove = (array->mazePointsCount - index - 1) * sizeof(MazePoint);
    memmove(elementToRemove, nextElement, bytesToMove);
    array->mazePointsCount--;
}


/**
 * @brief - frees the maze points array
 * @param - MazePointsArray struct
 * @return - void
 */
void freeMazePointsArray(MazePointsArray *array) {
    for (int i = 0; i < array->mazePointsCount; i++) {
        free(array->mazePoints[i].moveThrowFaces);
    }
    free(array->mazePoints);
}

/**
 * @brief - adds an int to the array
 * @param - int** array
 * @param - int* size
 * @param - int element
 * @return - void
 * @note - reallocates the memory if needed
 */
void addToIntArray(int** array, int* size, int element) {
    if (*size % 20 == 0) {
        int *newArray = realloc(*array, (*size + 10) * sizeof(int));
        if (newArray == NULL) {
            fprintf(stderr, "Memory reallocation error int.\n");
            exit(1);
        }
        *array = newArray;
    }
    (*array)[*size] = element;
    (*size) += 1;
}

/**
 * @brief - removes an int at index form the array
 * @param - int* array
 * @param - int* size
 * @param - int index
 * @return - void
 */
void removeIntAtIndexFromArray(int* array, int* size, int index) {
    if (index < 0 || index >= *size) {
        // Index out of bounds, handle the error as needed
        fprintf(stderr, "Index out of bounds.\n");
        return;
    }
    int* elementToRemove = array + index;
    int* nextElement = elementToRemove + 1;
    size_t bytesToMove = (*size - index - 1) * sizeof(int);
    memmove(elementToRemove, nextElement, bytesToMove);
    (*size)--;
}

/**
 * @brief - adds an enum Sides to the array
 * @param - enum Sides** array
 * @param - int* size
 * @param - enum Sides element
 * @return - void
 */
void addToEnumSidesarray(enum Sides** array, int* size, enum Sides element) {
    if (*size % 10 == 0) {
        enum Sides* new_array = realloc(*array, (*size + 10) * sizeof(enum Sides));

        if (new_array == NULL) {
            fprintf(stderr, "Memory reallocation error.\n");
            exit(1);
        }
        *array = new_array; // Update the pointer to the new array
    }
    (*array)[*size] = element;
    (*size) += 1;
}

/**
 * @brief - frees the map
 * @param - Map struct
 * @return - void
 */
void freeMap(Map mapOfMaze) {
    free(mapOfMaze.cells);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// MAZE FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief - returns the cell value from the map
 * @param - Map struct
 * @param - int row
 * @param - int col
 * @return - unsigned char, cell value
 * @note - if the cell is outside the map, returns '?'
 */
unsigned char getCell(Map *map, int row, int col) {
    if (row < 1 || row > map->rows || col < 1 || col > map->cols) // check if the cell is inside the map
        return '?';
    int numOfCols = map->cols;
    return map->cells[(row-1)*numOfCols + (col-1)];
}

/**
 * @brief - returns the border value of the cell
 * @param - Map struct
 * @param - int row
 * @param - int col
 * @param - int side
 * @return - bool, 0, 1, border is there or not
 */
bool isBorder(Map *map, int row, int col, int side) {
    int cell = convertCharToInt(getCell(map, row, col));
    return (cell >> side) & 0x01;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// MAZE LOADING AND VALIDATION
///////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief - loads the maze from the file
 * @param - char* filename
 * @return - Map struct
 * @note - exits the program if the file cannot be opened
 */
Map loadMaze(char* filename){

    Map maze;
    FILE* file = fopen(filename, "r");

    if (file == NULL) {
        fprintf(stderr, "Error: cannot open file!\n");
        exit(1);
    }

    // load the maze size
    maze.rows = -1;
    maze.cols = -1;

    if (fscanf(file, "%d %d", &maze.rows, &maze.cols)!= 2) {
        printf("Invalid\n");
        fprintf(stderr, "Error parsing values from the first line.\n");
        fclose(file);
        exit(1);
    }

    if (maze.rows == -1 || maze.cols == -1) {
        printf("Invalid\n");
        fprintf(stderr, "Error: cannot read the maze size!\n");
        fclose(file);
        exit(1);
    }

    // alocate the memmory for the cells, as well as the file line
    maze.cells = malloc((maze.rows * maze.cols) * sizeof(unsigned char)); 

    char ch;
    int counter = 0;
    int rowCounter = 0;

    while (1) {

        ch = fgetc(file);

        if (ch == EOF || counter == maze.rows * maze.cols) 
            break;

        if (rowCounter == maze.cols) { // this may need some changing .. user row Counter I guess
            rowCounter = 0;
            continue;
        }

        if ((ch >= '0' && ch <= '7') && rowCounter < maze.cols) {
            maze.cells[counter++] = ch;
            rowCounter++;
            continue;
        }
    }

    if (counter != maze.rows * maze.cols) {
        printf("Invalid\n");
        fprintf(stderr, "Error: cannot read the maze size!\n");
        fclose(file);
        exit(1);
    }
    
    fclose(file);
    return maze;
}

/**
 * @brief - represents the maze
 * @param - Map struct
 * @return - void
 * @note - prints the maze to the stdout
 */
void representMaze(Map mapOfMaze) {
    for (int i = 0; i < mapOfMaze.rows; i++) {
        for (int j = 0; j < mapOfMaze.cols; j++) {
            printf("%c ", mapOfMaze.cells[i*mapOfMaze.cols + j]);
        }
        printf("\n");
    }
}


/**
 * @brief - checks the maze validity
 * @param - Map struct
 * @return - bool, 0, 1, maze is valid or not
 */
bool checkMazeValidity(Map mapOfMaze) {
    bool cellPointsUp;
    bool mazeValid = true;

    // go from the top left, to the bottom right, first by cols, than by rows
    //check the right cell to this one. In case the cell points up, check the one on the bottom

    for (int i = 1; i <= mapOfMaze.rows; i++) {
        for (int j = 1; j <= mapOfMaze.cols; j++) {
            cellPointsUp = cellPointingUp(i, j);
            if (j != mapOfMaze.cols && (isBorder(&mapOfMaze, i, j, DIAGONAL_RIGHT) != isBorder(&mapOfMaze, i, j+1, DIAGONAL_LEFT))) // check the right border of the cell against the one that is right side from it
                return false;
            if (cellPointsUp && i != mapOfMaze.rows && (isBorder(&mapOfMaze, i, j, STRAIGHT) != isBorder(&mapOfMaze, i+1, j, STRAIGHT)))  // check the border against the cell, that is bellow it
                return false;
        }
    }
    return mazeValid;
}
///////////////////////////////////////////////////////////////////////////////////////////////
// MAZE ENTRY POINT FUNCTIONS - FINDING ENTRY POINTS
///////////////////////////////////////////////////////////////////////////////////////////////


/**
 * @brief - finds the entry points of the maze
 * @param - Map struct
 * @return - EntryPointsArray struct
 */
EntryPointsArray findEntryPoints(Map *map) {

    EntryPointsArray entryArray = {NULL, 0};
    int row = 1; 
    int col = 1;
    
    for (col = 1; col <= map->cols; col++) { // top side
        if (!isBorder(map, row, col, STRAIGHT) && !cellPointingUp(row, col))  
            addEntryPointToArray(&entryArray, (EntryPoint){row, col, STRAIGHT});
    }
    col = map->cols; // right side
    for (row = 1; row <= map->rows; row++) {
        if (!isBorder(map, row, col, DIAGONAL_RIGHT))
            addEntryPointToArray(&entryArray, (EntryPoint){row, col, DIAGONAL_RIGHT});
    }
    row = map->rows; // bottom side
    for (col = 1; col <= map->cols; col++) {
        if (!isBorder(map, row, col, STRAIGHT) && cellPointingUp(row, col))
            addEntryPointToArray(&entryArray, (EntryPoint){row, col, STRAIGHT});
    }
    col = 1; // left side
    for (row = 1; row <= map->rows; row++) { // left side
        if (!isBorder(map, row, col, DIAGONAL_LEFT))
            addEntryPointToArray(&entryArray, (EntryPoint){row, col, DIAGONAL_LEFT});
    }
    return entryArray;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// MAZE SOLVING FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief - returns the entry side of the maze
 * @param - Map struct
 * @param - int row
 * @param - int col
 * @return - enum Sides, entry side
 * @note - exits the program if the point is not an entry point
 */
enum Sides startBorder(Map *map, int row, int col) {

    EntryPointsArray entryPointsArr = findEntryPoints(map);
    enum Sides entrySide;
    for (int i = 0; i < entryPointsArr.entryPointsCount; i++) { // here if i add -1 to the 2nd condition, ivalid read is gone ... ?
        if (entryPointsArr.entryPoints[i].row == row && entryPointsArr.entryPoints[i].col == col) {
            entrySide = entryPointsArr.entryPoints[i].entrySide;
            freeEntryPointsArray(&entryPointsArr);
            return entrySide;
        }
    }
    fprintf(stderr, "error, the point you have selected is not an entry point!\n");
    freeEntryPointsArray(&entryPointsArr);
    freeMap(*map);
    exit(1);
}

/**
 * @brief - chooses the face to move throw
 * @param - Map struct
 * @param - int row
 * @param - int col
 * @param - enum Sides* entrySide
 * @param - int leftRight
 * @return - void
 * @note - changes the entry side to the next face
 */
void chooseFaceToMoveThrow(Map *map, int currentRow, int currentCol, enum Sides *entrySide, int leftRight) {
    bool cellPointsUp = cellPointingUp(currentRow, currentCol);
    int direction;
    if ((cellPointsUp && leftRight == 0) || (!cellPointsUp && leftRight == 1))
        direction = 0;
    else
        direction = 1;

    for (int i = 0; i < 3; i++) { 
        fns(entrySide, direction);
        if (!isBorder(map, currentRow, currentCol, *entrySide)) {
            break;
        }
    }
}

/**
 * @brief - moves the current possition in the maze, based on the entry side
 * @param - int* currentRow
 * @param - int* currentCol
 * @param - enum Sides* entrySide
 * @return - void
 */
void moveDirection(int *currentRow, int *currentCol, enum Sides *entrySide) {
    // left right
    if (*entrySide == DIAGONAL_LEFT) {
        *currentCol -= 1;
        *entrySide = DIAGONAL_RIGHT;
        return;
    }

    if (*entrySide == DIAGONAL_RIGHT) {
        *currentCol += 1;
        *entrySide = DIAGONAL_LEFT;
        return;
    }

    // up down
    if (*entrySide == STRAIGHT && cellPointingUp(*currentRow, *currentCol)) 
        *currentRow += 1;
    else 
        *currentRow -= 1;
}

/**
 * @brief - solves the maze
 * @param - Map struct
 * @param - int leftRight, 0, 1, left or right
 * @param - int row, int col (entry point)
 * @return - void
 * @note - prints the path to the stdout
 */
void solve_maze(Map *mapOfMaze, int leftright, int row, int col) {

    int currentRow = row;
    int currentCol = col;
    enum Sides entryDirection = startBorder(mapOfMaze, row, col);

    printf("%d,%d\n", currentRow, currentCol); // entry point

    while (1) {
        chooseFaceToMoveThrow(mapOfMaze, currentRow, currentCol, &entryDirection, leftright);
        moveDirection(&currentRow, &currentCol, &entryDirection);

        // if we end up outside the map, the maze is solved
        if (currentRow == 0 || currentRow == mapOfMaze->rows+1 || currentCol == 0 || currentCol == mapOfMaze->cols+1) {
            break;
        }
        // if we end up at the starting point, the maze cannot be solved
        if (currentRow == row && currentCol == col) {
            return;
        }
        // stepping close to the exit
        printf("%d,%d\n", currentRow, currentCol);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////
// MAZE SIMPLIFICATION FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief - chooses the next maze point to be solved
 * @param - MazePointsArray struct
 * @return - int, index of the maze point
 * @note - returns -1 if all maze points are finished
 */
int chooseMazePoint(MazePointsArray array) {
    int i = -1;
    for (int j = 0; j < array.mazePointsCount; j++) { // find unfinished maze point
        if (!array.mazePoints[j].finished) {
            i = j;
            break;
        }
    }
    return i;
}

/**
 * @brief - moves to the next decision point
 * @param - Map struct
 * @param - int* Row
 * @param - int* Col
 * @param - MazePoint* currentMazePoint
 * @return - int, 0, 1, -1, 0 - found a decision point, 1 - found a decision point and we are out of the map, -1 - we are back at the starting point
 */
int moveToNextDecisionPoint(Map *map, int *Row, int *Col, MazePoint *currentMazePoint) {
    // move in a direction, untill we find a decision point
    unsigned char cellValue;
    int distance = 0;
    enum Sides *moveThrowFaces = NULL;
    enum Sides currnetDir = currentMazePoint->currentDir;

    while (1) {
        cellValue = getCell(map, *Row, *Col);

        if ((cellValue == '0' || cellValue == '?') && distance != 0) { // in case we found a decision point
            // in case we are out of the map, change the cords back to the map
            *Row = (*Row == 0 ? 1 : *Row);
            *Row = (*Row == map->rows+1 ? map->rows : *Row);
            *Col = (*Col == 0 ? 1 : *Col);
            *Col = (*Col == map->cols+1 ? map->cols : *Col);
            
            currentMazePoint->endPoint.row = *Row;
            currentMazePoint->endPoint.col = *Col;
            currentMazePoint->finished = true;
            currentMazePoint->distance = distance;
            currentMazePoint->moveThrowFaces = moveThrowFaces;
            currentMazePoint->currentDir = currnetDir;
            if (cellValue == '0')
                return 1;
            return 0;
        }
        addToEnumSidesarray(&moveThrowFaces, &distance, currnetDir);
        moveDirection(Row, Col, &currnetDir);
        if (*Row == currentMazePoint->startPoint.row && *Col == currentMazePoint->startPoint.col) { 
            free(moveThrowFaces);
            distance = 0;
            return -1;
        }
        chooseFaceToMoveThrow(map, *Row, *Col, &currnetDir, 1); 
    }
}

/**
 * @brief - simplifies the maze
 * @param - Map struct
 * @param - int startRow
 * @param - int startCol
 * @return - MazePointsArray struct
 * @note - returns all the "decision points" in the maze (points where the path splits)
 */
MazePointsArray simplifyMaze(Map *map, int startRow, int startCol) { 
    
    MazePointsArray mazePointsArr = {NULL, 0};
    enum Sides startDirection = startBorder(map, startRow, startCol);
    chooseFaceToMoveThrow(map, startRow, startCol, &startDirection, 1);
    MazePoint cMazeP = {{startRow, startCol}, {startRow, startCol}, startDirection, 0, NULL, false};
    addMazePointToArray(&mazePointsArr, cMazeP);

    enum Sides entryDirection;
    unsigned char cellValue;
    int Row, Col;
    int skipOne;
    int mazePointIdx;

    while (1) {
        mazePointIdx = chooseMazePoint(mazePointsArr);
        if (mazePointIdx == -1)
            break;

        // move to the next decision point
        Row = mazePointsArr.mazePoints[mazePointIdx].startPoint.row;
        Col = mazePointsArr.mazePoints[mazePointIdx].startPoint.col;
        skipOne = moveToNextDecisionPoint(map, &Row, &Col, &mazePointsArr.mazePoints[mazePointIdx]);
        cellValue = getCell(map, Row, Col);
        // in case we are not out of the map, we will add two new mazepoints to the queue, which are connected to the current mazepoint
        if (cellValue != '?' && skipOne == 1) {
            entryDirection = mazePointsArr.mazePoints[mazePointIdx].currentDir;

            for (int j = 0; j < 2; j++) {
                // delcare the faces to go throw, for new maze point
                Row = mazePointsArr.mazePoints[mazePointIdx].endPoint.row;
                Col = mazePointsArr.mazePoints[mazePointIdx].endPoint.col;
                if (j == 1)
                    chooseFaceToMoveThrow(map, Row, Col, &entryDirection, 1);
                MazePoint newMazePoint1 = {
                    mazePointsArr.mazePoints[mazePointIdx].endPoint, 
                    {-1, -1}, entryDirection, 1, NULL, false
                    };
                addMazePointToArray(&mazePointsArr, newMazePoint1);
            }
        } else if (skipOne == -1) {
            removeMazePoint(&mazePointsArr, mazePointIdx);
        }
    }
    return mazePointsArr;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// A* algorithem
///////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief - finds the conection points of the current maze point
 * @param - Point startPoint
 * @param - MazePointsArray struct
 * @param - int resultArray[2], array to store the indexes
 * @return - void
 */
void findConectionPoints(Point startPoint, MazePointsArray* mazePointsArray, int resultArray[2]) {
    MazePoint comprasonPoint;
    int counter = 0;

    for (int i = 0; i < mazePointsArray->mazePointsCount; i++) {
        comprasonPoint = mazePointsArray->mazePoints[i];
        if (comprasonPoint.startPoint.row == startPoint.row && comprasonPoint.startPoint.col == startPoint.col) {
            resultArray[counter++] = i;
        }
        if (counter == 2)
            break;
    }
    for (int i = counter; i < 2; i++) {
        resultArray[i] = -1;
    }
}   

/**
 * @brief - adds the conection points to the priority queue
 * @param - int conectionPoints[2], array of indexes of the conection points
 * @param - int* priorityQueueSize, size of the priority queue
 * @param - int** priorityQueue, pointer to the priority queue
 * @return - void
 */
void addConectionPoints(int conectionPoints[2], int *priorityQueueSize, int **priorityQueue) {
    int conectionIdx;
    for (int i = 0; i < 2; i++) {
        if (conectionPoints[i] == -1)
            break;
        conectionIdx = conectionPoints[i];
        addToIntArray(priorityQueue, priorityQueueSize, conectionIdx);
    }
}

/**
 * @brief - runs the a* algorithem between one start and end point
 * @param - Point startPoint
 * @param - Point endPoint
 * @param - MazePointsArray struct
 * @param - int* pathLength, pointer to the path length
 * @param - ResultPathArray struct
 * @return - void
 */
void runAstar(Point startPoint, Point endPoint, MazePointsArray *mazePointsArray, int *pathLength, ResultPathArray *resultPathArray) {
    int *priorityQueue = NULL;
    int priorityQueueSize = 0;

    int *visited = NULL;
    int visitedSize = 0;

    int connectionPoints[2];
    findConectionPoints(startPoint, mazePointsArray, connectionPoints);
    addConectionPoints(connectionPoints, &priorityQueueSize, &priorityQueue);
    MazePoint currentMazePoint;

    double minDistance, distance;
    int currentIdx;

    while (1) {
        // in this case, we have gone throw all the connected paths, and none ware the exit point
        if (priorityQueueSize == 0) { 
            free(priorityQueue);
            free(visited); 
            resultPathArray->pathIdxes = NULL;
            resultPathArray->pathIdxesCount = -1;
            return;
        }
        // find the maze point with the shortest distance to the end point
        currentIdx = 0;
        currentMazePoint = mazePointsArray->mazePoints[priorityQueue[0]];
        distance = calculateEuklidianDistance(
            currentMazePoint.endPoint.row, 
            currentMazePoint.endPoint.col, 
            endPoint.row, 
            endPoint.col
            );
        minDistance = currentMazePoint.distance + distance;

        for (int i = 1; i < priorityQueueSize; i++) {
            currentMazePoint = mazePointsArray->mazePoints[priorityQueue[i]];
            distance = currentMazePoint.distance + calculateEuklidianDistance(
                currentMazePoint.endPoint.row, 
                currentMazePoint.endPoint.col, 
                endPoint.row, 
                endPoint.col
                );
            if (distance < minDistance) {
                minDistance = distance;
                currentIdx = i;
            }
        }
        // remove current mazepoint form the priority queue, as well as add the points, this point is connected to
        currentMazePoint = mazePointsArray->mazePoints[priorityQueue[currentIdx]];
        addToIntArray(&visited, &visitedSize, priorityQueue[currentIdx]);
        findConectionPoints((Point){
            currentMazePoint.endPoint.row, 
            currentMazePoint.endPoint.col
            }, mazePointsArray, connectionPoints);

        addConectionPoints(connectionPoints, &priorityQueueSize, &priorityQueue);
        removeIntAtIndexFromArray(priorityQueue, &priorityQueueSize, currentIdx);
        // we have found the wanted end point
        if (currentMazePoint.endPoint.row == endPoint.row && currentMazePoint.endPoint.col == endPoint.col) 
            break;
    }
    MazePoint pivot = mazePointsArray->mazePoints[visited[visitedSize-1]];
    (*pathLength) = pivot.distance;
    MazePoint cMazeP;

    // reconstruct the path, going from the end point to the start point, finding the next connecting point
    int *resultPath = NULL;
    int counter = 0;

    addToIntArray(&resultPath, &counter, visited[visitedSize-1]);

    while (1) {
        if (pivot.startPoint.row == startPoint.row && pivot.startPoint.col == startPoint.col) 
            break;
        int i;
        for (i = visitedSize-1; i >= 0; i--) { 
            cMazeP = mazePointsArray->mazePoints[visited[i]];
            if (cMazeP.endPoint.row == pivot.startPoint.row && cMazeP.endPoint.col == pivot.startPoint.col) {
                pivot = cMazeP;
                (*pathLength) += cMazeP.distance;
                break;
            }
        }
       addToIntArray(&resultPath, &counter, visited[i]);
    }
    free(visited);
    free(priorityQueue);

    resultPathArray->pathIdxes = resultPath;
    resultPathArray->pathIdxesCount = counter;
    return;
}

/**
 * @brief - finds the shortest path in the maze
 * @param - MazePointsArray struct
 * @param - Map struct
 * @param - Point startPoint
 * @return - ResultPathArray struct
 * @note - uses the a* algorithem to find the shortest path
 */
ResultPathArray findshortesPath(MazePointsArray *mazePointsArray, Map *map, Point startPoint) {

    EntryPointsArray entryPointsArray = findEntryPoints(map);
    EntryPoint currentPoint;
    ResultPathArray currentShortestPath;
    ResultPathArray shortestPath = {NULL, -1};

    int mindistance = 0;
    int currentDistance;

    // go throw all the entry point, exit point combination, and run the a* algorithem
    for (int i = 0; i < entryPointsArray.entryPointsCount; i++) {
        currentPoint = entryPointsArray.entryPoints[i];

        if (currentPoint.row == startPoint.row && currentPoint.col == startPoint.col) // skip start == end ..
            continue;

        Point endPoint = {currentPoint.row, currentPoint.col};
        currentDistance = 0;
        runAstar(startPoint, endPoint, mazePointsArray, &currentDistance, &currentShortestPath);

        if (currentShortestPath.pathIdxesCount == -1 || currentShortestPath.pathIdxes == NULL) // no result form a*
            continue;

        if (currentDistance < mindistance || mindistance == 0) { //in case the path is shorter than the previous one, save it
            mindistance = currentDistance;
            shortestPath.pathIdxes = currentShortestPath.pathIdxes;
            shortestPath.pathIdxesCount = currentShortestPath.pathIdxesCount;
        } else {
            free(currentShortestPath.pathIdxes);
        }
   }
   freeEntryPointsArray(&entryPointsArray);
   return shortestPath;
}

/**
 * @brief - reconstructs the path form the shortest path array, after the a* algorithem
 * @param - ResultPathArray struct
 * @param - MazePointsArray struct
 * @param - Point startPoint
 * @return - void
 */
void reconstructPath(ResultPathArray *shortestPath, MazePointsArray *mazePointArray, Point StartPoint) {

    int currentRow = StartPoint.row;
    int currentCol = StartPoint.col;
    int endRow, endCol;
    enum Sides currentDir;

    printf("%d,%d\n", currentRow, currentCol);
    for (int i = shortestPath->pathIdxesCount-1; i >= 0; i--) {
        endRow = mazePointArray->mazePoints[shortestPath->pathIdxes[i]].endPoint.row;
        endCol = mazePointArray->mazePoints[shortestPath->pathIdxes[i]].endPoint.col;
        for (int j = 0; j < mazePointArray->mazePoints[shortestPath->pathIdxes[i]].distance; j++) {
            if (currentRow == endRow && currentCol == endCol) 
                break;
            currentDir = mazePointArray->mazePoints[shortestPath->pathIdxes[i]].moveThrowFaces[j];
            moveDirection(&currentRow, &currentCol, &currentDir);
            printf("%d,%d\n", currentRow, currentCol);
        }
    }   
}

/**
 * @brief - help function
 * @param - char* argv[]
 * @return - void
 */
void help(char *argv[]) {
    printf("Usage: %s [options]\n", argv[0]);
    printf("Options:\n");
    printf("  --help: Display this help message\n");
    printf("  --test file.txt: Run a test with the specified maze file\n");
    printf("  --rpath R C file.txt: Find a right path in the maze\n");
    printf("  --lpath R C file.txt: Find a left path in the maze\n");
    printf("  --shortest R C file.txt: Find the shortest path in the maze\n");
}


/**
 * @brief - test the maze file
 * @param - char file_name[]
 * @return - void
 */
void test_file(char file_name[]) {
    Map mapOfMaze = loadMaze(file_name);
    //representMaze(mapOfMaze);
    bool mazeValid = checkMazeValidity(mapOfMaze);
    if (!mazeValid) {
        printf("Invalid\n");
        return;
    }
    EntryPointsArray entryPointsArray = findEntryPoints(&mapOfMaze);
    if (entryPointsArray.entryPointsCount == 0) {
        printf("Invalid\n");
        return;
    }
    freeEntryPointsArray(&entryPointsArray);
    freeMap(mapOfMaze);
    printf("Valid\n");
}


/**
 * @brief - runs the line follow algorithem (lpath, rpath)
 * @param - int R (row)
 * @param - int C (colunm)
 * @param - char file_name[]
 */
void run_line_follow(int R, int C, char file_name[], int left_right) {
    Map mapOfMaze = loadMaze(file_name);
    solve_maze(&mapOfMaze, left_right, R, C);
    freeMap(mapOfMaze);
}


/**
 * @brief - finds the shortest path in the maze
 * @param - int R (row)
 * @param - int C (colunm)
 * @param - char file_name[]
 * @return - void
 */
void find_shortest_path(int R, int C, char file_name[]) {
    Map mapOfMaze = loadMaze(file_name);
    MazePointsArray mazePoinsArray = simplifyMaze(&mapOfMaze, R, C);
    if (mazePoinsArray.mazePointsCount == 1) {
        printf("%d,%d\n", R, C);
        freeMazePointsArray(&mazePoinsArray);
        freeMap(mapOfMaze);
        return;
    }
    ResultPathArray shortesPath = findshortesPath(&mazePoinsArray, &mapOfMaze, (Point){R, C});
    if (shortesPath.pathIdxesCount == -1) {
        freeMazePointsArray(&mazePoinsArray);
        freeMap(mapOfMaze);
        return;
    }
    reconstructPath(&shortesPath, &mazePoinsArray, (Point){R, C});
    free(shortesPath.pathIdxes);
    freeMazePointsArray(&mazePoinsArray);
    freeMap(mapOfMaze);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// MAIN FUNCTION
///////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief - main function
 */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Error: Insufficient arguments. Use './maze --help' for usage information.\n");
        return 1;
    }
    if (strcmp(argv[1], "--help") == 0) 
        help(argv);
    else if (strcmp(argv[1], "--test") == 0 && argc == 3) 
        test_file(argv[2]);
    else if (strcmp(argv[1], "--rpath") == 0 && argc == 5) 
       run_line_follow(atoi(argv[2]), atoi(argv[3]), argv[4], 0);
    else if (strcmp(argv[1], "--lpath") == 0 && argc == 5) 
        run_line_follow(atoi(argv[2]), atoi(argv[3]), argv[4], 1);
    else if (strcmp(argv[1], "--shortest") == 0 && argc == 5) 
        find_shortest_path(atoi(argv[2]), atoi(argv[3]), argv[4]);
    else {
        printf("Error: Invalid command-line arguments. Use './maze --help' for usage information.\n");
        return 1;
    }
    return 0;
}
