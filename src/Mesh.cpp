#include "Mesh.hpp"
#include <iostream>
#include <map>
#include <deque>

Mesh::~Mesh() {

}

Mesh::Mesh(std::vector<VertexData> vbuffer, std::vector<uint> ibuffer) {
    /* CSRGraph csr = GAR_CreateMeshGraph(ibuffer); */
    /* std::vector<GAR_ClusterElements> eclusters =  GAR_GenerateClusters(csr); */



    /* convert the triangle list to GAR_Cluster list - this should move to GAR eventually */
    /* InitializeWith_unifiedClusters(vbuffer, ibuffer, eclusters); */
    /* GAR_GroupClusters(clusters, triIndicies); */
    InitializeWith_hierarchyClusters(vbuffer, ibuffer);
    CreateHierBuffer();

    /* DEBUG */
    /* for (uint i = 0; i < verticies.size(); ++i) { */
    /*     ClusterVertex v = verticies[i]; */
    /*     std::cout << "{ " << v.x << ", " << v.x << ", " << v.x << ", " << v.id << " }\n"; */
    /* } */

    /* create shaders */
    std::string vertexShader = m_shader.LoadShader("./shaders/cvert.glsl");
    std::string fragmentShader = m_shader.LoadShader("./shaders/cfrag.glsl");
    m_shader.CreateShader(vertexShader,fragmentShader);

    /* create buffers */
    /* Simplify(); */
    /* CreateGroupedBuffer(); */


    /* initialize tranform */
    m_transform.LoadIdentity();
}

void Mesh::InitializeWith_hierarchyClusters (std::vector<VertexData>& vbuffer, std::vector<uint>& ibuffer) {
    const bool clusterShading = true;
    for (uint i = 0; i < vbuffer.size(); ++i) {
        VertexData vtx = vbuffer[i];
        /* id is not used with this method */
        simpleVtx.push_back(GAR_Vec3f(vtx.x, vtx.y, vtx.z));
    }
    auto hc_tri = GAR_ComputeMesh(ibuffer, simpleVtx);
    hierarchyClusters.swap(hc_tri.first);
    triIndicies.swap(hc_tri.second);

    if (clusterShading) {
        for (uint i = 0; i < hierarchyClusters.size(); ++i) {
            std::map<uint, uint> clusterVerticies;
            GAR_HierarchyCluster& hc = hierarchyClusters[i];
            
            for (uint j = hc.data.tris; j < hc.data.tris + hc.data.numTri; ++j) {
                for (uint k = 0; k < 3; ++k) {
                    uint originalIndex = triIndicies[j].verticies[k];
                    if (clusterVerticies.count(originalIndex)) {
                        triIndicies[j].verticies[k] = clusterVerticies[originalIndex];
                    } else {
                        clusterVerticies.emplace(originalIndex, verticies.size());
                        triIndicies[j].verticies[k] = clusterVerticies[originalIndex];
                        GAR_Vec3f& vtx = simpleVtx[originalIndex];
                        verticies.push_back(GAR_ClusterVertex(vtx.x, vtx.y, vtx.z, i));
                    }
                }
            }
        }
    } else {
        for (uint i = 0; i < simpleVtx.size(); ++i) {
            GAR_Vec3f& vtx = simpleVtx[i];
            verticies.push_back(GAR_ClusterVertex(vtx.x, vtx.y, vtx.z, i));
        }
    }
}

void Mesh::InitializeWith_unifiedClusters(std::vector<VertexData>& vbuffer, std::vector<uint>& ibuffer,
        std::vector<GAR_ClusterElements>& eclusters) {

    /* add our verticies */
    for (uint i = 0; i < vbuffer.size(); ++i) {
        VertexData vtx = vbuffer[i];
        /* id is not used with this method */
        verticies.push_back(GAR_ClusterVertex(vtx.x, vtx.y, vtx.z, 2));
        simpleVtx.push_back(GAR_Vec3f(vtx.x, vtx.y, vtx.z));
    }

    for (uint i = 0; i < eclusters.size(); ++i) {
        GAR_ClusterElements* ce = &eclusters[i];
        GAR_Cluster c;
        c.tris = triIndicies.size();
        c.numTri = ce->numPrim;
        clusters.push_back(c);
        for (uint j = 0; j < ce->numPrim; ++j) {
            GAR_Tri t;
            uint idx_ibuf = ce->primativeIndicies[j] * 3;
            for (uint k = 0; k < 3; ++k) {
                t.verticies[k] = ibuffer[idx_ibuf + k];
            }
            triIndicies.push_back(t);
        }
    }
}

