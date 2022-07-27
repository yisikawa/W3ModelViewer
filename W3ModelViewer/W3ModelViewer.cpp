// W3ModelViewer.cpp : このファイルには 'main' 関数が含まれています。プログラム実行の開始と終了がそこで行われます。
//

#include <iostream>
#include <Irrlicht.h>
#include <irrString.h>
#include <irrArray.h>
#include <IReadFile.h>
#include <IFileSystem.h>
#include "../IO_MeshLoader_W3ENT.h"
#include "../MeshCombiner.h"

using namespace irr;
using namespace scene;


#ifdef _IRR_WINDOWS_
#pragma comment(lib, "Irrlicht.lib")
#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif

class MeshSize
{
public:
	static float _scaleFactor;
};
void enableWireframe(scene::IAnimatedMeshSceneNode* node,bool enabled)
{
	if (node)
		node->setMaterialFlag(video::EMF_WIREFRAME, enabled);
}

void enableRigging(scene::IAnimatedMeshSceneNode* node,bool enabled)
{
	if (!node)
		return;

	if (enabled)
		node->setDebugDataVisible(scene::EDS_SKELETON);
	else
		node->setDebugDataVisible(scene::EDS_OFF);

}

unsigned int getJointsCount(scene::IAnimatedMeshSceneNode* node)
{
	if (node)
		return node->getJointCount();
	return 0;
}

core::vector3df getMeshDimensions(scene::IAnimatedMeshSceneNode* node)
{
	if (node)
		return node->getMesh()->getBoundingBox().MaxEdge - node->getMesh()->getBoundingBox().MinEdge;
	return core::vector3df(0.f, 0.f, 0.f);
}

void changeOptions(scene::ICameraSceneNode* camera, f32 cameraSpeed, f32 cameraRotationSpeed)
{ 
	core::list<scene::ISceneNodeAnimator*> anims = camera->getAnimators();
	core::list<scene::ISceneNodeAnimator*>::Iterator it;
	for (it = anims.begin(); it != anims.end(); it++)
	{
		if ((*it)->getType() == scene::ESNAT_CAMERA_MAYA)
		{
			break;
		}
	}
	scene::ISceneNodeAnimatorCameraMaya* anim = (scene::ISceneNodeAnimatorCameraMaya*)(*it);
	anim->setMoveSpeed(cameraSpeed);
	anim->setRotateSpeed(cameraRotationSpeed);
}

void setMaterialsSettings(scene::IAnimatedMeshSceneNode* node)
{
	// materials with normal maps are not handled
	for (u32 i = 0; i < node->getMaterialCount(); ++i)
	{
		video::SMaterial& material = node->getMaterial(i);
		if (material.MaterialType == video::EMT_NORMAL_MAP_SOLID
			|| material.MaterialType == video::EMT_PARALLAX_MAP_SOLID)
		{
			material.MaterialType = video::EMT_SOLID;
		}
		else if (material.MaterialType == video::EMT_NORMAL_MAP_TRANSPARENT_ADD_COLOR
			|| material.MaterialType == video::EMT_PARALLAX_MAP_TRANSPARENT_ADD_COLOR)
		{
			material.MaterialType = video::EMT_TRANSPARENT_ADD_COLOR;
		}
		else if (material.MaterialType == video::EMT_NORMAL_MAP_TRANSPARENT_VERTEX_ALPHA
			|| material.MaterialType == video::EMT_PARALLAX_MAP_TRANSPARENT_VERTEX_ALPHA)
		{
			material.MaterialType = video::EMT_TRANSPARENT_VERTEX_ALPHA;
		}
	}

	node->setMaterialFlag(video::EMF_LIGHTING, false);
	node->setMaterialFlag(video::EMF_BACK_FACE_CULLING, false);

	for (u32 i = 1; i < _IRR_MATERIAL_MAX_TEXTURES_; ++i)
		node->setMaterialTexture(i, nullptr);
}

bool loadRig(IrrlichtDevice* device, scene::IAnimatedMeshSceneNode* _current_node, const io::path filename)
{
	io::IReadFile* file = device->getFileSystem()->createAndOpenFile(filename);
	if (!file)
	{
		return false;
	}

	scene::IO_MeshLoader_W3ENT loader(device->getSceneManager(), device->getFileSystem());
	scene::IAnimatedMesh* mesh = loader.createMesh(file);
	file->drop();

	if (mesh)
		mesh->drop();

	TW3_CSkeleton skeleton = loader.Skeleton;

	scene::ISkinnedMesh* newMesh = copySkinnedMesh(device->getSceneManager(), _current_node->getMesh(), false);

	bool success = skeleton.applyToModel(newMesh);
	if (success)
	{
		// Apply the skinning
		TW3_DataCache::_instance.setOwner(newMesh);
		TW3_DataCache::_instance.apply();
	}

	newMesh->setDirty();
	newMesh->finalize();

	_current_node->setMesh(newMesh);

	setMaterialsSettings(_current_node);
	return success;
}

