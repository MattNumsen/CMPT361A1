/*
CMPT 361 Assignment 1 - FruitTetris implementation Sample Skeleton Code

- This is ONLY a skeleton code showing:
How to use multiple buffers to store different objects
An efficient scheme to represent the grids and blocks

- Compile and Run:
Type make in terminal, then type ./FruitTetris

This code is extracted from Connor MacLeod's (crmacleo@sfu.ca) assignment submission
by Rui Ma (ruim@sfu.ca) on 2014-03-04. 

Modified in Sep 2014 by Honghua Li (honghual@sfu.ca).
*/

#include "include/Angel.h"
#include <cstdlib>
#include <iostream>

using namespace std;


// xsize and ysize represent the window size - updated if window is reshaped to prevent stretching of the game
int xsize = 400; 
int ysize = 720;
int shapeToUse;
int orientation; //globally track which peice is being used, and which orientation it is in
int timerVal = 500;

// current tile
vec2 tile[4]; // An array of 4 2d vectors representing displacement from a 'center' piece of the tile, on the grid
bool tileIsNull[4]; //An array tracking whether or not
vec2 tilepos = vec2(5, 19); // The position of the current tile using grid coordinates ((0,0) is the bottom left corner)

// An array storing all possible orientations of all possible tiles
// The 'tile' array will always be some element [i][j] of this array (an array of vec2)

vec2 allShapesAllRotations[6][4][4] =
	{	//Lshape
		{{vec2(-1,-1), vec2(-1, 0), vec2(0,0), vec2(1, 0)},
		{vec2(1, -1), vec2(0, -1), vec2(0,0), vec2(0,1)},     
		{vec2(1, 1), vec2(1,0), vec2(0, 0), vec2(-1,0)},  
		{vec2(-1,1), vec2(0, 1), vec2(0, 0), vec2(0,-1)}},
		//LshapeRightHanded
		{{vec2(1,-1), vec2(1, 0), vec2(0,0), vec2(-1, 0)},
		{vec2(-1, -1), vec2(0, -1), vec2(0,0), vec2(0,1)},     
		{vec2(-1, 1), vec2(-1,0), vec2(0, 0), vec2(1,0)},  
		{vec2(1,1), vec2(0, 1), vec2(0, 0), vec2(0,-1)}},
		//The S shapes make a slight difference to their rotation pattern, in that if an S shape, without moving down, 
		//is rotated twice, it will occupy the SAME space it did previously, instead of being translated up by one unit (and then back down in 2 more rotations)
		//Sshape
		{{vec2(-1, -1), vec2(0,-1), vec2(0, 0), vec2(1,0)},
		{vec2(1, -1), vec2(1, 0), vec2(0,0), vec2(0,1)},     
		{vec2(1,0), vec2(0,0), vec2(0,-1), vec2(-1, -1)},
		{vec2(0,1), vec2(0,0), vec2(1, 0), vec2(1, -1)}},
		//SshapeRightHanded
		{{vec2(1, -1), vec2(0,-1), vec2(0, 0), vec2(-1,0)},
		{vec2(-1, -1), vec2(-1, 0), vec2(0,0), vec2(0,1)},     
		{vec2(-1,0), vec2(0,0), vec2(0,-1), vec2(1, -1)},
		{vec2(0,1), vec2(0,0), vec2(-1, 0), vec2(-1, -1)}},
		//Ishape
		{{vec2(0, 1), vec2(0,0), vec2(0, -1), vec2(0,-2)},
		{vec2(-2, 0), vec2(-1, 0), vec2(0,0), vec2(1, 0)},     
		{vec2(0, -2), vec2(0,-1), vec2(0, 0), vec2(0, 1)},
		{vec2(1, 0), vec2(0, 0), vec2(-1,0), vec2(-2, 0)}},

		//allRotationsTshape
		{{vec2(-1, 0), vec2(0,0), vec2(1, 0), vec2(0,-1)},
		{vec2(0, -1), vec2(0, 0), vec2(0,1), vec2(1, 0)},     
		{vec2(1, 0), vec2(0,0), vec2(-1, 0), vec2(0,1)},
		{vec2(0, 1), vec2(0, 0), vec2(0,-1), vec2(-1, 0)}}};