void Mesh::InitializeWith_distinctClusters(std::vector<VertexData>& vbuffer, std::vector<uint>& ibuffer,
        std::vector<GAR_ClusterElements>& eclusters) {
    for (uint i = 0; i < eclusters.size(); ++i) {
        GAR_ClusterElements* ce = &eclusters[i];
        GAR_Cluster c;
        c.tris = triIndicies.size();
        c.numTri = ce->numPrim;
        clusters.push_back(c);

        std::map<uint, uint> clusterVerticies;
        for (uint j = 0; j < ce->numPrim; ++j) {
            GAR_Tri t;
            uint idx_ibuf = ce->primativeIndicies[j] * 3;

            for (uint k = 0; k < 3; ++k) {
                uint idx_v = ibuffer[idx_ibuf + k];
                VertexData vtx_d = vbuffer[idx_v];
                GAR_ClusterVertex vtx = GAR_ClusterVertex(vtx_d.x, vtx_d.y, vtx_d.z, i);
                if (clusterVerticies.count(idx_v) == 0) {
                    clusterVerticies[idx_v] = verticies.size();
                    verticies.push_back(vtx);
                    simpleVtx.push_back(GAR_Vec3f(vtx_d.x, vtx_d.y, vtx_d.z));
                }
                t.verticies[k] = clusterVerticies[idx_v]; 
            }

            triIndicies.push_back(t);
        }
    }
}

void Mesh::Simplify() {
    groups = GAR_GroupClusters(clusters, triIndicies, 0, 5); 
    for (auto& group : groups) {
        std::set<uint> deleteMe;
        std::cerr << "this should not be called\n";
        exit(1);
        GAR_SimplifyGroup(group, triIndicies, simpleVtx, deleteMe);
    }
}

void Mesh::RenderCluster(uint idx_any) {
    /* uint idx = idx_any % clusters.size(); */


    // Call our helper function to just bind everything
    Bind();
	//Render data
    glDrawElements(GL_TRIANGLES,
                   triIndicies.size() * 3, // The number of indices, not triangles.
                   GL_UNSIGNED_INT,             // Make sure the data type matches
                        nullptr);               // Offset pointer to the data. 
                                                // nullptr because we are currently bound
}

Transform* Mesh::GetTransform() {
    return &m_transform;
}


// Bind everything we need in our object
// Generally this is called in update() and render()
// before we do any actual work with our object
void Mesh::Bind(){
    // Make sure we are updating the correct 'buffers'

    // Bind to our vertex array
    glBindVertexArray(m_VAOId);
    // Bind to our vertex information
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexPositionBuffer);
    // Bind to the elements we are drawing
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBufferObject);

    // Diffuse map is 0 by default, but it is good to set it explicitly
    /* m_textureDiffuse.Bind(0); */
    // We need to set the texture slot explicitly for the normal map  
    /* m_normalMap.Bind(1); */
    // Select our appropriate shader
    m_shader.Bind();
}

