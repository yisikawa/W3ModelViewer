#include "Utils_RedEngine.h"

#include "Utils_Loaders_Irr.h"


#include <iostream>


RedEngineContentType getRedEngineFileContentType(io::path filename)
{
    if (core::hasFileExtension ( filename, "w2ent" ))
        return RECT_WITCHER_ENTITY;
    else if (core::hasFileExtension ( filename, "w2mesh" ))
        return RECT_WITCHER_MESH;
    else if (core::hasFileExtension ( filename, "w2rig" ))
        return RECT_WITCHER_RIG;
    else if (core::hasFileExtension ( filename, "w2anims" ))
        return RECT_WITCHER_ANIMATIONS;
    else if (core::hasFileExtension ( filename, "w2mi" ))
        return RECT_WITCHER_MATERIAL;
    else
        return RECT_WITCHER_OTHER;
}

RedEngineVersion getTWFileFormatVersion(io::IReadFile* file)
{
    if (!file)
        return REV_UNKNOWN;

    const long pos = file->getPos();

    file->seek(4);
    s32 version = readS32(file);
    file->seek(pos);

    if (version == 115)
        return REV_WITCHER_2;
    else if (version >= 162)
        return REV_WITCHER_3;
    else
        return REV_UNKNOWN;
}

bool hasRedEngineMagicCode(io::IReadFile* file)
{
    if (!file)
        return false;

    const long pos = file->getPos();
    core::stringc magic = readString(file, 4);
    file->seek(pos);

    return (magic == "CR2W");
}

RedEngineVersion getRedEngineFileType(io::IReadFile* file)
{
    if (!hasRedEngineMagicCode(file))
        return REV_UNKNOWN;

    return getTWFileFormatVersion(file);
}


bool loadTW3FileHeader(io::IReadFile* file, RedEngineFileHeader &header)
{
    if (!file)
        return false;

    const long initialPos = file->getPos();

    file->seek(12);

    core::array<s32> headerData = readDataArray<s32>(file, 38);
    // debug
    /*
    for (int i = 0; i < 38; ++i)
    {
        std::cout << "Header data [" << i << "]: " << headerData[i] << std::endl;
    }
    */

    s32 stringChunkStart = headerData[7];
    s32 stringChunkSize = headerData[8];
    s32 calculatedStringChunkSize = stringChunkStart + stringChunkSize;

    s32 stringChunkEnd = headerData[10]; // or the adress of a new chunk ?
    s32 nbStrings = headerData[11];

    // in many case seem similar to file count, but no
    //s32 nbFiles = headerData[14];

    int nbStringsRead = 0;
    file->seek(stringChunkStart);
    while (file->getPos() < calculatedStringChunkSize)
    {
        core::stringc str = readStringUntilNull(file);
        if (nbStringsRead < nbStrings)
        {
            header.Strings.push_back(str);
            std::cout << "-->" << str.c_str() << std::endl;
            nbStringsRead++;
        }
        else
        {
            header.Files.push_back(str);
            std::cout << "--> FILE: " << str.c_str() << std::endl;
        }
    }

    file->seek(initialPos);

    return true;
}