vec4 allColors[5] = 
	{vec4(1.0, 0.5, 0.0, 1.0),
	vec4(1.0, 0.0, 0.0, 1.0),
	vec4(0.0, 1.0, 0.0, 1.0),
	vec4(0.73, 0.16, 0.96, 1.0),
	vec4(1.0, 1.0, 0.0, 1.0)};

vec4 orange = vec4(1.0, 0.5, 0.0, 1.0); 
vec4 red 	= vec4(1.0, 0.0, 0.0, 1.0);
vec4 green 	= vec4(0.0, 1.0, 0.0, 1.0);
vec4 purple	= vec4(0.73, 0.16, 0.96, 1.0); //values taken from https://www.opengl.org/discussion_boards/showthread.php/132502-Color-tables
vec4 yellow = vec4(1.0, 1.0, 0.0, 1.0);
vec4 white  = vec4(1.0, 1.0, 1.0, 1.0);
vec4 black  = vec4(0.0, 0.0, 0.0, 1.0); 


 
//board[x][y] represents whether the cell (x,y) is occupied
bool board[10][20]; 
bool markedForDeletion[10][20];
//An array containing the colour of each of the 10*20*2*3 vertices that make up the board
//Initially, all will be set to black. As tiles are placed, sets of 6 vertices (2 triangles; 1 square)
//will be set to the appropriate colour in this array before updating the corresponding VBO
vec4 boardcolours[1200];

// location of vertex attributes in the shader program
GLuint vPosition;
GLuint vColor;

// locations of uniform variables in shader program
GLuint locxsize;
GLuint locysize;

// VAO and VBO
GLuint vaoIDs[3]; // One VAO for each object: the grid, the board, the current piece
GLuint vboIDs[6]; // Two Vertex Buffer Objects for each VAO (specifying vertex positions and colours, respectively)

//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------

// When the current tile is moved or rotated (or created), update the VBO containing its vertex position data
void updatetile()
{
	// Bind the VBO containing current tile vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[4]); 

	// For each of the 4 'cells' of the tile,
	for (int i = 0; i < 4; i++) 
	{
		// Calculate the grid coordinates of the cell
		GLfloat x = tilepos.x + tile[i].x; 
		GLfloat y = tilepos.y + tile[i].y;

		// Create the 4 corners of the square - these vertices are using location in pixels
		// These vertices are later converted by the vertex shader
		vec4 p1 = vec4(33.0 + (x * 33.0), 33.0 + (y * 33.0), .4, 1); 
		vec4 p2 = vec4(33.0 + (x * 33.0), 66.0 + (y * 33.0), .4, 1);
		vec4 p3 = vec4(66.0 + (x * 33.0), 33.0 + (y * 33.0), .4, 1);
		vec4 p4 = vec4(66.0 + (x * 33.0), 66.0 + (y * 33.0), .4, 1);

		// Two points are used by two triangles each
		vec4 newpoints[6] = {p1, p2, p3, p2, p3, p4}; 

		// Put new data in the VBO
		glBufferSubData(GL_ARRAY_BUFFER, i*6*sizeof(vec4), 6*sizeof(vec4), newpoints); 
	}

	glBindVertexArray(0);
}

//-------------------------------------------------------------------------------------------------------------------

