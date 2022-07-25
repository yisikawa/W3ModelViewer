// W3ModelViewer.cpp : このファイルには 'main' 関数が含まれています。プログラム実行の開始と終了がそこで行われます。
//

#include <iostream>
#include <Irrlicht.h>
#include <irrString.h>
#include <irrArray.h>
#include <IReadFile.h>
#include <IFileSystem.h>
#include "../IO_MeshLoader_W3ENT.h"

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

bool loadRig(IrrlichtDevice* device, const io::path filename)
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

	scene::ISkinnedMesh* newMesh = copySkinnedMesh(device->getSceneManager(), _currentLodData->_node->getMesh(), false);

	bool success = skeleton.applyToModel(newMesh);
	if (success)
	{
		// Apply the skinning
		TW3_DataCache::_instance.setOwner(newMesh);
		TW3_DataCache::_instance.apply();
	}

	newMesh->setDirty();
	newMesh->finalize();

	_currentLodData->_node->setMesh(newMesh);

	setMaterialsSettings(_currentLodData->_node);
	return success;
}

bool loadAnims(IrrlichtDevice* device, const io::path filename)
{
	io::IReadFile* file = device->getFileSystem()->createAndOpenFile(filename);
	if (!file)
	{
		return false;
	}

	scene::IO_MeshLoader_W3ENT loader(device->getSceneManager(), device->getFileSystem());

	scene::ISkinnedMesh* newMesh = copySkinnedMesh(_device->getSceneManager(), _currentLodData->_node->getMesh(), true);

	// use the loader to add the animation to the new model
	loader.meshToAnimate = newMesh;
	scene::IAnimatedMesh* mesh = loader.createMesh(file);
	file->drop();

	if (mesh)
		mesh->drop();


	newMesh->setDirty();
	newMesh->finalize();

	_currentLodData->_node->setMesh(newMesh);

	setMaterialsSettings(_currentLodData->_node);

	return true;
}

void loadMeshPostProcess()
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

IAnimatedMesh* loadMesh(IrrlichtDevice* _device, QString filename)
{
	_device->getSceneManager()->getParameters()->setAttribute("TW_GAME_PATH", cleanPath(Settings::_baseDir).toStdString().c_str());
	_device->getSceneManager()->getParameters()->setAttribute("TW_TW3_TEX_PATH", cleanPath(Settings::_TW3TexPath).toStdString().c_str());
	_device->getSceneManager()->getParameters()->setAttribute("TW_TW3_LOAD_SKEL", Settings::_TW3LoadSkeletonEnabled);
	_device->getSceneManager()->getParameters()->setAttribute("TW_TW3_LOAD_BEST_LOD_ONLY", Settings::_TW3LoadBestLODEnabled);

	_device->getSceneManager()->getParameters()->setAttribute("TW_TW2_LOAD_BEST_LOD_ONLY", Settings::_TW2LoadBestLODEnabled);

	ConfigNodeType tw1ToLoad = (ConfigNodeType)0;
	if (Settings::_TW1LoadStaticMesh) tw1ToLoad = (ConfigNodeType)((int)tw1ToLoad | (int)ConfigNodeTrimesh);
	if (Settings::_TW1LoadSkinnedMesh) tw1ToLoad = (ConfigNodeType)((int)tw1ToLoad | (int)ConfigNodeSkin);
	if (Settings::_TW1LoadPaintedMesh) tw1ToLoad = (ConfigNodeType)((int)tw1ToLoad | (int)ConfigNodeTexturePaint);
	_device->getSceneManager()->getParameters()->setAttribute("TW_TW1_NODE_TYPES_TO_LOAD", (int)tw1ToLoad);

	// Clear the previous data
	TW3_DataCache::_instance.clear();


	const io::path irrFilename = qStringToIrrPath(filename);
	io::path extension;
	core::getFileNameExtension(extension, irrFilename);

	scene::IAnimatedMesh* mesh = nullptr;

#ifdef COMPILE_WITH_ASSIMP
	IrrAssimp assimp(_device->getSceneManager());
#endif

	if (isLoadableByIrrlicht(irrFilename))
	{
		mesh = _device->getSceneManager()->getMesh(irrFilename);
	}
#ifdef COMPILE_WITH_ASSIMP
	else if (assimp.isLoadable(irrFilename))
	{
		mesh = assimp.getMesh(irrFilename);

		if (!mesh)
		{
			LoggerManager::Instance()->addLineAndFlush(assimp.getError(), true);
		}
	}
#endif

	return mesh;
}


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

    io::IFileSystem *fs = device->getFileSystem();
    video::IVideoDriver* driver = device->getVideoDriver();
    scene::ISceneManager* smgr = device->getSceneManager();
    IO_MeshLoader_W3ENT w3ent(smgr, fs);

	io::IReadFile* file = fs->createAndOpenFile(io::path("Z:/uncooked/characters/models/animals/cat/t_01__cat.w2ent"));
	IAnimatedMesh* mesh = w3ent.createMesh(file);



	/*
	To let the mesh look a little bit nicer, we change its material. We
	disable lighting because we do not have a dynamic light in here, and
	the mesh would be totally black otherwise. Then we set the frame loop,
	such that the predefined STAND animation is used. And last, we apply a
	texture to the mesh. Without it the mesh would be drawn using only a
	color.
	*/
	IAnimatedMeshSceneNode* node = smgr->addAnimatedMeshSceneNode(mesh);
	if (node)
	{
		setMaterialsSettings(node);
	//	node->setMaterialFlag(video::EMF_LIGHTING, false);
	//	node->setMD2Animation(scene::EMAT_STAND);
	//	node->setMaterialTexture(0, driver->getTexture("../../media/sydney.bmp"));
	}

	/*
	To look at the mesh, we place a camera into 3d space at the position
	(0, 30, -40). The camera looks from there to (0,5,0), which is
	approximately the place where our md2 model is.
	*/
	scene::ICameraSceneNode* camera;
	camera = device->getSceneManager()->addCameraSceneNodeMaya(nullptr);
	camera->setPosition(core::vector3df(0.f, 30.f, -40.f));
	camera->setTarget(core::vector3df(0.f, 0.f, 0.f));
	const f32 aspectRatio = static_cast<float>(640) / 480;
	camera->setAspectRatio(aspectRatio);
	camera->setFarValue(10000.f);
