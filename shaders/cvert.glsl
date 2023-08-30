// ==================================================================
#version 330 core
// Read in our attributes stored from our vertex buffer object
// We explicitly state which is the vertex information
// (The first 3 floats are positional data, we are putting in our vector)
layout(location=0)in vec3 position; 
layout(location=1)in int id; // Our second attribute - normals.

// If we have texture coordinates we can now use this as well
out vec3 FragPos;
flat out int f_id;

// If we are applying our camera, then we need to add some uniforms.
// Note that the syntax nicely matches glm's mat4!

uniform mat4 modelTransformMatrix; // Object space
uniform mat4 projectionMatrix;
uniform vec3 viewPos;  // Where our camera is

void main()
{
  gl_Position = projectionMatrix * modelTransformMatrix * vec4(position, 1.0f);;

  f_id = id;
}
// ==================================================================
