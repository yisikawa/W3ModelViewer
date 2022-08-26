
#include <fstream>
#include <sstream>
#include <iostream>
#include <irrlicht.h>
#include "IO_MeshLoader_W3ENT.h"
#include "CGUIFileSaveDialog.h"
#include "IrrAssimp.h"
#include "W3_ui.h"

using namespace irr;
using namespace scene;
using namespace gui;//Some global variables 

extern IrrlichtDevice* gDevice;
extern scene::IAnimatedMeshSceneNode* gModel;
extern IO_MeshLoader_W3ENT* gW3ENT;
extern core::stringc gGamePath;
extern core::stringc gExportPath;
extern core::stringc gTexPath ;
core::array<struct ExporterInfos> gExporters;
core::array<struct ModelList> gAnimals, gMonsters, gBackgrounds, gMainNpcs, gSecondNpcs, gGeralt;

bool setAnims(s32 pos);
void loadModel(const c8* fn, const c8* name);
bool addMesh(const c8* fn);
void setMaterialsSettings(scene::IAnimatedMeshSceneNode* node);
bool loadRig(IrrlichtDevice* device, scene::IAnimatedMeshSceneNode* _current_node, const io::path filename);
bool loadAnims(IrrlichtDevice* device, scene::IAnimatedMeshSceneNode* _current_node, const io::path filename);
void enableRigging(scene::IAnimatedMeshSceneNode* node, bool enabled);
void ExportModel(IrrlichtDevice* device, scene::IAnimatedMeshSceneNode* _current_node, const io::path directory,
	const io::path filename, struct ExporterInfos exporter);
void OnMenuItemSelected(IGUIContextMenu* menu);
void OnAnimalsListSelected(IGUIComboBox* combo);
void OnMonstersListSelected(IGUIComboBox* combo);
void OnBackgroundsListSelected(IGUIComboBox* combo);
void OnMainNpcsListSelected(IGUIComboBox* combo);
void OnSecondNpcsListSelected(IGUIComboBox* combo);
void OnGeraltListSelected(IGUIComboBox* combo);
void OnAnimationtSelected(IGUIComboBox* combo);


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
		if (noAssimpExportExtensions.binary_search(extension) != -1)
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


