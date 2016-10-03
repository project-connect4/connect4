#ifndef PHYSISCS
#define PHYSISCS

#include "Constants.h"

#define kTEXT_CHAR_WIDTH    15
#define kTEXT_CHAR_HEIGHT   15
#define kTEXT_SPACING       -5

#define kBALL_SHAPE  38

uint32_t encodeShape(uint16_t xpos, uint16_t ypos, uint8_t color, uint8_t shape);
uint16_t row(int i);
uint16_t col(int i);
void drawLabel(uint16_t xpos, uint16_t ypos, const char str[], uint8_t color, uint32_t *pp);
int shapeForChar(char theChar);
void clearBram();

#endif
