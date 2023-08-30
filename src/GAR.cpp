#include "GAR.hpp"

#include <metis.h>

#include <set>
#include <map>
#include <iostream>
#include <time.h>
#include <algorithm>
#include <iterator>

CSRGraph GAR_CreateMeshGraph(std::vector<uint>& ibuffer) {
    /* this assumes tris */

    /* construct CSR */
    /* It took me a while to figure this out
     * eptr.size() = num tri + 1
     * eind.size() = num tri * 3
     *
     * eptr[i] = first tri edge vertex in eind
     * tri (or larger primative) is defined by edges (pairs of verticies)
     *
     * See mesh.c in METIS library for more details
     */

    std::cout << "making csr graph for ibuffer size " << ibuffer.size() << std::endl;
    if (ibuffer.size() == 0) {
        std::cout << "tried to create a mesh graph for a 0-length ibuffer\n";
        exit(1);
    }

    /* I think METIS can't handle when the indicies are not 0-indexed */
    std::vector<idx_t> eptr; 
    std::vector<idx_t> eind;

    for (uint i = 0; i < ibuffer.size() - 2; i += 3) {
        eptr.push_back(eind.size());
        for (uint j = 0; j < 3; ++j) {
            eind.push_back(ibuffer[i + j]);
        }
    }
    eptr.push_back(eind.size());

    CSRGraph out(eptr, eind);
    return out;
}

/* void GAR_EClustersToClusters (std::vector<) */

std::vector<GAR_ClusterElements>
GAR_GenerateClusters(CSRGraph& meshGraph, uint clustnum, uint tries) {
    /* partitioning */
    idx_t options[METIS_NOPTIONS];
    METIS_SetDefaultOptions(options);

    options[METIS_OPTION_OBJTYPE] = METIS_OBJTYPE_CUT;
    options[METIS_OPTION_NUMBERING] = 0;
    options[METIS_OPTION_SEED] = time(NULL);
    /* I would like this but it isn't supported */
    /* options[METIS_OPTION_CONTIG] = 1; */
    
    idx_t ne = meshGraph.first.size() - 1; /* Number of elements (tris) */
    idx_t nn = meshGraph.second.size(); /* Number of nodes (verticies) */
    idx_t* eptr = meshGraph.first.data(); /* Tri start, see GAR_CreateMeshGraph */
    idx_t* eind = meshGraph.second.data(); /* Tri edge indicies, see GAR_CreateMeshGraph */
    /* idx_t* vwgt = nullptr; */
    /* idx_t* vsize = nullptr; */
    idx_t ncommon = 2; /* Number of shared verticies to create a connection between polys */

    /* TODO: figure out what exactly this needs to be */
    idx_t nparts;
    if (clustnum == 0) {
        nparts = ne / (MESH_SIZE * 6 / 7); /* Number of partitions, I made it somewhat less than 64 */
    } else {
        nparts = clustnum;
    }

    if (nparts < 2) {
        /* std::cerr << "Not enough partitions\n"; */
        if (ne >= MESH_SIZE) {
            /* std::cerr << "\t...and yet too many tris! (I'm going to fix this)\n"; */
            nparts = 2;
        } else {
            std::vector<GAR_ClusterElements> out;
            GAR_ClusterElements c{0};
            c.numPrim = ne;
            for (uint i = 0; i < ne; ++i) {
                c.primativeIndicies[i] = i;
            }
            out.push_back(c);
            return out;
        }
    }

    /* idx_t* tpwgts = nullptr; /1* idk what this is *1/ */

    /* outputs, options immediately precedes in args */
    idx_t objval; /* total cuts made */
    idx_t* epart = new idx_t[ne]; /* I think this is the partition id of the elements */
    idx_t* npart = new idx_t[nn]; /* I think this is the partitioning of edges, but I'm not sure */


    idx_t return_code = METIS_PartMeshDual(
            &ne,
            &nn,
            eptr,
            eind,
            nullptr, /* I need to actually define these */
            nullptr,
            &ncommon,
            &nparts,
            nullptr,
            options,
            &objval,
            epart,
            npart
            );

    switch(return_code) {
        case METIS_OK:
            /* std::cout << "Partition success!" << std::endl; */
            break;
        case METIS_ERROR:
            std::cout << "Error partitioning" << std::endl;
            exit(1);
        case METIS_ERROR_MEMORY:
            std::cout << "Error partitioning: out of memory" << std::endl;
            exit(1);
        case METIS_ERROR_INPUT:
            std::cout << "Error partitioning: bad input" << std::endl;
            exit(1);
    }


    /* turn into clusters */
    std::vector<GAR_ClusterElements> out;
    out.resize(nparts);
    for (uint i = 0; i < ne; ++i) {
        GAR_ClusterElements* cluster = &out[epart[i]];

        if (cluster->numPrim >= MESH_SIZE) {
            std::cout << "Meshlet too big! (" << tries << " tries left)"<< std::endl;
            if (tries > 0) {
                delete[] epart;
                delete[] npart;
                return GAR_GenerateClusters(meshGraph, clustnum, tries - 1);
            } else {
                exit(1);
            }
        }

        cluster->primativeIndicies[cluster->numPrim] = i;
        cluster->numPrim += 1;
    }

    int max = 0;
    int count = 0;
    for (uint i = 0; i < nparts; ++i) {
        /* std::cout << "Cluster #" << i << " has " << (int) out[i].numPrim << " tris" << std::endl; */
        int num = (int) out[i].numPrim;
        if (num == max) {
            count += 1;
        }
        if (num > max) {
            max = num;
            count = 1;
        }
    }
    /* std::cout << "Maximum of " << max << " tris in " << count << " clusters" << std::endl; */

    delete[] epart;
    delete[] npart;


    return out;
}

