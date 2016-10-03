#include "AI.h"
#include "Constants.h"
#include "Drawer.h"
#include "math.h"

////////////////////////////////////////////////

static XGpio input, output;
static uint32_t *balls = kPOINTERBRAM;
static uint32_t *grid_on = kPOINTERBRAM;
static int maxDemoMatches;

///////////////////// INTERFACE /////////////////////

void init_gpio();
Board* newBoard();
Board* freeBoard(Board *board);
Statistics init_stats();

char winningPlayer(Board *board);
int newGame(Board *board, GameMode gameMode, Statistics *stats);

void animateLEDs();
void gameOverAnimation(uint8_t m[2][4], int winner, BOOL isDemo, int matchNumber);
void waitTilReset();
void displayStatistics(Statistics stats);

uint64_t randFromClock();
GameMode askForGameMode();

void demoWelcomeScreen();

/////////////////////////////////////////////////////




/////////////////////// MAIN ////////////////////////

int main() {
    
    init_gpio();
    init_platform();
    Statistics stats = init_stats();
    BOOL isDemo = false;
    int numberOfDemoMatches = 0;
    maxDemoMatches = 10;

    // Initial animation.
    animateLEDs();
    
    while (1) {

        DEBOUNCE;
        
        // Removes everything from the screen.
        clearBram();
        
        // Allocates and inits a new board.
        Board *board = newBoard();
        
        GameMode mode = GameModeInvalid;

        if (isDemo) {
            mode = GameModeDemo;
        } else {
            mode = askForGameMode();
            if (mode == GameModeDemo) {
                isDemo = true;
                numberOfDemoMatches = 0;
                demoWelcomeScreen();
            }
        }

        // Takes care of the current match and returns the winner.
        int winner = newGame(board, mode, &stats);

        // Checks if there's a winner, else a reset has been requested
        if (winner != 0) {
            
            // Animates the board and highlights the winning cells.
            gameOverAnimation(board->winningCells, winner, isDemo, ++numberOfDemoMatches);

            // Different behavior in case of demo mode
            if (isDemo) { 

                if (winner == kCPU_HARD) {
                    stats.victoriesCPU1 += 1;
                } else if (winner == kCPU_EASY) {
                    stats.victoriesCPU2 += 1;
                } else {
                    stats.ties += 1;
                }

                int numOfMatches = stats.victoriesCPU1 + stats.victoriesCPU2 + stats.ties;

                // If we've reached the last demo match, we stop and display the results
                if (numOfMatches >= maxDemoMatches) {

                	displayStatistics(stats);

                	stats = init_stats();
                    isDemo = false;
                    numberOfDemoMatches = 0;
                }

            } else {

                // Asks the user to press a button to continue
                waitTilReset();
            }
        }
        
        // Deallocates the board and returns NULL
        board = freeBoard(board);
    }
    
    cleanup_platform();
    return 0;
}

/////////////////////////////////////////////////////




////////////// MEM. MANAGEMENT & INITS //////////////

void init_gpio() {
    XGpio_Initialize(&input, XPAR_AXI_GPIO_0_DEVICE_ID);
    XGpio_Initialize(&output, XPAR_AXI_GPIO_1_DEVICE_ID);
    XGpio_SetDataDirection(&input, 1, 0xF);
    XGpio_SetDataDirection(&input, 2, 0xF);
    XGpio_SetDataDirection(&output, 1, 0x0);
}

Board* newBoard() {
    
    Board *board = (Board *) malloc(sizeof(Board));
    if (board == NULL) return NULL;
    
    board->n_balls = 0;
    board->emptyCells = kBOARDS_COLS * kBOARDS_ROWS;
    
    int i,j;
    
    for (i=0;i<kBOARDS_ROWS;++i) {
        for(j=0;j<kBOARDS_COLS;++j) {
            board->matrix[i][j] = kEMPTY;
        }
    }
    
    for(i=0;i<2;++i){
        for(j=0;j<4;++j){
            board->winningCells[i][j] = 0;
        }
    }
    
    return board;
}