bool MyEventReceiver::OnEvent(const SEvent& event)
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
			IGUIFileOpenDialog* dialog = (IGUIFileOpenDialog*)event.GUIEvent.Caller;
			//					CGUIFileSaveDialog* dialog2 = (CGUIFileSaveDialog*)event.GUIEvent.Caller;
			core::getFileNameExtension(extension, core::stringc(dialog->getFileName()).c_str());
			extension.make_lower();
			core::stringc str0 = core::stringc(dialog->getFileName()).c_str();
			core::stringc str = env->getFileSystem()->getFileDir(str0) + "/";
			// load the model file, selected in the file open dialog
			switch (dialog->getID())
			{
			case GUI_ID_LOAD_ENT:
				if (extension == ".w2ent" || extension == ".w2mesh")
				{
					loadModel(core::stringc(dialog->getFileName()).c_str(), "");
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
				ExportModel(gDevice, gModel, str, str0, gExporters[0]);
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
			else if (id == GUI_ID_ANIMS_LIST)
			{
				OnAnimationtSelected((IGUIComboBox*)event.GUIEvent.Caller);
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
		env->addFileOpenDialog(L"Please select a model file to load", true, 0,
			GUI_ID_LOAD_ENT, false, (irr::c8*)gGamePath.c_str());
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
	case GUI_ID_INFO_OFF: // View -> Debug Information
		menu->setItemChecked(menu->getSelectedItem() + 1, false);
		menu->setItemChecked(menu->getSelectedItem() + 2, false);
		menu->setItemChecked(menu->getSelectedItem() + 3, false);
		menu->setItemChecked(menu->getSelectedItem() + 4, false);
		menu->setItemChecked(menu->getSelectedItem() + 5, false);
		menu->setItemChecked(menu->getSelectedItem() + 6, false);
		if (gModel)
			gModel->setDebugDataVisible(scene::EDS_OFF);
		break;
	case GUI_ID_BOUNDING_BOX: // View -> Debug Information
		menu->setItemChecked(menu->getSelectedItem(), !menu->isItemChecked(menu->getSelectedItem()));
		if (gModel)
			gModel->setDebugDataVisible((scene::E_DEBUG_SCENE_TYPE)(gModel->isDebugDataVisible() ^ scene::EDS_BBOX));
		break;
	case GUI_ID_NORMALS: // View -> Debug Information
		menu->setItemChecked(menu->getSelectedItem(), !menu->isItemChecked(menu->getSelectedItem()));
		if (gModel)
			gModel->setDebugDataVisible((scene::E_DEBUG_SCENE_TYPE)(gModel->isDebugDataVisible() ^ scene::EDS_NORMALS));
		break;
	case GUI_ID_SKELETON: // View -> Debug Information
		menu->setItemChecked(menu->getSelectedItem(), !menu->isItemChecked(menu->getSelectedItem()));
		if (gModel)
			gModel->setDebugDataVisible((scene::E_DEBUG_SCENE_TYPE)(gModel->isDebugDataVisible() ^ scene::EDS_SKELETON));
		break;
	case GUI_ID_WIRE_OVERLAY: // View -> Debug Information
		menu->setItemChecked(menu->getSelectedItem(), !menu->isItemChecked(menu->getSelectedItem()));
		if (gModel)
			gModel->setDebugDataVisible((scene::E_DEBUG_SCENE_TYPE)(gModel->isDebugDataVisible() ^ scene::EDS_MESH_WIRE_OVERLAY));
		break;
	case GUI_ID_HALF_TRANSPARENT: // View -> Debug Information
		menu->setItemChecked(menu->getSelectedItem(), !menu->isItemChecked(menu->getSelectedItem()));
		if (gModel)
			gModel->setDebugDataVisible((scene::E_DEBUG_SCENE_TYPE)(gModel->isDebugDataVisible() ^ scene::EDS_HALF_TRANSPARENCY));
		break;
	case GUI_ID_BUFFERS_BOUNDING_BOXES: // View -> Debug Information
		menu->setItemChecked(menu->getSelectedItem(), !menu->isItemChecked(menu->getSelectedItem()));
		if (gModel)
			gModel->setDebugDataVisible((scene::E_DEBUG_SCENE_TYPE)(gModel->isDebugDataVisible() ^ scene::EDS_BBOX_BUFFERS));
		break;
	case GUI_ID_INFO_ALL: // View -> Debug Information
		menu->setItemChecked(menu->getSelectedItem() - 1, true);
		menu->setItemChecked(menu->getSelectedItem() - 2, true);
		menu->setItemChecked(menu->getSelectedItem() - 3, true);
		menu->setItemChecked(menu->getSelectedItem() - 4, true);
		menu->setItemChecked(menu->getSelectedItem() - 5, true);
		menu->setItemChecked(menu->getSelectedItem() - 6, true);
		if (gModel)
			gModel->setDebugDataVisible(scene::EDS_FULL);
		break;
	case GUI_ID_MAT_SOLID: // View -> Material -> Solid
		if (gModel)
			gModel->setMaterialType(video::EMT_SOLID);
		break;
	case GUI_ID_MAT_TRANSPARENT: // View -> Material -> Transparent
		if (gModel)
			gModel->setMaterialType(video::EMT_TRANSPARENT_ADD_COLOR);
		break;
	case GUI_ID_MAT_ALPHA_REF: // View -> Material -> Reflection
		if (gModel)
			gModel->setMaterialType(video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF);
		break;
	case GUI_ID_QUIT: // File -> Quit
		gDevice->closeDevice();
		break;
	}
}

/*
	Handle the event that one of the texture-filters was selected in the corresponding combobox.
*/
void OnAnimationtSelected(IGUIComboBox* combo)
{
	s32 pos = combo->getSelected();
	setAnims(pos - 1);
}

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
	if (gAnimals[pos - 1].rigFiles.size() >= 1)
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
	if (gMonsters[pos - 1].rigFiles.size() >= 1)
	{
		core::stringc file = gGamePath + gMonsters[pos - 1].rigFiles[0];
		gW3ENT->Skeleton.clear();
		loadRig(gDevice, gModel, io::path(file));
	}
	if (gMonsters[pos - 1].animFiles.size() >= 1)
	{
		core::stringc file = gGamePath + gMonsters[pos - 1].animFiles[0];
		loadAnims(gDevice, gModel, io::path(file));

	}
}

