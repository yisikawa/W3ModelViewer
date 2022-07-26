#ifndef IRRASSIMPEXPORT_H
#define IRRASSIMPEXPORT_H

#include <ISkinnedMesh.h>

#include <assimp/scene.h>          // Output data structure
#include <assimp/postprocess.h>    // Post processing flags
#include <assimp/Exporter.hpp>

#include "IrrAssimpUtils.h"

class IrrAssimpExport
{
    public:
        IrrAssimpExport();
        virtual ~IrrAssimpExport();
        void writeFile(irr::scene::IAnimatedMesh* mesh, irr::core::stringc format, irr::core::stringc filename);

    private:
        aiScene* m_assimpScene;

        void createMeshes(const irr::scene::IAnimatedMesh* irrMesh);
        void createMaterials(const irr::scene::IAnimatedMesh* irrMesh);
        void createAnimations(const irr::scene::ISkinnedMesh* irrMesh);
        aiNode* createNode(const irr::scene::ISkinnedMesh::SJoint* irrJoint);

        irr::core::array<irr::u16> getMeshesMovedByBone(const irr::scene::ISkinnedMesh::SJoint* joint);
        std::map<irr::u16, irr::core::array<const irr::scene::ISkinnedMesh::SJoint*> > m_bonesPerMesh;
        std::map<std::pair<irr::u16, const irr::scene::ISkinnedMesh::SJoint*>, irr::u32> m_weightsCountPerMeshesAndBones;
        irr::core::array<irr::u32> m_attachedBuffers;
};

#endif // IRRASSIMPEXPORT_H