//	smgr->addCameraSceneNode(0, core::vector3df(0, 30, -40), core::vector3df(0, 5, 0));

	/*
	Ok, now we have set up the scene, lets draw everything: We run the
	device in a while() loop, until the device does not want to run any
	more. This would be when the user closes the window or presses ALT+F4
	(or whatever keycode closes a window).
	*/
	while (device->run())
	{
		/*
		Anything can be drawn between a beginScene() and an endScene()
		call. The beginScene() call clears the screen with a color and
		the depth buffer, if desired. Then we let the Scene Manager and
		the GUI Environment draw their content. With the endScene()
		call everything is presented on the screen.
		*/
		driver->beginScene(true, true, video::SColor(255, 100, 101, 140));

		smgr->drawAll();

		driver->endScene();
	}

	/*
	After we are done with the render loop, we have to delete the Irrlicht
	Device created before with createDevice(). In the Irrlicht Engine, you
	have to delete all objects you created with a method or function which
	starts with 'create'. The object is simply deleted by calling ->drop().
	See the documentation at irr::IReferenceCounted::drop() for more
	information.
	*/
	device->drop();
}

// プログラムの実行: Ctrl + F5 または [デバッグ] > [デバッグなしで開始] メニュー
// プログラムのデバッグ: F5 または [デバッグ] > [デバッグの開始] メニュー

// 作業を開始するためのヒント: 
//    1. ソリューション エクスプローラー ウィンドウを使用してファイルを追加/管理します 
//   2. チーム エクスプローラー ウィンドウを使用してソース管理に接続します
//   3. 出力ウィンドウを使用して、ビルド出力とその他のメッセージを表示します
//   4. エラー一覧ウィンドウを使用してエラーを表示します
//   5. [プロジェクト] > [新しい項目の追加] と移動して新しいコード ファイルを作成するか、[プロジェクト] > [既存の項目の追加] と移動して既存のコード ファイルをプロジェクトに追加します
//   6. 後ほどこのプロジェクトを再び開く場合、[ファイル] > [開く] > [プロジェクト] と移動して .sln ファイルを選択します
