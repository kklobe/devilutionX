#pragma once

#include <cstdint>
#include <limits>
#include <string>
namespace devilution {

enum game_info : uint8_t {
	GAMEINFO_NAME,
	GAMEINFO_PASSWORD,
};

enum conn_type : uint8_t {
	SELCONN_ZT,
	SELCONN_TCP,
	SELCONN_LOOPBACK,
};

struct PCXHeader {
	Uint8 Manufacturer;
	Uint8 Version;
	Uint8 Encoding;
	Uint8 BitsPerPixel;
	Uint16 Xmin;
	Uint16 Ymin;
	Uint16 Xmax;
	Uint16 Ymax;
	Uint16 HDpi;
	Uint16 VDpi;
	Uint8 Colormap[48];
	Uint8 Reserved;
	Uint8 NPlanes;
	Uint16 BytesPerLine;
	Uint16 PaletteInfo;
	Uint16 HscreenSize;
	Uint16 VscreenSize;
	Uint8 Filler[54];
};

struct _SNETCAPS {
	Uint32 size;
	Uint32 flags;
	Uint32 maxmessagesize;
	Uint32 maxqueuesize;
	Uint32 maxplayers;
	Uint32 bytessec;
	Uint32 latencyms;
	Uint32 defaultturnssec;
	Uint32 defaultturnsintransit;
};

struct _SNETEVENT {
	Uint32 eventid;
	Uint32 playerid;
	void *data;
	Uint32 databytes;
};

// Note to self: Linker error => forgot a return value in cpp

// We declare the StormLib methods we use here.
// StormLib uses the Windows calling convention on Windows for these methods.
#ifdef _WIN32
#define WINAPI __stdcall
#else
#define WINAPI
#endif

#ifdef __cplusplus
struct CCritSect {
	SDL_mutex *m_critsect;

