#ifndef TW3_CSKELETON_H
#define TW3_CSKELETON_H

#include <ISkinnedMesh.h>

using namespace irr;

class TW3_CSkeleton
{
public:
    TW3_CSkeleton();

    void clear();

//    u32 nbBones;
    u32 nbRigs;
    core::array<core::stringc> rigNames;
    core::array<short> parentId;
    core::array<core::matrix4> rigMatrix;

    core::array<core::vector3df> rigPositions;
    core::array<core::quaternion> rigRotations;
    core::array<core::vector3df> rigScales;

    bool applyToModel(scene::ISkinnedMesh* mesh);
    bool applyToModel2(scene::ISkinnedMesh* mesh);
};

#endif // TW3_CSKELETON_H