Board* freeBoard(Board *board) {
    if (board != NULL) free(board);
    return NULL;
}

Statistics init_stats() {
	Statistics stats;
	stats.timeOfCPU1 = 0;
	stats.timeOfCPU2 = 0;
	stats.victoriesCPU1 = 0;
	stats.victoriesCPU2 = 0;
	stats.ties = 0;
	return stats;
}

/////////////////////////////////////////////////////




////////////////// ANIMATIONS CTRL //////////////////

Point makePoint(uint16_t x,uint16_t y) {
    Point pt; pt.x = x; pt.y = y;
    return pt;
}

Point makePointOnGrid(int theCol, int theRow) {
    Point pt; pt.x = xForColumn(theCol); pt.y = yForRow(theRow);
    return pt;
}

float factorForAnimation(AnimationType animType, float progress) {
    if (animType == AnimationTypeLin) {
        return progress;
    } else if (animType == AnimationTypeLog) {
        return sign(progress) *log(fabs(progress)*9 +1);
    } else if (animType == AnimationTypeArctan) {
        return atan(progress*57) *0.6366197724;
    } else if (animType == AnimationTypeCosh) {
        return (cosh(progress*3)-1)/0.543;
    } else if (animType == AnimationTypeQuad) {
        return sign(progress) *progress*progress;
    } else if (animType == AnimationTypeSin) {
        return sin(progress*1.5707963268);
    } else if (animType == AnimationTypeGravity) {
        return sign(progress) *4.9 *pow(0.45175*progress,2);
    }
    return 0;
}

void async_animateShape(uint8_t color, uint8_t shape, uint32_t *pp, Point from, Point to, AnimationType animType, float progress) {
    
    float factor = factorForAnimation(animType,progress);
    int dx = (to.x - from.x) *factor;
    int dy = (to.y - from.y) *factor;
    
    *pp = encodeShape(from.x+dx,from.y+dy,color,shape);
}

float durationOfFall(Point from, Point to) {

	double metersPerPixel = 0.0005703125;

	double dy = abs(to.y -from.y) *metersPerPixel;

	return (float)pow((2*dy)/9.8, 0.5);
}

void sync_animateShape(uint8_t color, uint8_t shape, uint32_t *pp, Point from, Point to, AnimationType animType, float duration_s) {
    
	if (animType == AnimationTypeGravity) duration_s = durationOfFall(from,to);

    uint32_t nframes = duration_s * 60;
    int i;
    for (i = 0; i <= nframes; ++i) {
        async_animateShape(color,shape,pp,from,to,animType,(float)i/nframes);
        if (i<nframes) usleep(1/60.0 *1000000);
    }
}

/////////////////////////////////////////////////////




///////////////////// GAME CTRL /////////////////////

BOOL insertInColumnAtIndex(int indx, char player, Board *board, uint16_t ypos) {
    if (indx < 0|| indx >= kBOARDS_COLS || board->matrix[0][indx] != kEMPTY) {
        return false;
    }
    int i;
    for (i = kBOARDS_ROWS -1; i>=0; --i) {
        if (board->matrix[i][indx] == kEMPTY) {
            
            uint8_t color = BLACK;

			switch (player) {
				case kPLAYER_1:
					color = kPLAYER_1_COL;
					break;
				case kPLAYER_2:
					color = kPLAYER_2_COL;
					break;
				case kCPU_HARD:
					color = kCPU_HARD_COL;
					break;
				case kCPU_EASY:
					color = kCPU_EASY_COL;
					break;
			}

            sync_animateShape(color, kBALL_SHAPE, &balls[board->n_balls], makePoint(xForColumn(indx),ypos), makePointOnGrid(indx, i), AnimationTypeGravity, 0.1835);

            board->matrix[i][indx] = player;
            --(board->emptyCells);
            return true;
        }
    }
    return false;
}

