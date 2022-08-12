#include "IrrAssimpUtils.h"

//#ifdef _MSC_VER
//#pragma comment (lib, "assimp-vc143-mtd.lib")
//#endif // _MSC_VER

aiString irrToAssimpPath(const irr::io::path& path)
{
#ifdef _IRR_WCHAR_FILESYSTEM
    irr::core::stringc rPath = path;
    return aiString(rPath.c_str());
#else
    return aiString(path.c_str());
#endif
}
