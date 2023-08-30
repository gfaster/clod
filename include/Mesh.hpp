#ifndef GAR_MESH
#define GAR_MESH

#include <vector>
#include "VertexData.hpp"
#include "GAR.hpp"
#include "Shader.hpp"
#include "Transform.hpp"
#include <glad/glad.h>
#include "glm/vec3.hpp"
#include "glm/gtc/matrix_transform.hpp"



class Mesh {
  public:
    Mesh(std::vector<VertexData> verticies, std::vector<uint> indicies);
    ~Mesh();

    void Render();
    void RenderCluster(uint idx_any); /* idx modulated - this can be arbitrarily large */

    void Update(unsigned int screenWidth, unsigned int screenHeight);
    void Bind();

    Transform* GetTransform();

  private:
    /* for now I want to just create discret LOD meshes */
    std::vector<GAR_ClusterVertex> verticies;
    std::vector<GAR_Group> groups;
    std::vector<GAR_Vec3f> simpleVtx;
    std::vector<GAR_Tri> triIndicies;
    std::vector<GAR_Cluster> clusters; /* will eventually be HierarchyClusters */
    std::vector<GAR_HierarchyCluster> hierarchyClusters;
    std::vector<uint8> lods; /* lod buffer for clusters */

    void Simplify();

    void InitializeWith_distinctClusters(std::vector<VertexData>& vbuf, std::vector<uint>& ibuf,
                                         std::vector<GAR_ClusterElements>& eclusters);

    void InitializeWith_unifiedClusters(std::vector<VertexData>& vbuf, std::vector<uint>& ibuf,
                                        std::vector<GAR_ClusterElements>& eclusters);


    void InitializeWith_hierarchyClusters (std::vector<VertexData>& vbuffer,
                                                 std::vector<uint>& ibuffer);

    std::vector<uint> FindGoodClusters(GAR_Vec3f cameraPos);
    std::vector<uint> FindGoodClustersDiscrete(GAR_Vec3f cameraPos);
    void UpdateHier(GAR_Vec3f cameraPos);

    void CreateClusteredBuffer();
    void CreateGroupedBuffer();
    void CreateHierBuffer();

    Transform m_transform;
    glm::mat4 m_projectionMatrix;

    // Vertex Array Object
    GLuint m_VAOId;
    // Vertex Buffer
    GLuint m_vertexPositionBuffer;
    // Index Buffer Object
    GLuint m_indexBufferObject;

    Shader m_shader;
};




#endif