void GAR_ClusterOBJExport(std::vector<uint>& ibuffer, std::vector<VertexData>& vbuffer,
                          GAR_ClusterElements& cluster) {
    /* print verticies */
    uint numv = 1; /* start at 1 bc OBJ is 1-indexed */
    std::map<VertexData*, uint> s_addedVerticies;
    for (uint i = 0; i < cluster.numPrim; ++i) {
        uint idx_prim = cluster.primativeIndicies[i];
        for (uint j = 0; j < 3; ++j) {
            VertexData* vertex = &vbuffer[ibuffer[idx_prim * 3 + j]];
            if (s_addedVerticies.count(vertex) == 0) {
                s_addedVerticies.emplace(vertex, numv);
                std::cout << "v " << vertex->x << " " << vertex->y << " " << vertex->z << std::endl;
                numv += 1;
            }
        }
    }

    /* print faces */
    for (uint i = 0; i < cluster.numPrim; ++i) {
        std::cout <<"f";

        uint idx_prim = cluster.primativeIndicies[i];
        for (uint j = 0; j < 3; ++j) {
            VertexData* vertex = &vbuffer[ibuffer[idx_prim * 3 + j]];

            std::cout << " " << s_addedVerticies[vertex];
        }

        std::cout << std::endl;
    }
}


/* this assumes that same verticies are the same */
std::vector<GAR_Group> GAR_GroupClusters(std::vector<GAR_Cluster>& clusters, std::vector<GAR_Tri>& tris,
        uint partnum, uint tries) {
    /* create CSR based on clustering. This is a graph, not a mesh. */


    /* METIS doesn't like it if we don't have more than one split */
    if (clusters.size() <= 4) {
        std::vector<GAR_Group> out;
        GAR_Group group {
            {}, /* highClusters */
            {}, /* lowClusters */
            (uint8) clusters.size(), /* numHigh */
            0 /* numLow */
        };
        for (uint i = 0; i < clusters.size(); ++i) {
            group.highClusters[i] = clusters[i];
        }

        out.push_back(group);
        return out;
    }

    /* find what neighbors each vertex belongs to */
    std::map<uint, std::map<uint, uint>> v_clust; /* vertex: clusters with vertex and the edge count */
    for (uint i = 0; i < clusters.size(); ++i) {
        GAR_Cluster* c = &clusters[i];
        for (uint j = 0; j < c->numTri; ++j) {
            for (uint k = 0; k < 3; ++k) {
                uint v = tris[c->tris + j].verticies[k];
                ++v_clust[v][i];
            }
        }
    }

    /* building the CSR */
    std::vector<idx_t> xadj; /* start indicies of nodes */
    std::vector<idx_t> adjncy; /* adjacent nodes to node xadj[i] to xadj[i+1] */
    std::vector<idx_t> adjwgt; /* adjwgt[i] is weight of adjncy[i] */

    for (uint i = 0; i < clusters.size(); ++i) {
        GAR_Cluster* c = &clusters[i];

        /* get the size of each border */
        std::map<idx_t, idx_t> borderWeight;
        for (uint j = 0; j < c->numTri; ++j) {
            for (uint k = 0; k < 3; ++k) {
                uint v = tris[c->tris + j].verticies[k];
                for (auto& [neighbor, count]: v_clust[v]) {
                    if (neighbor == i) { continue; }
                    borderWeight[neighbor] += count;
                }
            }
        }

        /* Add CSR entry */
        xadj.push_back(adjncy.size());
        for (auto& [neighbor, weight]: borderWeight) {
            if (weight == 1) { continue; } /* non-manifold groups are no good */
            adjncy.push_back(neighbor);
            adjwgt.push_back(weight);
        }
    }
    xadj.push_back(adjncy.size());


    if (partnum == 0) {
        partnum = std::max((size_t) 2, clusters.size() / 3);
    }

    /* partition */

    idx_t options[METIS_NOPTIONS];
    METIS_SetDefaultOptions(options);

    options[METIS_OPTION_OBJTYPE] = METIS_OBJTYPE_CUT;
    options[METIS_OPTION_NUMBERING] = 0;
    timespec tp{};
    clock_gettime(CLOCK_REALTIME, &tp);
    options[METIS_OPTION_SEED] = (idx_t) tp.tv_nsec;
    /* I'm not exactly sure what this means 100% */
    /* options[METIS_OPTION_CONTIG] = 1; */ 

    idx_t nvtxs = clusters.size();
    idx_t ncon = 1; // probably either 1 or 2
    /* xadj, adjncy */
    /* NULL: vwgt, vsize */
    /* adjwgt */
    idx_t nparts = partnum;
    /* NULL: tpwgts, ubvec */
    /* options */
    idx_t objval;
    idx_t* part = new idx_t[nvtxs];

    idx_t return_code = METIS_PartGraphKway(&nvtxs, &ncon, xadj.data(), adjncy.data(), nullptr,
            nullptr, adjwgt.data(), &nparts, nullptr, nullptr, options, &objval, part);

    switch(return_code) {
        case METIS_OK:
            /* std::cout << "Grouping success!" << std::endl; */
            break;
        case METIS_ERROR:
            std::cout << "Error grouping" << std::endl;
            exit(1);
        case METIS_ERROR_MEMORY:
            std::cout << "Error grouping: out of memory" << std::endl;
            exit(1);
        case METIS_ERROR_INPUT:
            std::cout << "Error grouping: bad input" << std::endl;
            exit(1);
    }

    /* I think it's possible (but unlikely) for this to have more than 4 per group */

    /* build our groups */
    std::vector<GAR_Group> groups;
    groups.resize(nparts, {0});
    for (uint i = 0; i < clusters.size(); ++i) {
        GAR_Group* g = &groups[part[i]];
        GAR_Cluster* c = &clusters[i];
        if (g->numHigh >= 4) {
            std::cout << "Group has too many clusters, " << tries << " left" << std::endl;
            if (tries <= 0) { exit(1); }
            delete[] part;
            return GAR_GroupClusters(clusters, tris, (uint) (partnum * 1.1), tries - 1);
            /* exit(1); */
            continue;
        }
        g->highClusters[g->numHigh] = *c;
        g->numHigh += 1;
    }

    /* debug */
    float sum = 0;
    for (auto& g : groups) {
        uint8 num = g.numHigh;
        sum += num;
    }
    /* std::cout << "Average clusters in groups: " << sum / groups.size() << std::endl; */

    delete[] part;

    return groups;
}


