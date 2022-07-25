#ifndef QIRRLICHTWIDGET_HPP
#define QIRRLICHTWIDGET_HPP

#include <IrrlichtDevice.h>
#include <IMaterialRendererServices.h>
#include <IShaderConstantSetCallBack.h>
#include <IAnimatedMeshSceneNode.h>
#include <ICameraSceneNode.h>



using namespace irr;

class NormalsDebuggerShaderCallBack : public video::IShaderConstantSetCallBack
{
public:
    NormalsDebuggerShaderCallBack() :
        Device(nullptr),
        WorldViewProjID(-1),
        TransWorldID(-1),
        InvWorldID(-1),
        FirstUpdate(true)
    {
    }

    virtual void OnSetConstants(video::IMaterialRendererServices* services, s32 userData)
    {
        video::IVideoDriver* driver = services->getVideoDriver();

        // get shader constants id.
        if (FirstUpdate)
        {
            WorldViewProjID = services->getVertexShaderConstantID("mWorldViewProj");
            TransWorldID = services->getVertexShaderConstantID("mTransWorld");
            InvWorldID = services->getVertexShaderConstantID("mInvWorld");

            FirstUpdate = false;
        }

        // set inverted world matrix
        // if we are using highlevel shaders (the user can select this when
        // starting the program), we must set the constants by name.
        core::matrix4 invWorld = driver->getTransform(video::ETS_WORLD);
        invWorld.makeInverse();
        services->setVertexShaderConstant(InvWorldID, invWorld.pointer(), 16);

        // set clip matrix
        core::matrix4 worldViewProj;
        worldViewProj = driver->getTransform(video::ETS_PROJECTION);
        worldViewProj *= driver->getTransform(video::ETS_VIEW);
        worldViewProj *= driver->getTransform(video::ETS_WORLD);
        services->setVertexShaderConstant(WorldViewProjID, worldViewProj.pointer(), 16);

        // set transposed world matrix
        core::matrix4 world = driver->getTransform(video::ETS_WORLD);
        world = world.getTransposed();
        services->setVertexShaderConstant(TransWorldID, world.pointer(), 16);
    }

    void SetDevice(IrrlichtDevice* device)
    {
        Device = device;
    }

private:
    IrrlichtDevice* Device;

    s32 WorldViewProjID;
    s32 TransWorldID;
    s32 InvWorldID;

    bool FirstUpdate;
};

enum LOD
{
    LOD_0,
    LOD_1,
    LOD_2,
    Collision
};

struct LOD_data
{
    LOD_data() : _node(nullptr)
    {
        clearLodData();
    }

    void clearLodData()
    {
        if (_node)
        {
            _node->remove();
            _node = nullptr;
        }

        _additionalTextures.clear();
    }

    scene::IAnimatedMeshSceneNode* _node;
    QVector<QVector<QString> > _additionalTextures;

    QSet<QString> getTexturesSetForLayer(int layer)
    {
        Q_ASSERT(layer >= 1 && layer < _IRR_MATERIAL_MAX_TEXTURES_);
        QSet<QString> texturesSet;
        for (int i = 0; i < _additionalTextures.size(); ++i)
        {
            QString path = _additionalTextures[i][layer-1];
            if (!path.isEmpty())
                texturesSet.insert(path);
        }
        return texturesSet;
    }
};



#endif // QIRRLICHTWIDGET_HPP
