#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include <glad/glad.h>
#include "glm/vec3.hpp"
#include "glm/gtc/matrix_transform.hpp"

// The purpose of this class is to store
// transformations of 3D entities (cameras, objects, etc.)
class Transform{
public:

    // Constructor for a new transform
    Transform();
    // Destructor for the transform
    ~Transform();
    // Resets matrix transformations to the identity matrix
    void LoadIdentity();
    // Perform a translation of an object
    void Translate(float x, float y, float z);
    // Perform rotation about an axis
    void Rotate(float radians, float x, float y, float z);
    // Perform rotation about an axis
    void Scale(float x, float y, float z);
    // Returns the transformation matrix
    GLfloat* GetTransformMatrix();
    // Apply Transform
    // Takes in a transform and sets internal
    // matrix.
    void ApplyTransform(Transform t);
    // Returns the transformation matrix
    glm::mat4 GetInternalMatrix();
private:
    // Stores the actual transformation matrix
    glm::mat4 m_modelTransformMatrix;
};


#endif
