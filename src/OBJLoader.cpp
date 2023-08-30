#include "OBJLoader.hpp"


std::vector<VertexData> OBJLoader::getVerticies() {
    return verticies_data;
}

std::vector<unsigned int> OBJLoader::getVertexIndicies() {
    return vertex_indicies;
}


OBJLoader::OBJLoader(std::string fileName) {
    filepath = fileName;
    std::ifstream inFile;
    
    inFile.open(fileName);

    if(inFile.is_open()) {
        std::string line;
        while(std::getline(inFile, line)){
            parse_line(line);
        }

        inFile.close();
    } else {
        std::cout << "could not open file: " << fileName << std::endl;
        exit(1);
    }

}

void OBJLoader::parse_line(std::string line) {
    if (line.size() <= 2) {
        return;
    }

    // https://stackoverflow.com/a/20326454/7487237
    // damn you Bill!
    line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());

    // https://stackoverflow.com/a/10392443
    std::string data = line.substr(line.find(" ") + 1);

    // determine type of data on line
    switch(line[0]) {
        case 'v':
            switch (line[1]) {
                case 'n': // normal
                    parse_location(data, normals);
                    break;
                case 't':
                    parse_location(data, textureUVs);
                    break;
                case ' ':
                    parse_location(data, verticies);
                    break;
                default:
                    std::cerr << "line malformed: " << line << std::endl;
            }
            break;
        case 'f':
            parse_face(data);
            break;
        case 'm':
            matFile = data;
            break;
        default:
            break;
    }
}

OBJLoader::~OBJLoader() {
}


std::string OBJLoader::matTexGetHelper(uint index, char c) {
    std::ifstream inFile;
    std::string folder = filepath.substr(0, filepath.rfind("/") + 1);
    std::string fullpath = folder + matFile;


    inFile.open(fullpath);

    if(inFile.is_open()) {
        std::string line;
        while(std::getline(inFile, line)){
            if (line.size() > index && line[index] == c) {
                std::string data = line.substr(line.find(" ") + 1);
                data.erase(std::remove(data.begin(), data.end(), '\r'), data.end());
                /* std::cout << folder + data; */
                return folder + data;
            }
        }
    }
    inFile.close();
    return "";
}

std::string OBJLoader::getTextureFileName() {
    std::string specifiedFile = matTexGetHelper(5, 'd');
    if (specifiedFile.length() > 0) { return specifiedFile; }
    std::string folder = filepath.substr(0, filepath.rfind("/") + 1);
    // https://stackoverflow.com/a/12774387/7487237
    std::string defaultFile = folder + "Base_Color.ppm";
    std::ifstream f(defaultFile.c_str());
    if (f.good()) {
        std::cout << "using default texture" << std::endl;
        return defaultFile;
    }
    std::cerr << "Couldn't find texture" << std::endl;
    return std::string("");
}

std::string OBJLoader::getNormalMapFileName() {
    std::string specifiedFile = matTexGetHelper(4, 'B');
    if (specifiedFile.length() > 0) { return specifiedFile; }
    std::string folder = filepath.substr(0, filepath.rfind("/") + 1);
    // https://stackoverflow.com/a/12774387/7487237
    std::string defaultFile = folder + "Normal.ppm";
    std::ifstream f(defaultFile.c_str());
    if (f.good()) {
        std::cout << "using default texture" << std::endl;
        return defaultFile;
    }
    std::cerr << "Couldn't find normal map" << std::endl;
    return std::string("");
}


void OBJLoader::parse_location(std::string s, std::vector<float> & loc) {
    std::stringstream ss(s);
    float c;
    while(ss >> c) {
        loc.push_back(c);
    }
}



void OBJLoader::parse_face(std::string s) {
    // https://stackoverflow.com/a/2896627
    // https://stackoverflow.com/a/3871346
    // sstream handles double spaces just fine

    int slashcount = (int)std::count(s.begin(), s.end(), '/');
    std::replace( s.begin(), s.end(), '/', ' ');

    if (slashcount == 0) {
        // just verticies
        // f # # #
        parse_face_helper(s, true, false, false);
        return;
    }
    if (slashcount == 3) {
        // has texture and no normals
        // f #/# #/# #/#
        parse_face_helper(s, true, true, false);
        return;
    } else {
        std::stringstream ss(s);
        unsigned int data;
        unsigned int count = 0;
        while(ss >> data) {
            ++count;
        }
        if (count == 9) {
            // textures and normals
            // f #/#/# #/#/# #/#/#
            parse_face_helper(s, true, true, true);
            return;
        } else {
            // just normals
            // f #//# #//# #//#
            parse_face_helper(s, true, false, true);
            return;
        }
    }

}

// Gets a pointer to the 1-indexed location for either verticies, normals, or uvs
float* OBJLoader::location_lookup(unsigned int objIndex, std::vector<float> & source, unsigned int stride) {
    uint trueIndex = (objIndex - 1) * stride;
    if (trueIndex >= source.size()) {
        std::cerr << "Tried to get index (" << trueIndex 
            << ") out of size (" << source.size() <<")" << std::endl;
        return nullptr;
    }

    return &(source[trueIndex]);
}

void OBJLoader::parse_face_helper(std::string s, bool has_vertex, bool has_texture, bool has_normal) {
    std::replace( s.begin(), s.end(), '/', ' ');
    std::stringstream ss(s);

    float* loc;
    unsigned int objIndex;
    for (int i = 0; i < 3; ++i) {

        VertexData vdata;
        if (has_vertex) {
            ss >> objIndex;
            loc = location_lookup(objIndex, verticies, 3);
            vdata.x = loc[0];
            vdata.y = loc[1];
            vdata.z = loc[2];
        }
        if (has_texture) {
            ss >> objIndex;
            loc = location_lookup(objIndex, textureUVs, 2);
            vdata.s = loc[0];
            vdata.t = loc[1];
        }
        if (has_normal) {
            ss >> objIndex;
            loc = location_lookup(objIndex, normals, 3);
            vdata.nx = loc[0];
            vdata.ny = loc[1];
            vdata.nz = loc[2];
        }

        uniqAdd(vdata);
    }
}


// adds vertex to verticies_data if unique
// adds the index of vertex in verticies_data to vertex_indicies
void OBJLoader::uniqAdd( VertexData vertex ) {
    auto inv = verticies_added.find(vertex);

    if (inv == verticies_added.end()) {
        uint index = verticies_data.size();
        verticies_added.emplace(vertex, index);
        verticies_data.push_back(vertex);
        vertex_indicies.push_back(index);
    } else {
        vertex_indicies.push_back(inv->second);
    }
}