void OnBackgroundsListSelected(IGUIComboBox* combo)
{
	s32 pos = combo->getSelected();
	if (pos <= 0) return;
	if (gBackgrounds[pos - 1].meshFiles.size() >= 1) {
		core::stringc file = gGamePath + gBackgrounds[pos - 1].meshFiles[0];
		loadModel(file.c_str(), gBackgrounds[pos - 1].modelName.c_str());
		for (u32 i = 1; i < gBackgrounds[pos - 1].meshFiles.size(); i++)
		{
			core::stringc file = gGamePath + gBackgrounds[pos - 1].meshFiles[i];
			addMesh(file.c_str());
		}
	}
	//if (gBackgrounds[pos - 1].rigFiles.size() >= 1)
	//{
	//	core::stringc file = gGamePath + gBackgrounds[pos - 1].rigFiles[0];
	//	gW3ENT->Skeleton.clear();
	//	loadRig(gDevice, gModel, io::path(file));
	//}
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
	//if (gMainNpcs[pos - 1].rigFiles.size() >= 1)
	//{
	//	core::stringc file = gGamePath + gMainNpcs[pos - 1].rigFiles[0];
	//	gW3ENT->Skeleton.clear();
	//	loadRig(gDevice, gModel, io::path(file));
	//}
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
	//if (gSecondNpcs[pos - 1].rigFiles.size() >= 1)
	//{
	//	core::stringc file = gGamePath + gSecondNpcs[pos - 1].rigFiles[0];
	//	gW3ENT->Skeleton.clear();
	//	loadRig(gDevice, gModel, io::path(file));
	//}
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


void enableWireframe(scene::IAnimatedMeshSceneNode* node, bool enabled)
{
	if (node)
		node->setMaterialFlag(video::EMF_WIREFRAME, enabled);
}

void enableRigging(scene::IAnimatedMeshSceneNode* node, bool enabled)
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

void addModelList(core::array<struct ModelList>* list, core::stringc fileName)
{

	std::string str_buf;
	std::string str_split_buf;

	for (u32 i = 0; i < list->size(); i++)
		list[i].clear();
	list->clear();
	std::ifstream ifs_csv(fileName.c_str());
	while (getline(ifs_csv, str_buf))
	{
		struct ModelList item;
		std::istringstream i_stream(str_buf);
		getline(i_stream, str_split_buf, ',');
		item.modelName = str_split_buf.c_str();
		while (getline(i_stream, str_split_buf, ',')) {
			// csvÉtÉ@ÉCÉãÇì«Ç›çûÇﬁ
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

gui::IGUIToolBar* bar;
gui::IGUIComboBox* animations;
gui::IGUIComboBox* animals;
gui::IGUIComboBox* monsters;
gui::IGUIComboBox* backgrounds;
gui::IGUIComboBox* mainnpcs;
gui::IGUIComboBox* secondnpcs;
gui::IGUIComboBox* geralt;

void addAnimList(core::stringc name)
{
	size_t ret;
	wchar_t str[100];
	mbstowcs_s(&ret, str, name.c_str(), _TRUNCATE);
	animations->addItem((const wchar_t*)str);
}

void clearAnimList()
{

	animations->clear();
}

void init_ui()
{
	registerExporters();
	addModelList(&gAnimals, "../animals.csv");
	addModelList(&gMonsters, "../monsters.csv");
	addModelList(&gBackgrounds, "../backgrounds.csv");
	addModelList(&gMainNpcs, "../mainnpcs.csv");
	addModelList(&gSecondNpcs, "../secondnpcs.csv");
	addModelList(&gGeralt, "../geralt.csv");

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
	menu->addItem(L"View", -1, true, true);
	gui::IGUIContextMenu* submenu;
	submenu = menu->getSubMenu(0);
	submenu->addItem(L"Load Model File ...", GUI_ID_LOAD_ENT, true, false);
	submenu->addItem(L"Load Rig File ...", GUI_ID_LOAD_RIG, true, false);
	submenu->addItem(L"Load Anim File ...", GUI_ID_LOAD_ANIM, true, false);
	submenu->addItem(L"Load Add File ...", GUI_ID_ADD_MESH, true, false);
	submenu->addItem(L"Export File ...", -1, true, true);
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

	submenu = menu->getSubMenu(1);
	submenu->addItem(L"toggle information", GUI_ID_TOGGLE_INFO, true, true);
	submenu->addItem(L"model material", -1, true, true);

	submenu = menu->getSubMenu(1)->getSubMenu(0);
	submenu->addItem(L"Off", GUI_ID_INFO_OFF);
	submenu->addItem(L"Bounding Box", GUI_ID_BOUNDING_BOX);
	submenu->addItem(L"Normals", GUI_ID_NORMALS);
	submenu->addItem(L"Skeleton", GUI_ID_SKELETON);
	submenu->addItem(L"Wire overlay", GUI_ID_WIRE_OVERLAY);
	submenu->addItem(L"Half-Transparent", GUI_ID_HALF_TRANSPARENT);
	submenu->addItem(L"Buffers bounding boxes", GUI_ID_BUFFERS_BOUNDING_BOXES);
	submenu->addItem(L"All", GUI_ID_INFO_ALL);

	submenu = menu->getSubMenu(1)->getSubMenu(1);
	submenu->addItem(L"Solid", GUI_ID_MAT_SOLID);
	submenu->addItem(L"Transparent", GUI_ID_MAT_TRANSPARENT);
	submenu->addItem(L"Alpha Channel", GUI_ID_MAT_ALPHA_REF);
	// create toolbar
	bar = env->addToolBar();
	animations = env->addComboBox(core::rect<s32>(10, 4, 160, 23), bar, GUI_ID_ANIMS_LIST);
	animals = env->addComboBox(core::rect<s32>(170, 4, 320, 23), bar, GUI_ID_ANIMALS_LIST);
	monsters = env->addComboBox(core::rect<s32>(330, 4, 480, 23), bar, GUI_ID_MONSTERS_LIST);
	backgrounds = env->addComboBox(core::rect<s32>(490, 4, 640, 23), bar, GUI_ID_BACKGROUNDS_LIST);
	mainnpcs = env->addComboBox(core::rect<s32>(650, 4, 800, 23), bar, GUI_ID_MAIN_NPCS_LIST);
	secondnpcs = env->addComboBox(core::rect<s32>(810, 4, 960, 23), bar, GUI_ID_SECOND_NPCS_LIST);
	geralt = env->addComboBox(core::rect<s32>(970, 4, 1120, 23), bar, GUI_ID_GERALT_LIST);
	animations->addItem(L"No Animation");
	animals->addItem(L"No Animals");
	for (u32 i = 0; i < gAnimals.size(); i++)
	{
		size_t ret;
		wchar_t name[100];
		mbstowcs_s(&ret, name, gAnimals[i].modelName.c_str(), _TRUNCATE);
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
}