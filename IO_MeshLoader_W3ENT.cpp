#include <string>
#include "IO_MeshLoader_W3ENT.h"

#include <ISceneManager.h>
#include <IVideoDriver.h>
#include <IFileSystem.h>
#include <IMeshManipulator.h>

//#include "Utils_RedEngine.h"
#include "Utils_Halffloat.h"
//#include "Utils_Loaders_Irr.h"
//#include "MeshCombiner.h"

namespace irr
{
namespace scene
{
void IO_MeshLoader_W3ENT::clear()
{
    Materials.clear();
    Skeleton.clear();
    meshToAnimate = nullptr;
    AnimatedMesh = nullptr;
    FrameOffset = 0;
    NbBonesPos = 0;
    Strings.clear();
    Files.clear();
    Meshes.clear();
}

//! Constructor
IO_MeshLoader_W3ENT::IO_MeshLoader_W3ENT(scene::ISceneManager* smgr, io::IFileSystem* fs)
: meshToAnimate(nullptr),
  SceneManager(smgr),
  FileSystem(fs),
  AnimatedMesh(nullptr),
  FrameOffset(0),
  NbBonesPos(0),
  ConfigLoadSkeleton(true),
  ConfigLoadOnlyBestLOD(false)
{
	#ifdef _DEBUG
    setDebugName("CW3ENTMeshFileLoader");
	#endif
}


//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".bsp")
bool IO_MeshLoader_W3ENT::isALoadableFileExtension(const io::path& filename) const
{
    io::IReadFile* file = SceneManager->getFileSystem()->createAndOpenFile(filename);
    if (!file)
        return false;

    bool checkIsLoadable = (getRedEngineFileType(file) == REV_WITCHER_3);
    file->drop();

    return checkIsLoadable;
}


//! creates/loads an animated mesh from the file.
//! \return Pointer to the created mesh. Returns 0 if loading failed.
//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
//! See IReferenceCounted::drop() for more information.
IAnimatedMesh* IO_MeshLoader_W3ENT::createMesh(io::IReadFile* f)
{
	if (!f)
        return nullptr;

    #ifdef _IRR_WCHAR_FILESYSTEM
        ConfigGamePath = SceneManager->getParameters()->getAttributeAsStringW("TW_GAME_PATH");
        ConfigExportPath = SceneManager->getParameters()->getAttributeAsStringW("TW_EXPORT_PATH");
        ConfigGameTexturesPath = SceneManager->getParameters()->getAttributeAsStringW("TW_TW3_TEX_PATH");
    #else
        ConfigGamePath = SceneManager->getParameters()->getAttributeAsString("TW_GAME_PATH");
        ConfigExportPath = SceneManager->getParameters()->getAttributeAsStringW("TW_EXPORT_PATH");
        ConfigGameTexturesPath = SceneManager->getParameters()->getAttributeAsString("TW_TW3_TEX_PATH");
    #endif

    ConfigLoadSkeleton = SceneManager->getParameters()->getAttributeAsBool("TW_TW3_LOAD_SKEL");
    ConfigLoadOnlyBestLOD = SceneManager->getParameters()->getAttributeAsBool("TW_TW3_LOAD_BEST_LOD_ONLY");

    //Clear up
    Strings.clear();
    Materials.clear();
    Files.clear();
    Meshes.clear();

    AnimatedMesh = SceneManager->createSkinnedMesh();

	if (load(f))
	{
		AnimatedMesh->finalize();
	}
	else
	{
		AnimatedMesh->drop();
        AnimatedMesh = nullptr;
	}

	return AnimatedMesh;
}

void checkMaterial(video::SMaterial mat)
{
    if (mat.getTexture(0))
        std::cout << "SLOT 1 = " <<mat.getTexture(0)->getName().getPath().c_str() << std::endl;
    else
        std::cout << "Le material n'a pas de tex slot 1" << std::endl;
}

bool IO_MeshLoader_W3ENT::W3_load(io::IReadFile* file)
{
    struct RedEngineFileHeader header;
    loadTW3FileHeader(file, header);
    Strings = header.Strings;
    Files = header.Files;

    file->seek(12);
    core::array<s32> headerData = readDataArray<s32>(file, 38);
    s32 contentChunkStart = headerData[19];
    s32 contentChunkSize = headerData[20];

    core::array<struct W3_DataInfos> meshes;
    file->seek(contentChunkStart);
    for (s32 i = 0; i < contentChunkSize; ++i)
    {
        struct W3_DataInfos infos;

        u16 dataType = readU16(file);
        core::stringc dataTypeName = Strings[dataType];

        file->seek(6, true);

        file->read(&infos.size, 4);
        file->read(&infos.adress, 4);

        file->seek(8, true);

        s32 back = file->getPos();
        if (dataTypeName == "CMesh")
        {
            meshes.push_back(infos);
        }
        else if (dataTypeName == "CMaterialInstance")
        {
            video::SMaterial mat = W3_CMaterialInstance(file, infos);
            checkMaterial(mat);
            Materials.insert(std::make_pair(i+1, mat));
        }
        else if (dataTypeName == "CEntityTemplate")
        {
            W3_CUnknown(file, infos);
//            W3_CEntityTemplate(file, *infos);
        }
        else if (dataTypeName == "CEntity")
        {
            W3_CEntity(file, infos);
        }
        else if (dataTypeName == "CMeshComponent")
        {
            W3_CMeshComponent(file, infos);
        }
        else if (dataTypeName == "CSkeleton")
        {
            W3_CSkeleton(file, infos);
        }
        else if (dataTypeName == "CAnimationBufferBitwiseCompressed" && meshToAnimate)
        {
            W3_CAnimationBufferBitwiseCompressed(file, infos);
        }
        else
        {
            W3_CUnknown(file, infos);
        }
        file->seek(back);
    }

    for (u32 i = 0; i < meshes.size(); ++i)
    {
        W3_CMesh(file, meshes[i]);
    }

    return true;
}



bool IO_MeshLoader_W3ENT::W3_ReadBuffer(io::IReadFile* file, SBufferInfos bufferInfos, SMeshInfos meshInfos)
{
    unsigned char* skinningData;
    SVertexBufferInfos vBufferInf;
    u32 nbVertices = 0;
    u32 firstVertexOffset = 0;
    u32 nbIndices = 0;
    u32 firstIndiceOffset = 0;
    for (u32 i = 0; i < bufferInfos.verticesBuffer.size(); ++i)
    {
        nbVertices += bufferInfos.verticesBuffer[i].nbVertices;
        if (nbVertices > meshInfos.firstVertex)
        {
            vBufferInf = bufferInfos.verticesBuffer[i];
            // the index of the first vertex in the buffer
            firstVertexOffset = meshInfos.firstVertex - (nbVertices - vBufferInf.nbVertices);
            break;
        }
    }
    for (u32 i = 0; i < bufferInfos.verticesBuffer.size(); ++i)
    {
        nbIndices += bufferInfos.verticesBuffer[i].nbIndices;
        if (nbIndices > meshInfos.firstIndice)
        {
            vBufferInf = bufferInfos.verticesBuffer[i];
            firstIndiceOffset = meshInfos.firstIndice - (nbIndices - vBufferInf.nbIndices);
            break;
        }
    }

    // Check if it's the best LOD
    if (ConfigLoadOnlyBestLOD && vBufferInf.lod != 1)
        return false;

    core::stringc bufferFilename = file->getFileName() + ".1.buffer";
    io::IReadFile* bufferFile = FileSystem->createAndOpenFile(bufferFilename);
    if (!bufferFile)
    {
        return false;
    }


    scene::SSkinMeshBuffer* buffer = AnimatedMesh->addMeshBuffer();
    buffer->VertexType = video::EVT_STANDARD;
    buffer->Vertices_Standard.reallocate(meshInfos.numVertices);

    u32 vertexSize = 8;
    if (meshInfos.vertexType == EMVT_SKINNED)
        vertexSize += meshInfos.numBonesPerVertex * 2;

    bufferFile->seek(vBufferInf.verticesCoordsOffset + firstVertexOffset * vertexSize);

    const video::SColor defaultColor(255, 255, 255, 255);
    for (u32 i = 0; i < meshInfos.numVertices; ++i)
    {
        u16 x, y, z, w;

        bufferFile->read(&x, 2);
        bufferFile->read(&y, 2);
        bufferFile->read(&z, 2);
        bufferFile->read(&w, 2);

        // skip skinning data
        if (meshInfos.vertexType == EMVT_SKINNED && !ConfigLoadSkeleton)
        {
            bufferFile->seek(meshInfos.numBonesPerVertex * 2, true);
        }
        else if (meshInfos.vertexType == EMVT_SKINNED)
        {
            skinningData = new unsigned char[meshInfos.numBonesPerVertex * 2]();
 //           unsigned char skinningData[meshInfos.numBonesPerVertex * 2];
            bufferFile->read(&skinningData[0], meshInfos.numBonesPerVertex * 2);

            for (u32 j = 0; j < meshInfos.numBonesPerVertex; ++j)
            {
                unsigned char boneId = skinningData[j];
                unsigned char weightStrength = skinningData[j + meshInfos.numBonesPerVertex];

                if (boneId >= AnimatedMesh->getJointCount()) // If bone don't exist
                    continue;

                if (weightStrength != 0)
                {
                    scene::ISkinnedMesh::SJoint* joint = AnimatedMesh->getAllJoints()[boneId];
                    u32 bufferId = AnimatedMesh->getMeshBufferCount() - 1;
                    f32 fWeightStrength = weightStrength / 255.f;

                    scene::ISkinnedMesh::SWeight* weight = AnimatedMesh->addWeight(joint);
                    weight->buffer_id = bufferId;
                    weight->strength = fWeightStrength;
                    weight->vertex_id = i;

                    TW3_DataCache::_instance.addVertexEntry(boneId, bufferId, i, fWeightStrength);
                }
            }
            delete[] skinningData;
        }

        buffer->Vertices_Standard.push_back(video::S3DVertex());
        buffer->Vertices_Standard[i].Pos = core::vector3df(x, y, z) / 65535.f * bufferInfos.quantizationScale + bufferInfos.quantizationOffset;
        buffer->Vertices_Standard[i].Color = defaultColor;
    }
    bufferFile->seek(vBufferInf.uvOffset + firstVertexOffset * 4);

    for (u32 i = 0; i < meshInfos.numVertices; ++i)
    {
        u16 u, v;
        bufferFile->read(&u, 2);
        bufferFile->read(&v, 2);

        f32 uf = halfToFloat(u);
        f32 vf = halfToFloat(v);

        buffer->Vertices_Standard[i].TCoords = core::vector2df(uf, vf);
    }


    bufferFile->seek(vBufferInf.normalsOffset + firstVertexOffset * 8);
    for (u32 i = 0; i < meshInfos.numVertices; ++i)
    {
        core::array<u8> bytes = readDataArray<u8>(bufferFile, 4);
        bufferFile->seek(4, true);

        u16 x = ((bytes[0]&0b11111111) | ((bytes[1]&0b11) << 8));
        u16 y = ((bytes[1]&0b11111100) | ((bytes[2]&0b00001111) << 8)) >> 2;
        u16 z = ((bytes[2]&0b11110000) | ((bytes[3]&0b00111111) << 8)) >> 4;

        f32 fx = ((s16)x - 512) / 512.f;
        f32 fy = ((s16)y - 512) / 512.f;
        f32 fz = ((s16)z - 512) / 512.f;


        buffer->Vertices_Standard[i].Normal = core::vector3df(fx, fy, fz);
        buffer->Vertices_Standard[i].Normal.normalize();
    }



    // Indices -------------------------------------------------------------------
    bufferFile->seek(bufferInfos.indicesBufferOffset + vBufferInf.indicesOffset + firstIndiceOffset * 2);

    buffer->Indices.set_used(meshInfos.numIndices);
    for (u32 i = 0; i < meshInfos.numIndices; ++i)
    {
        const u16 indice = readU16(bufferFile);

        // Indice need to be inversed for the normals
        if (i % 3 == 0)
            buffer->Indices[i] = indice;
        else if (i % 3 == 1)
            buffer->Indices[i+1] = indice;
        else if (i % 3 == 2)
            buffer->Indices[i-1] = indice;
    }

    bufferFile->drop();

    return true;
}

bool IO_MeshLoader_W3ENT::ReadPropertyHeader(io::IReadFile* file, struct SPropertyHeader& propHeader)
{
    u16 propName = readU16(file);
    u16 propType = readU16(file);

    if (propName == 0 || propType == 0 || propName >= Strings.size() || propType >= Strings.size())
        return false;

    propHeader.propName = Strings[propName];
    propHeader.propType = Strings[propType];

    const long back = file->getPos();
    propHeader.propSize = readS32(file);
    //file->seek(-4, true);

    propHeader.endPos = back + propHeader.propSize;

    return true;
}

inline u32 IO_MeshLoader_W3ENT::ReadUInt32Property(io::IReadFile* file)
{
    return readU32(file);
}

inline u8 IO_MeshLoader_W3ENT::ReadUInt8Property(io::IReadFile* file)
{
    return readU8(file);
}

inline f32 IO_MeshLoader_W3ENT::ReadFloatProperty(io::IReadFile* file)
{
    return readF32(file);
}

bool IO_MeshLoader_W3ENT::ReadBoolProperty(io::IReadFile* file)
{
    u8 valueChar = readU8(file);
    bool value = (valueChar == 0) ? false : true;
    return value;
}

struct SAnimationBufferBitwiseCompressedData IO_MeshLoader_W3ENT::ReadSAnimationBufferBitwiseCompressedDataProperty(io::IReadFile* file)
{
    file->seek(1, true);
    struct SAnimationBufferBitwiseCompressedData dataInf;

