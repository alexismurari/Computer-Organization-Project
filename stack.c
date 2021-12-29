#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

//Title Screen array 'Stacky' moved at the end for clarity

// Function prototypes
void draw_line(short int x0, short int x1, short int y0, short int y1, short int color);
void clear_screen();
void plot_pixel(int x, int y, short int line_color);
void swap(short int* a, short int* b);
void vsync();
void printBlock(int i, int j, int k);
void printTower(int currentState[4][10][10]);
void updateState(int currentState[4][10][10], int nextState[10][10]);
int spacePressed();

volatile int pixel_buffer_start; // global variable
volatile int *sBit = (int *)0xFF20302C;
volatile int *pixel_ctrl_ptr;

volatile int *timer = (int *)0xFF202000;
volatile int *keys = (int *)0xFF20005C;

int main(void){
    bool gameOver = false;
    bool reset = true;
    pixel_ctrl_ptr = (int *)0xFF203020;
	
    /* set front pixel buffer to start of FPGA On-chip memory */
    *(pixel_ctrl_ptr + 1) = 0xC8000000; // first store the address in the
                                    // back buffer

    /* now, swap the front/back buffers, to set the front buffer location */
    vsync();

    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *pixel_ctrl_ptr;
    clear_screen(); // pixel_buffer_start points to the pixel buffer

    /* set back pixel buffer to start of SDRAM memory */
    *(pixel_ctrl_ptr + 1) = 0xC0000000;
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer

    int currentTower[4][10][10];
    int edgeCap;
    bool buttonPressed = false;
    bool leftRight;
    int speed, xPos, xSize, xStart, yPos, ySize, yStart;//, topXStart, topXSize, topYStart, topYSize;
    int score = 0;
	int letter = 128;
	char* text_ptr; 
  
    /*************************************************
        movement info:
        x = left / right == k
        y = forward / backward == j
        Starting position: (-10,0), moving right
        speed = 1 -> right or forward (into screen)
        speed = -1 -> left or backward (out of screen)
    **************************************************/

    // Game loop
    while(1){
        if(gameOver){
            edgeCap = *keys;
            *keys = 0xF;
            if(edgeCap > 0){
                reset = true;
                gameOver= false;
				score = 0;
            }
        }

        while(!gameOver){
			char blankText[] = " ";
			text_ptr = blankText;
			letter = 0;
			while(letter < 100){
             		*(char *)(0xC9000000 + letter + 258) = *(text_ptr);
             		letter++;
           	}
			char textScore[] = "SCORE: ";
			char buf[10];
			letter = 0;
			sprintf(buf, "%s %02d", textScore, score);
			text_ptr = buf;
            while(*(text_ptr)){
             	*(char *)(0xC9000000 + letter + 130) = *(text_ptr);
             	letter++;
             	text_ptr++;
            }
            
            if(reset){
                score = 0;
                *keys = 0xF;
                
                // Initialize & fill up the intire tower
                for(int height=0; height<4; height++){
                    for (int row = 0; row < 10; row++){
                        for (int col=0; col < 10; col++){
                            currentTower[height][row][col] = 1;
                        } 
                    } 
                }

                // If false, going front & back. It true, going left & right.
                leftRight = false;

                // speed = 1 -> right or forward
                // speed = -1 -> left or backward
                speed = 1;
                xStart = 0;
                xSize = 10;
                yStart = 0;
                ySize = 10;
                
                buttonPressed = false;
                reset = false;
            }
			// Since we're not game over, add 1 to score
            score++;
        
            clear_screen();
            
            *keys = 0xF;
            buttonPressed = false;
            leftRight = !leftRight;

            //Initialize position
            if(leftRight){
                xPos = xStart-10;
                yPos = yStart;
            }
            else{
                xPos = xStart;
                yPos = yStart-10;
            }


            // Loop until the button is pressed
            while(!buttonPressed){

                //Clear the screen then print the current tower
                clear_screen();
                printTower(currentTower);
                
                //Check if there was a key press
                edgeCap = *keys;
                *keys = 0xF; 
                if(edgeCap == 0){                            
                    //Check what direction the block is going
                    //Then update position and then draw the block
                    if(leftRight){
                        //Update X position then print block
                        xPos += speed;
                        if((xPos + xSize) == 23 || xPos == -13){
                            speed = -speed;
                        }
                        
                        for (int x = xPos; x < (xPos+xSize); x++){
                            for(int y = yPos; y < yPos + ySize; y++){
                                if(true){
                                    printBlock(4, y , x);
                                }
                            }
                        } 
                    }

                    else {
                        //Update Y location then print block
                        yPos += speed;
                        if((yPos + ySize) == 23 || yPos == -13){
                            speed = -speed;
                        }
                        
                        for (int x = xPos; x < (xPos+xSize); x++){
                            for(int y = yPos; y < (yPos+ySize); y++){
                                if(true){
                                    printBlock(4, y , x);
                                }
                            }
                        } 
                    }
                    vsync(); // swap front and back buffers on VGA vertical sync
                    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
                }

                else if(edgeCap >= 1 && edgeCap < 8){ //Check if keys 0~2 have been pressed (key 3 is reset)
                    buttonPressed = true;
                    //Initialize overlap array
                    int overlap[10][10];

                    int currentTile[10][10] = {{0,0,0,0,0,0,0,0,0,0},
                                            {0,0,0,0,0,0,0,0,0,0},
                                            {0,0,0,0,0,0,0,0,0,0},
                                            {0,0,0,0,0,0,0,0,0,0},
                                            {0,0,0,0,0,0,0,0,0,0},
                                            {0,0,0,0,0,0,0,0,0,0},
                                            {0,0,0,0,0,0,0,0,0,0},
                                            {0,0,0,0,0,0,0,0,0,0},
                                            {0,0,0,0,0,0,0,0,0,0},
                                            {0,0,0,0,0,0,0,0,0,0}};

                    
                    // Get current tile position
                    for(int y = yPos; y < yPos + ySize; y++){
                        for(int x = xPos; x < xPos + xSize; x++){
                            if( (x >= 0) && (x < 10) && (y >= 0) && (y < 10) ){
                                    currentTile[y][x] = 1; 
                            }
                        }
                    }

                    //Update overlap
                    for(int y = 0; y < 10; y++){
                        for(int x = 0; x < 10; x++){
                            overlap[y][x] = currentTower[3][y][x] & currentTile[y][x];
                        }
                    }
                    
                    //Check if there were any overlaps
                    //At first overlap, update yStart & xStart
                    bool stillThere = false;
                    for(int y = 0; y < 10; y++){
                        for(int x = 0; x < 10; x++){
                            if(overlap[y][x] == 1 && !stillThere){
                                stillThere = true;
                                yStart = y;
                                xStart = x;
                            }
                        }
                    }
                    
                    gameOver = !stillThere;
					if(gameOver){
						char overText[] = "GAME OVER - PRESS ANY KEY TO RESET";
						text_ptr = overText;
						letter = 0;
						while(*(text_ptr)){
             				*(char *)(0xC9000000 + letter + 258) = *(text_ptr);
             				letter++;
             				text_ptr++;
           				}
					}

                    //If there is overlap, update the new tile & the tower
                    if(stillThere){

                        // Update xSize
                        xSize = 0;
                        for(int x = xStart; x < 10; x++){
                            if(overlap[yStart][x] == 1){
                                xSize++;
                            }
                        }

                        // Update ySize
                        ySize = 0;
                        for(int y = yStart; y < 10; y++){
                            if(overlap[y][xStart] == 1){
                                ySize++;
                            }
                        }

                        //then finally update the tower!
                        updateState(currentTower, overlap);
                    }
                    

                } else if(edgeCap == 8){
                    buttonPressed = true;
                    reset = true;
                }
            }
        }
    }
}


