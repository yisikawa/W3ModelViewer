// W3ModelViewer.cpp : このファイルには 'main' 関数が含まれています。プログラム実行の開始と終了がそこで行われます。
//
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <Irrlicht.h>
#include <irrString.h>
#include <irrArray.h>
#include <IReadFile.h>
#include <IFileSystem.h>
#include "../IO_MeshLoader_W3ENT.h"
#include "../MeshCombiner.h"
#include "../CGUIFileSaveDialog.h"
#include "../IrrAssimp.h"

using namespace irr;
using namespace scene;
using namespace gui;


#ifdef _IRR_WINDOWS_
#pragma comment(lib, "Irrlicht.lib")
#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif

void loadModel(const c8* fn, const c8* name);
bool addMesh(const c8* fn);
void setMaterialsSettings(scene::IAnimatedMeshSceneNode* node);
bool loadRig(IrrlichtDevice* device, scene::IAnimatedMeshSceneNode* _current_node, const io::path filename);
bool loadAnims(IrrlichtDevice* device, scene::IAnimatedMeshSceneNode* _current_node, const io::path filename);
void enableRigging(scene::IAnimatedMeshSceneNode* node, bool enabled);
std::string GetConfigString(const std::string& filePath, const char* pSectionName, const char* pKeyName);
void ExportModel(IrrlichtDevice* device, scene::IAnimatedMeshSceneNode* _current_node, const io::path directory,
	const io::path filename, struct ExporterInfos exporter);
//Some global variables 
IrrlichtDevice* gDevice = nullptr;
scene::IAnimatedMeshSceneNode* gModel = nullptr;
scene::ICameraSceneNode* gCamera = nullptr;
core::stringc gStartUpModelFile;
core::stringw gMessageText;
core::stringw gCaption;
IO_MeshLoader_W3ENT* gW3ENT;
core::stringc gGamePath = "";
core::stringc gExportPath = "";
core::stringc gTexPath = "";
core::array<struct ExporterInfos> gExporters;

struct ModelList
{
	ModelList() : 
		modelName(core::stringc()),
		meshFiles(core::array<core::stringc>()),
		rigFiles(core::array<core::stringc>()),
		animFiles(core::array<core::stringc>())
	{}
	core::stringc modelName;
	core::array<core::stringc> meshFiles;
	core::array<core::stringc> rigFiles;
	core::array<core::stringc> animFiles;
};

core::array<struct ModelList> gAnimals,gMonsters,gBackgrounds,gMainNpcs,gSecondNpcs,gGeralt;

// For the gui id's
enum 
{
	GUI_ID_LOAD_ENT,
	GUI_ID_LOAD_RIG,
	GUI_ID_LOAD_ANIM,
	GUI_ID_ADD_MESH,
	GUI_ID_EXPORT_DAE,
	GUI_ID_EXPORT_X,
	GUI_ID_EXPORT_OBJ_MAT,
	GUI_ID_EXPORT_OBJ_NOMAT,
	GUI_ID_EXPORT_3DS,
	GUI_ID_EXPORT_FBX_BIN,
	GUI_ID_EXPORT_FBX_ASC,
	GUI_ID_QUIT,
	GUI_ID_ANIMALS_LIST,
	GUI_ID_MONSTERS_LIST,
	GUI_ID_BACKGROUNDS_LIST,
	GUI_ID_MAIN_NPCS_LIST,
	GUI_ID_SECOND_NPCS_LIST,
	GUI_ID_GERALT_LIST,
	GUI_ID_MAX
};
struct IrrlichtExporterInfos
{
	IrrlichtExporterInfos(scene::EMESH_WRITER_TYPE irrExporter = scene::EMWT_OBJ, s32 irrFlags = scene::EMWF_NONE)
		: _irrExporter(irrExporter)
		, _irrFlags(irrFlags)
	{}

	scene::EMESH_WRITER_TYPE _irrExporter;
	s32 _irrFlags;
};

enum ExportType
{
	Exporter_Irrlicht,
	Exporter_Redkit,
	Exporter_Assimp
};

struct ExporterInfos
{
	ExportType _exporterType;
	core::stringc _exporterName;
	core::stringc _extension;

