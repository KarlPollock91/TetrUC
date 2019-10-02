//Written by Karl Pollock
//October 2019
//KarlPollock91@gmail.com

#include <avr/io.h>
#include <stdbool.h>
#include <stdlib.h>
#include "system.h"
#include "pio.h"
#include "navswitch.h"


typedef struct point {
    uint8_t x;
    uint8_t y;
} Point;

typedef struct tetronimo_type_s {
    Point blocks[4][4];
    uint8_t num_rotations;
} Tetromino;

typedef struct cursor_s {
    uint8_t x;
    uint8_t y;
    Tetromino tetromino;
    uint8_t rotation;
} Cursor;


//Something to do, switch these around so that the more horizontal orientations
//are at the start of the array.


static Tetromino L_block = {{{{0, 0}, {0,1}, {0,2}, {1,0}},
                        {{0, 0}, {0, -1}, {1, 0}, {2, 0}},
                        {{0, 0}, {-1, 0}, {0, -1}, {0, -2}},
                        {{0, 0}, {-1, 0}, {-2, 0}, {0, 1}}}, 4};

static Tetromino T_block = {{{{0, 0}, {-1, 0}, {1, 0}, {0, 1}},
                        {{0, 0}, {1, 0}, {0, 1}, {0, -1}},
                        {{0, 0}, {-1, 0}, {1, 0}, {0, -1}},
                        {{0, 0}, {-1, 0}, {0, 1}, {0, -1}}}, 4};

static Tetromino square_block = {{{{0, 0}, {-1, 0}, {0, 1}, {-1, 1}}}, 1};

static Tetromino I_block = {{{{0, 0}, {0, 1}, {0, 2}, {0, -1}},
                        {{0, 0}, {-1, 0}, {1, 0}, {2, 0}}}, 2};

static Tetromino J_block = {{{{0, 0}, {-1, 0}, {0, 1}, {0, 2}},
                        {{0, 0}, {0, 1}, {1, 0}, {2, 0}},
                        {{0, 0}, {1, 0}, {0, -1}, {0, -2}},
                        {{0, 0}, {-1, 0}, {-2, 0}, {0, -1}}}, 4};

static pio_t ledmat_rows[] =
{
    LEDMAT_ROW1_PIO, LEDMAT_ROW2_PIO, LEDMAT_ROW3_PIO, LEDMAT_ROW4_PIO,
    LEDMAT_ROW5_PIO, LEDMAT_ROW6_PIO, LEDMAT_ROW7_PIO
};

static pio_t ledmat_cols[] =
{
    LEDMAT_COL1_PIO, LEDMAT_COL2_PIO, LEDMAT_COL3_PIO,
    LEDMAT_COL4_PIO, LEDMAT_COL5_PIO
};


//LED intialisation.
void led_matrix_init(void)
{
    for (int i = 0; i < 7; i++) {
        pio_config_set(ledmat_rows[i], PIO_OUTPUT_HIGH);
    }
    for (int i = 0; i < 5; i++) {
        pio_config_set(ledmat_cols[i], PIO_OUTPUT_HIGH);
    }
}

//Initialise the board as empty.
void initialise_board(bool board[7][5])
{
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 5; j++) {
            board[i][j] = false;
        }
    }
}

//Clear turn all rows and columns off.
void clear_screen(void)
{
    for (int i = 0; i < 7; i++) {
        pio_output_high(ledmat_rows[i]);
    }
    for (int i = 0; i < 5; i++) {
        pio_output_high(ledmat_cols[i]);
    }
}

//Checks to see if the tetrominos have reached the top and the game should end
bool is_game_over(bool board[7][5])
{
    bool game_over = false;
    for (int i = 0; i < 4; i++) {
        if (board[0][i]) {
            game_over = true;
        }
    }
    return game_over;
}

//Check to see if the space below the current block is clear. Return true if so.
bool can_drop(Cursor* cursor, bool board[7][5])
{
    uint8_t x;
    uint8_t y;

    bool success = true;
    for (int i = 0; i < 4; i++) {
        x = (cursor->tetromino.blocks[cursor->rotation][i].x + cursor->x);
        y = (cursor->tetromino.blocks[cursor->rotation][i].y + cursor->y);
        if (y == 6) {
            success = false;
        } else if (board[y+1][x]) {
            success = false;
        }
    }
    return success;
}