void Mesh::CreateGroupedBuffer() {
    uint stride = 3;
    uint start_idx = groups[0].lowClusters[0].tris * 3;
    uint vcount = simpleVtx.size() * 3;
    uint icount = triIndicies.size() * 3 - start_idx;


    std::cout << "group buf create\n";

    static_assert(sizeof(GLfloat)==sizeof(float),
            "GLFloat and gloat are not the same size on this architecture");

    // VertexArrays
    glGenVertexArrays(1, &m_VAOId);

    glBindVertexArray(m_VAOId);

    // Vertex Buffer Object (VBO)
    // Create a buffer (note we’ll see this pattern of code often in OpenGL)
    // TODO: Read this and understand what is going on
    glGenBuffers(1, &m_vertexPositionBuffer); // selecting the buffer is
    // done by binding in OpenGL
    // We tell OpenGL then how we want to 
    // use our selected(or binded)
    //  buffer with the arguments passed 
    // into the function.
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexPositionBuffer);
    glBufferData(GL_ARRAY_BUFFER, vcount*sizeof(float), simpleVtx.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    // Finally pass in our vertex data
    glVertexAttribPointer(  0,   // Attribute 0, which will match layout in shader
            3,   // size (Number of components (2=x,y)  (3=x,y,z), etc.)
            GL_FLOAT, // Type of data
            GL_FALSE, // Is the data normalized
            sizeof(float)*stride, // Stride - Amount of bytes between each vertex.
            // If we only have vertex data, then
            // this is sizeof(float)*3 (or as a
            // shortcut 0).
            // That means our vertices(or whatever data) 
            // is tightly packed, one after the other.
            // If we add in vertex color information(3 more floats), 
            // then this becomes 6, as we
            // move 6*sizeof(float)
            // to get to the next chunk of data.
            // If we have normals, then we
            // need to jump 3*sizeof(GL_FLOAT)
            // bytes to get to our next vertex.
            0               // Pointer to the starting point of our
            // data. If we are just grabbing vertices, 
            // this is 0. But if we have
            // some other attribute,
            // (stored in the same data structure),
            // this may vary if the very
            // first element is some different attribute.
            // If we had some data after
            // (say normals), then we 
            // would have an offset of 
            // 3*sizeof(GL_FLOAT) for example
        );

    /* our ids */
    glEnableVertexAttribArray(1);
    glVertexAttribPointer( 1, 1, GL_INT, GL_FALSE, stride * sizeof(float), (void*) (0 * sizeof(float)));

    // Another Vertex Buffer Object (VBO)
    // This time for your index buffer.
    // TODO: put these static_asserts somewhere
    static_assert(sizeof(unsigned int)==sizeof(GLuint),"Gluint not same size!");

    glGenBuffers(1, &m_indexBufferObject);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBufferObject);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, icount*sizeof(unsigned int), 
            ((uint*) triIndicies.data()) + start_idx, GL_STATIC_DRAW);
}

void Mesh::UpdateHier(GAR_Vec3f cameraPos) {
    std::vector<uint> renderClusters_idx = FindGoodClusters(cameraPos);
    /* std::vector<uint> renderClusters_idx = FindGoodClustersDiscrete(cameraPos); */
    std::vector<GAR_Tri> renderTris;
    for (auto& idx : renderClusters_idx) {
        GAR_Cluster c = hierarchyClusters[idx].data;
        renderTris.insert(renderTris.end(), &triIndicies[c.tris], &triIndicies[c.tris + c.numTri]);
    }
    uint icount = renderTris.size() * 3;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, icount*sizeof(unsigned int), 
            ((uint*) renderTris.data()), GL_DYNAMIC_DRAW);
}

void Mesh::CreateHierBuffer() {
    uint stride = 4;
    uint vcount = verticies.size() * 4;



    std::cout << "group buf create\n";

    static_assert(sizeof(GLfloat)==sizeof(float),
            "GLFloat and gloat are not the same size on this architecture");

    // VertexArrays
    glGenVertexArrays(1, &m_VAOId);

    glBindVertexArray(m_VAOId);

    // Vertex Buffer Object (VBO)
    // Create a buffer (note we’ll see this pattern of code often in OpenGL)
    // TODO: Read this and understand what is going on
    glGenBuffers(1, &m_vertexPositionBuffer); // selecting the buffer is
    // done by binding in OpenGL
    // We tell OpenGL then how we want to 
    // use our selected(or binded)
    //  buffer with the arguments passed 
    // into the function.
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexPositionBuffer);
    glBufferData(GL_ARRAY_BUFFER, vcount*sizeof(float), verticies.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    // Finally pass in our vertex data
    glVertexAttribPointer(  0,   // Attribute 0, which will match layout in shader
            3,   // size (Number of components (2=x,y)  (3=x,y,z), etc.)
            GL_FLOAT, // Type of data
            GL_FALSE, // Is the data normalized
            sizeof(float)*stride, // Stride - Amount of bytes between each vertex.
            // If we only have vertex data, then
            // this is sizeof(float)*3 (or as a
            // shortcut 0).
            // That means our vertices(or whatever data) 
            // is tightly packed, one after the other.
            // If we add in vertex color information(3 more floats), 
            // then this becomes 6, as we
            // move 6*sizeof(float)
            // to get to the next chunk of data.
            // If we have normals, then we
            // need to jump 3*sizeof(GL_FLOAT)
            // bytes to get to our next vertex.
            0               // Pointer to the starting point of our
            // data. If we are just grabbing vertices, 
            // this is 0. But if we have
            // some other attribute,
            // (stored in the same data structure),
            // this may vary if the very
            // first element is some different attribute.
            // If we had some data after
            // (say normals), then we 
            // would have an offset of 
            // 3*sizeof(GL_FLOAT) for example
        );

    /* our ids */
    glEnableVertexAttribArray(1);
    glVertexAttribPointer( 1, 1, GL_INT, GL_FALSE, stride * sizeof(uint), (void*) (3 * sizeof(float)));

    // Another Vertex Buffer Object (VBO)
    // This time for your index buffer.
    // TODO: put these static_asserts somewhere
    static_assert(sizeof(unsigned int)==sizeof(GLuint),"Gluint not same size!");

    glGenBuffers(1, &m_indexBufferObject);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBufferObject);
}