void printTower(int currentState[4][10][10]){
    for(int z = 0; z < 4; z++){
        for (int y = 0; y < 10; y++){
            for (int x = 0; x < 10; x++){
                if(currentState[z][y][x] == 1){
                    printBlock(z, y , x);
                }
            } 
        } 
    }
}


// Bottom left front of tower in the beginning: (90,200)
// Bottom right front of tower " " ": (200,200)
// Bottom right back  " " " " ": (230,150)
// Size on top 4 corners from bottom left clockwise = (0,0) -> (3,5) -> (13,5) -> (10,0)
void printBlock(int i, int j, int k){
    int x = 90 + (j*3 + k*10);
    int y = 200 - (j*5 + i*20);

    draw_line(x, y, x, y+20, 0xFFFF - i*0x1111); // left vertical line
    draw_line(x+10, y, x+10, y+20, 0xFFFF - i*0x1111); // middle vertical
    draw_line(x+13, y-5, x+13, y+15, 0xFFFF - i*0x1111); // right vertical
    draw_line(x, y+20, x+10, y+20, 0xFFFF - i*0x1111); // bottom horizontal line
    draw_line(x, y, x+10, y, 0xFFFF - i*0x1111); // middle horizontal
    draw_line(x+3, y-5, x+13, y-5, 0xFFFF - i*0x1111); // top horizontal
    draw_line(x, y, x+3, y-5, 0xFFFF - i*0x1111); // left diagonal
    draw_line(x+10, y, x+13, y-5, 0xFFFF - i*0x1111); // middle diagonal
    draw_line(x+10, y+20, x+13, y+15, 0xFFFF - i*0x1111); // right diagonal

    //call fill the block
}

