#ifndef UTILS_TW_H
#define UTILS_TW_H

#include <IFileSystem.h>
#include <irrArray.h>

using namespace irr;

enum RedEngineVersion
{
    REV_WITCHER_2,
    REV_WITCHER_3,
    REV_UNKNOWN
};

enum RedEngineContentType
{
    RECT_WITCHER_ENTITY,
    RECT_WITCHER_MESH,
    RECT_WITCHER_RIG,
    RECT_WITCHER_ANIMATIONS,
    RECT_WITCHER_MATERIAL,
    RECT_WITCHER_OTHER
};



struct RedEngineFileHeader
{
    RedEngineFileHeader() : Version(0), Strings(core::array<core::stringc>()), Files(core::array<core::stringc>())
    {
    }
    s32 Version;
    core::array<core::stringc> Strings;
    core::array<core::stringc> Files;
};

RedEngineContentType getRedEngineFileContentType(io::path filename);
RedEngineVersion getTWFileFormatVersion(io::IReadFile* file);
bool hasRedEngineMagicCode(io::IReadFile* file);
RedEngineVersion getRedEngineFileType(io::IReadFile* file);

bool loadTW3FileHeader(io::IReadFile* file, struct RedEngineFileHeader &header);

#endif // UTILS_TW_H