uint GAR_FindMappedIndex(uint idx, std::map<uint, uint>& collapseMapping) {
    /* this can be optimized by updating chains */
    uint tries = 0;
    while (collapseMapping.count(idx) > 0) {
        idx = collapseMapping[idx];
        tries += 1;
        if (tries > 1000) {
            std::cout << tries << " tires now - there is probably a loop\n";
            exit(1);
        }
    }
    return idx;
}

/* returns vertex indicies as if GAR_Tri was flattened out */
/* ex: tris[6].verticies[1] = 3 * 6 + 1 */
/* holy cow this is just a lie wtf - it's the vertex index like you would expect */
/* assumes 2-manifold */
std::set<uint> GAR_FindBoundary(std::vector<GAR_Cluster>& clusters, std::vector<GAR_Tri>& tris) {
    /* iterate over all triangle edges and find ones that are interior */
    std::set<std::pair<uint,uint>> edges;
    std::set<std::pair<uint,uint>> matchedEdges;
    for (auto& cluster : clusters) {
        for (uint i = 0; i < cluster.numTri; ++i) {
            uint idx_tri = i + cluster.tris;
            for (uint k = 0; k < 3; ++k) {
                uint first = tris[idx_tri].verticies[k];
                uint second = tris[idx_tri].verticies[(k + 1) % 3];

                /* second in emplace return is if new element inserted */
                std::pair<uint, uint> edge1(first, second);
                if (! edges.insert(edge1).second) {
                    matchedEdges.insert(edge1);
                }
                std::pair<uint, uint> edge2(second, first);
                if (! edges.insert(edge2).second) {
                    matchedEdges.insert(edge2);
                }
            }
        }
    }

    /* add vertices not in a matched edge to ret */
    std::set<uint> out;
    for (auto& edge : edges) {
        if (matchedEdges.count(edge) == 0) {
            out.insert(edge.first);
            out.insert(edge.second);
        }
    }

    return out;
}

