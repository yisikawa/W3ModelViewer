#ifndef SKINNED_VERTEX_H
#define SKINNED_VERTEX_H


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

#endif // SKINNED_VERTEX_H
