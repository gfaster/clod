#include "VertexData.hpp"





/*
 *
 * This was copied form here: https://stackoverflow.com/a/19195373/7487237
 *
 * This *is* kinda complex so I figured I would try to understand it better
 *
 * The more I learn about c++ the more I hate it.
 *
 */

// template just means it can operate on generic classes
// the first argument is the running hash
// second arg is the field - for my case it'll be uints and floats
template <class T>
inline void hash_combine(std::size_t & s, const T & v) {
    std::hash<T> h;
    // this magic number is explained here https://stackoverflow.com/a/4948967/7487237
    // basically it's just a *very* random number
    // specifically it's the binary expansion of 1/phi
    s^= h(v) + 0x9e3779b9 + (s<< 6) + (s>> 2);
}



std::size_t VHash<VertexData>::operator()(VertexData const& s) const {
    std::size_t res = 0;
    hash_combine(res,s.x);
    hash_combine(res,s.y);
    hash_combine(res,s.z);
    hash_combine(res,s.s);
    hash_combine(res,s.t);
    hash_combine(res,s.nx);
    hash_combine(res,s.ny);
    hash_combine(res,s.nz);
    return res;
}
