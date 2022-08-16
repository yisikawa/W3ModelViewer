#ifndef W3_UI
#define W3_UI

#include <Irrlicht.h>
#include <irrString.h>
#include <irrArray.h>
#include <IReadFile.h>
#include <IFileSystem.h>

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

class MyEventReceiver : public IEventReceiver
{

public:
//	;

	virtual bool OnEvent(const SEvent& event);
	/*
		Handle "menu item clicked" events.
	*/ 
};

#endif