bool GAR_SimplifyGroup(GAR_Group& group, std::vector<GAR_Tri>& tris,
        std::vector<GAR_Vec3f>& verticies, std::set<uint>& boundaries) {
    /*
     * This is all very hard, I think I'm just going to use the 
     * minimum edge length collapse metric
     */

    uint startSize_verticies = tris.size();

    /* Build adjacency */
    /* I want to actually maybe ignore edges, and just do vertex proximity */
    std::map<uint, std::set<uint>> adjVerticies;
    std::vector<GAR_Cluster> vclusters;
    uint totalTri = 0;
    /* std::set<uint> groupVerticies; */
    /* for (uint g = 0; g < group.numHigh; ++g) { */
    /*     GAR_Cluster c = group.highClusters[g]; */
    /*     vclusters.push_back(c); */
    /*     totalTri += c.numTri; */
    /*     for (uint i = 0; i < c.numTri; ++i) { */
    /*         GAR_Tri t = tris[c.tris + i]; */
    /*         for (uint j = 0; j < 3; ++j) { */
    /*             adjVerticies[t.verticies[j]].push_back(t.verticies[(j+1) % 3]); */
    /*         } */
    /*     } */
    /* } */
    
    /* add connected verticies */
    for (uint g = 0; g < group.numHigh; ++g) {
        GAR_Cluster c = group.highClusters[g];
        vclusters.push_back(c);
        totalTri += c.numTri;
        for (uint i = 0; i < c.numTri; ++i) {
            GAR_Tri t = tris[c.tris + i];
            for (uint j = 0; j < 3; ++j) {
                adjVerticies[t.verticies[j]].emplace(t.verticies[(j+1) % 3]);
            }
        }
    }

    /* add very close verticies */
    /* this is slow, but clusters are fairly small */
    /* for (auto& [v1_idx, v1Adjs] : adjVerticies) { */
    /*     GAR_Vec3f v1 = verticies[v1_idx]; */
    /*     for (auto& [v2_idx, v2Adjs]: adjVerticies) { */
    /*         if (v1_idx == v2_idx) { continue; } */
    /*         GAR_Vec3f v2 = verticies[v2_idx]; */
    /*         float maxSqDist = 0; */
    /*         for (auto& adj_idx : v2Adjs) { */
    /*             GAR_Vec3f adj = verticies[adj_idx]; */
    /*             float sqDist = (adj.x - v2.x) * (adj.x - v2.x) + */ 
    /*                            (adj.y - v2.y) * (adj.y - v2.y) + */
    /*                            (adj.z - v2.z) * (adj.z - v2.z); */
    /*             if (sqDist > maxSqDist) { */
    /*                 maxSqDist = sqDist; */
    /*             } */
    /*         } */

    /*         float sqDist = (v1.x - v2.x) * (v1.x - v2.x) + */ 
    /*                        (v1.y - v2.y) * (v1.y - v2.y) + */
    /*                        (v1.z - v2.z) * (v1.z - v2.z); */

    /*         if (sqDist > maxSqDist) { */
    /*             TODO: Renable */
    /*             v2Adjs.emplace(v1_idx); */
    /*         } */
            
    /*     } */
    /* } */

    std::set<uint> groupBorder = GAR_FindBoundary(vclusters, tris);
    std::vector<uint> boundary_temp;
    std::set_intersection(boundaries.begin(), boundaries.end(),
            groupBorder.begin(), groupBorder.end(),
            std::back_inserter(boundary_temp));
    std::set<uint> boundary(boundary_temp.begin(), boundary_temp.end());
    /* boundary = groupBorder; */

    /* for (auto& v : boundary) { std::cout << v << std::endl; } */
    /* std::cout << boundary.size() << std::endl; */

    /* collapse the lowest value edge until too few remain */
    uint trisRemain = (uint) totalTri;
    std::map<uint, uint> collapseMapping; 
    while (true) {
        /* find the most useless edge */
        float minLength = 99999999999;
        int minEdgeStartV = -1;
        int minEdgeEndV = -1;
        /* iterate over all edges */
        for (auto& [startV, adjs] : adjVerticies) {
            uint idx_v1 = GAR_FindMappedIndex(startV, collapseMapping);
            GAR_Vec3f v1 = verticies[idx_v1];
            for (auto& adj : adjs) {
                uint idx_v2 = GAR_FindMappedIndex(adj, collapseMapping);
                GAR_Vec3f v2 = verticies[idx_v2];

                /* TODO: improve this metric */
                float sqlength = (v1.x - v2.x) * (v1.x - v2.x) + 
                    (v1.y - v2.y) * (v1.y - v2.y) +
                    (v1.z - v2.z) * (v1.z - v2.z);

                /* std::cout << sqlength << std::endl; */

                if (sqlength < minLength &&
                        boundary.count(idx_v1) == 0 && boundary.count(idx_v2) == 0 &&
                        idx_v1 != idx_v2) {
                    minLength = sqlength;
                    minEdgeStartV = idx_v1;
                    minEdgeEndV = idx_v2;
                }

            }
        }

        /* do collapse */
        if (minEdgeStartV == -1 || minEdgeEndV == -1) {
            /* std::cerr << "no shortest was found!" <<std::endl; */
            break;
        }
        GAR_Vec3f v1 = verticies[minEdgeStartV];
        GAR_Vec3f v2 = verticies[minEdgeEndV];
        /* TODO: pick better pos */
        GAR_Vec3f newV = GAR_Vec3f((v1.x + v2.x) / 2, (v1.y + v2.y) / 2, (v1.z + v2.z) / 2);
        verticies.push_back(newV);
        collapseMapping[minEdgeStartV] = verticies.size() - 1;
        collapseMapping[minEdgeEndV] = verticies.size() - 1;
        trisRemain -= 2;

        if (trisRemain < MESH_SIZE * 2 * 4/5) {
            break;
        }
    }

    /* partition the simplified mesh */
    std::vector<uint> ibuffer;
    std::map<uint, uint> ungapMapping; /* ugh. METIS can't handle gaps */
    std::vector<uint> regapMapping;
    for (uint g = 0; g < group.numHigh; ++g) {
        GAR_Cluster c = group.highClusters[g];
        for (uint i = 0; i < c.numTri; ++i) {
            GAR_Tri t = tris[c.tris + i];
            for (uint k = 0; k < 3; ++k) {
                uint gapped_idx = GAR_FindMappedIndex(t.verticies[k], collapseMapping);
                if (ungapMapping.count(gapped_idx) == 0) {
                    uint ungap_idx = ungapMapping.size();
                    ungapMapping[gapped_idx] = ungap_idx;
                    regapMapping.push_back(gapped_idx);
                }
            }
            uint idx_v1 = GAR_FindMappedIndex(t.verticies[0], collapseMapping);
            uint idx_v2 = GAR_FindMappedIndex(t.verticies[1], collapseMapping);
            uint idx_v3 = GAR_FindMappedIndex(t.verticies[2], collapseMapping);

            if (idx_v1 == idx_v2 || idx_v2 == idx_v3 || idx_v1 == idx_v3) {
                continue;
            } else {
                ibuffer.push_back(ungapMapping[idx_v1]);
                ibuffer.push_back(ungapMapping[idx_v2]);
                ibuffer.push_back(ungapMapping[idx_v3]);
            }
        }
    }

    if (ibuffer.size() / 3 != trisRemain) {
        /* std::cerr << "ALERT: incorrect tri prediction\n\tActual: " */
        /*     << ibuffer.size() / 3 << "\n\tExpected: " << trisRemain << std::endl; */
    }

    if (ibuffer.size() / 3 > 2 * MESH_SIZE) {
        std::cerr << "We weren't able to simplify enough!\n";
        std::cerr << "Boundary of size " << boundary.size()
            << " left us with " << ibuffer.size() / 3 << " tris\n";
        verticies.erase(verticies.begin() + startSize_verticies, verticies.end());
        return false;
    }
    
    if (ibuffer.size() == 0) {
        std::cerr << "somehow have zero length ibuffer after simplify" << std::endl;
        verticies.erase(verticies.begin() + startSize_verticies, verticies.end());
        return false;
    }

    CSRGraph g = GAR_CreateMeshGraph(ibuffer);

    std::vector<GAR_ClusterElements> elements = GAR_GenerateClusters(g, 2, 5);
    if (elements.size() > 2) {
        std::cerr << "too many elements in group split" << std::endl;
    }

    /* build clusters and append to original tri list */
    for (auto& element : elements) {
        GAR_Cluster c;
        c.numTri = element.numPrim;
        c.tris = tris.size();
        group.lowClusters[group.numLow] = c;
        ++group.numLow;
        for (uint i = 0; i < element.numPrim; ++i) {
            GAR_Tri t;
            t.verticies[0] = regapMapping[ibuffer[element.primativeIndicies[i] * 3 + 0]];
            t.verticies[1] = regapMapping[ibuffer[element.primativeIndicies[i] * 3 + 1]];
            t.verticies[2] = regapMapping[ibuffer[element.primativeIndicies[i] * 3 + 2]];
            tris.push_back(t);
        }
    }

    return true;
}


