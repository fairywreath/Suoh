#include <iostream>

#include <Core/Logger.h>

#include "MeshConvert.h"
#include "argh.h"

using namespace Suoh;

// #include <Graphics/Scene/VtxData.h>

#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <meshoptimizer.h>

MeshData g_meshData;

int main(int, char* argv[])
{
    LOG_SET_OUTPUT(&std::cout);
    argh::parser cmdl(argv);

    if (cmdl.size() != 4)
    {
        LOG_ERROR("MeshConvert requires exactly 3 arguments!");
        return -1;
    }

    /*
     * Current (temporary) usage:
     *  first argument -  src mesh format,
     *  second argument - new internal binary mesh format.
     *  third argument - new draw data binaray format.
     */
    MeshConvert::ConvertToMeshFile(cmdl[2], cmdl[3], cmdl[1]);

    return 0;
}