	// Assimp specifics
	core::stringc _assimpExporterId;

	// Irrlicht specifics
	IrrlichtExporterInfos _irrlichtInfos;
};



void registerExporters()
{
	core::array<core::stringc> noAssimpExportExtensions;
	noAssimpExportExtensions.push_back(".x");
	noAssimpExportExtensions.push_back(".obj");
	noAssimpExportExtensions.push_back(".dae");
	noAssimpExportExtensions.push_back(".fbx");
	noAssimpExportExtensions.push_back(".3ds");
	noAssimpExportExtensions.sort();
	gExporters.clear();

	//core::array<core::stringc> noAssimpExportExtensions;
	//for (int i = 0; i < gExporters.size(); ++i)
	//{
	//	const core::stringc extension = gExporters[i]._extension;
	//	noAssimpExportExtensions.push_back(extension);
	//}

	core::array<struct ExportFormat> formats = IrrAssimp::getExportFormats();
	for (u32 i = 0; i < formats.size(); ++i)
	{
		const struct ExportFormat format = formats[i];
		const core::stringc extension = core::stringc(".") + format.fileExtension.c_str();
		if (noAssimpExportExtensions.binary_search(extension)!=-1)
		{
			const core::stringc exportString = extension + " by Assimp library (" + format.description.c_str() + ")";
			gExporters.push_back({ Exporter_Assimp, exportString, extension, format.id.c_str(), IrrlichtExporterInfos() });
		}
	}
	for (u32 i = 0; i < gExporters.size(); ++i)
	{
		core::stringc str = gExporters[i]._exporterName;
	}
	return;
}