// Called at the start of play and every time a tile is placed
void newtile()
{
	// Update the geometry VBO of current tile
	shapeToUse = rand()%6; //random number between 0 and 5 to pick a shape
	orientation = rand()%4; //random number ... to pick the orientation
	int position = rand()%8 + 1; //random number ... to pick the horizontal spawn position -- pivotpoint shouln't be at x=0
	int ypos = 18;
	bool canMove  = true;


	if (shapeToUse == 4){ //make sure it isn't off the board because of one specific orientation 
		if ((orientation == 1) || (orientation == 3)){
			if (position == 1){
				position++;
			} else if (position == 8) {
				position--;
			}
		} 
	}
	//check if it is in the bounds
	//check if it is colliding with anything
	int xPosition;
	int yPosition;

	for (int i = 0; i<4; i++){//check if the space BELOW each square of our tile available (SOME CHECKS MAY BE REDUNDANT)
		xPosition = position + (int)allShapesAllRotations[shapeToUse][orientation][i].x;
		yPosition = ypos + (int)allShapesAllRotations[shapeToUse][orientation][i].y;
		if ((board[xPosition][yPosition]) && (yPosition < 20)) {//True if the space is occupied
			canMove = false;
			exit(EXIT_SUCCESS);
			glutPostRedisplay();
		} 
	}
	if (canMove) {
		tilepos = vec2(position , ypos); // Put the tile at the top of the board
		for (int i = 0; i < 4; i++)
			tile[i] = allShapesAllRotations[shapeToUse][orientation][i];
		updatetile(); 
	} 
	// Update the color VBO of current tile
	vec4 newcolours[24];
	int color = rand()%5;
	for (int i = 0; i < 24; i+=6) {
		color = rand()%5;
		newcolours[i] = allColors[color];
		newcolours[i+1] = allColors[color];
		newcolours[i+2] = allColors[color];
		newcolours[i+3] = allColors[color];
		newcolours[i+4] = allColors[color];
		newcolours[i+5] = allColors[color];
	}


	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[5]); // Bind the VBO containing current tile vertex colours
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(newcolours), newcolours); // Put the colour data in the VBO
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);
}

//-------------------------------------------------------------------------------------------------------------------

void initGrid()
{
	// ***Generate geometry data
	vec4 gridpoints[64]; // Array containing the 64 points of the 32 total lines to be later put in the VBO
	vec4 gridcolours[64]; // One colour per vertex
	// Vertical lines 
	for (int i = 0; i < 11; i++){
		gridpoints[2*i] = vec4((33.0 + (33.0 * i)), 33.0, 0, 1);
		gridpoints[2*i + 1] = vec4((33.0 + (33.0 * i)), 693.0, 0, 1);
		
	}
	// Horizontal lines
	for (int i = 0; i < 21; i++){
		gridpoints[22 + 2*i] = vec4(33.0, (33.0 + (33.0 * i)), 0, 1);
		gridpoints[22 + 2*i + 1] = vec4(363.0, (33.0 + (33.0 * i)), 0, 1);
	}
	// Make all grid lines white
	for (int i = 0; i < 64; i++)
		gridcolours[i] = white;


	// *** set up buffer objects
	// Set up first VAO (representing grid lines)
	glBindVertexArray(vaoIDs[0]); // Bind the first VAO
	glGenBuffers(2, vboIDs); // Create two Vertex Buffer Objects for this VAO (positions, colours)

	// Grid vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[0]); // Bind the first grid VBO (vertex positions)
	glBufferData(GL_ARRAY_BUFFER, 64*sizeof(vec4), gridpoints, GL_STATIC_DRAW); // Put the grid points in the VBO
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0); 
	glEnableVertexAttribArray(vPosition); // Enable the attribute
	
	// Grid vertex colours
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[1]); // Bind the second grid VBO (vertex colours)
	glBufferData(GL_ARRAY_BUFFER, 64*sizeof(vec4), gridcolours, GL_STATIC_DRAW); // Put the grid colours in the VBO
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vColor); // Enable the attribute
}


