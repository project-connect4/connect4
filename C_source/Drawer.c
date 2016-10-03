#include "Drawer.h"
#include <ctype.h>


uint32_t encodeShape(uint16_t xpos, uint16_t ypos, uint8_t color, uint8_t shape){
    if (xpos > 640) xpos = 640;
    if (ypos > 480) ypos = 480;
    if (color > 0b111) color = 0b111;
    return (xpos<<22) + (ypos<<13) + (color<<10) + (shape<<2);
}

void clearBram(){
	uint32_t *pp = kPOINTERBRAM;
	int i;
	for(i=0;i<51;++i){
		pp[i]=0;
	}
}

int shapeForChar(char theChar) {
    if (toupper((int)theChar) >= 'A' && toupper((int)theChar)<= 'Z') {
        // letter
        return toupper((int)theChar) -'A' +12;
    } else if (theChar >= '0' && theChar <= '9') {
        // Digit
        return theChar -'0';
    } else if (theChar == ':') {
        // Special char
        return 10;
    } else if (theChar == '?') {
        // Special char
        return 11;
    } else if (theChar == '*') {
        // Special char
        return kBALL_SHAPE;
    } else if (theChar == '#') {
        // Special char
        return -2;
    } else {
        return -1;
    }
}

void drawLabel(uint16_t xpos, uint16_t ypos, const char str[], uint8_t color, uint32_t *pp) {

    int i,theStrLen = strlen(str);

    uint16_t expectedLabelWidth = theStrLen*(kTEXT_CHAR_WIDTH + kTEXT_SPACING);

    uint16_t curr_xpos = xpos -expectedLabelWidth/2;

    for (i=0; i<theStrLen; ++i) {
        char thisChar = str[i];

        int shape = shapeForChar(thisChar);

        if (shape == -2) {
            *pp = 0;
            ++pp;
        } else if (shape != -1) {
            *pp = encodeShape(curr_xpos, ypos, color, shape);
            ++pp;
        }

        curr_xpos += kTEXT_CHAR_WIDTH + kTEXT_SPACING;
    }
}
