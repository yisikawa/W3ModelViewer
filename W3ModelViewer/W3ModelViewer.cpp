// W3ModelViewer.cpp : このファイルには 'main' 関数が含まれています。プログラム実行の開始と終了がそこで行われます。
//

#include <iostream>
#include <irrString.h>
#include <irrArray.h>
#include <IReadFile.h>
#include <IFileSystem.h>
#include <Irrlicht.h>
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
    io::IFileSystem *fs = device->getFileSystem();
    video::IVideoDriver* driver = device->getVideoDriver();
    scene::ISceneManager* smgr = device->getSceneManager();
    IO_MeshLoader_W3ENT w3ent(smgr, fs);

    io::IReadFile* file = fs->createAndOpenFile(io::path("Z:\\uncooked\\characters\\models\\animals\\cat\\t_01__cat.w2ent"));
    w3ent.createMesh(file);
    std::cout << "Hello World!\n";
    system("pause");
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