void initBoard()
{
	// *** Generate the geometric data
	vec4 boardpoints[1200];
	for (int i = 0; i < 1200; i++)
		boardcolours[i] = black; // Let the empty cells on the board be black
	// Each cell is a square (2 triangles with 6 vertices)
	for (int i = 0; i < 20; i++){
		for (int j = 0; j < 10; j++)
		{		
			vec4 p1 = vec4(33.0 + (j * 33.0), 33.0 + (i * 33.0), .5, 1);
			vec4 p2 = vec4(33.0 + (j * 33.0), 66.0 + (i * 33.0), .5, 1);
			vec4 p3 = vec4(66.0 + (j * 33.0), 33.0 + (i * 33.0), .5, 1);
			vec4 p4 = vec4(66.0 + (j * 33.0), 66.0 + (i * 33.0), .5, 1);
			
			// Two points are reused
			boardpoints[6*(10*i + j)    ] = p1;
			boardpoints[6*(10*i + j) + 1] = p2;
			boardpoints[6*(10*i + j) + 2] = p3;
			boardpoints[6*(10*i + j) + 3] = p2;
			boardpoints[6*(10*i + j) + 4] = p3;
			boardpoints[6*(10*i + j) + 5] = p4;
		}
	}

	// Initially no cell is occupied
	for (int i = 0; i < 10; i++)
		for (int j = 0; j < 20; j++)
			board[i][j] = false; 


	// *** set up buffer objects
	glBindVertexArray(vaoIDs[1]);
	glGenBuffers(2, &vboIDs[2]);

	// Grid cell vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[2]);
	glBufferData(GL_ARRAY_BUFFER, 1200*sizeof(vec4), boardpoints, GL_STATIC_DRAW);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);

	// Grid cell vertex colours
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[3]);
	glBufferData(GL_ARRAY_BUFFER, 1200*sizeof(vec4), boardcolours, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vColor);
}

// No geometry for current tile initially
void initCurrentTile()
{
	glBindVertexArray(vaoIDs[2]);
	glGenBuffers(2, &vboIDs[4]);

	// Current tile vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[4]);
	glBufferData(GL_ARRAY_BUFFER, 24*sizeof(vec4), NULL, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);

	// Current tile vertex colours
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[5]);
	glBufferData(GL_ARRAY_BUFFER, 24*sizeof(vec4), NULL, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vColor);
}

void init()
{
	// Load shaders and use the shader program
	GLuint program = InitShader("vshader.glsl", "fshader.glsl");
	glUseProgram(program);

	// Get the location of the attributes (for glVertexAttribPointer() calls)
	vPosition = glGetAttribLocation(program, "vPosition");
	vColor = glGetAttribLocation(program, "vColor");

	// Create 3 Vertex Array Objects, each representing one 'object'. Store the names in array vaoIDs
	glGenVertexArrays(3, &vaoIDs[0]);

	// Initialize the grid, the board, and the current tile
	initGrid();
	initBoard();
	initCurrentTile();

	// The location of the uniform variables in the shader program
	locxsize = glGetUniformLocation(program, "xsize"); 
	locysize = glGetUniformLocation(program, "ysize");

	// Game initialization
	newtile(); // create new next tile

	// set to default
	glBindVertexArray(0);
	glClearColor(0, 0, 0, 0);
}

//-------------------------------------------------------------------------------------------------------------------

// Rotates the current tile, if there is room
void rotate() //global variables shapetouse and orientation indicate which shape is being used, and which orientation it is in
{     
	int originalOrientation = orientation;

	if (++orientation > 3) //increment the orientation. If it is larger than 3, then we go back to 0.
		orientation=0;

	int xPosition, yPosition;
	int left = 10;
	int right= -1;
	bool overlap = false;

	for (int i = 0; i < 4; i++){ //check the bounds on the potential NEW orientation
		xPosition = (int)tilepos.x + (int)allShapesAllRotations[shapeToUse][orientation][i].x;
		yPosition = (int)tilepos.y + (int)allShapesAllRotations[shapeToUse][orientation][i].y;

		if(board[xPosition][yPosition]) //if ANY of the pieces overlap, then overlap is true and we will NOT MOVE
			overlap = true;

		left = min(xPosition, left);
		right = max(xPosition, right);

	}
	if (left < 0){ //the direction was left, and it would take us out of the grid
		orientation = originalOrientation;
		return;
	}
	if (right > 9){ //the direction was right and it would take us out of the grid
		orientation = originalOrientation;
		return;
	}

	if (!overlap){ //if there is NO overlap
		for (int i = 0; i < 4; i++){
			tile[i] = allShapesAllRotations[shapeToUse][orientation][i];
		}
		updatetile();
		return;
	}
	orientation = originalOrientation;
	return;
}