std::vector<GAR_Cluster> GAR_GetClustersFromGroup(std::vector<GAR_Group>& groups) {
    std::set<uint> addedClusters;
    std::vector<GAR_Cluster> out;
    for (auto& group : groups) {
        for (uint i = 0; i < group.numHigh; ++i) {
            if (addedClusters.insert(group.highClusters[i].tris).second) {
                out.push_back(group.highClusters[i]);
            }
        }
        for (uint i = 0; i < group.numLow; ++i) {
            if (addedClusters.insert(group.lowClusters[i].tris).second) {
                out.push_back(group.lowClusters[i]);
            }
        }
    }

    return out;
}

std::pair<float, GAR_Vec3f> GAR_CalculateClusterRadiusCenter(GAR_Cluster& cluster,
                            std::vector<GAR_Tri>& tris, std::vector<GAR_Vec3f>& verticies) {
    /* find center */
    GAR_Vec3f runningTotal(0,0,0);
    std::set<uint> addedVerticies;
    uint numV = 0;
    for (uint i = cluster.tris; i < cluster.tris + cluster.numTri; ++i) {
        GAR_Tri tri = tris[i];
        for (auto& idx : tri.verticies) {
            if (addedVerticies.insert(idx).second) {
                numV += 1;
                runningTotal.x += verticies[idx].x;
                runningTotal.y += verticies[idx].y;
                runningTotal.z += verticies[idx].z;
            }
        }
    }
    GAR_Vec3f center(runningTotal.x / numV, runningTotal.y / numV, runningTotal.z / numV);
    /* std::cout << center.x << " " << center.y << " " << center.z << "\n"; */

    /* calculate radius */
    /* radius is actually r^2 */
    /* all verticies must be within r */
    /* testing radius of max triangle */
    float radius = 0;
    for (uint i = cluster.tris; i < cluster.tris + cluster.numTri; ++i) {
        GAR_Tri tri = tris[i];

        /* find the center of this triangle */
        GAR_Vec3f triCenter(0,0,0);
        for (auto& idx : tri.verticies) {
            triCenter.x += verticies[idx].x;
            triCenter.y += verticies[idx].y;
            triCenter.z += verticies[idx].z;
        }
        triCenter.x /= 3;
        triCenter.y /= 3;
        triCenter.z /= 3;

        /* calculate it's radius */
        for (auto& idx : tri.verticies) {
            float triRadius = (triCenter.x - verticies[idx].x) * (triCenter.x - verticies[idx].x) + 
                              (triCenter.y - verticies[idx].y) * (triCenter.y - verticies[idx].y) +
                              (triCenter.z - verticies[idx].z) * (triCenter.z - verticies[idx].z);
            if (triRadius > radius) {
                radius = triRadius;
            }
        }
        
    }


    std::pair<float, GAR_Vec3f> out(radius, center);
    return out;
}