class MyEventReceiver : public IEventReceiver
{

public:
	;

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
					io::path extension;
					IGUIFileOpenDialog* dialog =(IGUIFileOpenDialog*)event.GUIEvent.Caller;
//					CGUIFileSaveDialog* dialog2 = (CGUIFileSaveDialog*)event.GUIEvent.Caller;
					core::getFileNameExtension(extension, core::stringc(dialog->getFileName()).c_str());
					extension.make_lower();
					core::stringc str0 = core::stringc(dialog->getFileName()).c_str();
					core::stringc str = env->getFileSystem()->getFileDir(str0) + "/";
					// load the model file, selected in the file open dialog
					switch (dialog->getID())
					{
					case GUI_ID_LOAD_ENT:
						if (extension == ".w2ent" || extension == ".w2mesh" )
						{
							loadModel(core::stringc(dialog->getFileName()).c_str(),"");
						}
						break;
					case GUI_ID_LOAD_RIG:
						if (extension == ".w2rig")
						{
							gW3ENT->Skeleton.clear();
							loadRig(gDevice, gModel, io::path(core::stringc(dialog->getFileName()).c_str()));
							enableRigging(gModel, true);
						}
						break;
					case GUI_ID_LOAD_ANIM:
						if (extension == ".w2anims")
						{
							loadAnims(gDevice, gModel, io::path(core::stringc(dialog->getFileName()).c_str()));
						}
						break;
					case GUI_ID_ADD_MESH:
						if (extension == ".w2ent" || extension == ".w2mesh")
						{
							addMesh(core::stringc(dialog->getFileName()).c_str());
						}
						break;
					case GUI_ID_EXPORT_DAE:
						ExportModel(gDevice, gModel,	str,str0, gExporters[0]);
						break;
					case GUI_ID_EXPORT_X:
						ExportModel(gDevice, gModel, str, str0, gExporters[1]);
						break;
					case GUI_ID_EXPORT_OBJ_MAT:
						ExportModel(gDevice, gModel, str, str0, gExporters[2]);
						break;
					case GUI_ID_EXPORT_OBJ_NOMAT:
						ExportModel(gDevice, gModel, str, str0, gExporters[3]);
						break;
					case GUI_ID_EXPORT_3DS:
						ExportModel(gDevice, gModel, str, str0, gExporters[4]);
						break;
					case GUI_ID_EXPORT_FBX_BIN:
						ExportModel(gDevice, gModel, str, str0, gExporters[5]);
						break;
					case GUI_ID_EXPORT_FBX_ASC:
						ExportModel(gDevice, gModel, str, str0, gExporters[6]);
						break;
					}

				}
				break;

			case EGET_COMBO_BOX_CHANGED:

				// control anti-aliasing/filtering
				if (id == GUI_ID_ANIMALS_LIST)
				{
					OnAnimalsListSelected((IGUIComboBox*)event.GUIEvent.Caller);
				}
				else if (id == GUI_ID_MONSTERS_LIST)
				{
					OnMonstersListSelected((IGUIComboBox*)event.GUIEvent.Caller);
				}
				else if (id == GUI_ID_BACKGROUNDS_LIST)
				{
					OnBackgroundsListSelected((IGUIComboBox*)event.GUIEvent.Caller);
				}
				else if (id == GUI_ID_MAIN_NPCS_LIST)
				{
					OnMainNpcsListSelected((IGUIComboBox*)event.GUIEvent.Caller);
				}
				else if (id == GUI_ID_SECOND_NPCS_LIST)
				{
					OnSecondNpcsListSelected((IGUIComboBox*)event.GUIEvent.Caller);
				}
				else if (id == GUI_ID_GERALT_LIST)
				{
					OnGeraltListSelected((IGUIComboBox*)event.GUIEvent.Caller);
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
		CGUIFileSaveDialog* dialog;
		s32 id = menu->getItemCommandId(menu->getSelectedItem());
		IGUIEnvironment* env = gDevice->getGUIEnvironment();


		switch (id)
		{
		case GUI_ID_LOAD_ENT: // FilOnButtonSetScalinge -> Open Model
			env->addFileOpenDialog(L"Please select a model file to load",true,0,
				GUI_ID_LOAD_ENT,false,(irr::c8*)gGamePath.c_str());
			break;
		case GUI_ID_LOAD_RIG: // File -> Set Model Archive
			env->addFileOpenDialog(L"Please select a Rig file to load", true, 0,
				GUI_ID_LOAD_RIG, false, (irr::c8*)gGamePath.c_str());
			break;
		case GUI_ID_LOAD_ANIM: // File -> LoadAsOctree
			env->addFileOpenDialog(L"Please select a Animation file to load", true, 0,
				GUI_ID_LOAD_ANIM, false, (irr::c8*)gGamePath.c_str());
			break;
		case GUI_ID_ADD_MESH: // File -> LoadAsOctree
			env->addFileOpenDialog(L"Please select a Mesh file to add", true, 0,
				GUI_ID_ADD_MESH, false, (irr::c8*)gGamePath.c_str());
			break;
		case GUI_ID_EXPORT_DAE: // File -> LoadAsOctree
			dialog = new CGUIFileSaveDialog(L"Export COLLADA Files",
				env, env->getRootGUIElement(), GUI_ID_EXPORT_DAE, (irr::c8*)gExportPath.c_str());
			dialog->drop();
			break;
		case GUI_ID_EXPORT_X: // File -> LoadAsOctree
			dialog = new CGUIFileSaveDialog(L"Export X Files",
				env, env->getRootGUIElement(), GUI_ID_EXPORT_X, (irr::c8*)gExportPath.c_str());
			dialog->drop();
			break;
		case GUI_ID_EXPORT_OBJ_MAT: // File -> LoadAsOctree
			dialog = new CGUIFileSaveDialog(L"Export Wavefront OBJ format",
				env, env->getRootGUIElement(), GUI_ID_EXPORT_OBJ_MAT, (irr::c8*)gExportPath.c_str());
			dialog->drop();
		break;
		case GUI_ID_EXPORT_OBJ_NOMAT: // File -> LoadAsOctree
			dialog = new CGUIFileSaveDialog(L"Export Wavefront OBJ format without material files",
				env, env->getRootGUIElement(), GUI_ID_EXPORT_OBJ_NOMAT, (irr::c8*)gExportPath.c_str());
			dialog->drop();
			break;
		case GUI_ID_EXPORT_3DS: // File -> LoadAsOctree
			dialog = new CGUIFileSaveDialog(L"Export Autodesk 3DS (legacy)",
				env, env->getRootGUIElement(), GUI_ID_EXPORT_3DS, (irr::c8*)gExportPath.c_str());
			dialog->drop();
		break;
		case GUI_ID_EXPORT_FBX_BIN: // File -> LoadAsOctree
			dialog = new CGUIFileSaveDialog(L"Export Autodesk FBX (binary)",
				env, env->getRootGUIElement(), GUI_ID_EXPORT_FBX_BIN, (irr::c8*)gExportPath.c_str());
			dialog->drop();
			break;
		case GUI_ID_EXPORT_FBX_ASC: // File -> LoadAsOctree
			dialog = new CGUIFileSaveDialog(L"Export Autodesk FBX (ascii)",
				env, env->getRootGUIElement(), GUI_ID_EXPORT_FBX_ASC, (irr::c8*)gExportPath.c_str());
			dialog->drop();
			break;
		case GUI_ID_QUIT: // File -> Quit
			gDevice->closeDevice();
			break;
		}
	}

	/*
		Handle the event that one of the texture-filters was selected in the corresponding combobox.
	*/
	void OnAnimalsListSelected(IGUIComboBox* combo)
	{
		s32 pos = combo->getSelected();
		if (pos <= 0) return;
		if (gAnimals[pos - 1].meshFiles.size() >= 1) {
			core::stringc file = gGamePath + gAnimals[pos - 1].meshFiles[0];
			loadModel(file.c_str(), gAnimals[pos - 1].modelName.c_str());
			for (u32 i = 1; i < gAnimals[pos - 1].meshFiles.size(); i++)
			{
				core::stringc file = gGamePath + gAnimals[pos - 1].meshFiles[i];
				addMesh(file.c_str());
			}
		}
		if(gAnimals[pos - 1].rigFiles.size()>=1 )
		{
			core::stringc file = gGamePath + gAnimals[pos - 1].rigFiles[0];
			gW3ENT->Skeleton.clear();
			loadRig(gDevice, gModel, io::path(file));
		}
		if (gAnimals[pos - 1].animFiles.size() >= 1)
		{
			core::stringc file = gGamePath + gAnimals[pos - 1].animFiles[0];
			loadAnims(gDevice, gModel, io::path(file));

		}
	}

	void OnMonstersListSelected(IGUIComboBox* combo)
	{
		s32 pos = combo->getSelected();
		if (pos <= 0) return;
		if (gMonsters[pos - 1].meshFiles.size() >= 1) {
			core::stringc file = gGamePath + gMonsters[pos - 1].meshFiles[0];
			loadModel(file.c_str(), gMonsters[pos - 1].modelName.c_str());
			for (u32 i = 1; i < gMonsters[pos - 1].meshFiles.size(); i++)
			{
				core::stringc file = gGamePath + gMonsters[pos - 1].meshFiles[i];
				addMesh(file.c_str());
			}
		}
	}

	void OnBackgroundsListSelected(IGUIComboBox* combo)
	{
		s32 pos = combo->getSelected();
		if (pos <= 0) return;
		if (gBackgrounds[pos - 1].meshFiles.size() >= 1) {
			core::stringc file = gGamePath + gBackgrounds[pos - 1].meshFiles[0];
			loadModel(file.c_str(),gBackgrounds[pos - 1].modelName.c_str());
			for (u32 i = 1; i < gBackgrounds[pos - 1].meshFiles.size(); i++)
			{
				core::stringc file = gGamePath + gBackgrounds[pos - 1].meshFiles[i];
				addMesh(file.c_str());
			}
		}
	}

	void OnMainNpcsListSelected(IGUIComboBox* combo)
	{
		s32 pos = combo->getSelected();
		if (pos <= 0) return;
		if (gMainNpcs[pos - 1].meshFiles.size() >= 1) {
			core::stringc file = gGamePath + gMainNpcs[pos - 1].meshFiles[0];
			loadModel(file.c_str(), gMainNpcs[pos - 1].modelName.c_str());
			for (u32 i = 1; i < gMainNpcs[pos - 1].meshFiles.size(); i++)
			{
				core::stringc file = gGamePath + gMainNpcs[pos - 1].meshFiles[i];
				addMesh(file.c_str());
			}
		}
	}

	void OnSecondNpcsListSelected(IGUIComboBox* combo)
	{
		s32 pos = combo->getSelected();
		if (pos <= 0) return;
		if (gSecondNpcs[pos - 1].meshFiles.size() >= 1) {
			core::stringc file = gGamePath + gSecondNpcs[pos - 1].meshFiles[0];
			loadModel(file.c_str(), gSecondNpcs[pos - 1].modelName.c_str());
			for (u32 i = 1; i < gSecondNpcs[pos - 1].meshFiles.size(); i++)
			{
				core::stringc file = gGamePath + gSecondNpcs[pos - 1].meshFiles[i];
				addMesh(file.c_str());
			}
		}
	}


	void OnGeraltListSelected(IGUIComboBox* combo)
	{
		s32 pos = combo->getSelected();
		if (pos <= 0) return;
		if (gGeralt[pos - 1].meshFiles.size() >= 1) {
			core::stringc file = gGamePath + gGeralt[pos - 1].meshFiles[0];
			loadModel(file.c_str(), gGeralt[pos - 1].modelName.c_str());
			for (u32 i = 1; i < gGeralt[pos - 1].meshFiles.size(); i++)
			{
				core::stringc file = gGamePath + gGeralt[pos - 1].meshFiles[i];
				addMesh(file.c_str());
			}
		}
		//if (gGeralt[pos - 1].rigFiles.size() >= 1)
		//{
		//	core::stringc file = gGamePath + gGeralt[pos - 1].rigFiles[0];
		//	gW3ENT->Skeleton.clear();
		//	loadRig(gDevice, gModel, io::path(file));
		//}
	}
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
//			material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
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
	//node->setMaterialFlag(video::EMF_FRONT_FACE_CULLING, false);
	//node->setMaterialFlag(video::EMF_BACK_FACE_CULLING, false);

	for (u32 i = 1; i < _IRR_MATERIAL_MAX_TEXTURES_; ++i)
		node->setMaterialTexture(i, nullptr);
}