/* I want this in the shader */
std::vector<uint> Mesh::FindGoodClusters(GAR_Vec3f cameraPos) {
    std::vector<uint> out;
    std::deque<uint> toEvalulate;
    /* in the future, we will have multiple roots */
    /* right now, we have at least 1, and at most 2, which is why we enqueue these */
    toEvalulate.push_back(hierarchyClusters.size() - 1);
    toEvalulate.push_back(hierarchyClusters.size() - 2); 

    glm::mat4 trmat = m_transform.GetInternalMatrix();



    uint count = 0;
    while (toEvalulate.size() > 0) {
        uint hc_idx = toEvalulate.front();
        GAR_HierarchyCluster* hc = &hierarchyClusters[hc_idx];
        toEvalulate.pop_front();

        glm::vec4 center(hc->center.x, hc->center.y, hc->center.z, 1);
        glm::vec4 pos = trmat * center;

        float tx = pos.x;
        float ty = pos.y;
        float tz = pos.z;

        float sqdist = (cameraPos.x - tx) * (cameraPos.x - tx) + 
                       (cameraPos.y - ty) * (cameraPos.y - ty) +
                       (cameraPos.z - tz) * (cameraPos.z - tz);
        float distfact = 0.0003;
        float r1 = hc->radius;

        /* check if we can render this cluster, or if we should check the children */
        if (r1 > (distfact * sqdist)) {
            for (uint i = 0; i < hc->numChildren; ++i) {
                if (hc->firstParentFlag & (1 << i)) {
                    toEvalulate.push_back(hc->children[i]);
                }
            }
            if (hc->numChildren == 0) {
                /* this is a leaf cluster, we always want to render it */
                out.push_back(hc_idx);
            }
        } else {
            out.push_back(hc_idx);
        }

        /* this function has the nasty habit of going into infinite loops */
        if (count > 1000000) {
            std::cerr << "there is a loop here somewhere, killing\n";
            std::cerr << "total of " << toEvalulate.size() << "left in queue\n";
            exit(1);
        }
        count += 1;
    }

    /* sum up total tris for printing */
    uint triNum = 0;
    for (uint i = 0; i < out.size(); ++i) {
        triNum += hierarchyClusters[out[i]].data.numTri;
    }
    std::cout << "total tris: " << triNum << std::endl;

    return out;
}


std::vector<uint> Mesh::FindGoodClustersDiscrete(GAR_Vec3f cameraPos) {
    std::vector<uint> out;

    /* glm::mat4 trmat = m_transform.GetInternalMatrix(); */



    uint level = 4;
    for (uint i = 0; i < hierarchyClusters.size(); ++i) {
        if (hierarchyClusters[i].level == level) {
            out.push_back(i);
        }
    }

    std::cout << out.size() << std::endl;

    return out;
}