char winningPlayer(Board *board) {
	int winner=kEMPTY;
    int offset[2][8] = {{0, 0, 1, 1, 1,-1,-1,-1},
                        {1,-1, 1, 0,-1, 1, 0,-1}};
    int i,j,k,s;

    // For each row
    for (i=0; i<kBOARDS_ROWS; ++i) {

        // For each column
        for (j=0; j<kBOARDS_COLS; ++j) {

            // The player in the current position
            char player = board->matrix[i][j];
            if (player == kEMPTY) continue;

        	board->winningCells[0][0]=(uint8_t)j;
        	board->winningCells[1][0]=(uint8_t)i;

            // For each possible direction
            for (k=0; k<8; ++k) {
                int i_off = offset[0][k];
                int j_off = offset[1][k];

                BOOL validWin = true;

                // For each step in the same direction
                for (s=1; s<kLEN_TO_WIN; ++s) {

                    int i_cur = i +i_off*s;
                    int j_cur = j +j_off*s;

                	board->winningCells[0][s]=(uint8_t)j_cur;
                	board->winningCells[1][s]=(uint8_t)i_cur;

                    // Invalid position. Useless to continue.
                    if (i_cur < 0 || j_cur < 0 || i_cur >= kBOARDS_ROWS || j_cur >= kBOARDS_COLS) {
                        validWin = false;
                        break;
                    }
                    // No win. Break.
                    if (board->matrix[i_cur][j_cur] != player) {
                        validWin = false;
                        break;
                    }
                }

                if (validWin) {
                	winner=player;
                	return winner;
                }
            }

        }
    }

    return winner;
}

