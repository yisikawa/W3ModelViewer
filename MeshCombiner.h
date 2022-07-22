#ifndef MESHCOMBINER
#define MESHCOMBINER

#include <ISceneManager.h>
#include <IAnimatedMesh.h>

using namespace irr;

void combineMeshes(scene::ISkinnedMesh* newMesh, scene::IAnimatedMesh *addition, bool preserveBones);


#endif // MESHCOMBINER

