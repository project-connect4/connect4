#include "AI.h"
#include <math.h>

#define kMAGIC_EXP 8
#define kMAGIC_RAT 2
#define nextPlayerIndex(_curr) ((_curr+1)%2)

static int CPUinsertInColumnAtIndex(int indx, char player, Board *board);
static BOOL CPUremoveFromColumnAtIndex(int indx, Board *board);
static void traverse(Board *board, char players[], int turn, char cpuChar, int step, int maxSteps, double *fitness, int choiceIndex);
static int fittestIndex(double fitness[]);
static char winningPlayerFromPosition(Board board, int xpos, int ypos);


int CPUsChoice(Board *board, char players[], int turn, int maxAiSteps, BOOL isDemo) {
    
    XTime clk;  
    XTime_GetTime(&clk);
	
    if (isDemo && (clk)%kERROR_FACTOR == 0) {

        xil_printf("Mistake made by CPU %s\n", (players[turn] == kCPU_HARD ? "HARD" : "EASY"));

		while (1) {

			int ans = (rand()*clk)%kBOARDS_COLS;

			if (canInsertInColumnAtIndex(ans, board)) {
				return ans;
			}
		}
	}

    double *fitness = calloc(kBOARDS_COLS, sizeof(double));

    traverse(board, players, turn, players[turn], 1, maxAiSteps, fitness, -1);

    int i, ans;

    for (i=0; i<kBOARDS_COLS; ++i) {

        ans = fittestIndex(fitness);
        if (canInsertInColumnAtIndex(ans, board)) break;
        else {
            fitness[ans] = INT32_MIN;
        }
    }

    free(fitness);

    return ans;
}

static void traverse(Board *board, char players[], int turn, char cpuChar, int step, int maxSteps, double *fitness, int choiceIndex) {
    
    int i;
    
    // For each column
    for (i=0; i<kBOARDS_COLS; ++i) {
        
        if (step == 1) choiceIndex = i;
        
        int row = CPUinsertInColumnAtIndex(i, players[turn], board);

        // Tries to insert
        if (row != -1) {
            
            // Inserted
            char winningChar = winningPlayerFromPosition(*board, i, row);
            
            if (winningChar == kCPU_HARD) {
                
                if (cpuChar == kCPU_HARD)
                    fitness[choiceIndex] += pow(step, -(kMAGIC_EXP));
                else
                    fitness[choiceIndex] -= pow(step, -(kMAGIC_EXP)) *(kMAGIC_RAT);
                
            } else if (winningChar == kCPU_EASY) {
                
                if (cpuChar == kCPU_EASY)
                    fitness[choiceIndex] += pow(step, -(kMAGIC_EXP));
                else
                    fitness[choiceIndex] -= pow(step, -(kMAGIC_EXP)) *(kMAGIC_RAT);
                
            } else if (winningChar == kPLAYER_1 || winningChar == kPLAYER_2) {
                
                fitness[choiceIndex] -= pow(step, -(kMAGIC_EXP)) *(kMAGIC_RAT);
                
            } else if ((step+1) <= maxSteps){
                
                // Recur
                traverse(board, players, nextPlayerIndex(turn), cpuChar, step+1, maxSteps, fitness, choiceIndex);
            }
            
            // Backtrack
            CPUremoveFromColumnAtIndex(i, board);
        }
    }
}

static char winningPlayerFromPosition(Board board, int xpos, int ypos) {

    if (board.matrix == NULL) {
        return kEMPTY;
    }

    
    int offsetNew[2][4] = {{ 1, 1, 1, 0},
                           {-1, 1, 0, 1}};

    int k, s, iters;

    // The player in the current position
    char player = board.matrix[ypos][xpos];
    if (player == kEMPTY) return kEMPTY;

    // For each possible direction
    for (k=0; k<4; ++k) {

        int i_off = offsetNew[0][k];
        int j_off = offsetNew[1][k];

        int span = 1;

        // For each step in the same direction
        for (s=0; s<=1; ++s) {

            int theSign = (s==0 ? 1 : -1);

            for (iters = 1; iters<=kBOARDS_COLS; ++iters) {

                int i_cur = ypos +theSign*(i_off*iters);
                int j_cur = xpos +theSign*(j_off*iters);

                // Invalid position. Useless to continue.
                if (i_cur < 0 || j_cur < 0 || i_cur >= kBOARDS_ROWS || j_cur >= kBOARDS_COLS) {
                    break;
                }

                if (board.matrix[i_cur][j_cur] != player) {
                    break;
                }

                span += 1;
            }
        }

        if (span >= kLEN_TO_WIN) {
            return player;
        }
    }

    return kEMPTY;
}

static int fittestIndex(double fitness[]) {
    int i, best_i = 0;
    for (i = 0; i<kBOARDS_COLS; ++i)
        if (fitness[i] > fitness[best_i])
            best_i = i;
    return best_i;
}

static int CPUinsertInColumnAtIndex(int indx, char player, Board *board) {
    if (indx < 0|| indx >= kBOARDS_COLS || board->matrix[0][indx] != kEMPTY) {
        return -1;
    }
    int i;
    for (i = kBOARDS_ROWS -1; i>=0; --i) {
        if (board->matrix[i][indx] == kEMPTY) {
            board->matrix[i][indx] = player;
            --(board->emptyCells);
            return i;
        }
    }
    return -1;
}

static BOOL CPUremoveFromColumnAtIndex(int indx, Board *board) {
    if (indx < 0|| indx >= kBOARDS_COLS || board->matrix[kBOARDS_ROWS -1][indx] == kEMPTY) {
        return false;
    }
    int i;
    for (i = 0; i < kBOARDS_ROWS; ++i) {
        if (board->matrix[i][indx] != kEMPTY) {
            board->matrix[i][indx] = kEMPTY;
            ++(board->emptyCells);
            return true;
        }
    }
    return false;
}
