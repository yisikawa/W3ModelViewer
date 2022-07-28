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
using namespace gui;


#ifdef _IRR_WINDOWS_
#pragma comment(lib, "Irrlicht.lib")
#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif

//Some global variables 
IrrlichtDevice* gDevice = nullptr;
scene::IAnimatedMeshSceneNode* gModel = nullptr;
scene::ICameraSceneNode* gCamera = nullptr;
core::stringc gStartUpModelFile;
core::stringw gMessageText;
core::stringw gCaption;



// For the gui id's
enum EGUI_IDS
{
	GUI_ID_OPEN_ENT  = 1,
	GUI_ID_OPEN_RIG  = 2,
	GUI_ID_OPEN_ANIM = 3,
	GUI_ID_QUIT,
	GUI_ID_MAX
};

/*
	Handle "menu item clicked" events.
*/
void OnMenuItemSelected(IGUIContextMenu* menu)
{
	s32 id = menu->getItemCommandId(menu->getSelectedItem());
	IGUIEnvironment* env = gDevice->getGUIEnvironment();

	switch (id)
	{
	case GUI_ID_OPEN_ENT: // FilOnButtonSetScalinge -> Open Model
		env->addFileOpenDialog(L"Please select a model file to open");
		break;
	case GUI_ID_OPEN_RIG: // File -> Set Model Archive
		env->addFileOpenDialog(L"Please select a Rig file to open");
		break;
	case GUI_ID_OPEN_ANIM: // File -> LoadAsOctree
		env->addFileOpenDialog(L"Please select a Animation file to open");
		break;
	case GUI_ID_QUIT: // File -> Quit
		gDevice->closeDevice();
		break;
	}
}

class MeshSize
{
public:
	float _scaleFactor = 1.f;
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

void changeOptions(scene::ICameraSceneNode* camera, f32 cameraSpeed, 
	f32 cameraRotationSpeed)
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
	const scene::IAnimatedMesh* mesh = _currentLodData->_node->getMesh();_scaleFactor
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
	core::stringc fileNPCEnt = "Z:/uncooked/characters/models/background_npc/novigrad_citizen/novigrad_background_man_01.w2ent";
	core::stringc fileGardEnt = "Z:/uncooked/characters/models/background_npc/nilfgaard_knight/nilfgaard_knight_background_man_01.w2ent";
	core::stringc fileCatEnt = "Z:/uncooked/characters/models/animals/cat/t_01__cat.w2ent";
	core::stringc fileCatMesh = "Z:/uncooked/characters/models/animals/cat/model/t_01__cat.w2mesh";
	core::stringc fileCatRig = "Z:/uncooked/characters/base_entities/cat_base/cat_base.w2rig";
	core::stringc fileGardRig = "Z:uncooked/characters/base_entities/man_background_base/soldier_background_base.w2rig";
	core::stringc fileNPCRig = "Z:/uncooked/characters/base_entities/man_base/man_base.w2rig";
	core::stringc fileCatAnim = "Z:/uncooked/animations/animals/cat/cat_animation.w2anims";
	core::stringc fileGardAnim = "Z:/uncooked/animations/background_npc/background_knights/background_knights_animation.w2anims";
	core::stringc fileNPCAnim = "Z:/uncooked/animations/man/community/man_carry_items.w2anims";

    gDevice = createDevice(video::EDT_DIRECT3D9, core::dimension2d<u32>(800, 600));
    if (gDevice == 0)
        return 1; // could not create selected driver
	gDevice->setResizable(true);
	gDevice->getSceneManager()->getParameters()->setAttribute("TW_GAME_PATH","Z:/uncooked/");
	gDevice->getSceneManager()->getParameters()->setAttribute("TW_TW3_TEX_PATH", "Z:/uncooked/");
	gDevice->getSceneManager()->getParameters()->setAttribute("TW_TW3_LOAD_SKEL", true);
	gDevice->getSceneManager()->getParameters()->setAttribute("TW_TW3_LOAD_BEST_LOD_ONLY", true);

	io::IFileSystem* fs = gDevice->getFileSystem();
	video::IVideoDriver* driver = gDevice->getVideoDriver();
	IGUIEnvironment* env = gDevice->getGUIEnvironment();
	scene::ISceneManager* smgr = gDevice->getSceneManager();
// create menu
	gui::IGUIContextMenu* menu = env->addMenu();
	menu->addItem(L"File", -1, true, true);
//	menu->addItem(L"View", -1, true, true);
	gui::IGUIContextMenu* submenu;
	submenu = menu->getSubMenu(0);
	submenu->addItem(L"Open Model File ...", GUI_ID_OPEN_ENT);
	submenu->addItem(L"Open Rig File ...", GUI_ID_OPEN_RIG);
	submenu->addItem(L"Open Anim File ...", GUI_ID_OPEN_ANIM);
	submenu->addSeparator();
	submenu->addItem(L"Quit", GUI_ID_QUIT);

	// create toolbar
	gui::IGUIToolBar* bar = env->addToolBar();
	IGUIStaticText* fpstext = env->addStaticText(L"",
		core::rect<s32>(400, 4, 570, 23), true, false, bar);
	gCaption += " - [";
	gCaption += driver->getName();
	gCaption += "]";
	gDevice->setWindowCaption(gCaption.c_str());

    IO_MeshLoader_W3ENT* w3ent = new IO_MeshLoader_W3ENT(smgr, fs);
	io::IReadFile* file = fs->createAndOpenFile(io::path(fileNPCEnt));
	IAnimatedMesh* mesh = w3ent->createMesh(file);


	gModel = gDevice->getSceneManager()->addAnimatedMeshSceneNode(mesh);
	if (gModel)
	{
		gModel->setScale(core::vector3df(30.f, 30.f, 30.f));
		gModel->setRotation(core::vector3df(gModel->getRotation().X, gModel->getRotation().Y-90, gModel->getRotation().Z-90));
		setMaterialsSettings(gModel);
		//	loadMeshPostProcess();
	}
	loadRig(gDevice, gModel, fileNPCRig);
	enableRigging(gModel, true);
	loadAnims(gDevice, gModel, fileNPCAnim);

	gCamera = gDevice->getSceneManager()->addCameraSceneNodeMaya(nullptr);
	gCamera->setPosition(core::vector3df(0.f, 30.f, -40.f));
	core::vector3df target = gModel->getPosition(); target.Y += 30.;
	gCamera->setTarget(target);
	const f32 aspectRatio = static_cast<float>(640) / 480;
	gCamera->setAspectRatio(aspectRatio);
	gCamera->setFarValue(1000.f);

	while (gDevice->run() && driver )
	{
		if (gDevice->isWindowActive())
		{
			driver->beginScene(true, true, video::SColor(255, 100, 101, 140));

			smgr->drawAll();
			env->drawAll();

			driver->endScene();
			// update information about current frame-rate
			core::stringw str(L"FPS: ");
			str.append(core::stringw(driver->getFPS()));
		}
		else
			gDevice->yield();
	}
	delete w3ent;
	gDevice->drop();
}
