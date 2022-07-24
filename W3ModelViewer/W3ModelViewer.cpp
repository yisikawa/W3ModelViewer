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

//    io::IReadFile* file = fs->createAndOpenFile(io::path("Z:\\uncooked\\characters\\models\\animals\\cat\\t_01__cat.w2ent"));
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
		node->setMaterialFlag(video::EMF_LIGHTING, false);
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
	smgr->addCameraSceneNode(0, core::vector3df(0, 30, -40), core::vector3df(0, 5, 0));

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