bool loadAnims(IrrlichtDevice* device, scene::IAnimatedMeshSceneNode* _current_node, const io::path filename)
{
	io::IReadFile* file = device->getFileSystem()->createAndOpenFile(filename);
	if (!file)
	{
		return false;
	}

	scene::IO_MeshLoader_W3ENT loader(device->getSceneManager(), device->getFileSystem());

	scene::ISkinnedMesh* newMesh = copySkinnedMesh(device->getSceneManager(), _current_node->getMesh(), true);

	// use the loader to add the animation to the new model
	loader.meshToAnimate = newMesh;
	scene::IAnimatedMesh* mesh = loader.createMesh(file);
	file->drop();

	if (mesh)
		mesh->drop();


	newMesh->setDirty();
	newMesh->finalize();

	_current_node->setMesh(newMesh);

	setMaterialsSettings(_current_node);

	return true;
}

/*
void QIrrlichtWidget::loadMeshPostProcess()
{
	const scene::IAnimatedMesh* mesh = _currentLodData->_node->getMesh();

	MeshSize::_scaleFactor = 1.f;

	// Save the path of normals/specular maps
	_currentLodData->_additionalTextures.resize(mesh->getMeshBufferCount());
	for (u32 i = 0; i < mesh->getMeshBufferCount(); ++i)
	{
		const video::SMaterial material = mesh->getMeshBuffer(i)->getMaterial();
		_currentLodData->_additionalTextures[i].resize(_IRR_MATERIAL_MAX_TEXTURES_ - 1);
		for (u32 j = 1; j < _IRR_MATERIAL_MAX_TEXTURES_; ++j)
		{
			QString texturePath = QString();
			const video::ITexture* texture = material.getTexture(j);
			if (texture)
				texturePath = irrPathToQString(texture->getName().getPath());

			_currentLodData->_additionalTextures[i][j - 1] = texturePath;
		}
	}

	setMaterialsSettings(_currentLodData->_node);
}
*/



int main()
{
    RedEngineFileHeader header;

    video::E_DRIVER_TYPE driverType;

    driverType = video::EDT_DIRECT3D9;
    IrrlichtDevice* device =
        createDevice(driverType, core::dimension2d<u32>(640, 480));
    if (device == 0)
        return 1; // could not create selected driver.
	device->getSceneManager()->getParameters()->setAttribute("TW_GAME_PATH","Z:/uncooked/");
	device->getSceneManager()->getParameters()->setAttribute("TW_TW3_TEX_PATH", "Z:/uncooked/");
	device->getSceneManager()->getParameters()->setAttribute("TW_TW3_LOAD_SKEL", true);
	device->getSceneManager()->getParameters()->setAttribute("TW_TW3_LOAD_BEST_LOD_ONLY", true);

	IAnimatedMeshSceneNode* node = nullptr;
    io::IFileSystem *fs = device->getFileSystem();
    video::IVideoDriver* driver = device->getVideoDriver();
    scene::ISceneManager* smgr = device->getSceneManager();
    IO_MeshLoader_W3ENT w3ent(smgr, fs);

	io::IReadFile* file = fs->createAndOpenFile(io::path("Z:/uncooked/characters/models/animals/cat/t_01__cat.w2ent"));
	IAnimatedMesh* mesh = w3ent.createMesh(file);


	node = device->getSceneManager()->addAnimatedMeshSceneNode(mesh);
	if (node)
	{
		node->setScale(core::vector3df(20, 20, 20));
		node->setRotation(core::vector3df(node->getRotation().X, node->getRotation().Y - 90, node->getRotation().Z));
		setMaterialsSettings(node);
		//	loadMeshPostProcess();
	}
	loadRig(device, node, "Z:/uncooked/characters/base_entities/cat_base/cat_base.w2rig");
	enableRigging(node, true);
	// loadAnims(device, node, "Z:/uncooked/animations/animals/cat/cat_animation.w2anims");

	scene::ICameraSceneNode* camera;
	camera = device->getSceneManager()->addCameraSceneNodeMaya(nullptr);
	camera->setPosition(core::vector3df(0.f, 30.f, -40.f));
	camera->setTarget(node->getPosition());
	const f32 aspectRatio = static_cast<float>(640) / 480;
	camera->setAspectRatio(aspectRatio);
	camera->setFarValue(10000.f);

	while (device->run())
	{
		driver->beginScene(true, true, video::SColor(255, 100, 101, 140));

		smgr->drawAll();

		driver->endScene();
	}

	device->drop();
}
