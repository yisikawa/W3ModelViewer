#ifndef IRRASSIMPIMPORT_H
#define IRRASSIMPIMPORT_H

#include <IMeshLoader.h>

#include <assimp/scene.h>          // Output data structure
#include <assimp/postprocess.h>    // Post processing flags
#include <assimp/Importer.hpp>

#include "IrrAssimpUtils.h"

#include "SkinnedVertex.h"

class IrrAssimpImport : public irr::scene::IMeshLoader
{
    public:
        explicit IrrAssimpImport(irr::scene::ISceneManager* smgr);
        virtual ~IrrAssimpImport();

        virtual irr::scene::IAnimatedMesh* createMesh(irr::io::IReadFile* file);
        virtual bool isALoadableFileExtension(const irr::io::path& filename) const;

        irr::core::stringc error;

    private:
        irr::scene::ISceneManager* m_sceneManager;
        irr::io::IFileSystem* m_fileSystem;
        irr::core::array<irr::video::SMaterial> m_materials;
        const aiScene* m_assimpScene;
        irr::io::path m_filePath;
        irr::scene::ISkinnedMesh* m_irrMesh;

        void createMaterials();
        void createMeshes();
        void createAnimation();
        void createNode(const aiNode* node);
        irr::scene::ISkinnedMesh::SJoint* findJoint(const irr::core::stringc jointName);
        aiNode* findNode(aiString jointName);
        irr::video::ITexture* getTexture(irr::core::stringc path, irr::core::stringc fileDir);

        // skinning
        irr::core::array<SkinnedVertex> m_skinnedVertex;
        void skinJoint(irr::scene::ISkinnedMesh::SJoint* joint, aiBone* bone);
        void buildSkinnedVertexArray(irr::scene::IMeshBuffer* buffer);
        void applySkinnedVertexArray(irr::scene::IMeshBuffer* buffer);
};

#endif // IRRASSIMPIMPORT_H