int newGame(Board *board, GameMode gameMode, Statistics *stats) {
    
    DEBOUNCE;
    clearBram();

    char players[2];
    BOOL isDemo = false;

    switch(gameMode) {
        case GameModePlayerVsPlayer:
            players[0]=kPLAYER_1;
            players[1]=kPLAYER_2;
            break;
        case GameModePlayerVsCPUHard:
            players[0]=kPLAYER_1;
            players[1]=kCPU_HARD;
            break;
        case GameModePlayerVsCPUEasy:
            players[0]=kPLAYER_1;
            players[1]=kCPU_EASY;
            break;
        case GameModeDemo:
            players[0]=kCPU_HARD;
            players[1]=kCPU_EASY;
            isDemo = true;
            break;
        default:
            return 0;
    }


    uint32_t *pp = kPOINTERBRAM;

    int turn = randFromClock()%2;
    
    grid_on[0] = 1;
    

    while (1) {

        DEBOUNCE;
        
        // Check if it's a tie
        if (board->emptyCells <= 0) {
            return kEMPTY;
        }
        
        // PUSH ONE BALL
        ++(board->n_balls);
        
        uint8_t choice = 0;
        uint16_t currBallY = yForRow(-1);

        uint8_t color = colorForPlayer(players[turn]);

        drawLabel(560, 200,"* speaks", color, &pp[43]);
        
        if (players[turn] == kCPU_HARD) {

            XTime tStart, tEnd;

            XTime_GetTime(&tStart);
            
            choice = CPUsChoice(board, players, turn, kCPU_HARD_MAX_DEPTH, isDemo);

            XTime_GetTime(&tEnd);

            uint32_t tTot = (tEnd - tStart)*10000 /COUNTS_PER_SECOND;

            stats->timeOfCPU1 += tTot;
            
        } else if (players[turn] == kCPU_EASY) {

            XTime tStart, tEnd;

            XTime_GetTime(&tStart);
            
            choice = CPUsChoice(board, players, turn, kCPU_EASY_MAX_DEPTH, isDemo);

            XTime_GetTime(&tEnd);

            uint32_t tTot = (tEnd - tStart)*10000 /COUNTS_PER_SECOND;

            stats->timeOfCPU2 += tTot;
            
        } else {
            
            for (choice=3; !canInsertInColumnAtIndex(choice,board); choice=(choice+1)%7);
            
            char button_data = -1;
            
            int current_selection = -1;
            int current_selection_count = 0;
            
            uint32_t *curBall = &balls[board->n_balls];
            
            float animDur_s = .3;
            float animProg = 0;
            int animDir = 1;
            
            // USER CHOICE
            while ((button_data != BUTTON_1) && (button_data != BUTTON_2)) {
                
                button_data = XGpio_DiscreteRead(&input, 1);
                
                if (button_data == BUTTON_1+BUTTON_2) return 0;
                
                int selection = -1;
                if (button_data != 0 && button_data == current_selection) {
                    if (current_selection_count == 2) {
                        selection = button_data;
                    } ++current_selection_count;
                } else {
                    current_selection = button_data;
                    current_selection_count = -1;
                }
                
                // 10 is the "amplitude" of the displacement
                currBallY = yForRow(-1) +10*factorForAnimation(AnimationTypeSin,animProg);
                
                *curBall = encodeShape(xForColumn(choice), currBallY, color, kBALL_SHAPE);
                
                if (selection == BUTTON_3) {
                    
                    // MOVE CHOICE TO LEFT
                    
                    int t;
                    for (t = choice-1; 1; t = (t-1 >= 0 ? t-1 : 6)) {
                        if (canInsertInColumnAtIndex(t, board)) break;
                    }
                    sync_animateShape(color, kBALL_SHAPE, curBall, makePoint(xForColumn(choice),currBallY), makePointOnGrid(t,-1), AnimationTypeLin, .3);
                    choice = t;
                    animDir = 1;
                    animProg = 0;
                    
                } else if (selection == BUTTON_0){
                    
                    // MOVE CHOICE TO RIGHT
                    
                    int t;
                    for (t = (choice+1)%7; 1; t = (t+1)%7) {
                        if (canInsertInColumnAtIndex(t, board)) break;
                    }
                    sync_animateShape(color, kBALL_SHAPE, curBall, makePoint(xForColumn(choice),currBallY), makePointOnGrid(t,-1), AnimationTypeLin, .3);
                    choice = t;
                    animDir = 1;
                    animProg = 0;
                    
                }
                
                float animStep = animDir *(1.0/(60.0 *animDur_s));
                
                if (animProg +animStep >= 1 || animProg +animStep <= -1) {
                    animDir = -animDir;
                    animStep = -animStep;
                }
                
                animProg += animStep;
                
                usleep(1/60.0 *1000000);
            }
        }
        
        insertInColumnAtIndex(choice, players[turn], board, currBallY);

        char winner = winningPlayer(board);
        if (winner == kEMPTY) {
            turn = (turn +1)%2;
            continue;
        } else return winner;
    }
}

/////////////////////////////////////////////////////




////////////////////// SERVICE //////////////////////

void animateLEDs() {
    
    int seq[6] = {0b1000,0b0100,0b0010,0b0001,0b0010,0b0100};
    int seqLen = 6;
    int reps = 10;
    int interval_ms = 30;
    
    int i;
    for (i = 0; i<seqLen*reps; ++i) {
        XGpio_DiscreteWrite(&output, 1, seq[i%seqLen]);
        usleep(interval_ms*1000);
    }
}