std::vector<GAR_HierarchyCluster> GAR_CreateHierarchy(GAR_Group& root, std::vector<GAR_Group>& groups, 
        std::vector<GAR_Vec3f>& verticies, std::vector<GAR_Tri>& tris) {

    /* create mapping of all the clusters */
    /* also initialize data, center, and radius */
    /* std::cout << "hier calc" << groups.size() << " " << tris.size() << std::endl; */
    std::map<uint, uint> groupClusters; /* Cluster.tris : idx_hcluster */
    std::vector<GAR_Cluster> allClusters = GAR_GetClustersFromGroup(groups);
    uint numHClusters = allClusters.size();

    /* initialize clusters */
    std::vector<GAR_HierarchyCluster> out;
    out.resize(numHClusters, 
            {
            {},               /* data */
            {},               /* children */
            GAR_Vec3f(0,0,0), /* center */
            0.0f,             /* radius */
            GAR_Vec3f(0,0,0), /* testNonFirstParentRadius */
            0.0f,             /* testNonFirstParentRadius */
            0,                /* numChildren */
            0,                /* firstParentFlag */
            255,              /* level */
            }
            );
    for (uint i = 0; i < allClusters.size(); ++i) {
        groupClusters[allClusters[i].tris] = i;
        GAR_HierarchyCluster* hc = &out[i];
        hc->data = allClusters[i];
        std::pair<float, GAR_Vec3f> rc = GAR_CalculateClusterRadiusCenter(hc->data, tris, verticies);
        hc->radius = rc.first;
        hc->center = rc.second;
    }

    std::vector<bool> foundParent; /* for setting firstParentFlag */
    std::vector<uint> primaryParent; /* the primary parent of each cluster*/
    std::vector<uint> partner; /* the other parent (spouse?)*/
    primaryParent.resize(numHClusters, numHClusters);
    partner.resize(numHClusters, numHClusters);
    foundParent.resize(numHClusters, false);
    for (auto& group : groups) {
        for (uint i = 0; i < group.numLow; ++i) {
            GAR_Cluster c = group.lowClusters[i];
            uint hc_idx = groupClusters[c.tris];
            GAR_HierarchyCluster* hc = &out[hc_idx];
            hc->numChildren = group.numHigh;

            /* set children and update parent flag */
            for (uint j = 0; j < group.numHigh; ++j) {
                uint child_hc_idx = groupClusters[group.highClusters[j].tris];
                GAR_HierarchyCluster* child_hc = &out[child_hc_idx];
                hc->children[j] = child_hc_idx;
                if (! foundParent[child_hc_idx]) {
                    foundParent[child_hc_idx] = true;
                    primaryParent[child_hc_idx] = hc_idx;
                    hc->firstParentFlag |= 1 << j;
                } else {
                    child_hc->testNonFirstParentRadius = hc->radius;
                    child_hc->testNonFirstParentCenter = hc->center;
                    /* partner[primaryParent[child_hc_idx]] = hc_idx; */
                    partner[hc_idx] = primaryParent[child_hc_idx];
                }
            } /* end for j */

        } /* end for i */
    }

    /* make sure lower detail clusters share a center and radius */
    for (uint i = 0; i < numHClusters; ++i) {
        GAR_HierarchyCluster* hc = &out[i];
        if ( partner[i] != numHClusters) {
            GAR_HierarchyCluster* pc = &out[partner[i]];
            GAR_Vec3f c1 = hc->center;
            GAR_Vec3f c2 = pc->center;
            GAR_Vec3f newc = GAR_Vec3f((c1.x + c2.x) / 2, (c1.y + c2.y) / 2, (c1.z + c2.z) / 2);
            float newcdist = (newc.x - c1.x) * (newc.x - c1.x) +
                             (newc.y - c1.y) * (newc.y - c1.y) +
                             (newc.z - c1.z) * (newc.z - c1.z);

            float newr = (hc->radius + pc->radius) / 2 + newcdist;
            hc->center = newc;
            pc->center = newc;
            hc->radius = newr;
            pc->radius = newr;
        }
    }

    /* find levels and make radii monotonic*/
    bool allFound = false;
    uint iterCount = 0;
    while (! allFound && iterCount < 10000) {
        allFound = true;
        for (uint j = 0; j < out.size(); ++j) {
            GAR_HierarchyCluster& cluster = out[j];
            if (cluster.level == 255) {
                allFound = false;
            }

            if (cluster.numChildren == 0) {
                cluster.level = 0;
            } else {
                uint highestChild = 0;
                float maxChildRadius = cluster.radius;
                bool doIncreaseRadius = false;
                for (uint i = 0; i < cluster.numChildren; ++i) {
                    GAR_HierarchyCluster& child = out[cluster.children[i]];
                    if (child.level == 255) {
                        break;
                    } 
                    if (child.level > highestChild) {
                        highestChild = child.level;
                    }
                    if (child.radius > maxChildRadius) {
                        maxChildRadius = child.radius;
                        doIncreaseRadius = true;
                        allFound = false;
                    }
                }

                cluster.level = highestChild + 1;
                if (doIncreaseRadius) {
                    cluster.radius = maxChildRadius + 0.01;

                    if (partner[j] != numHClusters ) {
                        GAR_HierarchyCluster& partnerCluster = out[partner[j]];
                        partnerCluster.radius = maxChildRadius + 0.01;
                    }
                }

            }
        }
    }

    return out;
}


