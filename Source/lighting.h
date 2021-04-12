/**
 * @file lighting.h
 *
 * Interface of light and vision.
 */
#pragma once

namespace devilution {

struct LightListStruct {
	int _lx;
	int _ly;
	int _lradius;
	int _lid;
	bool _ldel;
	bool _lunflag;
	int _lunx;
	int _luny;
	int _lunr;
	int _xoff;
	int _yoff;
	bool _lflags;
};

extern LightListStruct VisionList[MAXVISION];
extern BYTE lightactive[MAXLIGHTS];
extern LightListStruct LightList[MAXLIGHTS];
extern int numlights;
extern int numvision;
extern char lightmax;
extern bool dolighting;
extern int visionid;
extern BYTE *pLightTbl;
extern bool lightflag;

void DoLighting(int nXPos, int nYPos, int nRadius, int Lnum);
void DoUnVision(int nXPos, int nYPos, int nRadius);
void DoVision(int nXPos, int nYPos, int nRadius, bool doautomap, bool visible);
void FreeLightTable();
void InitLightTable();
void MakeLightTable();
#ifdef _DEBUG
void ToggleLighting();
#endif
void InitLightMax();
void InitLighting();
int AddLight(int x, int y, int r);
void AddUnLight(int i);
void ChangeLightRadius(int i, int r);
void ChangeLightXY(int i, int x, int y);
void ChangeLightOff(int i, int x, int y);
void ChangeLight(int i, int x, int y, int r);
void ProcessLightList();
void SavePreLighting();
void InitVision();
int AddVision(int x, int y, int r, bool mine);
void ChangeVisionRadius(int id, int r);
void ChangeVisionXY(int id, int x, int y);
void ProcessVisionList();
void lighting_color_cycling();

/* rdata */

extern const char CrawlTable[2749];
extern const BYTE vCrawlTable[23][30];

}