GameMode askForGameMode() {

    DEBOUNCE;

    char switch_data = -1;
    char old_switch_data = -1;
    uint32_t *pp = kPOINTERBRAM;

    GameMode mode = GameModeInvalid;

    while (XGpio_DiscreteRead(&input, 1) == 0) {

        switch_data = XGpio_DiscreteRead(&input, 2);

        if (switch_data != old_switch_data) {

            XGpio_DiscreteWrite(&output, 1, switch_data&0b0111);
            old_switch_data = switch_data;

            clearBram();

            drawLabel(640/2,480/2-50,"connect 4",CYAN,&pp[1]);
            drawLabel(640/2,450,"press any button to start",YELLOW,&pp[9]);
            
            if ((switch_data>>2)%2 != 0) {
                
                drawLabel(640/2,480/2,"DEMO MODE",RED,&pp[30]);
                mode = GameModeDemo;
                
            } else if (switch_data%2 == 0) {

                drawLabel(640/2,480/2,"PLAYER VS player",WHITE,&pp[30]);
                mode = GameModePlayerVsPlayer;

            } else {

                drawLabel(640/2,480/2,"PLAYER VS CPU",WHITE,&pp[30]);
                
                if ((switch_data>>1)%2 == 0) {
                    drawLabel(640/2,480/2+30,"EASY",GREEN,&pp[41]);
                    mode = GameModePlayerVsCPUEasy;  
                } else {
                    drawLabel(640/2,480/2+30,"HARD",RED,&pp[41]);
                    mode = GameModePlayerVsCPUHard;
                }
            }
        }

        usleep(20000);
    }

    DEBOUNCE;

    return mode;
}

void gameOverAnimation(uint8_t m[2][4], int winner, BOOL isDemo, int matchNumber) {
    
    clearBram();
    grid_on[0] = 1;
    
    uint16_t labelXCenter = 560;
    uint16_t labelYCenter = 200;
    
    uint32_t *pp = kPOINTERBRAM;
    
    uint8_t playerColor = YELLOW;

    if (isDemo) {

        char counterStr[30] = "";
        sprintf(counterStr, "%i out of %i", matchNumber, maxDemoMatches);
        drawLabel(labelXCenter, labelYCenter-30, counterStr, WHITE, &pp[16]); // len 11
    }

    switch (winner) {
        case kPLAYER_1:
            drawLabel(labelXCenter,labelYCenter,"Player 1 wins",kPLAYER_1_COL,&pp[1]);
            playerColor = kPLAYER_1_COL;
            break;
        case kPLAYER_2:
            drawLabel(labelXCenter,labelYCenter,"Player 2 wins",kPLAYER_2_COL,&pp[1]);
            playerColor = kPLAYER_2_COL;
            break;
        case kCPU_HARD:
            if (isDemo) {
                drawLabel(labelXCenter,labelYCenter,"CPU HARD wins",kCPU_HARD_COL,&pp[1]);
            } else {
                drawLabel(labelXCenter,labelYCenter,"CPU wins",kCPU_HARD_COL,&pp[1]);
            }
            playerColor = kCPU_HARD_COL;
            break;
        case kCPU_EASY:
            if (isDemo) {
                drawLabel(labelXCenter,labelYCenter,"CPU EASY wins",kCPU_EASY_COL,&pp[1]);
            } else {
                drawLabel(labelXCenter,labelYCenter,"CPU wins",kCPU_EASY_COL,&pp[1]);
            }
            playerColor = kCPU_EASY_COL;
            break;
        default:
            drawLabel(labelXCenter,labelYCenter,"It is a tie",WHITE,&pp[1]);
            usleep(1500000);
            return;
    }

    int offset = (isDemo? 12 : 12);
    
    int i;
    for (i=0; i<=30; ++i) {
        
        uint8_t thisColor = (i%2 == 0 ? playerColor : BLACK);
        
        pp[offset+0] = encodeShape(xForColumn(m[0][0]), yForRow(m[1][0]), thisColor, kBALL_SHAPE);
        pp[offset+1] = encodeShape(xForColumn(m[0][1]), yForRow(m[1][1]), thisColor, kBALL_SHAPE);
        pp[offset+2] = encodeShape(xForColumn(m[0][2]), yForRow(m[1][2]), thisColor, kBALL_SHAPE);
        pp[offset+3] = encodeShape(xForColumn(m[0][3]), yForRow(m[1][3]), thisColor, kBALL_SHAPE);
        
        usleep(100000);
    }
}

void waitTilReset() {
    
	DEBOUNCE;

    uint32_t *pp = kPOINTERBRAM;
    
    while (XGpio_DiscreteRead(&input, 1) == 0) {
        
        drawLabel(640/2, 450, "press any button to restart", YELLOW, &pp[18]);
        
        usleep(10000);
    }

    DEBOUNCE;
}

