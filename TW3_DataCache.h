#ifndef TW3_DATACACHE_H
#define TW3_DATACACHE_H

#include <ISkinnedMesh.h>

#include <vector>

class SkinnedVertex
{
public:
    SkinnedVertex() :
        moved(false),
        position(irr::core::vector3df(0.f, 0.f, 0.f)),
        normal(irr::core::vector3df(0.f, 0.f, 0.f))
    {
    }

    bool moved;
    irr::core::vector3df position;
    irr::core::vector3df normal;
};

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
    void apply();
    void skin();

    u32 _bufferID;
};

#endif // TW3_DATACACHE_H
