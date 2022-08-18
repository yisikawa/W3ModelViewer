// W3ModelViewer.cpp : このファイルには 'main' 関数が含まれています。プログラム実行の開始と終了がそこで行われます。
//
#include <string>
#include "../IO_MeshLoader_W3ENT.h"
#include "../IrrAssimp.h"
#include "../W3_ui.h"

using namespace irr;
using namespace scene;
using namespace gui;


#ifdef _IRR_WINDOWS_
#pragma comment(lib, "Irrlicht.lib")
#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif

void init_ui();
std::string GetConfigString(const std::string& filePath, const char* pSectionName, const char* pKeyName);

//Some global variables 
MyEventReceiver gReceiver;
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
//core::array<struct ExporterInfos> gExporters;


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
	gui::IGUIContextMenu* menu = (gui::IGUIContextMenu*)gDevice->getGUIEnvironment()->getRootGUIElement()->getElementFromId(GUI_ID_TOGGLE_INFO, true);
	if (menu)
		for (int item = 1; item < 6; ++item)
			menu->setItemChecked(item, false);
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
		TW3_DataCache::_instance.boneApply2Rig();
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
void convertAndCopyTextures(scene::IAnimatedMesh* mesh, const io::path exportFolder, bool shouldCopyTextures)
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




int main()
{
	std::string filePath = "../config.ini";
	gGamePath = GetConfigString(filePath, "System", "TW_GAME_PATH").c_str();
	gExportPath = GetConfigString(filePath, "System", "TW_EXPORT_PATH").c_str();
	gTexPath = GetConfigString(filePath, "System", "TW_TW3_TEX_PATH").c_str();
	auto WindowWidth = atoi(GetConfigString(filePath, "System", "WindowWidth").c_str());
	auto WindowHeight = atoi(GetConfigString(filePath, "System", "WindowHeight").c_str());
    gDevice = createDevice(video::EDT_DIRECT3D9,
		core::dimension2d<u32>(WindowWidth, WindowHeight),
		16,false,false,false,&gReceiver);
    if (gDevice == 0)
        return 1; // could not create selected driver
	gDevice->setResizable(true);
	gDevice->getSceneManager()->getParameters()->setAttribute("TW_GAME_PATH", gGamePath.c_str());
	gDevice->getSceneManager()->getParameters()->setAttribute("TW_EXPORT_PATH", gExportPath.c_str());
	gDevice->getSceneManager()->getParameters()->setAttribute("TW_TW3_TEX_PATH", gTexPath.c_str());
	gDevice->getSceneManager()->getParameters()->setAttribute("TW_TW3_LOAD_SKEL", true);
	gDevice->getSceneManager()->getParameters()->setAttribute("TW_TW3_LOAD_BEST_LOD_ONLY", true);

	gW3ENT = new IO_MeshLoader_W3ENT(gDevice->getSceneManager(), gDevice->getFileSystem());
	gCamera = gDevice->getSceneManager()->addCameraSceneNodeMaya(nullptr);
	gCamera->setFarValue(1000.f);
	init_ui();
	while (gDevice->run() && gDevice->getVideoDriver())
	{
		if (gDevice->isWindowActive())
		{
			gDevice->getVideoDriver()->beginScene(true, true, video::SColor(255, 100, 101, 140));

			gDevice->getSceneManager()->drawAll();
			gDevice->getGUIEnvironment()->drawAll();

			gDevice->getVideoDriver()->endScene();

		}
		else
			gDevice->yield();
	}
	delete gW3ENT;
	gDevice->drop();
}
