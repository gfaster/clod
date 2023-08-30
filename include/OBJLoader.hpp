#ifndef PARSER
#define PARSER


#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <utility>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include "glm/vec3.hpp"

#include "VertexData.hpp"


class OBJLoader {

  public:

    OBJLoader(std::string fileName);

    ~OBJLoader();

    enum LineType {
      Vertex, // 3 float
      VertexNormal, // 3 float
      Texture, //2 float
      Face_v, // 3 uint
      Face_vtn, // 9 uint
      Face_vn, // 6 uint
      Face_vt, // 6 uint
      Ignore, // means getNext returns nullptr
      Face, // Should never be returned
      Mat, // Should never be returned
      End // means getNext returns nullptr
    };

    std::vector<VertexData> getVerticies();
    std::vector<unsigned int> getVertexIndicies();
    std::string getTextureFileName();
    std::string getNormalMapFileName();

  private:

    void parse_line(std::string s);
    void parse_location(std::string s, std::vector<float> & loc);
    void parse_face(std::string s);

    std::string matTexGetHelper(uint index, char c);

    // gets a pointer to the data referenced by a obj file index
    float* location_lookup(unsigned int objIndex, std::vector<float> & source, unsigned int stride);
    void parse_face_helper(std::string s, bool has_vertex, bool has_texture, bool has_normal);

    
    std::string matFile;

    std::string filepath;

    std::vector<float> verticies;
    std::vector<float> normals;
    std::vector<float> textureUVs;

    // adds vertex to verticies_data if unique
    // adds the index of vertex in verticies_data to vertex_indicies
    void uniqAdd(VertexData vertex);

    std::vector<unsigned int> vertex_indicies;
    std::unordered_map<VertexData, unsigned int, VHash<VertexData>> verticies_added;
    std::vector<VertexData> verticies_data;


};
#endif
