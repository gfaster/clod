#ifndef VERTEXDATA
#define VERTEXDATA

#include <cstdint>
#include <functional>

struct VertexData{
  float x,y,z;
  float s,t;
  float nx,ny,nz;

  VertexData(float _x, float _y, float _z, float _s, float _t, float _nx, float _ny, float _nz): x(_x),y(_y),z(_z),s(_s),t(_t), nx(_nx), ny(_ny), nz(_nz) { }

  VertexData(): VertexData(0,0,0,0,0,0,0,0) { }

  bool operator== (const VertexData &rhs) const {
      if( (x == rhs.x) && (y == rhs.y) && (z == rhs.z) && (s == rhs.s) && (t == rhs.t) && (nx == rhs.nx) && (ny == rhs.ny) && (nz == rhs.nz)){
          return true;
      }
      return false;
  }

};

template <class T>
class VHash;

template <>
struct VHash<VertexData> {
    std::size_t operator()(VertexData const& s) const;
};

#endif