	CCritSect()
	{
		m_critsect = SDL_CreateMutex();
		if (m_critsect == NULL) {
			ErrSdl();
		}
	}
	~CCritSect()
	{
		SDL_DestroyMutex(m_critsect);
	}
	void Enter()
	{
		if (SDL_LockMutex(m_critsect) < 0) {
			ErrSdl();
		}
	}
	void Leave()
	{
		if (SDL_UnlockMutex(m_critsect) < 0) {
			ErrSdl();
		}
	}
};
#endif

// Game states
#define GAMESTATE_PRIVATE 0x01
#define GAMESTATE_FULL    0x02
#define GAMESTATE_ACTIVE  0x04
#define GAMESTATE_STARTED 0x08
#define GAMESTATE_REPLAY  0x80

#define PS_CONNECTED 0x10000
#define PS_TURN_ARRIVED 0x20000
#define PS_ACTIVE 0x40000

#define LEAVE_ENDING 0x40000004
#define LEAVE_DROP 0x40000006

#if defined(__GNUC__) || defined(__cplusplus)
extern "C" {
#endif

bool SNetCreateGame(const char *pszGameName, const char *pszGamePassword, const char *pszGameStatString, DWORD dwGameType, char *GameTemplateData, int GameTemplateSize, int playerCount, const char *creatorName, const char *a11, int *playerID);
bool SNetDestroy();

/*  SNetDropPlayer @ 106
 *
 *  Drops a player from the current game.
 *
 *  playerid:     The player ID for the player to be dropped.
 *  flags:
 *
 *  Returns true if the function was called successfully and false otherwise.
 */
bool SNetDropPlayer(int playerid, DWORD flags);

/*  SNetGetGameInfo @ 107
 *
 *  Retrieves specific game information from Storm, such as name, password,
 *  stats, mode, game template, and players.
 *
 *  type:         The type of data to retrieve. See GAMEINFO_ flags.
 *  dst:          The destination buffer for the data.
 *  length:       The maximum size of the destination buffer.
 *
 *  Returns true if the function was called successfully and false otherwise.
 */
bool SNetGetGameInfo(game_info type, void *dst, unsigned int length);

/*  SNetGetTurnsInTransit @ 115
 *
 *  Retrieves the number of turns (buffers) that have been queued
 *  before sending them over the network.
 *
 *  turns: A pointer to an integer that will receive the value.
 *
 *  Returns true if the function was called successfully and false otherwise.
 */
bool
    SNetGetTurnsInTransit(
        DWORD *turns);

// Network provider structures
typedef struct _client_info {
	DWORD dwSize; // 60
	char *pszName;
	char *pszVersion;
	DWORD dwProduct;
	DWORD dwVerbyte;
	DWORD dwUnk5;
	DWORD dwMaxPlayers;
	DWORD dwUnk7;
	DWORD dwUnk8;
	DWORD dwUnk9;
	DWORD dwUnk10; // 0xFF
	char *pszCdKey;
	char *pszCdOwner;
	DWORD dwIsShareware;
	DWORD dwLangId;
} client_info;

typedef struct _user_info {
	DWORD dwSize; // 16
	char *pszPlayerName;
	char *pszUnknown;
	DWORD dwUnknown;
} user_info;

bool SNetJoinGame(int id, char *gameName, char *gamePassword, char *playerName, char *userStats, int *playerid);

/*  SNetLeaveGame @ 119
 *
 *  Notifies Storm that the player has left the game. Storm will
 *  notify all connected peers through the network provider.
 *
 *  type: The leave type. It doesn't appear to be important, no documentation available.
 *
 *  Returns true if the function was called successfully and false otherwise.
 */
bool SNetLeaveGame(int type);

bool SNetPerformUpgrade(DWORD *upgradestatus);
bool SNetReceiveMessage(int *senderplayerid, char **data, int *databytes);
bool SNetReceiveTurns(int a1, int arraysize, char **arraydata, DWORD *arraydatabytes, DWORD *arrayplayerstatus);

typedef void(*SEVTHANDLER)(struct _SNETEVENT *);

/*  SNetSendMessage @ 127
 *
 *  Sends a message to a player given their player ID. Network message
 *  is sent using class 01 and is retrieved by the other client using
 *  SNetReceiveMessage().
 *
 *  playerID:   The player index of the player to receive the data.
 *              Conversely, this field can be one of the following constants:
 *                  SNPLAYER_ALL      | Sends the message to all players, including oneself.
 *                  SNPLAYER_OTHERS   | Sends the message to all players, except for oneself.
 *  data:       A pointer to the data.
 *  databytes:  The amount of bytes that the data pointer contains.
 *
 *  Returns true if the function was called successfully and false otherwise.
 */
bool SNetSendMessage(int playerID, void *data, unsigned int databytes);

// Macro values to target specific players
#define SNPLAYER_ALL    -1
#define SNPLAYER_OTHERS -2

#define MPQ_FLAG_READ_ONLY 1
#define SFILE_OPEN_FROM_MPQ 0
#define SFILE_OPEN_LOCAL_FILE 0xFFFFFFFF

/*  SNetSendTurn @ 128
 *
 *  Sends a turn (data packet) to all players in the game. Network data
 *  is sent using class 02 and is retrieved by the other client using
 *  SNetReceiveTurns().
 *
 *  data:       A pointer to the data.
 *  databytes:  The amount of bytes that the data pointer contains.
 *
 *  Returns true if the function was called successfully and false otherwise.
 */
bool SNetSendTurn(char *data, unsigned int databytes);

bool SFileOpenFile(const char *filename, HANDLE *phFile);

// Functions implemented in StormLib
bool WINAPI SFileOpenArchive(const char *szMpqName, DWORD dwPriority, DWORD dwFlags, HANDLE *phMpq);
bool WINAPI SFileCloseArchive(HANDLE hArchive);
bool WINAPI SFileOpenFileEx(HANDLE hMpq, const char *szFileName, DWORD dwSearchScope, HANDLE *phFile);
bool WINAPI SFileReadFile(HANDLE hFile, void *buffer, DWORD nNumberOfBytesToRead, DWORD *read, LONG *lpDistanceToMoveHigh);
DWORD WINAPI SFileGetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh);
DWORD WINAPI SFileSetFilePointer(HANDLE, int, int*, int);
bool WINAPI SFileCloseFile(HANDLE hFile);

/*  SBmpLoadImage @ 323
 *
 *  Load an image from an available archive into a buffer.
 *
 *  pszFileName:  The name of the graphic in an active archive.
 *  pPalette:     An optional buffer that receives the image palette.
 *  pBuffer:      A buffer that receives the image data.
 *  dwBuffersize: The size of the specified image buffer.
 *  pdwWidth:     An optional variable that receives the image width.
 *  pdwHeight:    An optional variable that receives the image height.
 *  pdwBpp:       An optional variable that receives the image bits per pixel.
 *
 *  Returns true if the image was supported and loaded correctly, false otherwise.
 */
bool
    SBmpLoadImage(
        const char *pszFileName,
        SDL_Color *pPalette,
        BYTE *pBuffer,
        DWORD dwBuffersize,
        DWORD *pdwWidth,
        DWORD *pdwHeight,
        DWORD *pdwBpp);

bool getIniBool(const char *sectionName, const char *keyName, bool defaultValue = false);
float getIniFloat(const char *sectionName, const char *keyName, float defaultValue);
bool getIniValue(const char *sectionName, const char *keyName, char *string, int stringSize, const char *defaultString = "");
void setIniValue(const char *sectionName, const char *keyName, const char *value, int len = 0);
void SaveIni();
int getIniInt(const char *keyname, const char *valuename, int defaultValue);
void setIniInt(const char *keyname, const char *valuename, int value);
void setIniFloat(const char *keyname, const char *valuename, float value);

void SVidPlayBegin(const char *filename, int a2, int a3, int a4, int a5, int flags, HANDLE *video);
void SVidPlayEnd(HANDLE video);

/*  SErrGetLastError @ 463
 *
 *  Retrieves the last error that was specifically
 *  set for the Storm library.
 *
 *  Returns the last error set within the Storm library.
 */
DWORD SErrGetLastError();

/*  SErrSetLastError @ 465
 *
 *  Sets the last error for the Storm library and the Kernel32 library.
 *
 *  dwErrCode:  The error code that will be set.
 */
void SErrSetLastError(DWORD dwErrCode);

// Values for dwErrCode
#define STORM_ERROR_GAME_TERMINATED              0x85100069
#define STORM_ERROR_INVALID_PLAYER               0x8510006a
#define STORM_ERROR_NO_MESSAGES_WAITING          0x8510006b
#define STORM_ERROR_NOT_IN_GAME                  0x85100070
#define STORM_ERROR_REQUIRES_UPGRADE             0x85100077

/*  SStrCopy @ 501
 *
 *  Copies a string from src to dest (including NULL terminator)
 *  until the max_length is reached.
 *
 *  dest:         The destination array.
 *  src:          The source array.
 *  max_length:   The maximum length of dest.
 *
 */
void SStrCopy(char *dest, const char *src, int max_length);

bool SFileSetBasePath(const char *);
bool SVidPlayContinue(void);
bool SNetGetOwnerTurnsWaiting(DWORD *);
bool SNetUnregisterEventHandler(event_type, SEVTHANDLER);
bool SNetRegisterEventHandler(event_type, SEVTHANDLER);
bool SNetSetBasePlayer(int);
int SNetInitializeProvider(Uint32 provider, struct GameData *gameData);
int SNetGetProviderCaps(struct _SNETCAPS *);
bool SFileEnableDirectAccess(bool enable);

#if defined(__GNUC__) || defined(__cplusplus)
}

// Additions to Storm API:

// Sets the file's 64-bit seek position.
inline std::uint64_t SFileSetFilePointer(HANDLE hFile, std::int64_t offset, int whence)
{
    int high = static_cast<std::uint64_t>(offset) >> 32;
    int low = static_cast<int>(offset);
    low = SFileSetFilePointer(hFile, low, &high, whence);
    return (static_cast<std::uint64_t>(high) << 32) | low;
}

// Returns the current 64-bit file seek position.
inline std::uint64_t SFileGetFilePointer(HANDLE hFile)
{
    // We use `SFileSetFilePointer` with offset 0 to get the current position
    // because there is no `SFileGetFilePointer`.
    return SFileSetFilePointer(hFile, 0, DVL_FILE_CURRENT);
}

#endif

} // namespace devilution
