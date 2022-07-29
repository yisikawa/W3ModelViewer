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

void setMaterialsSettings(scene::IAnimatedMeshSceneNode* node);
bool loadRig(IrrlichtDevice* device, scene::IAnimatedMeshSceneNode* _current_node, const io::path filename);
bool loadAnims(IrrlichtDevice* device, scene::IAnimatedMeshSceneNode* _current_node, const io::path filename);
void enableRigging(scene::IAnimatedMeshSceneNode* node, bool enabled);

//Some global variables 
IrrlichtDevice* gDevice = nullptr;
scene::IAnimatedMeshSceneNode* gModel = nullptr;
scene::ICameraSceneNode* gCamera = nullptr;
core::stringc gStartUpModelFile;
core::stringw gMessageText;
core::stringw gCaption;
IO_MeshLoader_W3ENT* gW3ENT;
const core::stringc gGamePath = "Z:/uncooked/";
const core::stringc gTexPath = "Z:/uncooked/";
core::stringc gStartUpEnt = "Z:/uncooked/characters/models/background_npc/novigrad_citizen/novigrad_background_man_01.w2ent";
core::stringc gStartUpRig = "Z:/uncooked/characters/base_entities/man_base/man_base.w2rig";
core::stringc gStartUpAnim = "Z:/uncooked/animations/man/community/man_carry_items.w2anims";


// For the gui id's
enum EGUI_IDS
{
	GUI_ID_OPEN_ENT  = 1,
	GUI_ID_OPEN_RIG  = 2,
	GUI_ID_OPEN_ANIM = 3,
	GUI_ID_QUIT,
	GUI_ID_MAX
};

void loadModel(const c8* fn)
{
	// modify the name if it a .pk3 file

	io::path filename(fn);

	io::path extension;
	core::getFileNameExtension(extension, filename);
	extension.make_lower();

	// if a texture is loaded apply it to the current model..
	if (extension == ".w2ent"|| extension == ".w2mesh" )
	{
		gW3ENT->clear();
		TW3_DataCache::_instance.clear();
		io::IFileSystem* fs = gDevice->getFileSystem();
		io::IReadFile* file = fs->createAndOpenFile(io::path(fn));
		IAnimatedMesh* mesh = gW3ENT->createMesh(file);
		if (mesh)
			gModel->remove();
		gModel = gDevice->getSceneManager()->addAnimatedMeshSceneNode(mesh);
		if (gModel)
		{
			gModel->setScale(core::vector3df(30.f, 30.f, 30.f));
			gModel->setRotation(core::vector3df(gModel->getRotation().X, gModel->getRotation().Y - 90, gModel->getRotation().Z - 90));
			setMaterialsSettings(gModel);
			gCamera->setPosition(core::vector3df(0.f, 30.f, -40.f));
			core::vector3df target = gModel->getPosition(); target.Y += 30.;
			gCamera->setTarget(target);
			//	loadMeshPostProcess();
		}
		return;
	}
	else if (extension == ".w2rig")
	{
		gW3ENT->Skeleton.clear();
		TW3_DataCache::_instance.clear();
		loadRig(gDevice, gModel, io::path(fn));
		enableRigging(gModel, true);
		return;
	}
	else if (extension == ".w2anims")
	{
		TW3_DataCache::_instance.clear();
		loadAnims(gDevice, gModel, io::path(fn));
		return;
	}
}

class MyEventReceiver : public IEventReceiver
{
public:
	virtual bool OnEvent(const SEvent& event)
	{
		if (event.EventType == EET_GUI_EVENT)
		{
			s32 id = event.GUIEvent.Caller->getID();
			IGUIEnvironment* env = gDevice->getGUIEnvironment();

			switch (event.GUIEvent.EventType)
			{
			case EGET_MENU_ITEM_SELECTED:
				// a menu item was clicked
				OnMenuItemSelected((IGUIContextMenu*)event.GUIEvent.Caller);
				break;

			case EGET_FILE_SELECTED:
				{
					// load the model file, selected in the file open dialog
					IGUIFileOpenDialog* dialog =
						(IGUIFileOpenDialog*)event.GUIEvent.Caller;
					loadModel(core::stringc(dialog->getFileName()).c_str());
				}
				break;
			default:
				break;
			}
		}

		return false;
	}

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
			env->addFileOpenDialog(L"Please select a model file to open",true,0,-1,false,(irr::c8*)gGamePath.c_str());
			break;
		case GUI_ID_OPEN_RIG: // File -> Set Model Archive
			env->addFileOpenDialog(L"Please select a Rig file to open", true, 0, -1, false, (irr::c8*)gGamePath.c_str());
			break;
		case GUI_ID_OPEN_ANIM: // File -> LoadAsOctree
			env->addFileOpenDialog(L"Please select a Animation file to open", true, 0, -1, false, (irr::c8*)gGamePath.c_str());
			break;
		case GUI_ID_QUIT: // File -> Quit
			gDevice->closeDevice();
			break;
		}
	}
};

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

	MyEventReceiver receiver;
    gDevice = createDevice(video::EDT_DIRECT3D9, core::dimension2d<u32>(800, 600),
		16,false,false,false,&receiver);
    if (gDevice == 0)
        return 1; // could not create selected driver
	gDevice->setResizable(true);
	gDevice->getSceneManager()->getParameters()->setAttribute("TW_GAME_PATH", "Z:/uncooked/");
	gDevice->getSceneManager()->getParameters()->setAttribute("TW_TW3_TEX_PATH", "Z:/uncooked/");
	gDevice->getSceneManager()->getParameters()->setAttribute("TW_TW3_LOAD_SKEL", true);
	gDevice->getSceneManager()->getParameters()->setAttribute("TW_TW3_LOAD_BEST_LOD_ONLY", true);

	io::IFileSystem* fs = gDevice->getFileSystem();
	video::IVideoDriver* driver = gDevice->getVideoDriver();
	IGUIEnvironment* env = gDevice->getGUIEnvironment();
	scene::ISceneManager* smgr = gDevice->getSceneManager();
// set a nicer font
	IGUISkin* skin = env->getSkin();
	IGUIFont* font = env->getFont("../fonthaettenschweiler.bmp");
	if (font)
		skin->setFont(font);
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

    gW3ENT = new IO_MeshLoader_W3ENT(smgr, fs);
	io::IReadFile* file = fs->createAndOpenFile(io::path(gStartUpEnt));
	IAnimatedMesh* mesh = gW3ENT->createMesh(file);


	gModel = gDevice->getSceneManager()->addAnimatedMeshSceneNode(mesh);
	if (gModel)
	{
		gModel->setScale(core::vector3df(30.f, 30.f, 30.f));
		gModel->setRotation(core::vector3df(gModel->getRotation().X, gModel->getRotation().Y-90, gModel->getRotation().Z-90));
		setMaterialsSettings(gModel);
		//	loadMeshPostProcess();
	}
	loadRig(gDevice, gModel, gStartUpRig);
	enableRigging(gModel, true);
	loadAnims(gDevice, gModel, gStartUpAnim);

	gCamera = gDevice->getSceneManager()->addCameraSceneNodeMaya(nullptr);
	gCamera->setPosition(core::vector3df(0.f, 30.f, -40.f));
	core::vector3df target = gModel->getPosition(); target.Y += 30.;
	gCamera->setTarget(target);
	const f32 aspectRatio = static_cast<float>(800) / 600;
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
	delete gW3ENT;
	gDevice->drop();
}
