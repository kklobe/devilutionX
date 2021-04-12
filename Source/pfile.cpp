/**
 * @file pfile.cpp
 *
 * Implementation of the save game encoding functionality.
 */
#include <string>

#include "all.h"
#include "paths.h"
#include "../3rdParty/Storm/Source/storm.h"
#include "file_util.h"

namespace devilution {

#define PASSWORD_SPAWN_SINGLE "adslhfb1"
#define PASSWORD_SPAWN_MULTI "lshbkfg1"
#define PASSWORD_SINGLE "xrgyrkj1"
#define PASSWORD_MULTI "szqnlsk1"

namespace {

std::string GetSavePath(DWORD save_num)
{
	std::string path = GetPrefPath();
	const char *ext = ".sv";
	if (gbIsHellfire)
		ext = ".hsv";

	if (gbIsSpawn) {
		if (!gbIsMultiplayer) {
			path.append("spawn");
		} else {
			path.append("share_");
		}
	} else {
		if (!gbIsMultiplayer) {
			path.append("single_");
		} else {
			path.append("multi_");
		}
	}

	char save_num_str[21];
	snprintf(save_num_str, sizeof(save_num_str) / sizeof(char), "%d", save_num);
	path.append(save_num_str);
	path.append(ext);
	return path;
}

bool GetPermSaveNames(DWORD dwIndex, char *szPerm)
{
	const char *fmt;

	if (dwIndex < giNumberOfLevels)
		fmt = "perml%02d";
	else if (dwIndex < giNumberOfLevels * 2) {
		dwIndex -= giNumberOfLevels;
		fmt = "perms%02d";
	} else
		return false;

	sprintf(szPerm, fmt, dwIndex);
	return true;
}

bool GetTempSaveNames(DWORD dwIndex, char *szTemp)
{
	const char *fmt;

	if (dwIndex < giNumberOfLevels)
		fmt = "templ%02d";
	else if (dwIndex < giNumberOfLevels * 2) {
		dwIndex -= giNumberOfLevels;
		fmt = "temps%02d";
	} else
		return false;

	sprintf(szTemp, fmt, dwIndex);
	return true;
}

void pfile_rename_temp_to_perm()
{
	DWORD dwIndex;
	bool bResult;
	char szTemp[MAX_PATH];
	char szPerm[MAX_PATH];

	dwIndex = 0;
	while (GetTempSaveNames(dwIndex, szTemp)) {
		bResult = GetPermSaveNames(dwIndex, szPerm);
		assert(bResult);
		dwIndex++;
		if (mpqapi_has_file(szTemp)) {
			if (mpqapi_has_file(szPerm))
				mpqapi_remove_hash_entry(szPerm);
			mpqapi_rename(szTemp, szPerm);
		}
	}
	assert(!GetPermSaveNames(dwIndex, szPerm));
}

} // namespace

/** List of character names for the character selection screen. */
static char hero_names[MAX_CHARACTERS][PLR_NAME_LEN];
bool gbValidSaveFile;

const char *pfile_get_password()
{
	if (gbIsSpawn)
		return gbIsMultiplayer ? PASSWORD_SPAWN_MULTI : PASSWORD_SPAWN_SINGLE;
	return gbIsMultiplayer ? PASSWORD_MULTI : PASSWORD_SINGLE;
}

static DWORD pfile_get_save_num_from_name(const char *name)
{
	DWORD i;

	for (i = 0; i < MAX_CHARACTERS; i++) {
		if (!strcasecmp(hero_names[i], name))
			break;
	}

	return i;
}

static BYTE *pfile_read_archive(HANDLE archive, const char *pszName, DWORD *pdwLen)
{
	DWORD nread;
	HANDLE file;
	BYTE *buf;

	if (!SFileOpenFileEx(archive, pszName, 0, &file))
		return NULL;

	*pdwLen = SFileGetFileSize(file, NULL);
	if (*pdwLen == 0)
		return NULL;

	buf = DiabloAllocPtr(*pdwLen);
	if (!SFileReadFile(file, buf, *pdwLen, &nread, NULL))
		return NULL;
	SFileCloseFile(file);

	*pdwLen = codec_decode(buf, *pdwLen, pfile_get_password());
	if (*pdwLen == 0)
		return NULL;

	return buf;
}

static bool pfile_read_hero(HANDLE archive, PkPlayerStruct *pPack)
{
	DWORD read;
	BYTE *buf;

	buf = pfile_read_archive(archive, "hero", &read);
	if (buf == NULL)
		return false;

	bool ret = false;
	if (read == sizeof(*pPack)) {
		memcpy(pPack, buf, sizeof(*pPack));
		ret = true;
	}

	mem_free_dbg(buf);
	return ret;
}

static void pfile_encode_hero(const PkPlayerStruct *pPack)
{
	BYTE *packed;
	DWORD packed_len;

	packed_len = codec_get_encoded_len(sizeof(*pPack));
	packed = (BYTE *)DiabloAllocPtr(packed_len);
	memcpy(packed, pPack, sizeof(*pPack));
	codec_encode(packed, sizeof(*pPack), packed_len, pfile_get_password());
	mpqapi_write_file("hero", packed, packed_len);
	mem_free_dbg(packed);
}

static bool pfile_open_archive(DWORD save_num)
{
	if (OpenMPQ(GetSavePath(save_num).c_str(), save_num))
		return true;

	return false;
}

static void pfile_flush(bool is_single_player, DWORD save_num)
{
	mpqapi_flush_and_close(GetSavePath(save_num).c_str(), is_single_player, save_num);
}

static HANDLE pfile_open_save_archive(DWORD save_num)
{
	HANDLE archive;

	if (SFileOpenArchive(GetSavePath(save_num).c_str(), 0, 0, &archive))
		return archive;
	return NULL;
}

static void pfile_SFileCloseArchive(HANDLE *hsArchive)
{
	if (*hsArchive == NULL)
		return;

	SFileCloseArchive(*hsArchive);
	*hsArchive = NULL;
}

PFileScopedArchiveWriter::PFileScopedArchiveWriter()
    : save_num_(pfile_get_save_num_from_name(plr[myplr]._pName))
{
	if (!pfile_open_archive(save_num_))
		app_fatal("Failed to open player archive for writing.");
}

PFileScopedArchiveWriter::~PFileScopedArchiveWriter()
{
	pfile_flush(!gbIsMultiplayer, save_num_);
}

void pfile_write_hero(bool write_game_data)
{
	PFileScopedArchiveWriter scoped_writer;
	if (write_game_data) {
		SaveGameData();
		pfile_rename_temp_to_perm();
	}
	PkPlayerStruct pkplr;
	PackPlayer(&pkplr, myplr, !gbIsMultiplayer);
	pfile_encode_hero(&pkplr);
	if (!gbVanilla) {
		SaveHotkeys();
		SaveHeroItems(&plr[myplr]);
	}
}

static void game_2_ui_player(const PlayerStruct *p, _uiheroinfo *heroinfo, bool bHasSaveFile)
{
	memset(heroinfo, 0, sizeof(*heroinfo));
	strncpy(heroinfo->name, p->_pName, sizeof(heroinfo->name) - 1);
	heroinfo->name[sizeof(heroinfo->name) - 1] = '\0';
	heroinfo->level = p->_pLevel;
	heroinfo->heroclass = p->_pClass;
	heroinfo->strength = p->_pStrength;
	heroinfo->magic = p->_pMagic;
	heroinfo->dexterity = p->_pDexterity;
	heroinfo->vitality = p->_pVitality;
	heroinfo->gold = p->_pGold;
	heroinfo->hassaved = bHasSaveFile;
	heroinfo->herorank = p->pDiabloKillLevel;
	heroinfo->spawned = gbIsSpawn;
}

bool pfile_create_player_description()
{
	char desc[128];
	_uiheroinfo uihero;

	myplr = 0;
	pfile_read_player_from_save();
	game_2_ui_player(&plr[myplr], &uihero, gbValidSaveFile);
	UiSetupPlayerInfo(gszHero, &uihero, GAME_ID);

	return true;
}

void pfile_flush_W()
{
	pfile_flush(true, pfile_get_save_num_from_name(plr[myplr]._pName));
}

bool pfile_ui_set_hero_infos(bool (*ui_add_hero_info)(_uiheroinfo *))
{
	memset(hero_names, 0, sizeof(hero_names));

	for (DWORD i = 0; i < MAX_CHARACTERS; i++) {
		HANDLE archive = pfile_open_save_archive(i);
		if (archive) {
			PkPlayerStruct pkplr;
			if (pfile_read_hero(archive, &pkplr)) {
				_uiheroinfo uihero;
				strcpy(hero_names[i], pkplr.pName);
				bool hasSaveGame = pfile_archive_contains_game(archive, i);
				if (hasSaveGame)
					pkplr.bIsHellfire = gbIsHellfireSaveGame;

				UnPackPlayer(&pkplr, 0, false);

				pfile_SFileCloseArchive(&archive);
				LoadHeroItems(&plr[0]);
				RemoveEmptyInventory(0);
				CalcPlrInv(0, false);

				game_2_ui_player(&plr[0], &uihero, hasSaveGame);
				ui_add_hero_info(&uihero);
			}
			pfile_SFileCloseArchive(&archive);
		}
	}

	return true;
}

bool pfile_archive_contains_game(HANDLE hsArchive, DWORD save_num)
{
	if (gbIsMultiplayer)
		return false;

	DWORD dwLen;
	BYTE *gameData = pfile_read_archive(hsArchive, "game", &dwLen);
	if (gameData == NULL)
		return false;

	Uint32 hdr = LOAD_LE32(gameData);
	mem_free_dbg(gameData);

	return IsHeaderValid(hdr);
}

void pfile_ui_set_class_stats(unsigned int player_class_nr, _uidefaultstats *class_stats)
{
	class_stats->strength = StrengthTbl[player_class_nr];
	class_stats->magic = MagicTbl[player_class_nr];
	class_stats->dexterity = DexterityTbl[player_class_nr];
	class_stats->vitality = VitalityTbl[player_class_nr];
}

bool pfile_ui_save_create(_uiheroinfo *heroinfo)
{
	DWORD save_num;
	PkPlayerStruct pkplr;

	save_num = pfile_get_save_num_from_name(heroinfo->name);
	if (save_num >= MAX_CHARACTERS) {
		for (save_num = 0; save_num < MAX_CHARACTERS; save_num++) {
			if (!hero_names[save_num][0])
				break;
		}
		if (save_num >= MAX_CHARACTERS)
			return false;
	}
	if (!pfile_open_archive(save_num))
		return false;
	mpqapi_remove_hash_entries(pfile_get_file_name);
	strncpy(hero_names[save_num], heroinfo->name, PLR_NAME_LEN);
	hero_names[save_num][PLR_NAME_LEN - 1] = '\0';
	CreatePlayer(0, heroinfo->heroclass);
	strncpy(plr[0]._pName, heroinfo->name, PLR_NAME_LEN);
	plr[0]._pName[PLR_NAME_LEN - 1] = '\0';
	PackPlayer(&pkplr, 0, true);
	pfile_encode_hero(&pkplr);
	game_2_ui_player(&plr[0], heroinfo, false);
	if (!gbVanilla) {
		SaveHotkeys();
		SaveHeroItems(&plr[0]);
	}
	pfile_flush(true, save_num);
	return true;
}

bool pfile_get_file_name(DWORD lvl, char *dst)
{
	const char *fmt;

	if (gbIsMultiplayer) {
		if (lvl)
			return false;
		fmt = "hero";
	} else {
		if (lvl < giNumberOfLevels)
			fmt = "perml%02d";
		else if (lvl < giNumberOfLevels * 2) {
			lvl -= giNumberOfLevels;
			fmt = "perms%02d";
		} else if (lvl == giNumberOfLevels * 2)
			fmt = "game";
		else if (lvl == giNumberOfLevels * 2 + 1)
			fmt = "hero";
		else
			return false;
	}
	sprintf(dst, fmt, lvl);
	return true;
}

bool pfile_delete_save(_uiheroinfo *hero_info)
{
	DWORD save_num;

	save_num = pfile_get_save_num_from_name(hero_info->name);
	if (save_num < MAX_CHARACTERS) {
		hero_names[save_num][0] = '\0';
		RemoveFile(GetSavePath(save_num).c_str());
	}
	return true;
}

void pfile_read_player_from_save()
{
	HANDLE archive;
	DWORD save_num;
	PkPlayerStruct pkplr;

	save_num = pfile_get_save_num_from_name(gszHero);
	archive = pfile_open_save_archive(save_num);
	if (archive == NULL)
		app_fatal("Unable to open archive");
	if (!pfile_read_hero(archive, &pkplr))
		app_fatal("Unable to load character");

	gbValidSaveFile = pfile_archive_contains_game(archive, save_num);
	if (gbValidSaveFile)
		pkplr.bIsHellfire = gbIsHellfireSaveGame;

	pfile_SFileCloseArchive(&archive);

	UnPackPlayer(&pkplr, myplr, false);

	LoadHeroItems(&plr[myplr]);
	RemoveEmptyInventory(myplr);
	CalcPlrInv(myplr, false);
}

bool LevelFileExists()
{
	char szName[MAX_PATH];

	GetPermLevelNames(szName);

	DWORD save_num = pfile_get_save_num_from_name(plr[myplr]._pName);
	if (!pfile_open_archive(save_num))
		app_fatal("Unable to read to save file archive");

	bool has_file = mpqapi_has_file(szName);
	pfile_flush(true, save_num);
	return has_file;
}

void GetTempLevelNames(char *szTemp)
{
	if (setlevel)
		sprintf(szTemp, "temps%02d", setlvlnum);
	else
		sprintf(szTemp, "templ%02d", currlevel);
}

void GetPermLevelNames(char *szPerm)
{
	DWORD save_num;
	bool has_file;

	save_num = pfile_get_save_num_from_name(plr[myplr]._pName);
	GetTempLevelNames(szPerm);
	if (!pfile_open_archive(save_num))
		app_fatal("Unable to read to save file archive");

	has_file = mpqapi_has_file(szPerm);
	pfile_flush(true, save_num);
	if (!has_file) {
		if (setlevel)
			sprintf(szPerm, "perms%02d", setlvlnum);
		else
			sprintf(szPerm, "perml%02d", currlevel);
	}
}

void pfile_remove_temp_files()
{
	if (gbIsMultiplayer)
		return;

	DWORD save_num = pfile_get_save_num_from_name(plr[myplr]._pName);
	if (!pfile_open_archive(save_num))
		app_fatal("Unable to write to save file archive");
	mpqapi_remove_hash_entries(GetTempSaveNames);
	pfile_flush(true, save_num);
}

void pfile_write_save_file(const char *pszName, BYTE *pbData, DWORD dwLen, DWORD qwLen)
{
	DWORD save_num;

	save_num = pfile_get_save_num_from_name(plr[myplr]._pName);
	codec_encode(pbData, dwLen, qwLen, pfile_get_password());
	if (!pfile_open_archive(save_num))
		app_fatal("Unable to write to save file archive");
	mpqapi_write_file(pszName, pbData, qwLen);
	pfile_flush(true, save_num);
}

BYTE *pfile_read(const char *pszName, DWORD *pdwLen)
{
	DWORD save_num;
	HANDLE archive;
	BYTE *buf;

	save_num = pfile_get_save_num_from_name(plr[myplr]._pName);
	archive = pfile_open_save_archive(save_num);
	if (archive == NULL)
		return NULL;

	buf = pfile_read_archive(archive, pszName, pdwLen);
	pfile_SFileCloseArchive(&archive);
	if (buf == NULL)
		return NULL;

	return buf;
}

void pfile_update(bool force_save)
{
	static Uint32 save_prev_tc;

	if (!gbIsMultiplayer)
		return;

	Uint32 tick = SDL_GetTicks();
	if (!force_save && tick - save_prev_tc <= 60000)
		return;

	save_prev_tc = tick;
	pfile_write_hero();
}

} // namespace devilution