void loadModel(const c8* fn, const c8* name)
{
	// modify the name if it a .pk3 file
	core::stringc modelName;
	io::path filename(fn);
	if (name == "")
	{
		s32 sep = filename.findLastChar("/");
		modelName = filename.subString(sep + 1, filename.size() - sep - 1);

		sep = modelName.findLastChar(".");
		if (sep > 0 )
			modelName = modelName.subString(0, sep);
		else
			modelName = modelName;
	}
	else
		modelName = name;
	io::path extension;
	core::getFileNameExtension(extension, filename);
	extension.make_lower();

	// if a texture is loaded apply it to the current model..
	if (extension != ".w2ent" && extension != ".w2mesh") return;
	if (gW3ENT) gW3ENT->clear();
	TW3_DataCache::_instance.clear();
	io::IFileSystem* fs = gDevice->getFileSystem();
	io::IReadFile* file = fs->createAndOpenFile(io::path(fn));
	IAnimatedMesh* mesh = gW3ENT->createMesh(file);
	if (mesh && gModel)
		gModel->remove();
	gModel = gDevice->getSceneManager()->addAnimatedMeshSceneNode(mesh);
	if (gModel)
	{
		gModel->setName(modelName);
		gModel->setScale(core::vector3df(15.f, 15.f, 15.f));
		gModel->setRotation(core::vector3df(gModel->getRotation().X, gModel->getRotation().Y - 90, gModel->getRotation().Z - 90));
		setMaterialsSettings(gModel);
		gCamera->setPosition(core::vector3df(0.f, 30.f, -40.f));
		core::vector3df target = gModel->getPosition(); target.Y += 10.;
		gCamera->setTarget(target);
		//	loadMeshPostProcess();
	}
}

