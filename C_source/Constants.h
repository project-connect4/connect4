#ifndef CONSTANTS
#define CONSTANTS

#include <stdio.h>
#include <stdlib.h>
#include "platform.h"
#include <xgpio.h>
#include "xparameters.h"
#include "sleep.h"
#include "xtime_l.h"



#define kBOARDS_COLS        7
#define kBOARDS_ROWS        6
#define kXFIRSTCENTER       67.5
#define kYFIRSTCENTER       87.5
#define kBOARDTHICKNESS     15
#define kLENGHTSPACE        45

#define kERROR_FACTOR	    20

#define kCPU_EASY_MAX_DEPTH  4
#define kCPU_HARD_MAX_DEPTH  7

#define kPLAYER_1       '*'
#define kPLAYER_2       'o'
#define kCPU_HARD       '$'
#define kCPU_EASY       '%'
#define kEMPTY          '_'

#define WHITE           0b111
#define BLACK           0b000
#define RED             0b001
#define GREEN           0b010
#define BLUE            0b100
#define CYAN            0b110
#define MAGENTA         0b101
#define YELLOW          0b011

#define kPLAYER_1_COL   CYAN
#define kPLAYER_2_COL   YELLOW
#define kCPU_HARD_COL   RED
#define kCPU_EASY_COL   GREEN

#define kLEN_TO_WIN     4

#define kFIRST_TURN     rand()%2

#define kPOINTERBRAM    (uint32_t*)0x40000000

#define NOINPUT			0b0000
#define BUTTON_0        0b0001
#define BUTTON_1        0b0010
#define BUTTON_2        0b0100
#define BUTTON_3        0b1000



#define sign(theArg) ((theArg > 0) - (theArg < 0))

#define DEBOUNCE while(XGpio_DiscreteRead(&input, 1) != 0) usleep(10000)

#define canInsertInColumnAtIndex(_index,_boardPT) (!(_index < 0 || _index >= kBOARDS_COLS || _boardPT->matrix[0][_index] != kEMPTY))

#define colorForPlayer(_player) (_player == kPLAYER_1 ? kPLAYER_1_COL : (_player == kPLAYER_2 ? kPLAYER_2_COL : (_player == kCPU_HARD ? kCPU_HARD_COL : (_player == kCPU_EASY ? kCPU_EASY_COL : BLACK))))

#define xForColumn(_indx) ((uint16_t)(kXFIRSTCENTER +(_indx)*(kLENGHTSPACE + kBOARDTHICKNESS)))

#define yForRow(_indx) ((uint16_t)(kYFIRSTCENTER +(_indx)*(kLENGHTSPACE + kBOARDTHICKNESS)))



typedef enum {false, true} BOOL;

typedef struct {
    uint16_t x,y;
} Point;

typedef enum {
    AnimationTypeLin,
    AnimationTypeLog,
    AnimationTypeArctan,
    AnimationTypeCosh,
    AnimationTypeQuad,
    AnimationTypeSin,
    AnimationTypeGravity
} AnimationType;

typedef enum {
    GameModePlayerVsCPUHard,
    GameModePlayerVsCPUEasy,
    GameModePlayerVsPlayer,
    GameModeDemo,
    GameModeInvalid
} GameMode;

typedef struct {
    uint32_t timeOfCPU1, timeOfCPU2; // in sec
    uint32_t victoriesCPU1, victoriesCPU2, ties;
} Statistics;

typedef struct {
    char matrix[kBOARDS_ROWS][kBOARDS_COLS];
    int emptyCells;
    int n_balls;
    uint8_t winningCells[2][4];
} Board;

#endif
