#include "GAR.hpp"
#include "OBJLoader.hpp"
#include "Mesh.hpp"
#include "SDLGraphicsProgram.hpp"

int main(int argc, char* argv[]) {

    std::string filename("./test_meshes/bunny_big.obj");
    /* std::string filename("../test_meshes/bunny_centered.obj"); */
    /* std::string filename("../test_meshes/man.obj"); */
    OBJLoader loader(filename);

    /* auto indicies = loader.getVertexIndicies(); */
    /* auto verticies = loader.getVerticies(); */

    /* CSRGraph csrMesh = GAR_CreateMeshGraph(indicies); */
    /* std::vector<GAR_ClusterElements> ce = GAR_GenerateClusters(csrMesh); */


    /* GAR_ClusterOBJExport(indicies, verticies, ce[0]); */


    SDLGraphicsProgram prog(720,720);



    prog.LoadObject(loader);
    prog.Loop();


    return 0;
}
