#ifndef CLUSTER
#define CLUSTER

#define MESH_SIZE 64

#include "VertexData.hpp"
#include "glm/vec3.hpp"
#include <metis.h>
#include <vector>
#include <map>
#include <set>

typedef unsigned char uint8;
typedef std::pair<std::vector<idx_t>, std::vector<idx_t>> CSRGraph;

const float MAX_RADIUS = 0.1f; // the size of the projected radius where we'll render a smaller LOD if it gets lower

struct GAR_Vec3f {
  float x;
  float y;
  float z;
  GAR_Vec3f(float _x, float _y, float _z): x(_x), y(_y), z(_z) { }
};

struct GAR_ClusterVertex {
  float x,y,z;
  int id;
  GAR_ClusterVertex(float _x,float _y,float _z, int _id): x(_x), y(_y), z(_z), id(_id) {}
};

struct GAR_Tri {
  uint verticies[3]; // VertexData array indicies
};

// In memory, this is actually an index buffer
// Remember that not every tri may be assigned
struct GAR_ClusterTris {
  GAR_Tri tris[MESH_SIZE];
};

struct GAR_Cluster {
  uint tris; /* index of first tri */
  uint8 numTri;
};

struct GAR_ClusterElements {
  uint primativeIndicies[MESH_SIZE];
  uint8 numPrim;
};

/* This is the cluster that is used for determining LOD
 * the DAG hierarchy must be the DAG equivallent of a heap (for ideal code)
 * In practice, child cluster tests to see if both parents would render it
 * if so, then test to render children
 *
 * firstParentFlag is a flag (fpf & 1<<n: children[n]) that determines if
 * this cluster should trigger its children. This is to avoid triggering 
 * a group multiple times.
 * 
 */
struct GAR_HierarchyCluster {
  GAR_Cluster data;
  uint children[4]; /* index into hClusters list */
  GAR_Vec3f center;
  float radius;
  GAR_Vec3f testNonFirstParentCenter;
  float testNonFirstParentRadius; // if 0, then this only has one parent
  uint8 numChildren;
  uint8 firstParentFlag;
  uint8 level;
};

struct GAR_Group {
  GAR_Cluster highClusters[4];
  GAR_Cluster lowClusters[2];
  uint8 numHigh;
  uint8 numLow;
};

CSRGraph GAR_CreateMeshGraph(std::vector<uint>& ibuffer);

std::vector<GAR_ClusterElements>
GAR_GenerateClusters(CSRGraph& meshGraph, uint clustnum, uint tries);

void GAR_ClusterOBJExport(std::vector<uint>& ibuffer, std::vector<VertexData>& vbuffer,
                          GAR_ClusterElements& cluster);

std::vector<GAR_Cluster> GAR_GetClustersFromECluster(std::vector<GAR_ClusterElements> clusters, 
                                                     std::vector<GAR_Tri>& tris);

bool GAR_SimplifyGroup(GAR_Group& group, std::vector<GAR_Tri>& tris,
    std::vector<GAR_Vec3f>& verticies, std::set<uint>& boundaries); 

std::set<uint> GAR_FindBoundary(std::vector<GAR_Cluster>& clusters, std::vector<GAR_Tri>& tris);

std::vector<GAR_Cluster> GAR_GetClustersFromGroup(std::vector<GAR_Group>& groups);

uint GAR_FindMappedIndex(uint idx, std::map<uint, uint>& collapseMapping); /* helper func */

std::vector<GAR_Group> GAR_GroupClusters(std::vector<GAR_Cluster>& clusters, std::vector<GAR_Tri>& tris,
        uint numparts, uint tries);

std::vector<GAR_Cluster> GAR_GetLowClustersFromGroups(std::vector<GAR_Group>& groups);

std::pair<float, GAR_Vec3f> GAR_CalculateClusterRadiusCenter(GAR_Cluster& cluster,
                            std::vector<GAR_Tri>& tris, std::vector<GAR_Vec3f>& verticies);

std::vector<GAR_HierarchyCluster> GAR_CreateHierarchy(GAR_Group& root, std::vector<GAR_Group>& groups, 
                                  std::vector<GAR_Vec3f>& verticies, std::vector<GAR_Tri>& tris);

std::pair<std::vector<GAR_HierarchyCluster>, std::vector<GAR_Tri>>
GAR_ComputeMesh(std::vector<uint>& ibuffer, std::vector<GAR_Vec3f>& vbuffer);

void GAR_PrintHCluster(GAR_HierarchyCluster& hc);

std::set<uint> GAR_FindClusterBoundaries(std::vector<GAR_Cluster>& clusters, std::vector<GAR_Tri> tris);
#endif
