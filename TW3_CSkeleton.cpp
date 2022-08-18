#include "TW3_CSkeleton.h"

#include "Utils_Loaders_Irr.h"

#include <iostream>

TW3_CSkeleton::TW3_CSkeleton() : nbRigs(0),
rigNames(core::array<core::stringc>()),
parentId(core::array<short>()),
rigMatrix(core::array<core::matrix4>()),
rigPositions(core::array<core::vector3df>()),
rigRotations(core::array<core::quaternion>()),
rigScales(core::array<core::vector3df>())
{

}

void TW3_CSkeleton::clear()
{
    rigNames.clear();
    parentId.clear();
    rigMatrix.clear();
    rigPositions.clear();
    rigRotations.clear();
    rigScales.clear();
}

// Definition in IrrAssimpExport
core::array<scene::ISkinnedMesh::SJoint*> getRootJoints(const scene::ISkinnedMesh* mesh);





void computeLocal(const scene::ISkinnedMesh* mesh, scene::ISkinnedMesh::SJoint* joint)
{
    // Parent bone is necessary to compute the local matrix from global
    scene::ISkinnedMesh::SJoint* jointParent = JointHelper::GetParent(mesh, joint);

    if (jointParent)
    {
        core::matrix4 globalParent = jointParent->GlobalMatrix;
        core::matrix4 invGlobalParent;
        globalParent.getInverse(invGlobalParent);

        joint->LocalMatrix = invGlobalParent * joint->GlobalMatrix;
    }
    else
        joint->LocalMatrix = joint->GlobalMatrix;
}




bool TW3_CSkeleton::applyToModel(scene::ISkinnedMesh* mesh)
{
    // Set the hierarchy
    core::array<scene::ISkinnedMesh::SJoint*> rigRoots;
    // Create the bones
    for (u32 i = 0; i < nbRigs; ++i)
    {
        core::stringc rigName = rigNames[i];
        scene::ISkinnedMesh::SJoint* joint = JointHelper::GetJointByName(mesh, rigName);
        if (!joint)
        {
            joint = mesh->addJoint();
            joint->Name = rigName;
        }
    }


    for (u32 i = 0; i < nbRigs; ++i)
    {
        scene::ISkinnedMesh::SJoint* joint = mesh->getAllJoints()[i]; // TODO: this is probably buggy (need to use JointHelper::GetJointByName(mesh, names[i]))
        s16 parent = parentId[i];
        if (parent != -1) // root
        {
            scene::ISkinnedMesh::SJoint* parentJoint = JointHelper::GetJointByName(mesh, rigNames[parent]);
            if (parentJoint)
                parentJoint->Children.push_back(joint);
        }
        else
        {
            rigRoots.push_back(joint);
        }
    }

    // Set the transformations
    for (u32 i = 0; i < nbRigs; ++i)
    {
        core::stringc rigName = rigNames[i];

        scene::ISkinnedMesh::SJoint* joint = JointHelper::GetJointByName(mesh, rigName);
        if (!joint)
            continue;

        core::matrix4 rigMat = rigMatrix[i];
        joint->LocalMatrix = rigMat;

        joint->Animatedposition = rigPositions[i];
        joint->Animatedrotation = rigRotations[i];
        joint->Animatedscale = rigScales[i];
    }

    // Compute the global matrix
    for (u32 i = 0; i < rigRoots.size(); ++i)
    {
        JointHelper::ComputeGlobalMatrixRecursive(mesh, rigRoots[i]);
    }

    return true;
}