/* tris should start out empty, it will be changed */
std::vector<GAR_Cluster> GAR_GetClustersFromECluster(std::vector<GAR_ClusterElements>& eclusters, 
                                                     std::vector<uint>& ibuffer, std::vector<GAR_Tri>& tris) {
    if (tris.size() !=0) {
        std::cerr << "tris should be empty" << std::endl;
    }
    std::vector<GAR_Cluster> clusters;
    for (uint i = 0; i < eclusters.size(); ++i) {
        GAR_ClusterElements* ce = &eclusters[i];
        GAR_Cluster c;
        c.tris = tris.size();
        c.numTri = ce->numPrim;
        clusters.push_back(c);
        for (uint j = 0; j < ce->numPrim; ++j) {
            GAR_Tri t;
            uint idx_ibuf = ce->primativeIndicies[j] * 3;
            for (uint k = 0; k < 3; ++k) {
                t.verticies[k] = ibuffer[idx_ibuf + k];
            }
            tris.push_back(t);
        }
    }

    return clusters;
}


std::vector<GAR_Cluster> GAR_GetLowClustersFromGroups(std::vector<GAR_Group>& groups) {
    std::vector<GAR_Cluster> out;
    for (auto& group : groups) {
        for (uint i = 0; i < group.numLow; ++i) {
            out.push_back(group.lowClusters[i]);
        }
    }
    return out;
}

void GAR_PrintHCluster(GAR_HierarchyCluster& hc) {
    std::cout << "HierCluster: \n";
    std::cout << "\tCluster info: numTri: " << (uint) hc.data.numTri << std::endl;
    std::cout << "\tLevel: " << (uint) hc.level << std::endl;
    std::cout << "\tCenter: " << hc.center.x << hc.center.y << hc.center.z << std::endl;
    std::cout << "\tRadius: " << hc.radius << std::endl;
    std::cout << "\tChildren:\n";
    for (uint i = 0; i < hc.numChildren; ++i) {
        std::cout << "\t\t idx: " << hc.children[i];
        if (hc.firstParentFlag & (1<<i)) {
            std::cout << " (primary)";
        }
        std::cout << std::endl;
    }
}

std::set<uint> GAR_FindClusterBoundaries(std::vector<GAR_Cluster>& clusters, std::vector<GAR_Tri> tris) {
    std::map<uint, uint> vertexCluster;
    std::set<uint> out;
    for (auto& cluster : clusters) {
        for (uint i = 0; i < cluster.numTri; ++i) {
            for (uint k = 0; k < 3; ++k) {
                uint idx = tris[cluster.tris + i].verticies[k];
                if (vertexCluster.count(idx)) {
                    if (vertexCluster[idx] != cluster.tris) {
                        vertexCluster[idx] = -1;
                    } 
                } else {
                    vertexCluster[idx] = cluster.tris;
                }
            }
        }
    }

    /* need to convert again to this funny other format */
    for (auto& cluster : clusters) {
        for (uint i = 0; i < cluster.numTri; ++i) {
            for (uint k = 0; k < 3; ++k) {
                uint key = tris[cluster.tris + i].verticies[k];
                /* uint idx = (cluster.tris + i) * 3 + k; */
                if (vertexCluster[key] == (uint) -1) {
                    out.emplace(key);
                }

            }
        }
    }

    return out;
}

