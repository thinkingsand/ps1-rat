#include <stdlib.h>
#include <stdio.h>
#include <libgte.h>
#include <libgpu.h>
#include <libgs.h>
#include <libetc.h>
#include <libspu.h>
#include <libds.h>
#include <strings.h>
#include <sys/types.h>

//Declare Stuff Here
void clear_vram();

// Screen resolution and dither mode
int SCREEN_WIDTH, SCREEN_HEIGHT;
#define CENTERX	SCREEN_WIDTH/2
#define CENTERY	SCREEN_HEIGHT/2
#define DITHER 1

// Increasing this value (max is 14) reduces sorting errors in certain cases
#define OT_LENGTH	12
#define OT_ENTRIES	1<<OT_LENGTH
#define PACKETMAX	2048

typedef struct {
	int r;
	int g;
	int b;
} Color;

// Camera coordinates
struct {
	VECTOR	position;
	SVECTOR rotation;
	GsCOORDINATE2 coord2;
} Camera;

GsOT		orderingTable[2];
GsOT_TAG	orderingTable_TAG[2][OT_ENTRIES];
int			myActiveBuff=0;
PACKET GPUOutputPacket[2][PACKETMAX*24];
Color BGColor;

//Creates a color from RGB
Color createColor(int r, int g, int b) {
	Color color = {.r = r, .g = g, .b = b};
	return color;
}

void SetBGColor (int r, int g, int  b) {
	BGColor = createColor(r, g, b);
}

void initializeScreen() {

	ResetGraph(0);
	//clear_vram();

	// Automatically adjust screen to PAL or NTCS from license
	if (*(char *)0xbfc7ff52=='E') { // SCEE string address
    	// PAL MODE
    	SCREEN_WIDTH = 320;
    	SCREEN_HEIGHT = 256;
    	printf("Setting the PlayStation Video Mode to (PAL %dx%d)\n",SCREEN_WIDTH,SCREEN_HEIGHT);
    	SetVideoMode(1);
    	printf("Video Mode is (%ld)\n",GetVideoMode());
   	} else {
     	// NTSC MODE
     	SCREEN_WIDTH = 320;
     	SCREEN_HEIGHT = 240;
     	printf("Setting the PlayStation Video Mode to (NTSC %dx%d)\n",SCREEN_WIDTH,SCREEN_HEIGHT);
     	SetVideoMode(0);
     	printf("Video Mode is (%ld)\n",GetVideoMode());
   }
	GsInitGraph(SCREEN_WIDTH, SCREEN_HEIGHT, GsINTER|GsOFSGPU, 1, 0);
	GsDefDispBuff(0, 0, 0, SCREEN_HEIGHT);

	// Prepare the ordering tables
	orderingTable[0].length	=OT_LENGTH;
	orderingTable[1].length	=OT_LENGTH;
	orderingTable[0].org		=orderingTable_TAG[0];
	orderingTable[1].org		=orderingTable_TAG[1];

	GsClearOt(0, 0, &orderingTable[0]);
	GsClearOt(0, 0, &orderingTable[1]);

	// Initialize debug font stream
	FntLoad(960, 0);
	FntOpen(-CENTERX + 7, -CENTERY + 15, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 512);

	// Setup 3D and projection matrix
	GsInit3D();
	GsSetProjection(CENTERX);
	GsInitCoordinate2(WORLD, &Camera.coord2);

	// Set default lighting mode
	//0 = No Fog
	//1 = Fog
	GsSetLightMode(0);

}

void clear_vram() {
    RECT rectTL;
    setRECT(&rectTL, 0, 0, 1024, 512);
    ClearImage2(&rectTL, 0, 0, 0);
    DrawSync(0);
    return;
}

void clear_display() {

	// Get active buffer ID and clear the OT to be processed for the next frame
	myActiveBuff = GsGetActiveBuff();
	GsSetWorkBase((PACKET*)GPUOutputPacket[myActiveBuff]);
	GsClearOt(0, 0, &orderingTable[myActiveBuff]);

}

void Display() {

	FntFlush(-1);

	DrawSync(0);
	VSync(0);
	GsSwapDispBuff();
	//the first 3 numbers are the background color
	//was 0, 64, 0
	GsSortClear(BGColor.r, BGColor.g, BGColor.b, &orderingTable[myActiveBuff]);
	GsDrawOt(&orderingTable[myActiveBuff]);

}