//Activate the column to draw on
void activate_column(uint8_t state)
{
    pio_output_low(ledmat_cols[state]);
}

//Draw the currently falling block.
void draw_cursor(Cursor* cursor, uint8_t state)
{
    uint8_t x;
    uint8_t y;

    //Sweep across the columns, left to right, drawing the blocks

    for (int i = 0; i < 4; i++) {
        x = (cursor->tetromino.blocks[cursor->rotation][i].x + cursor->x);
        y = (cursor->tetromino.blocks[cursor->rotation][i].y + cursor->y);
        if (x == state) {
            pio_output_low(ledmat_rows[y]);
        }
    }

}

//Draw the current board.
void draw_board(bool board[7][5], uint8_t state)
{
    for (int i = 0; i < 7; i++) {
        if (board[i][state]) {
            pio_output_low(ledmat_rows[i]);
        }
    }

}

//Resets the timer to zero.
void reset_tcnt1(void)
{
    TCNT1 = 0;
}

//Sets the cursor to be a new tetromino and tries to place it at the top of the screen.
void generate_new_tetromino(Cursor* cursor)
{
    Tetromino possible_tetrominos[5] = {T_block, I_block, square_block, L_block, J_block};
    uint8_t random_number = rand() % 5;

    cursor->x = 2;
    cursor->y = 0;
    cursor->tetromino = possible_tetrominos[random_number];
    cursor->rotation = 1;

    /*TODO: Once we've implimented a push left/right function we can decide whether or not
     * we can place the block and cause a gameover if not.
     */

    reset_tcnt1();
}

//Sets the cursor on the board, used when it's hit the bottom.
void set_tetromino(Cursor* cursor, bool board[7][5])
{
    uint8_t x;
    uint8_t y;

    for (int i = 0; i < 4; i++) {
        x = (cursor->tetromino.blocks[cursor->rotation][i].x + cursor->x);
        y = (cursor->tetromino.blocks[cursor->rotation][i].y + cursor->y);
        if (x < 5 && y < 7) {
            board[y][x] = 1;
        }
    }
}

 /* your equivalent of a print statement
    pio_output_low(ledmat_cols[2]);
    pio_output_low(ledmat_rows[2]);
    *
    */

//Note to self! Don't forget it's board[y][x], not the other way around!

int main (void)
{
    //Gameplay variables
    bool board[7][5];
    Cursor cursor = {};

    //Initialisation stuff
    system_init ();
    led_matrix_init();
    srand(TCNT1);

    initialise_board(board);
    navswitch_init();
    TCCR1A = 0x00;
    TCCR1B = 0x05;
    TCCR1C = 0x00;

    //Initialise cursor
    generate_new_tetromino(&cursor);

    //Variables for controlling the timed actions.
    uint16_t mark = 10000;
    bool toggle_ready = 0;
    bool ready_to_drop = 0;
    bool game_over = 0;

    //Variable for deciding which column to draw each loop
    uint8_t draw_state = 0;

    //Start game timer
    reset_tcnt1();

    while (!game_over)
    {
        //Draw elements
        clear_screen();
        draw_state = (draw_state + 1) % 5;
        activate_column(draw_state);
        draw_cursor(&cursor, draw_state);
        draw_board(board, draw_state);

        //Check to see if it's time to drop a row
        if (ready_to_drop) {
            //Check to see if it's clear below tetromino
            if (can_drop(&cursor, board)) {
                cursor.y += 1;
            } else {
                //Generate new tetromino and check for game over.
                set_tetromino(&cursor, board);
                generate_new_tetromino(&cursor);
                game_over = is_game_over(board);
                reset_tcnt1();
            }
            ready_to_drop = 0;
        }

        //Controls when to drop the tetromino a row.
        if ((TCNT1 % mark) > (mark / 2) && toggle_ready) {
            ready_to_drop = 1;
            toggle_ready = 0;
        } else if ((TCNT1 % mark) < (mark / 2)) {
            toggle_ready = 1;
        }


    }
    //Makeshift gameover to ensure game has ended
    if (game_over){
        clear_screen();
        pio_output_low(ledmat_cols[2]);
        pio_output_low(ledmat_rows[2]);
    }

}