void demoWelcomeScreen() {

    clearBram();

    uint32_t *pp = kPOINTERBRAM;

    drawLabel(320, 100, "WELCOME TO DEMO MODE", CYAN, &pp[1]);
    drawLabel(276, 224, "CPU HARD", RED, &pp[18]);
    drawLabel(422, 224, "CPU EASY", GREEN, &pp[25]);
    drawLabel(171, 258, "MAX DEPTH:", WHITE, &pp[32]);
    drawLabel(277, 258, "7", RED, &pp[41]);
    drawLabel(421, 258, "4", GREEN, &pp[42]);
    drawLabel(348, 224, "VS", WHITE, &pp[43]);


    int switch_data = -1;
    int old_switch_data = -1;

    DEBOUNCE;

    while (XGpio_DiscreteRead(&input, 1) == 0) {

        switch_data = XGpio_DiscreteRead(&input, 2);

        if (switch_data != old_switch_data) {

            XGpio_DiscreteWrite(&output, 1, switch_data&0b0111);
            old_switch_data = switch_data;

            drawLabel(320, 418, "####", WHITE, &pp[45]);

            char numb[10] = "";

            sprintf(numb, "%d", switch_data *10 +1);

            drawLabel(320, 418, numb, WHITE, &pp[45]);
        }

        usleep(20000);
    }

    maxDemoMatches = switch_data *10 +1;

    DEBOUNCE;
}

void displayStatistics(Statistics stats) {

    DEBOUNCE;

    clearBram();

    uint32_t *pp = kPOINTERBRAM;

    uint32_t totalTime = stats.timeOfCPU1 + stats.timeOfCPU2;
    uint32_t totalMatches = stats.victoriesCPU1 + stats.victoriesCPU2 + stats.ties;

    char timeHard[10] = "";
    char timeEasy[10] = "";

    char winsHard[10] = "";
    char winsEasy[10] = "";

    char winsTies[10] = "";


    sprintf(timeEasy, "%lu?",(uint32_t)ceil(stats.timeOfCPU2*100.0 /totalTime));
    sprintf(winsEasy, "%lu?",(uint32_t)round(stats.victoriesCPU2*100 /totalMatches));

    sprintf(timeHard, "%lu?",(uint32_t)(stats.timeOfCPU1*100.0 /totalTime));
    sprintf(winsHard, "%lu?",(uint32_t)round(stats.victoriesCPU1*100 /totalMatches));

    sprintf(winsTies, "%lu?",(uint32_t)round(stats.ties*100 /totalMatches));

    drawLabel(204, 221, "HARD", RED, &pp[1]);
    drawLabel(204, 251, "EASY", GREEN, &pp[5]);
    drawLabel(204, 325, "TIES", YELLOW, &pp[9]);
    drawLabel(311, 175, "TIME", WHITE, &pp[13]);
    drawLabel(451, 175, "VICTORIES", WHITE, &pp[17]);

    drawLabel(311, 221, timeHard, RED, &pp[26]); // TIME
    drawLabel(451, 221, winsHard, RED, &pp[32]); // VICTORIES

    drawLabel(311, 251, timeEasy, GREEN, &pp[29]); // TIME
    drawLabel(451, 251, winsEasy, GREEN, &pp[35]); // VICTS

    drawLabel(451, 325, winsTies, YELLOW, &pp[38]);

    drawLabel(154, 236, "CPU", WHITE, &pp[41]);
    drawLabel(320, 68, "STATS", CYAN, &pp[44]);


    while(XGpio_DiscreteRead(&input, 1) == 0)   usleep(10000);       
    

    DEBOUNCE;
}

/////////////////////////////////////////////////////




///////////////////// UTILITIES /////////////////////

uint64_t randFromClock() {
    XTime tickTocks;  
    XTime_GetTime(&tickTocks);
    return tickTocks;
}

/////////////////////////////////////////////////////