// When we place a block, update the tower
void updateState(int currentState[4][10][10], int nextState[10][10]){
	for(int z=0; z<4; z++){
        if(z<3){
            for (int y = 0; y < 10; y++){
                for (int x = 0; x < 10; x++){
            	    currentState[z][y][x] = currentState[z+1][y][x];
                } 
            } 
        }else{
            for (int y = 0; y < 10; y++){
                for (int x = 0; x < 10; x++){
            	    currentState[z][y][x] = nextState[y][x];
                } 
            }
        }
    }
}


// Function that plots a line.
void draw_line(short int x0, short int y0, short int x1, short int y1, short int color){
    int y_step;

    bool is_vertical = abs(y1 - y0) > abs(x1-x0);

    if(is_vertical){
        swap(&x0, &y0);
        swap(&x1, &y1);
    }

    if(x0 > x1){
        swap(&x0, &x1);
        swap(&y0, &y1);
    }

    int deltax = x1 - x0;
    int deltay = abs(y1 - y0);
    int error = -(deltax / 2);
    int y = y0;

    if(y0 < y1) { y_step = 1; } else { y_step = -1; }

    for(int x = x0; x <= x1; x++){
        if(is_vertical) { plot_pixel(y, x, color);}
        else{ plot_pixel(x, y, color);}

        error += deltay;

        if(error >= 0){
            y += y_step;
            error -= deltax;
        }
    }
}


// Clear de screen!
void clear_screen(){
    int x, y;
    int black = 0x0;

    for(x = 0; x < 320; x++) {
        for (y = 0; y < 240; y++){
            plot_pixel(x, y, black);
        }
    }
}


// But some color in a pixel
void plot_pixel(int x, int y, short int line_color){
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}


// Function to swap two integers, a and b
void swap(short int* a, short int* b){
    int temp = *a;
    *a = *b;
    *b = temp;
}


void vsync(){
    *pixel_ctrl_ptr = 1;

    register int status = *(pixel_ctrl_ptr+3);
    while(((*sBit) & 0x1) != 0){
        status = *(pixel_ctrl_ptr+3);
    }
}
	