void Mesh::CreateClusteredBuffer() {

    uint stride = 4; /* vertex pos + cluster index */
    uint vcount = verticies.size() * 4;
    uint icount = triIndicies.size() * 3;

    std::cout << "buf create\n";
    

    static_assert(sizeof(GLfloat)==sizeof(float),
            "GLFloat and gloat are not the same size on this architecture");

    // VertexArrays
    glGenVertexArrays(1, &m_VAOId);

    glBindVertexArray(m_VAOId);

    // Vertex Buffer Object (VBO)
    // Create a buffer (note we’ll see this pattern of code often in OpenGL)
    // TODO: Read this and understand what is going on
    glGenBuffers(1, &m_vertexPositionBuffer); // selecting the buffer is
    // done by binding in OpenGL
    // We tell OpenGL then how we want to 
    // use our selected(or binded)
    //  buffer with the arguments passed 
    // into the function.
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexPositionBuffer);
    glBufferData(GL_ARRAY_BUFFER, vcount*sizeof(float), verticies.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    // Finally pass in our vertex data
    glVertexAttribPointer(  0,   // Attribute 0, which will match layout in shader
            3,   // size (Number of components (2=x,y)  (3=x,y,z), etc.)
            GL_FLOAT, // Type of data
            GL_FALSE, // Is the data normalized
            sizeof(float)*stride, // Stride - Amount of bytes between each vertex.
            // If we only have vertex data, then
            // this is sizeof(float)*3 (or as a
            // shortcut 0).
            // That means our vertices(or whatever data) 
            // is tightly packed, one after the other.
            // If we add in vertex color information(3 more floats), 
            // then this becomes 6, as we
            // move 6*sizeof(float)
            // to get to the next chunk of data.
            // If we have normals, then we
            // need to jump 3*sizeof(GL_FLOAT)
            // bytes to get to our next vertex.
            0               // Pointer to the starting point of our
            // data. If we are just grabbing vertices, 
            // this is 0. But if we have
            // some other attribute,
            // (stored in the same data structure),
            // this may vary if the very
            // first element is some different attribute.
            // If we had some data after
            // (say normals), then we 
            // would have an offset of 
            // 3*sizeof(GL_FLOAT) for example
        );

    /* our ids */
    glEnableVertexAttribArray(1);
    glVertexAttribPointer( 1, 1, GL_INT, GL_FALSE, stride * sizeof(float), (void*) (3 * sizeof(float)));

    // Another Vertex Buffer Object (VBO)
    // This time for your index buffer.
    // TODO: put these static_asserts somewhere
    static_assert(sizeof(unsigned int)==sizeof(GLuint),"Gluint not same size!");

    glGenBuffers(1, &m_indexBufferObject);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBufferObject);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, icount*sizeof(unsigned int), triIndicies.data(),GL_STATIC_DRAW);
}


void Mesh::Update(unsigned int screenWidth, unsigned int screenHeight){
    // Call our helper function to just bind everything
    GAR_Vec3f viewPos(0.0, 0.0, 0.0);
    UpdateHier(viewPos);
    Bind();
    // TODO: Read and understand
    // For our object, we apply the texture in the following way
    // Note that we set the value to 0, because we have bound
    // our texture to slot 0.
    /* m_shader.SetUniform1i("u_DiffuseMap",0); */
    // If we want to load another texture, we assign it to another slot
    /* m_shader.SetUniform1i("u_NormalMap",1); */  
    // Here we apply the 'view' matrix which creates perspective.
    // The first argument is 'field of view'
    // Then perspective
    // Then the near and far clipping plane.
    // Note I cannot see anything closer than 0.1f units from the screen.
    // TODO: In the future this type of operation would be abstracted away
    //       in a camera class.
    m_projectionMatrix = glm::perspective(45.0f,((float)screenWidth)/((float)screenHeight),0.1f,200.0f);

    // Set the uniforms in our current shader
    m_shader.SetUniformMatrix4fv("modelTransformMatrix",m_transform.GetTransformMatrix());
    m_shader.SetUniformMatrix4fv("projectionMatrix", &m_projectionMatrix[0][0]);

    // Create a first 'light'
    // Set in a light source position
    m_shader.SetUniform1i("clusterTarget", 1);	
    // Set a view and a vector
    m_shader.SetUniform3f("viewPos",viewPos.x,viewPos.y,viewPos.z);

    /* std::cout << "update\n"; */
}

