#ifndef TW3_DATACACHE_H
#define TW3_DATACACHE_H

#include "SkinnedVertex.h"

using namespace irr;



struct BoneEntry
{
    BoneEntry(core::stringc name, core::matrix4 offsetMatrix)
        : _name(name), _offsetMatrix(offsetMatrix)
    {
    }

    core::stringc _name;
    core::matrix4 _offsetMatrix;
};

struct VertexSkinningEntry
{
    VertexSkinningEntry(u32 boneID, u16 meshBufferID, u32 vertexID, f32 strenght)
        : _boneID(boneID), _meshBufferID(meshBufferID), _vertexID(vertexID), _strenght(strenght)
    {
    }

    u32 _boneID;
    u16 _meshBufferID;
    u32 _vertexID;
    f32 _strenght;
};

class TW3_DataCache
{
    scene::ISkinnedMesh* _owner;

    core::array<struct BoneEntry> _bones;
    core::array<struct VertexSkinningEntry> _vertices;


    std::vector<std::vector<SkinnedVertex> > _skinnedVertex;
    void skinJoint(irr::scene::ISkinnedMesh::SJoint *joint, struct BoneEntry bone);
    void buildSkinnedVertexArray();
    void applySkinnedVertexArray();

public:
    TW3_DataCache();

    static TW3_DataCache _instance;

    void setOwner(scene::ISkinnedMesh* owner);
    void addBoneEntry(core::stringc name, core::matrix4 boneOffset);
    void addVertexEntry(u32 boneID, u16 meshBufferID, u32 vertexID, f32 strenght);
    void clear();
    void boneApply2Rig();
    void vertex2Skin();

    u32 _bufferID;
};

#endif // TW3_DATACACHE_H