bool addMesh(const c8* fn)
{
	// Clear the previous data
	TW3_DataCache::_instance.clear();

	io::path filename(fn);
	io::IFileSystem* fs = gDevice->getFileSystem();
	io::IReadFile* file = fs->createAndOpenFile(io::path(fn));
	IAnimatedMesh* mesh = gW3ENT->createMesh(file);
	if (mesh)
	{
		ISkinnedMesh* newMesh = copySkinnedMesh(gDevice->getSceneManager(), gModel->getMesh(), true);
		combineMeshes(newMesh, mesh, true);
		newMesh->finalize();
		gModel->setMesh(newMesh);
		setMaterialsSettings(gModel);
		//	loadMeshPostProcess();
		return true;
	}
	return false;
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



// Convert and copy a single texture
bool convertAndCopyTexture(const io::path texturePath, const io::path exportFolder, bool shouldCopyTextures, const io::path& outputTexturePath)
{
	video::IImage* image = gDevice->getVideoDriver()->createImageFromFile(texturePath);
	if (image)
	{
		gDevice->getVideoDriver()->writeImageToFile(image, outputTexturePath);
		image->drop();
	}
	return true;
}
// irr::scene::IMesh* mesh, irr::core::stringc format, irr::core::stringc path
// convert and copy the diffuse textures of a mesh
void convertAndCopyTextures(scene::IMesh* mesh, const io::path exportFolder, bool shouldCopyTextures)
{
	for (u32 i = 0; i < mesh->getMeshBufferCount(); ++i)
	{
		scene::IMeshBuffer* buffer = mesh->getMeshBuffer(i);
		video::ITexture* diffuseTexture = buffer->getMaterial().getTexture(0);
		if (diffuseTexture)
		{
			core::stringc texturePath = diffuseTexture->getName().getPath();
			s32 sep = texturePath.findLastChar("/");
			core::stringc filename = texturePath.subString(sep + 1, texturePath.size() - sep - 1);
			core::stringc outputTexturePath = exportFolder+filename;
			video::IImage* image = gDevice->getVideoDriver()->createImageFromFile(texturePath);
			if (image)
			{
				gDevice->getVideoDriver()->writeImageToFile(image, outputTexturePath);
				image->drop();
				// We apply the nex texture to the mesh, so the exported file will use it
				// TODO: Restore the original texture on the mesh after the export ?
				video::ITexture* tex = gDevice->getSceneManager()->getVideoDriver()->getTexture(outputTexturePath);
				buffer->getMaterial().setTexture(0, tex);
			}
		}
	}
}

void ExportModel(IrrlichtDevice* device, scene::IAnimatedMeshSceneNode* _current_node, const io::path directory,
	const io::path filename, struct ExporterInfos exporter)
{

	if (exporter._exporterType != Exporter_Redkit && (!_current_node || !_current_node->getMesh()))
		return ;
	core::stringc basename = filename;
	s32 sep = filename.findLastChar(".");
	if( sep>0 )
		basename = filename.subString(0,sep);
	const io::path exportMeshPath = basename + exporter._extension;
	const io::path exportFolderPath = directory;
	io::IWriteFile* file = gDevice->getFileSystem()->createAndWriteFile(exportMeshPath);
	if (!file)
		return;

	if (exporter._exporterType != Exporter_Redkit)
	{
		convertAndCopyTextures(_current_node->getMesh(), exportFolderPath, true);
	}


	if (exporter._exporterType == Exporter_Irrlicht)
	{
		scene::IMeshWriter* mw = nullptr;
		mw = gDevice->getSceneManager()->createMeshWriter(exporter._irrlichtInfos._irrExporter);
		if (mw)
		{
			mw->writeMesh(file, _current_node->getMesh(), exporter._irrlichtInfos._irrFlags);
			mw->drop();
		}

	}
	else
	{
		IrrAssimp assimp(gDevice->getSceneManager());
		assimp.exportMesh(_current_node->getMesh(), exporter._assimpExporterId.c_str(), exportMeshPath);

	}

	if (file)
		file->drop();

}

/*
class MeshSize
{
public:
	float _scaleFactor = 1.f;
};

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



void addModelList(core::array<struct ModelList> *list, core::stringc fileName)
{

	std::string str_buf;
	std::string str_split_buf;

	for (u32 i = 0; i < list->size(); i++)
		list[i].clear();
	list->clear();
	std::ifstream ifs_csv(fileName.c_str());
	while ( getline(ifs_csv, str_buf) )
	{
		struct ModelList item;
		std::istringstream i_stream(str_buf);
		getline(i_stream, str_split_buf, ',');
		item.modelName = str_split_buf.c_str();
		while (getline(i_stream, str_split_buf, ',')) {
			// csvファイルを読み込む
			if (str_split_buf == "-m")
			{
				getline(i_stream, str_split_buf, ',');
				u32 num = atoi(str_split_buf.c_str());
				for (u32 i = 0; i < num; i++)
				{
					getline(i_stream, str_split_buf, ',');
					item.meshFiles.push_back(str_split_buf.c_str());
				}
			}
			else if (str_split_buf == "-r")
			{
				getline(i_stream, str_split_buf, ',');
				u32 num = atoi(str_split_buf.c_str());
				for (u32 i = 0; i < num; i++)
				{
					getline(i_stream, str_split_buf, ',');
					item.rigFiles.push_back(str_split_buf.c_str());
				}
			}
			else if (str_split_buf == "-a")
			{
				getline(i_stream, str_split_buf, ',');
				u32 num = atoi(str_split_buf.c_str());
				for (u32 i = 0; i < num; i++)
				{
					getline(i_stream, str_split_buf, ',');
					item.animFiles.push_back(str_split_buf.c_str());
				}

			}
		}
		list->push_back(item);
	}
}

int main()
{
	registerExporters();
	addModelList(&gAnimals, "../animals.csv");
	addModelList(&gMonsters, "../monsters.csv");
	addModelList(&gBackgrounds, "../backgrounds.csv");
	addModelList(&gMainNpcs, "../mainnpcs.csv");
	addModelList(&gSecondNpcs, "../secondnpcs.csv");
	addModelList(&gGeralt, "../geralt.csv");
	std::string filePath = "../config.ini";
	gGamePath = GetConfigString(filePath, "System", "TW_GAME_PATH").c_str();
	gExportPath = GetConfigString(filePath, "System", "TW_EXPORT_PATH").c_str();
	gTexPath = GetConfigString(filePath, "System", "TW_TW3_TEX_PATH").c_str();
	auto WindowWidth = atoi(GetConfigString(filePath, "System", "WindowWidth").c_str());
	auto WindowHeight = atoi(GetConfigString(filePath, "System", "WindowHeight").c_str());

	MyEventReceiver receiver;
    gDevice = createDevice(video::EDT_DIRECT3D9,
		core::dimension2d<u32>(WindowWidth, WindowHeight),
		16,false,false,false,&receiver);
    if (gDevice == 0)
        return 1; // could not create selected driver
	gDevice->setResizable(true);
	gDevice->getSceneManager()->getParameters()->setAttribute("TW_GAME_PATH", gGamePath.c_str());
	gDevice->getSceneManager()->getParameters()->setAttribute("TW_EXPORT_PATH", gExportPath.c_str());
	gDevice->getSceneManager()->getParameters()->setAttribute("TW_TW3_TEX_PATH", gTexPath.c_str());
	gDevice->getSceneManager()->getParameters()->setAttribute("TW_TW3_LOAD_SKEL", true);
	gDevice->getSceneManager()->getParameters()->setAttribute("TW_TW3_LOAD_BEST_LOD_ONLY", false);

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
	submenu->addItem(L"Load Model File ...", GUI_ID_LOAD_ENT,true,false);
	submenu->addItem(L"Load Rig File ...", GUI_ID_LOAD_RIG,true, false);
	submenu->addItem(L"Load Anim File ...", GUI_ID_LOAD_ANIM, true, false);
	submenu->addItem(L"Load Add File ...", GUI_ID_ADD_MESH, true, false);
	submenu->addItem(L"Export File ...", -1,true,true);
	submenu->addSeparator();
	submenu->addItem(L"Quit", GUI_ID_QUIT, true, false);

	submenu = menu->getSubMenu(0)->getSubMenu(4);
	submenu->addItem(L"COLLADA .dae by Assimp", GUI_ID_EXPORT_DAE);
	submenu->addItem(L"X Files .x by Assimp", GUI_ID_EXPORT_X);
	submenu->addItem(L"OBJ with Mat .obj by Assimp", GUI_ID_EXPORT_OBJ_MAT);
	submenu->addItem(L"OBJ No Mat .obj by Assimp", GUI_ID_EXPORT_OBJ_NOMAT);
	submenu->addItem(L"Autodesk 3DS .3ds by Assimp", GUI_ID_EXPORT_3DS);
	submenu->addItem(L"Autodesk FBX (bin) .fbx by Assimp", GUI_ID_EXPORT_FBX_BIN);
	submenu->addItem(L"Autodesk FBX (asc) .fbx by Assimp", GUI_ID_EXPORT_FBX_ASC);
	// create toolbar
	gui::IGUIToolBar* bar = env->addToolBar();
	gui::IGUIComboBox* animals = env->addComboBox(core::rect<s32>(10, 4, 160, 23), bar, GUI_ID_ANIMALS_LIST);
	gui::IGUIComboBox* monsters = env->addComboBox(core::rect<s32>(170, 4, 320, 23), bar, GUI_ID_MONSTERS_LIST);
	gui::IGUIComboBox* backgrounds = env->addComboBox(core::rect<s32>(330, 4, 480, 23), bar, GUI_ID_BACKGROUNDS_LIST);
	gui::IGUIComboBox* mainnpcs = env->addComboBox(core::rect<s32>(490, 4, 640, 23), bar, GUI_ID_MAIN_NPCS_LIST);
	gui::IGUIComboBox* secondnpcs = env->addComboBox(core::rect<s32>(650, 4, 800, 23), bar, GUI_ID_SECOND_NPCS_LIST);
	gui::IGUIComboBox* geralt = env->addComboBox(core::rect<s32>(810, 4, 960, 23), bar, GUI_ID_GERALT_LIST);

	animals->addItem(L"No Animals");
	for (u32 i = 0; i < gAnimals.size(); i++)
	{
		size_t ret;
		wchar_t name[100];
		mbstowcs_s(&ret,name,gAnimals[i].modelName.c_str(), _TRUNCATE);
		animals->addItem((const wchar_t*)name);
	}
	monsters->addItem(L"No Monsters");
	for (u32 i = 0; i < gMonsters.size(); i++)
	{
		size_t ret;
		wchar_t name[100];
		mbstowcs_s(&ret, name, gMonsters[i].modelName.c_str(), _TRUNCATE);
		monsters->addItem((const wchar_t*)name);
	}
	backgrounds->addItem(L"No Backgrounds");
	for (u32 i = 0; i < gBackgrounds.size(); i++)
	{
		size_t ret;
		wchar_t name[100];
		mbstowcs_s(&ret, name, gBackgrounds[i].modelName.c_str(), _TRUNCATE);
		backgrounds->addItem((const wchar_t*)name);
	}
	mainnpcs->addItem(L"No Main Npcs");
	for (u32 i = 0; i < gMainNpcs.size(); i++)
	{
		size_t ret;
		wchar_t name[100];
		mbstowcs_s(&ret, name, gMainNpcs[i].modelName.c_str(), _TRUNCATE);
		mainnpcs->addItem((const wchar_t*)name);
	}
	secondnpcs->addItem(L"No Secondly Npcs");
	for (u32 i = 0; i < gSecondNpcs.size(); i++)
	{
		size_t ret;
		wchar_t name[100];
		mbstowcs_s(&ret, name, gSecondNpcs[i].modelName.c_str(), _TRUNCATE);
		secondnpcs->addItem((const wchar_t*)name);
	}
	geralt->addItem(L"No Geralt");
	for (u32 i = 0; i < gGeralt.size(); i++)
	{
		size_t ret;
		wchar_t name[100];
		mbstowcs_s(&ret, name, gGeralt[i].modelName.c_str(), _TRUNCATE);
		geralt->addItem((const wchar_t*)name);
	}
	gW3ENT = new IO_MeshLoader_W3ENT(smgr, fs);
	gCamera = gDevice->getSceneManager()->addCameraSceneNodeMaya(nullptr);
	gCamera->setFarValue(1000.f);

	while (gDevice->run() && driver)
	{
		if (gDevice->isWindowActive())
		{
			driver->beginScene(true, true, video::SColor(255, 100, 101, 140));

			smgr->drawAll();
			env->drawAll();

			driver->endScene();

		}
		else
			gDevice->yield();
	}
	delete gW3ENT;
	gDevice->drop();
}