//-------------------------------------------------------------------------------------------------------------------

// Checks if the specified row (0 is the bottom 19 the top) is full
// If every cell in the row is occupied, it will clear that cell and everything above it will shift down one row
void checkfullrow(int row)
{
	bool rowFull=true;
	for (int i = 0; i < 10; i++){
		if (!board[i][row]){
			rowFull=false;
			i=10;
		}
	}
	if (rowFull){
		for(int i=row; i<19; i++){
			for(int j=0; j<10; j++){
				board[j][i]=board[j][i+1];
				for (int k = 0; k < 6; k++){
					boardcolours[60*i + 6*j + k]=boardcolours[60*i + 60 + 6*j + k];
				}
			}
		}
		for(int j = 0; j< 10; j++){
			board[j][19]=false;
			for (int k = 0; k < 6; k++){
					boardcolours[60*19 + 6*j + k] = black; 
				}
		}
		checkfullrow(row); //in the event that two consecutive rows are formed, the previous mechanism for calling checkfullrow will miss this row
	}
}

//-------------------------------------------------------------------------------------------------------------------
// Reads the "MarkedForDeletion" array, and deletes all that are marked, then shifts tiles down appropriately
void removeMarked() 
{
	//start from the TOP, delete a block at a time, move those above it down, keep going.
	for (int row = 19; row > -1; row--){
		for (int col = 0; col < 10; col++){
			if (markedForDeletion[col][row]){
				for(int i=row; i<19; i++){
					board[col][i]=board[col][i+1];
					for (int k = 0; k < 6; k++){
						boardcolours[60*i + 6*col + k]=boardcolours[60*i + 60 + 6*col + k];	
					}
				}
				markedForDeletion[col][row] = false;
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------

/*
	Technique: Iterate through the board for each color. For each block of that color, check it's neighbor.
	If the neighbor is the same color, walk and keep track of the length/direction you've walked. 
	If the length reaches or exceeds three, mark those tiles for deletion. 
	Then, proceed to check the other directions. 
	Shapes such as the following will NOT be deleted
	|*| | |     	|*| | | 
	|*| | |   		|*|*| | 
	| |*|*|   		| | | | 

	//-------------------------------------------------------------------------------------------------------------------
	
	In the following shape, ALL blocks will be deleted:
	|*| |*|    	
	|*|*| |  		
	|*|*|*| 
	NOTE: The bottom left * is used as part of three triples, which is acceptable by this deisgn.

	//-------------------------------------------------------------------------------------------------------------------

	Execution on the following shape would yield the results shown
	|*| |*|    		| | | |     		
	|*|*| |  	= 	| | | |    			
	|*|*| | 		| |*| |    		
	NOTE: The bottom middle * is close to two triples, which share the bottom left *, however it is not part of its own triplet


*/

bool compareCol(vec4 col1, vec4 col2)
{
	return ((col1.x == col2.x ) && (col1.y == col2.y) && (col1.z == col2.z));
}


void checkforthree()
{
	vec4 currentColor;
	int length =0;
	bool anythingFound = false;
	for (int color = 0; color < 5; color++){ //check the board entirely for each color
		currentColor = allColors[color];	
		for (int row = 0; row < 20; row++){
			for (int col = 0; col < 10; col++){
				if (compareCol(boardcolours[row*60 + col*6],currentColor)){

					/*-----Look to the Right-----*/
					length=0;					
					while (compareCol(boardcolours[row*60 + (col + length)*6],currentColor)) {			
						length++;
					}

					if (length >= 3) { //this could probably be done better, but I couldn't figure something out
						for (int i = 0; i < length; i++){
							markedForDeletion[col + i][row] = true;
						}
						anythingFound = true;						
					} 
					

					/*-----Look Above-----*/
					length=0;					
					while (compareCol(boardcolours[(row + length)*60 + col*6],currentColor)) {			
						length++;
					}

					if (length >= 3) { //this could probably be done better, but I couldn't figure something out
						for (int i = 0; i < length; i++){
							markedForDeletion[col][row + i] = true;
						}
						anythingFound = true;						
					}

					/*-----Look Diagonally (up and right)----*/
					length=0;					
					while (compareCol(boardcolours[(row + length)*60 + (col + length)*6],currentColor)) {			
						length++;
					}

					if (length >= 3) { //this could probably be done better, but I couldn't figure something out
						for (int i = 0; i < length; i++){
							markedForDeletion[col + i][row + i] = true;
						}
						anythingFound = true;						
					}


					/*-----Look Diagonally (up and left)----*/
					length=0;					
					while (compareCol(boardcolours[(row + length)*60 + (col - length)*6],currentColor)) {			
						length++;
					}

					if (length >= 3) { //this could probably be done better, but I couldn't figure something out
						for (int i = 0; i < length; i++){
							markedForDeletion[col - i][row + i] = true;
						}
						anythingFound = true;						
					}
				}
			}
		}
	}
	if(anythingFound){
		removeMarked();
		checkforthree();
	}
}

//-------------------------------------------------------------------------------------------------------------------
// Places the current tile - update the board vertex colour VBO and the array maintaining occupied cells
void settile()
{
		int xPosition, yPosition, start, tracker, end;
		vec4 tilecolours[24];
		timerVal = 500;
		glBindBuffer(GL_ARRAY_BUFFER, vboIDs[5]);
		glGetBufferSubData(GL_ARRAY_BUFFER,0,24*sizeof(vec4),tilecolours);
		for (int i = 0; i<4; i++){
			xPosition = (int)tilepos.x + (int)tile[i].x;
			yPosition = (int)tilepos.y + (int)tile[i].y;
			board[xPosition][yPosition] = true;
			start = yPosition*60 + xPosition*6;
			tracker = 0;
			while (tracker < 6){
				boardcolours[start + tracker] = tilecolours[6*i + tracker];
				tracker++;
			}
		}
		start = min(19, (int)tilepos.y + 2); 	//Start at the highest potentially affected row (give or take), would likely work as +1
		end = max((int)tilepos.y - 3, -1);	
		
		for (int i = start; i > end; i--){
			checkfullrow(i);
		}
		checkforthree();
		glBindBuffer(GL_ARRAY_BUFFER, vboIDs[3]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, 1200*sizeof(vec4), boardcolours); 
}

//-------------------------------------------------------------------------------------------------------------------

// Given (x,y), tries to move the tile x squares to the right and y squares down
// Returns true if the tile was successfully moved, or false if there was some issue
bool movetile(vec2 direction)
{
	//concept: Check boundaries, then write changes, then update tile
	//check boundaries by finding the NEXT step most minimum/maximum x/y values
	int left=10, right=-1; //boundary trackers
	int xPosition, yPosition;
	bool overlap = false;
	for (int i = 0; i < 4; i++){
		//deterime if there is overlap
		xPosition = (int)tilepos.x + (int)tile[i].x + (int)direction.x;
		yPosition = (int)tilepos.y + (int)tile[i].y + (int)direction.y;

		if(board[xPosition][yPosition]) //if ANY of the pieces overlap, then overlap is true and we will NOT MOVE
			overlap = true;

		left = min(xPosition, left);
		right = max(xPosition, right);

	}
	if (left < 0){ //the direction was left, and it would take us out of the grid
		return false;
	}
	if (right > 9){ //the direction was right and it would take us out of the grid
		return false;
	}
	if (!overlap){ //if there is NO overlap
		tilepos.x += direction.x;
		updatetile();
		return true;
	}
	//at this point, we are within the bounds of the gridbox. Check if we overlap with any existing tiles!

	return false;
}
//-------------------------------------------------------------------------------------------------------------------

// Starts the game over - empties the board, creates new tiles, resets line counters
void restart()
{
	init();
}
//-------------------------------------------------------------------------------------------------------------------

// Draws the game
void display()
{

	glClear(GL_COLOR_BUFFER_BIT);

	glUniform1i(locxsize, xsize); // x and y sizes are passed to the shader program to maintain shape of the vertices on screen
	glUniform1i(locysize, ysize);

	glBindVertexArray(vaoIDs[1]); // Bind the VAO representing the grid cells (to be drawn first)
	glDrawArrays(GL_TRIANGLES, 0, 1200); // Draw the board (10*20*2 = 400 triangles)

	glBindVertexArray(vaoIDs[2]); // Bind the VAO representing the current tile (to be drawn on top of the board)
	glDrawArrays(GL_TRIANGLES, 0, 24); // Draw the current tile (8 triangles)

	glBindVertexArray(vaoIDs[0]); // Bind the VAO representing the grid lines (to be drawn on top of everything else)
	glDrawArrays(GL_LINES, 0, 64); // Draw the grid lines (21+11 = 32 lines)


	glutSwapBuffers();
}

//-------------------------------------------------------------------------------------------------------------------

// Reshape callback will simply change xsize and ysize variables, which are passed to the vertex shader
// to keep the game the same from stretching if the window is stretched
void reshape(GLsizei w, GLsizei h)
{
	xsize = w;
	ysize = h;
	glViewport(0, 0, w, h);
}

//-------------------------------------------------------------------------------------------------------------------

// Handle arrow key keypresses
void special(int key, int x, int y)
{
	vec2 left = vec2(-1,0);
	vec2 right = vec2(1,0);
	switch (key) {
		case  GLUT_KEY_UP 		:
			rotate();
			break;

		case  GLUT_KEY_DOWN		:
			timerVal = 100;
			break;

		case  GLUT_KEY_LEFT		:
			movetile(left);
			break;

		case  GLUT_KEY_RIGHT	:
			movetile(right);
			break;

		default					:
			break;
	}

}
//-------------------------------------------------------------------------------------------------------------------

// Handles standard keypresses
void keyboard(unsigned char key, int x, int y)
{
	switch(key) 
	{
		case 033: // Both escape key and 'q' cause the game to exit
		    exit(EXIT_SUCCESS);
		    break;
		case 'q':
			exit (EXIT_SUCCESS);
			break;
		case 'r': // 'r' key restarts the game
			restart();
			break;
	}
	glutPostRedisplay();
}

//-------------------------------------------------------------------------------------------------------------------

void idle(void)
{
	glutPostRedisplay();
}

void update(int value)
{
	bool canMove = true;
	bool writeToBoard = false;
	int lowpointOffset = 0;

	int xPosition;
	int yPosition;

	for (int i = 0; i<4; i++){//check if the space BELOW each square of our tile available (SOME CHECKS MAY BE REDUNDANT)
		xPosition = (int)tilepos.x + (int)tile[i].x;
		yPosition = (int)tilepos.y + (int)tile[i].y;
		if (board[xPosition][yPosition-1]) {//True if the space is occupied
			canMove=false;
		}
	}
	if (canMove){
		tilepos.y--;
		updatetile();
		for (int i = 0; i<4; i++){
			lowpointOffset = min((int)tile[i].y, lowpointOffset); 
		}
		if (tilepos.y == abs(lowpointOffset)) {
			writeToBoard = true;	
		}
	} else {
		writeToBoard = true;
	}
	if (writeToBoard){
		settile();
		newtile();
	}
	glutTimerFunc(timerVal, update, 0);
	glutPostRedisplay();
}

//-------------------------------------------------------------------------------------------------------------------

int main(int argc, char **argv)
{
	srand (time(NULL));
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowSize(xsize, ysize);
	glutInitWindowPosition(680, 178); // Center the game window (well, on a 1920x1080 display)
	glutCreateWindow("Fruit Tetris");
	glewInit();
	init();

	// Callback functions
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutSpecialFunc(special); //consider also needing to use glutSpecialUpFunc
	glutKeyboardFunc(keyboard);
	glutTimerFunc(timerVal, update, 0);
	glutIdleFunc(idle);

	glutMainLoop(); // Start main loop
	return 0;
}