    while(1)
    {
        struct SPropertyHeader propHeader;
        if (!ReadPropertyHeader(file, propHeader))
            break;

        if (propHeader.propName == "dataAddr")
            dataInf.dataAddr = ReadUInt32Property(file);
        if (propHeader.propName == "dataAddrFallback")
            dataInf.dataAddrFallback = ReadUInt32Property(file);
        if (propHeader.propName == "numFrames")
            dataInf.numFrames = readU16(file);
        if (propHeader.propName == "dt")
            dataInf.dt = readF32(file);
        if (propHeader.propName == "compression")
            dataInf.compression = readS8(file);

        file->seek(propHeader.endPos);
    }
    return dataInf;
}

core::array<core::array<struct SAnimationBufferBitwiseCompressedData> > IO_MeshLoader_W3ENT::ReadSAnimationBufferBitwiseCompressedBoneTrackProperty(io::IReadFile* file)
{
    s32 arraySize = readS32(file);
    file->seek(1, true);

    core::array<core::array<struct SAnimationBufferBitwiseCompressedData> > inf;
    inf.push_back(core::array<struct SAnimationBufferBitwiseCompressedData>());

    int i = 0;              // array index
    while(i <= arraySize)   // the array index = bone index => rig index
    {
        struct SPropertyHeader propHeader;
        if (!ReadPropertyHeader(file, propHeader))
        {
            ++i;
            if (i == arraySize) // end of the array
                break;

            // go back and re-read
            file->seek(-1, true);
            ReadPropertyHeader(file, propHeader);
            inf.push_back(core::array<struct SAnimationBufferBitwiseCompressedData>());

        }


        if (propHeader.propType == "SAnimationBufferBitwiseCompressedData")
        {
            struct SAnimationBufferBitwiseCompressedData animInf = ReadSAnimationBufferBitwiseCompressedDataProperty(file);

            if (propHeader.propName == "position")
                animInf.type = EATT_POSITION;
            else if (propHeader.propName == "orientation")
                animInf.type = EATT_ORIENTATION;
            else if (propHeader.propName == "scale")
                animInf.type = EATT_SCALE;

            inf[inf.size() - 1].push_back(animInf);
        }

        file->seek(propHeader.endPos);
    }

    return inf;
}

core::vector3df IO_MeshLoader_W3ENT::ReadVector3Property(io::IReadFile* file)
{
    float x, y, z, w;
    file->seek(1, true);

    file->seek(8, true);    // 2 index of the Strings table (Name + type -> X, Float) + prop size
    x = ReadFloatProperty(file);
    file->seek(8, true);
    y = ReadFloatProperty(file);
    file->seek(8, true);
    z = ReadFloatProperty(file);
    file->seek(8, true);
    w = ReadFloatProperty(file);

    return core::vector3df(x, y, z);
}

core::array<video::SMaterial> IO_MeshLoader_W3ENT::ReadMaterialsProperty(io::IReadFile* file)
{
    video::SMaterial mat;
    s32 nbChunks = readS32(file);

    core::array<video::SMaterial> materials;

    for (s32 i = 0; i < nbChunks; ++i)
    {
        u32 matID = readU32(file);
        u32 matFileID = 0xFFFFFFFF - matID;

        if (matFileID < Files.size()) // Refer to a w2mi file
        {
            std::cout << "w2mi file = " << Files[matFileID].c_str() << std::endl;
//            materials.push_back(ReadMaterialFile(ConfigGamePath + Files[matFileID]));
            mat = ReadW2MIFileOnly(ConfigGamePath + Files[matFileID]);
            materials.push_back(mat);
            //file->seek(3, true);
        }
        else
        {
            std::cout << "MATERIAL VALUE = " << matID << std::endl;
            if (Materials.find(matID) != Materials.end())
                materials.push_back(Materials[matID]);
            else
                std::cout << "Mat not found ! " << matID << std::endl;

            //Materials.push_back(Materials[value-1]);
        }

    }

    return materials;
}

EMeshVertexType IO_MeshLoader_W3ENT::ReadEMVTProperty(io::IReadFile* file)
{
    u16 enumStringId = readU16(file);

    EMeshVertexType vertexType = EMVT_STATIC;

    core::stringc enumString = Strings[enumStringId];
    if (enumString == "MVT_SkinnedMesh")
    {
        vertexType = EMVT_SKINNED;
    }

    return vertexType;
}

SAnimationBufferOrientationCompressionMethod IO_MeshLoader_W3ENT::ReadAnimationBufferOrientationCompressionMethodProperty(io::IReadFile* file)
{
    u16 enumStringId = readU16(file);
    core::stringc enumString = Strings[enumStringId];

    if (enumString == "ABOCM_PackIn48bitsW")
        return ABOCM_PackIn48bitsW;
    else
        std::cout << "NEW ORIENTATION COMPRESSION METHOD" << std::endl;

    return (SAnimationBufferOrientationCompressionMethod)0;
}

core::array<SMeshInfos> IO_MeshLoader_W3ENT::ReadSMeshChunkPackedProperty(io::IReadFile* file)
{
    core::array<SMeshInfos> meshes;
    SMeshInfos meshInfos;

    s32 nbChunks = readS32(file);

    file->seek(1, true);

    s32 chunkId = 0;

    while(1)
    {
        struct SPropertyHeader propHeader;

        // invalid property = next chunk
        if (!ReadPropertyHeader(file, propHeader))
        {
            meshes.push_back(meshInfos);
            chunkId++;

            if (chunkId >= nbChunks)
                break;
            else
            {
                SMeshInfos newMeshInfos;
                newMeshInfos.vertexType = meshInfos.vertexType;
                newMeshInfos.numBonesPerVertex = meshInfos.numBonesPerVertex;
                meshInfos = newMeshInfos;

                file->seek(-1, true);
                ReadPropertyHeader(file, propHeader);
            }
        }


        if (propHeader.propName == "numIndices")
        {
            meshInfos.numIndices = ReadUInt32Property(file);
        }
        else if (propHeader.propName == "numVertices")
        {
            meshInfos.numVertices = ReadUInt32Property(file);
        }
        else if (propHeader.propName == "firstVertex")
        {
            meshInfos.firstVertex = ReadUInt32Property(file);
        }
        else if (propHeader.propName == "firstIndex")
        {
            meshInfos.firstIndice = ReadUInt32Property(file);
        }
        else if (propHeader.propName == "vertexType")
        {
            meshInfos.vertexType = ReadEMVTProperty(file);
        }
        else if (propHeader.propName == "numBonesPerVertex")
        {
            meshInfos.numBonesPerVertex = ReadUInt8Property(file);
        }
        else if (propHeader.propName == "materialID")
        {
            meshInfos.materialID = ReadUInt32Property(file);
            std::cout << "material ID = " << meshInfos.materialID << std::endl;
        }

        file->seek(propHeader.endPos);
    }


    return meshes;
}

void IO_MeshLoader_W3ENT::ReadRenderChunksProperty(io::IReadFile* file, SBufferInfos* buffer)
{
    s32 nbElements = readS32(file); // array size (= bytes count here)

    u8 nbBuffers = readU8(file);

    for (u32 i = 0; i < nbBuffers; ++i)
    {
        SVertexBufferInfos buffInfos;
        file->seek(1, true); // Unknown

        file->read(&buffInfos.verticesCoordsOffset, 4);
        file->read(&buffInfos.uvOffset, 4);
        file->read(&buffInfos.normalsOffset, 4);

        file->seek(9, true); // Unknown
        file->read(&buffInfos.indicesOffset, 4);
        file->seek(1, true); // 0x1D

        file->read(&buffInfos.nbVertices, 2);
        file->read(&buffInfos.nbIndices, 4);
        file->seek(3, true); // Unknown
        buffInfos.lod = readU8(file); // lod ?

        buffer->verticesBuffer.push_back(buffInfos);
    }
}

video::SMaterial IO_MeshLoader_W3ENT::ReadIMaterialProperty(io::IReadFile* file)
{
    video::SMaterial mat;
    mat.MaterialType = video::EMT_SOLID;

    s32 nbProperty = readS32(file);
    std::cout << "nb property = " << nbProperty << std::endl;

    // Read the properties of the material
    for (s32 i = 0; i < nbProperty; ++i)
    {
        const s32 back = file->getPos();

        s32 propSize = readS32(file);

        u16 propId, propTypeId;
        file->read(&propId, 2);
        file->read(&propTypeId, 2);

        if (propId >= Strings.size())
            break;

        const s32 textureLayer = getTextureLayerFromTextureType(Strings[propId]);
        if (textureLayer != -1)
        {
            u8 texId = readU8(file);
            texId = 255 - texId;

            if (texId < Files.size())
            {
                video::ITexture* texture = nullptr;
                texture = getTexture(Files[texId]);

                if (texture)
                {
                    mat.setTexture(textureLayer, texture);

                    if (textureLayer == 1)  // normal map
                        mat.MaterialType = video::EMT_NORMAL_MAP_SOLID;
                    else
                        mat.MaterialType = video::EMT_SOLID;
//                        mat.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
                }
            }
        }

        file->seek(back + propSize);
    }

    return mat;
}

core::array<core::vector3df> IO_MeshLoader_W3ENT::ReadBonesPosition(io::IReadFile* file)
{
    s32 nbBones = readS32(file);

    file->seek(1, true);

    core::array<core::vector3df> positions;
    for (s32 i = 0; i < nbBones; ++i)
    {
        file->seek(8, true);
        float x = ReadFloatProperty(file);
        file->seek(8, true);
        float y = ReadFloatProperty(file);
        file->seek(8, true);
        float z = ReadFloatProperty(file);
        file->seek(8, true);
        float w = ReadFloatProperty(file);

        core::vector3df position = core::vector3df(x, y, z);
        positions.push_back(position);

        file->seek(3, true);
    }
    return positions;
}

void IO_MeshLoader_W3ENT::ReadRenderLODSProperty(io::IReadFile* file)
{
    // LOD distances ?
    u32 arraySize = readU32(file);
    for (u32 i = 0; i < arraySize; ++i)
    {
        f32 value = readF32(file);
    }
}

SBufferInfos IO_MeshLoader_W3ENT::ReadSMeshCookedDataProperty(io::IReadFile* file)
{
    SBufferInfos bufferInfos;

    file->seek(1, true);

    struct SPropertyHeader propHeader;
    while(ReadPropertyHeader(file, propHeader))
    {
        if (propHeader.propName == "indexBufferSize")
        {
            bufferInfos.indicesBufferSize = ReadUInt32Property(file);
        }
        else if (propHeader.propName == "indexBufferOffset")
        {
            bufferInfos.indicesBufferOffset = ReadUInt32Property(file);
        }
        else if (propHeader.propName == "vertexBufferSize")
        {
            bufferInfos.verticesBufferSize = ReadUInt32Property(file);
        }
        else if (propHeader.propName == "quantizationScale")
        {
            bufferInfos.quantizationScale = ReadVector3Property(file);
        }
        else if (propHeader.propName == "quantizationOffset")
        {
            bufferInfos.quantizationOffset = ReadVector3Property(file);
        }
        else if (propHeader.propName == "bonePositions")
        {
            core::array<core::vector3df> positions = ReadBonesPosition(file);
            NbBonesPos = positions.size();
        }
        //else if (propHeader.propName == "collisionInitPositionOffset")
        //    ReadVector3Property(file, &bufferInfos);
        else if (propHeader.propName == "renderChunks")
            ReadRenderChunksProperty(file, &bufferInfos);
        else if (propHeader.propName == "renderLODs")
            ReadRenderLODSProperty(file);
        //else
        //    ReadUnknowProperty(file);
        file->seek(propHeader.endPos);
    }

    return bufferInfos;
}

float read16bitsFloat(io::IReadFile* file)
{
    u32 bits = readU16(file) << 16;
    f32 f;
    memcpy(&f, &bits, 4);
    return f;
}

// need a test file
float read24bitsFloat(io::IReadFile* file)
{
    u32 bits = readU16(file) << 16;
    bits |= readU8(file) << 8;
    f32 f;
    memcpy(&f, &bits, 4);
    return f;
}

float readCompressedFloat(io::IReadFile* file, u8 compressionSize)
{
    if (compressionSize == 16)
        return read16bitsFloat(file);
    else if (compressionSize == 24) // Not tested yet !
        return read24bitsFloat(file);
    else
        return readF32(file); // Not tested yet !
}

/*
// old version
float bits12ToFloat(s16 value)
{
    if (value & 0x0800)
        value = value;
    else
        value = -value;

    value = (value & 0x000007FF);
    float fVal = value / 2047.f;
    return fVal;
}
*/

// Fixed by Ákos Köte, thx
float bits12ToFloat(s16 value)
{
    float fVal = (2047.0f - value) * (1 / 2048.0f);
    return fVal;
}

core::stringc getAnimTrackString(EAnimTrackType type)
{
    if (type == EATT_POSITION)
        return "EATT_POSITION";
    else if (type == EATT_ORIENTATION)
        return "EATT_ORIENTATION";
    else
        return "EATT_SCALE";
}

void IO_MeshLoader_W3ENT::readAnimBuffer(core::array<core::array<struct SAnimationBufferBitwiseCompressedData> >& inf, io::IReadFile* dataFile, SAnimationBufferOrientationCompressionMethod c)
{
    uint64_t b1, b2, b3, b4, b5, b6, bits;
    u8 compressionSize;
    u32 keyframe;
    f32 sx, sy, sz;
    f32 px, py, pz;
    f32 fx, fy, fz, fw;
    s16 x, y, z, w;
    core::vector3df euler;
    core::quaternion orientation;
    scene::ISkinnedMesh::SPositionKey* pkey;
    scene::ISkinnedMesh::SRotationKey* rkey;
    scene::ISkinnedMesh::SScaleKey* skey;


    for (u32 i = 1; i < inf.size(); ++i)
    {
        scene::ISkinnedMesh::SJoint* joint = meshToAnimate->getAllJoints()[i];

        for (u32 j = 0; j < inf[i].size(); ++j)
        {
            struct SAnimationBufferBitwiseCompressedData infos = inf[i][j];
            dataFile->seek(infos.dataAddr);

            // TODO
            for (u32 f = 0; f < infos.numFrames; ++f)
            {
                keyframe = f;
                keyframe += FrameOffset;

                compressionSize = 0; // no compression
                if (infos.compression == 1)
                    compressionSize = 24;
                else if (infos.compression == 2)
                    compressionSize = 16;
                if (infos.type == EATT_POSITION)
                {
                    px = readCompressedFloat(dataFile, compressionSize);
                    py = readCompressedFloat(dataFile, compressionSize);
                    pz = readCompressedFloat(dataFile, compressionSize);
                    pkey = meshToAnimate->addPositionKey(joint);
                    pkey->position = core::vector3df(px,py,pz);
                    pkey->frame = (irr::f32)keyframe;
                }
                else if (infos.type == EATT_ORIENTATION)
                {

                    if (c == ABOCM_PackIn48bitsW)
                    {
                        b1 = readU8(dataFile);
                        b2 = readU8(dataFile);
                        b3 = readU8(dataFile);
                        b4 = readU8(dataFile);
                        b5 = readU8(dataFile);
                        b6 = readU8(dataFile);
                        bits = b6 | (b5 << 8) | (b4 << 16) | (b3 << 24) | (b2 << 32) | (b1 << 40);
                        //dataFile->read(&bits, sizeof(uint64_t));

                        x = y = z = w = 0;
                        x = (irr::s16)((bits & 0x0000FFF000000000) >> 36);
                        y = (irr::s16)((bits & 0x0000000FFF000000) >> 24);
                        z = (irr::s16)((bits & 0x0000000000FFF000) >> 12);
                        w =  bits & 0x0000000000000FFF;

                        fx = bits12ToFloat(x);
                        fy = bits12ToFloat(y);
                        fz = bits12ToFloat(z);
                        fw = -bits12ToFloat(w);

                        orientation = core::quaternion(fx, fy, fz, fw);
                        orientation.normalize();
                        orientation.toEuler(euler);
                        euler *= core::RADTODEG;
                        rkey = meshToAnimate->addRotationKey(joint);
                        rkey->rotation = orientation;
                        rkey->frame = (irr::f32)keyframe;
                    }


                }
                else if (infos.type == EATT_SCALE)
                {
                    sx = readCompressedFloat(dataFile, compressionSize);
                    sy = readCompressedFloat(dataFile, compressionSize);
                    sz = readCompressedFloat(dataFile, compressionSize);

                    skey = meshToAnimate->addScaleKey(joint);
                    skey->scale = core::vector3df(sx, sy, sz);
                    skey->frame = (irr::f32)keyframe;
                }
            }
        }
    }
}

void IO_MeshLoader_W3ENT::W3_CUnknown(io::IReadFile* file, struct W3_DataInfos infos)
{
    file->seek(infos.adress + 1);

    struct SPropertyHeader propHeader;
    while (ReadPropertyHeader(file, propHeader))
    {
        file->seek(propHeader.endPos);
    }
}


void IO_MeshLoader_W3ENT::W3_CAnimationBufferBitwiseCompressed(io::IReadFile* file, struct W3_DataInfos infos)
{
    file->seek(infos.adress + 1);

    core::array<core::array<struct SAnimationBufferBitwiseCompressedData> > inf;
    core::array<s8> data;
    io::IReadFile* dataFile = nullptr;
    SAnimationBufferOrientationCompressionMethod compress= ABOCM_PackIn48bitsW;

    f32 animDuration = 1.0f;
    u32 numFrames = 0;
    u16 defferedData = 0;

    struct SPropertyHeader propHeader;
    while (ReadPropertyHeader(file, propHeader))
    {
        if (propHeader.propType == "array:129,0,SAnimationBufferBitwiseCompressedBoneTrack")
        {
            inf = ReadSAnimationBufferBitwiseCompressedBoneTrackProperty(file);

        }
        else if (propHeader.propName == "data")
        {
            u32 arraySize = readU32(file);
            data = readDataArray<s8>(file, arraySize);
        }
        else if (propHeader.propName == "orientationCompressionMethod")
        {
            compress = ReadAnimationBufferOrientationCompressionMethodProperty(file);
        }
        else if (propHeader.propName == "duration")
        {
            animDuration = readF32(file);
        }
        else if (propHeader.propName == "numFrames")
        {
            numFrames = readU32(file);
        }
        else if (propHeader.propName == "deferredData")
        {
            defferedData = readU16(file);
        }

        file->seek(propHeader.endPos);
    }

    f32 animationSpeed = (f32)numFrames / animDuration;
    meshToAnimate->setAnimationSpeed(animationSpeed);

    if (defferedData == 0)
        dataFile = FileSystem->createMemoryReadFile(data.pointer(), data.size(), "tempData");
    else
    {
        core::stringc filename = file->getFileName() + "."+toStr(defferedData) + ".buffer";
        std::cout << "Filename deffered = " << filename.c_str() << std::endl;
        dataFile = FileSystem->createAndOpenFile(filename);
    }


    if (dataFile)
    {
        readAnimBuffer(inf, dataFile, compress);
        dataFile->drop();
        FrameOffset += numFrames;
    }


}

core::matrix4 toRotationMatrix(core::quaternion q) {
    auto xy2 = q.X * q.Y * 2;
    auto xz2 = q.X * q.Z * 2;
    auto xw2 = q.X * q.W * 2;
    auto yz2 = q.Y * q.Z * 2;
    auto yw2 = q.Y * q.W * 2;
    auto zw2 = q.Z * q.W * 2;
    auto ww2 = q.W * q.W * 2;
    core::matrix4 rot;
    rot(0, 0) = (irr::f32)(ww2 + 2 * q.X * q.X - 1);
    rot(0, 1) = (irr::f32)(xy2 + zw2);
    rot(0, 2) = (irr::f32)(xz2 - yw2);
    rot(0, 3) = (irr::f32)0.;
    rot(1, 0) = (irr::f32)(xy2 - zw2);
    rot(1, 1) = (irr::f32)(ww2 + 2 * q.Y * q.Y - 1);
    rot(1, 2) = (irr::f32)(yz2 + xw2);
    rot(1, 3) = (irr::f32)0.;
    rot(2, 0) = (irr::f32)(xz2 + yw2);
    rot(2, 1) = (irr::f32)(yz2 - xw2);
    rot(2, 2) = (irr::f32)(ww2 + 2 * q.Z * q.Z - 1);
    rot(2, 3) = (irr::f32)0.;
    rot(3, 0) = (irr::f32)0.;
    rot(3, 1) = (irr::f32)0.;
    rot(3, 2) = (irr::f32)0.;
    rot(3, 3) = (irr::f32)1.;

    return rot;
}

TW3_CSkeleton IO_MeshLoader_W3ENT::W3_CSkeleton(io::IReadFile* file, struct W3_DataInfos infos)
{
    file->seek(infos.adress + 1);

    TW3_CSkeleton skeleton;

    struct SPropertyHeader propHeader;
    while (ReadPropertyHeader(file, propHeader))
    {
        if (propHeader.propName == "bones")
        {
            // array
            s32 nbRigs = readS32(file);
            file->seek(1, true);

            skeleton.nbRigs = nbRigs;

            for (s32 i = 0; i < nbRigs; ++i)
            {
                struct SPropertyHeader h;
                ReadPropertyHeader(file, h);  // name + StringANSI

                u8 nameSize = readU8(file);
                core::stringc name = readString(file, nameSize);
                skeleton.rigNames.push_back(name);

                // An other property (nameAsCName)
                file->seek(13, true); // nameAsCName + CName + size + CName string ID + 3 0x00 octets
            }
        }
        else if (propHeader.propName == "parentIndices")
        {
            s32 nbBones = readS32(file);

            for (s32 i = 0; i < nbBones; ++i)
            {
                s16 parentId = readS16(file);

                skeleton.parentId.push_back(parentId);
            }

        }

        file->seek(propHeader.endPos);
    }

    // Now there are the transformations
    file->seek(-2, true);
    //std::cout << file->getPos() << std::endl;


    for (u32 i = 0; i < skeleton.nbRigs; ++i)
    {
        // position (vector 4) + quaternion (4 float) + scale (vector 4)
        core::vector3df position;
        position.X = readF32(file);
        position.Y = readF32(file);
        position.Z = readF32(file);
        readF32(file); // the w component

        core::quaternion orientation;
        orientation.X = readF32(file);
        orientation.Y = readF32(file);
        orientation.Z = readF32(file);
        orientation.W = readF32(file);

        core::vector3df scale;
        scale.X = readF32(file);
        scale.Y = readF32(file);
        scale.Z = readF32(file);
        readF32(file); // the w component

        core::matrix4 posMat;
        posMat.setTranslation(position);

        core::matrix4 rotMat = toRotationMatrix(orientation);
        //core::vector3df euler;
        //orientation.toEuler(euler);

        //chechNaNErrors(euler);

        //rotMat.setRotationRadians(euler);

        core::matrix4 scaleMat;
        scaleMat.setScale(scale);

        core::matrix4 localTransform = posMat * rotMat * scaleMat;
        orientation.makeInverse();
        skeleton.rigMatrix.push_back(localTransform);
        skeleton.rigPositions.push_back(position);
        skeleton.rigRotations.push_back(orientation);
        skeleton.rigScales.push_back(scale);

    }

    Skeleton = skeleton;

    return skeleton;
}

void IO_MeshLoader_W3ENT::W3_CEntityTemplate(io::IReadFile* file, struct W3_DataInfos infos)
{
    file->seek(infos.adress + 1);

    struct SPropertyHeader propHeader;
    while (ReadPropertyHeader(file, propHeader))
    {
        u8* data;
        IO_MeshLoader_W3ENT w3Loader(SceneManager, FileSystem);

        if (propHeader.propName == "flatCompiledData") // array of u8
        {
            s32 arraySize = readS32(file);
            arraySize -= 4;

            data = new u8[arraySize]();
            file->read(data, arraySize);


            io::IReadFile* entityFile = SceneManager->getFileSystem()->createMemoryReadFile(data, arraySize, "tmpMemFile.w2ent_MEMORY", false);
            if (entityFile) {
                IO_MeshLoader_W3ENT w3Loader(SceneManager, FileSystem);
                IAnimatedMesh* m = w3Loader.createMesh(entityFile);
                if (m)
                    m->drop();
                entityFile->drop();
            }
            delete[] data;

        }

        file->seek(propHeader.endPos);
    }
}

void IO_MeshLoader_W3ENT::W3_CEntity(io::IReadFile* file, struct W3_DataInfos infos)
{
    file->seek(infos.adress + 1);
}

bool IO_MeshLoader_W3ENT::checkBones(io::IReadFile* file, char nbBones)
{
    const long back = file->getPos();
    for (char i = 0; i < nbBones; ++i)
    {
        u16 jointName = readU16(file);
        if (jointName == 0 || jointName >= Strings.size())
        {
            file->seek(back);
            return false;
        }
        core::stringc str = Strings[jointName].c_str();

    }
    file->seek(back);
    return true;
}

char readBonesNumber(io::IReadFile* file)
{
    s8 nbBones = readS8(file);

    s8 o = readS8(file);
    if (o != 1)
        file->seek(-1, true);

    return nbBones;
}

void IO_MeshLoader_W3ENT::W3_CMesh(io::IReadFile* file, struct W3_DataInfos infos)
{

    SBufferInfos bufferInfos;
    core::array<SMeshInfos> meshes;
    core::array<video::SMaterial> materials;

    bool isStatic = false;

    file->seek(infos.adress + 1);

    struct SPropertyHeader propHeader;
    while (ReadPropertyHeader(file, propHeader))
    {
        if (propHeader.propType == "SMeshCookedData")
        {
            bufferInfos = ReadSMeshCookedDataProperty(file);
        }
        else if (propHeader.propName == "chunks")
        {
            meshes = ReadSMeshChunkPackedProperty(file);
        }
        else if (propHeader.propName == "materials")
        {
            materials = ReadMaterialsProperty(file);
        }
        else if (propHeader.propName == "isStatic")
        {
            isStatic = ReadBoolProperty(file);
        }

        file->seek(propHeader.endPos);
   }


   if (!isStatic && NbBonesPos > 0 && ConfigLoadSkeleton)
   {
        ReadBones(file);
   }

   for (u32 i = 0; i < meshes.size(); ++i)
   {
       //if (materials[meshes[i].materialID].TextureLayer->Texture == NULL)
       //    continue;
       if (!W3_ReadBuffer(file, bufferInfos, meshes[i]))
            continue;
        AnimatedMesh->getMeshBuffer(AnimatedMesh->getMeshBufferCount() - 1)->getMaterial() = materials[meshes[i].materialID];

   }
}

void IO_MeshLoader_W3ENT::ReadBones(io::IReadFile* file)
{

    // cancel property
    file->seek(-4, true);

    /*
    file->seek(2, true);
    char offsetInd;
    file->read(&offsetInd, 1);
    file->seek(offsetInd * 7, true);
    */


    // TODO
    long pos;
    char nbRead;
    u32 count = 0;
    do
    {
        pos = file->getPos();
        nbRead = readBonesNumber(file);

        if (nbRead == NbBonesPos)
        {
            if (!checkBones(file, nbRead))
            {
                nbRead = -1;
                return;
            }
        }

        if (file->getPos() >= file->getSize())
            return;
        if (++count >= 200)
            return;
    }   while (nbRead != NbBonesPos);

    file->seek(pos);


    // Name of the bones
    char nbBones = readBonesNumber(file);


    for (char i = 0; i < nbBones; ++i)
    {
        u16 jointName = readU16(file);

        scene::ISkinnedMesh::SJoint* joint = nullptr;
        //if (!AnimatedMesh->getJointCount())
             joint = AnimatedMesh->addJoint();
        //else
        //     joint = AnimatedMesh->addJoint(AnimatedMesh->getAllJoints()[0]);
        joint->Name = Strings[jointName];
    }

    // The matrix of the bones
    readBonesNumber(file);
    for (char i = 0; i < nbBones; ++i)
    {
        ISkinnedMesh::SJoint* joint = AnimatedMesh->getAllJoints()[i];
        core::matrix4 matrix;

        // the matrix
        for (u32 j = 0; j < 16; ++j)
        {
            f32 value = readF32(file);
            matrix[j] = value;
        }


        core::vector3df position = matrix.getTranslation();
        core::matrix4 invRot;
        matrix.getInverse(invRot);
        //invRot.rotateVect(position);

        core::vector3df rotation = invRot.getRotationDegrees();
        //rotation = core::vector3df(0, 0, 0);
        position = - position;
        core::vector3df scale = matrix.getScale();

        if (joint)
        {
            //Build GlobalMatrix:
            core::matrix4 positionMatrix;
            positionMatrix.setTranslation(position);
            core::matrix4 rotationMatrix;
            rotationMatrix.setRotationDegrees(rotation);
            core::matrix4 scaleMatrix;
            scaleMatrix.setScale(scale);

            joint->GlobalMatrix = scaleMatrix * rotationMatrix * positionMatrix;
            joint->LocalMatrix = joint->GlobalMatrix;

            joint->Animatedposition = joint->LocalMatrix.getTranslation();
            joint->Animatedrotation = core::quaternion(joint->LocalMatrix.getRotationDegrees()).makeInverse();
            joint->Animatedscale = joint->LocalMatrix.getScale();

            TW3_DataCache::_instance.addBoneEntry(joint->Name, matrix);
        }
    }

    // 1 float per bone ???
    readBonesNumber(file);
    for (char i = 0; i < nbBones; ++i)
    {
        float value = readF32(file);
    }

    // 1 s32 par bone. parent ID ? no
    readBonesNumber(file);
    for (char i = 0; i < nbBones; ++i)
    {
        u32 unk = readU32(file);
    }
}


scene::ISkinnedMesh* IO_MeshLoader_W3ENT::ReadW2MESHFile(core::stringc filename)
{
    ISkinnedMesh* mesh = nullptr;
    io::IReadFile* meshFile = FileSystem->createAndOpenFile(filename);
    if (meshFile)
    {
        IO_MeshLoader_W3ENT w3Loader(SceneManager, FileSystem);
        mesh = reinterpret_cast<ISkinnedMesh*>(w3Loader.createMesh(meshFile));
        meshFile->drop();
    }
    return mesh;
}

// In the materials, file can be w2mi (material) or w2mg (shader)
// We check that and load the material in the case of a w2mi file
video::SMaterial IO_MeshLoader_W3ENT::ReadMaterialFile(core::stringc filename)
{
    if (core::hasFileExtension(filename, "w2mi"))
        return video::SMaterial();
//        return ReadW2MIFile(filename);
    else if (core::hasFileExtension(filename, "w2mg"))
 //       return ReadW2MIFileOnly(filename);
        return video::SMaterial(); // shader, not handled 
}

video::SMaterial IO_MeshLoader_W3ENT::ReadW2MIFile(core::stringc filename)
{
    video::SMaterial material;
    io::IReadFile* matFile = FileSystem->createAndOpenFile(filename);
    if (matFile)
    {
        IO_MeshLoader_W3ENT w2miLoader(SceneManager, FileSystem);
        IAnimatedMesh* matMesh = nullptr;
        matMesh = w2miLoader.createMesh(matFile);
        if (matMesh)
            matMesh->drop();

        // Get the material from the w2mi file loaded
        if (w2miLoader.Materials.size() == 1)
            material = w2miLoader.Materials[0];

        matFile->drop();
    }

    return material;
}

video::SMaterial IO_MeshLoader_W3ENT::ReadW2MIFileOnly(core::stringc filename)
{
    video::SMaterial mat;
    struct RedEngineFileHeader header;
    core::array<video::SMaterial> materials;

    io::IReadFile* file = FileSystem->createAndOpenFile(filename);
    if (!file) return mat;

    loadTW3FileHeader(file, header);

    file->seek(12);
    core::array<s32> headerData = readDataArray<s32>(file, 38);
    s32 contentChunkStart = headerData[19];
    s32 contentChunkSize = headerData[20];

    core::array<struct W3_DataInfos> meshes;
    file->seek(contentChunkStart);
    for (s32 i = 0; i < contentChunkSize; ++i)
    {
        struct W3_DataInfos infos;

        u16 dataType = readU16(file);
        core::stringc dataTypeName = header.Strings[dataType];

        file->seek(6, true);
        file->read(&infos.size, 4);
        file->read(&infos.adress, 4);
        file->seek(8, true);

        s32 back = file->getPos();
        if (dataTypeName == "CMaterialInstance")
        {
            file->seek(infos.adress + 1);
            const s32 endOfChunk = infos.adress + infos.size;
 
            while (file->getPos() < endOfChunk)
            {
                struct SPropertyHeader propHeader;
                if (!ReadPropertyHeader(file, propHeader))
                {
                    file->seek(-2, true);
                    s32 nbProperty = readS32(file);
                    std::cout << "nb property = " << nbProperty << std::endl;

                    // Read the properties of the material
                    for (s32 j = 0; j < nbProperty; ++j)
                    {
                        const s32 back = file->getPos();

                        s32 propSize = readS32(file);

                        u16 propId, propTypeId;
                        file->read(&propId, 2);
                        file->read(&propTypeId, 2);

                        if (propId >= header.Strings.size())
                            break;

                        const s32 textureLayer = getTextureLayerFromTextureType(header.Strings[propId]);
                        if (textureLayer != -1)
                        {
                            u8 texId = readU8(file);
                            texId = 255 - texId;

                            if (texId < header.Files.size())
                            {
                                video::ITexture* texture = nullptr;
                                texture = getTexture(header.Files[texId]);

                                if (texture)
                                {
                                    mat.setTexture(textureLayer, texture);

                                    if (textureLayer == 1)  // normal map
                                        mat.MaterialType = video::EMT_NORMAL_MAP_SOLID;
                                    else
                                        mat.MaterialType = video::EMT_SOLID;
                                }
                            }
                        }

                        file->seek(back + propSize);
                    }
                    break;
                }
//                else if (propHeader.propName == "baseMaterial")
                else if ( propHeader.propName == "materials" )
                {
                    u32 fileId = readU32(file);
                    fileId = 0xFFFFFFFF - fileId;
                    if (core::hasFileExtension(header.Files[fileId], "w2mi"))
                    {
                        mat = ReadW2MIFileOnly(ConfigGamePath + header.Files[fileId]);
                        if (mat.TextureLayer->Texture)
                            return mat;
                    }
                }
                file->seek(propHeader.endPos);
            }
            checkMaterial(mat);
            return mat;
        }
        file->seek(back);
    }
    return mat;
}

video::SMaterial IO_MeshLoader_W3ENT::W3_CMaterialInstance(io::IReadFile* file, struct W3_DataInfos infos)
{
    file->seek(infos.adress + 1);

    video::SMaterial mat;

    const s32 endOfChunk = infos.adress + infos.size;

    while (file->getPos() < endOfChunk)
    {
        struct SPropertyHeader propHeader;
        if (!ReadPropertyHeader(file, propHeader))
        {
            file->seek(-2, true);
            return ReadIMaterialProperty(file);
        }

        // material in a w2mi file
        if (propHeader.propName == "baseMaterial")
        {
            u32 fileId = readU32(file);
            fileId = 0xFFFFFFFF - fileId;

            mat = ReadMaterialFile(ConfigGamePath + Files[fileId]);
        }

        file->seek(propHeader.endPos);
    }

    return mat;
}

// Check the file format version and load the mesh if it's ok
bool IO_MeshLoader_W3ENT::load(io::IReadFile* file)
{
    readString(file, 4); // CR2W

    const s32 fileFormatVersion = readS32(file);

    if (getTWFileFormatVersion(file) == REV_WITCHER_3)
    {
        return W3_load(file);
    }
    else
    {
        return false;
    }
}


video::ITexture* IO_MeshLoader_W3ENT::getTexture(io::path filename)
{
    io::path baseFilename;
    if (core::hasFileExtension(filename.c_str(), "xbm"))
    {
        core::cutFilenameExtension(baseFilename, filename);
    }
    else
        return SceneManager->getVideoDriver()->getTexture(filename);

    video::ITexture* texture = nullptr;

    // Check for textures extracted with the LUA tools
    filename = baseFilename + core::stringc(".dds");
    filename.replace("\\", "#");
    filename = ConfigGameTexturesPath + filename;

    if (FileSystem->existFile(filename))
        texture = SceneManager->getVideoDriver()->getTexture(filename);

    if (texture)
        return texture;

    // Else, if extracted with wcc_lite, we check all the possible filename
    core::array<io::path> possibleExtensions;
    possibleExtensions.push_back(".dds");
    possibleExtensions.push_back(".bmp");
    possibleExtensions.push_back(".jpg");
    possibleExtensions.push_back(".jpeg");
    possibleExtensions.push_back(".tga");
    possibleExtensions.push_back(".png");

    for (u32 i = 0; i < possibleExtensions.size(); ++i)
    {
        filename = ConfigGameTexturesPath + baseFilename + possibleExtensions[i];

        if (FileSystem->existFile(filename))
            texture = SceneManager->getVideoDriver()->getTexture(filename);

        if (texture)
            return texture;
    }


    return texture;
}

s32 IO_MeshLoader_W3ENT::getTextureLayerFromTextureType(core::stringc textureType)
{
    if (textureType == "Diffuse")
        return 0;
    else if (textureType == "Normal")
        return 1;
    else
        return -1;
}


} // end namespace scene
} // end namespace irr
