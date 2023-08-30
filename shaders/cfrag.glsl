// ==================================================================
#version 330 core

// The final output color of each 'fragment' from our fragment shader.
out vec4 FragColor;

// Take in our previous texture coordinates.
in vec3 FragPos;
flat in int f_id;


// If we have texture coordinates, they are stored in this sampler.
/* uniform sampler2D u_DiffuseMap; */ 
/* uniform sampler2D u_NormalMap; */ 


vec3 randomColor(uint id) {
	/* adopted from this https://stackoverflow.com/a/12996028/7487237 */
	uint x = id;
	x = ((x >> 16) ^ x) * 0x45d9f3bu;
	x = ((x >> 16) ^ x) * 0x45d9f3bu;
	x = (x >> 16) ^ x;

  vec3 ret;
	ret.r = float((x >> 0) % (1u << 8)) / float(1u<<8);
	ret.g = float((x >> 8) % (1u << 8)) / float(1u<<8);
	ret.b = float((x >> 16) % (1u << 8)) / float(1u<<8);

	return ret;
}

void main()
{
	if (f_id >= 0) {
		FragColor = vec4(randomColor(uint(f_id)), 1.0);
	} else {
		FragColor = vec4(1.0, 1.0, 1.0, 1.0);
	}

}
// ==================================================================
