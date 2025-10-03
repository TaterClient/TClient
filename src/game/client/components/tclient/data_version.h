#ifndef GAME_CLIENT_COMPONENTS_TCLIENT_DATA_VERSION_H
#define GAME_CLIENT_COMPONENTS_TCLIENT_DATA_VERSION_H
#else
#error data_version.h included multiple times
#endif

// Check validity of data/data_version.txt
// This is extracted to this file for ease of editing
// TODO: this is a stub

// #include <format>
// #include <string>

#include <base/system.h>

#include <engine/shared/linereader.h>

#include <game/localization.h>
#include <game/version.h>

#define DATA_VERSION_PATH "data_version.txt"

// static std::string VersionNumberToString(int Version)
// {
// 	return std::format("{}.{}.{}", Version / 1000, (Version / 100) % 10, Version % 10);
// }

// static const std::pair<std::string, std::string> DOMAINS[] = {
// 	{"DDNet", VersionNumberToString(DDNET_VERSION_NUMBER)},
// 	{"TClient", TCLIENT_VERSION},
// };

inline void CheckDataVersion(char *pError, int Length, IOHANDLE File)
{
	if(!File)
	{
		// str_format(pError, Length, TCLocalize("%s could not be read", DATA_VERSION_PATH), DATA_VERSION_PATH);
		return;
	}

	// CLineReader LineReader;
	// LineReader.OpenFile(File);

	// const char *pLine;
	// while((pLine = LineReader.Get()))
	// {
	// 	char aDomain[32];
	// 	const char *pVersion = str_next_token(pLine, " ", aDomain, sizeof(aDomain));
	// }

	io_close(File);
}