void GAR_PrintClusterStats(std::vector<GAR_Cluster>& clusters) {
    uint totalTri = 0;
    uint minTri = MESH_SIZE;
    uint maxTri = 0;
    for (auto& cluster : clusters) {
        totalTri += cluster.numTri;
        if (cluster.numTri < minTri) {
            minTri = cluster.numTri;
        }
        if (cluster.numTri > maxTri) {
            maxTri = cluster.numTri;
        }
    }
    std::cout << "\t" << clusters.size() << " total clusters\n"
        << "\t" << totalTri << " total tris\n"
        << "\t" << totalTri / ((float) clusters.size()) << " average per cluster\n"
        << "\t" << minTri << " min\n"
        << "\t" << maxTri << " max\n";
}

/* ibuffer will be cleared */
std::pair<std::vector<GAR_HierarchyCluster>, std::vector<GAR_Tri>>
GAR_ComputeMesh(std::vector<uint>& ibuffer, std::vector<GAR_Vec3f>& vbuffer) {
    
    uint clustersInLevel = 999999999;
    uint clustersInLevelPrev = clustersInLevel + 1; 

    std::vector<GAR_Group> groups;

    CSRGraph csr = GAR_CreateMeshGraph(ibuffer);

    std::vector<GAR_ClusterElements> eclusters = GAR_GenerateClusters(csr, 0, 2);

    std::vector<GAR_Tri> tris; /* edited by GAR_GetClustersFromECluster */
    std::vector<GAR_Cluster> levelClusters = GAR_GetClustersFromECluster(eclusters, ibuffer, tris);
    ibuffer.resize(0);

    std::set<uint> levelClusterBoundaries = GAR_FindClusterBoundaries(levelClusters, tris);
    clustersInLevel = eclusters.size();
    uint iterations = 0;
    while (clustersInLevel > 2) {
        std::cout << "LOD " << iterations << " has:\n";
        GAR_PrintClusterStats(levelClusters);

        std::vector<GAR_Group> levelGroups;
        bool groupedCorrectly = false;
        int attempts = 10;
        while (! groupedCorrectly) {
            groupedCorrectly = true;
            uint startTris = tris.size();
            uint startVerticies = vbuffer.size();

            levelGroups = GAR_GroupClusters(levelClusters, tris, 0, 5);
            for (uint i = 0; i < levelGroups.size(); ++i) {
                bool success = GAR_SimplifyGroup(levelGroups[i], tris, vbuffer, levelClusterBoundaries);
                if (! success) {
                    tris.erase(tris.begin() + startTris, tris.end());
                    vbuffer.erase(vbuffer.begin() + startVerticies, vbuffer.end());
                    if (attempts <= 0) {
                        std::cout << "failed grouping, killing\n";
                        exit(1);
                    }
                    std::cout << "failed grouping, trying again\n";
                    attempts -=1;
                    groupedCorrectly = false;
                    break;
                }
            }
        }
        std::vector<GAR_Cluster> newClusters = GAR_GetLowClustersFromGroups(levelGroups);
        clustersInLevelPrev = clustersInLevel;
        clustersInLevel = newClusters.size();
        groups.insert(groups.end(), levelGroups.begin(), levelGroups.end());
        levelClusters.swap(newClusters);

        levelClusterBoundaries = GAR_FindClusterBoundaries(levelClusters, tris);

        if (iterations > 1000) {
            std::cout << "too many simplication iterations\n";
            exit(1);
        }
        if (clustersInLevel >= clustersInLevelPrev) {
            std::cerr << "Failed to simplify at all\n";
            std::cerr << "Last iteration: " << clustersInLevelPrev << " clusters vs. "
                      << clustersInLevel << std::endl;
            exit(1);
        }
        iterations += 1;
    }

    GAR_Group& root = groups[groups.size() - 1];
    std::vector<GAR_HierarchyCluster> vec_hc = GAR_CreateHierarchy(root, groups, vbuffer, tris);

    /* uint count = 0; */
    /* for (auto& hc : vec_hc) { */
        /* GAR_PrintHCluster(hc); */
        /* ++count; */
        /* if (count > 5000000) break; */
    /* } */

    /* check for my zero verticies? */
    for (auto& tri : tris) {
        for (uint k = 0; k < 3; ++k) {
            if (tri.verticies[k] >= vbuffer.size()) {
                std::cout << "tri references vertex " << tri.verticies[k] << " when max index is " << vbuffer.size() << std::endl;
            }
        }
    }

    std::pair<std::vector<GAR_HierarchyCluster>, std::vector<GAR_Tri>> out(vec_hc, tris);
    return out;
}

