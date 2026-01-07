#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <cmath>
#include <cstdio>
#include <cfloat>
#include <algorithm>
#include <deque>
#include <fstream>
#include <sstream>
#include <string>
#include <ctime>

struct Node {
    Vector3 position;
    std::vector<int> connections; 
    std::vector<std::pair<int, int>> crossModuleConnections; 
    float scale; 
};

struct Wall {
    std::vector<int> nodeIndices;
    Texture2D texture;
    bool hasTexture;
};

struct GridModule {
    std::vector<Node> nodes;
    std::vector<Wall> walls;
    Vector3 center;
    int id;
};

struct AppState {
    std::vector<GridModule> modules;
    int nextModuleId;
};

struct NodeSelection {
    int moduleIdx;
    int nodeIdx;
};

std::vector<Node> Create3DGridStructure(Vector3 center, float totalSize, int gridDimension) {
    std::vector<Node> nodes;
    float spacing = totalSize / (float)(gridDimension - 1);
    float halfSize = totalSize / 2.0f;
    
    for (int z = 0; z < gridDimension; z++) {
        for (int y = 0; y < gridDimension; y++) {
            for (int x = 0; x < gridDimension; x++) {
                Vector3 pos = {
                    center.x - halfSize + x * spacing,
                    center.y - halfSize + y * spacing,
                    center.z - halfSize + z * spacing
                };
                nodes.push_back({pos, {}});
            }
        }
    }
    
    for (int z = 0; z < gridDimension; z++) {
        for (int y = 0; y < gridDimension; y++) {
            for (int x = 0; x < gridDimension; x++) {
                int currentIdx = z * gridDimension * gridDimension + y * gridDimension + x;
                
                if (x < gridDimension - 1) {
                    int rightIdx = z * gridDimension * gridDimension + y * gridDimension + (x + 1);
                    nodes[currentIdx].connections.push_back(rightIdx);
                }
                if (y < gridDimension - 1) {
                    int upIdx = z * gridDimension * gridDimension + (y + 1) * gridDimension + x;
                    nodes[currentIdx].connections.push_back(upIdx);
                }
                if (z < gridDimension - 1) {
                    int frontIdx = (z + 1) * gridDimension * gridDimension + y * gridDimension + x;
                    nodes[currentIdx].connections.push_back(frontIdx);
                }
            }
        }
    }
    return nodes;
}

int GetNodeUnderMouse(const GridModule& module, const Camera3D& camera, float sphereRadius) {
    Ray ray = GetMouseRay(GetMousePosition(), camera);
    int closestNode = -1;
    float closestDist = FLT_MAX;
    
    for (size_t i = 0; i < module.nodes.size(); i++) {
        RayCollision collision = GetRayCollisionSphere(ray, module.nodes[i].position, sphereRadius);
        if (collision.hit && collision.distance < closestDist) {
            closestDist = collision.distance;
            closestNode = (int)i;
        }
    }
    return closestNode;
}

int GetModuleUnderMouse(const std::vector<GridModule>& modules, const Camera3D& camera, float sphereRadius) {
    Ray ray = GetMouseRay(GetMousePosition(), camera);
    int closestModule = -1;
    float closestDist = FLT_MAX;
    
    for (size_t m = 0; m < modules.size(); m++) {
        for (size_t i = 0; i < modules[m].nodes.size(); i++) {
            RayCollision collision = GetRayCollisionSphere(ray, modules[m].nodes[i].position, sphereRadius);
            if (collision.hit && collision.distance < closestDist) {
                closestDist = collision.distance;
                closestModule = (int)m;
            }
        }
    }
    return closestModule;
}

int GetWallUnderMouse(const GridModule& module, const Camera3D& camera) {
    Ray ray = GetMouseRay(GetMousePosition(), camera);
    int closestWall = -1;
    float closestDist = FLT_MAX;
    
    for (size_t w = 0; w < module.walls.size(); w++) {
        const Wall& wall = module.walls[w];
        
        for (size_t i = 1; i < wall.nodeIndices.size() - 1; i++) {
            Vector3 p1 = module.nodes[wall.nodeIndices[0]].position;
            Vector3 p2 = module.nodes[wall.nodeIndices[i]].position;
            Vector3 p3 = module.nodes[wall.nodeIndices[i + 1]].position;
            
            RayCollision collision = GetRayCollisionTriangle(ray, p1, p2, p3);
            
            if (collision.hit && collision.distance < closestDist) {
                closestDist = collision.distance;
                closestWall = (int)w;
            }
        }
    }
    return closestWall;
}

Vector3 GetMouseWorldPosition(const Camera3D& camera, float distance) {
    Ray ray = GetMouseRay(GetMousePosition(), camera);
    return Vector3Add(ray.position, Vector3Scale(ray.direction, distance));
}

bool AreNodesCoplanar(const std::vector<Node>& nodes, const std::vector<int>& indices) {
    if (indices.size() < 3) return false;
    if (indices.size() == 3) return true;
    
    Vector3 p1 = nodes[indices[0]].position;
    Vector3 p2 = nodes[indices[1]].position;
    Vector3 p3 = nodes[indices[2]].position;
    
    Vector3 v1 = Vector3Subtract(p2, p1);
    Vector3 v2 = Vector3Subtract(p3, p1);
    Vector3 normal = Vector3Normalize(Vector3CrossProduct(v1, v2));
    
    for (size_t i = 3; i < indices.size(); i++) {
        Vector3 v3 = Vector3Subtract(nodes[indices[i]].position, p1);
        float dot = fabs(Vector3DotProduct(normal, v3));
        if (dot > 1.0f) return false;
    }
    
    return true;
}

void DeleteNode(std::vector<GridModule>& modules, int moduleIdx, int nodeIdx) {
    if (moduleIdx < 0 || moduleIdx >= (int)modules.size()) return;
    GridModule& module = modules[moduleIdx];
    if (nodeIdx < 0 || nodeIdx >= (int)module.nodes.size()) return;

    for (auto& node : module.nodes) {
        node.connections.erase(std::remove(node.connections.begin(), node.connections.end(), nodeIdx), node.connections.end());
        for (auto& conn : node.connections) {
            if (conn > nodeIdx) conn--;
        }
    }

    for (size_t m = 0; m < modules.size(); m++) {
        if ((int)m == moduleIdx) continue;
        for (auto& node : modules[m].nodes) {
            node.crossModuleConnections.erase(
                std::remove_if(node.crossModuleConnections.begin(), node.crossModuleConnections.end(),
                    [moduleIdx, nodeIdx](const std::pair<int,int>& p){ return p.first == moduleIdx && p.second == nodeIdx; }),
                node.crossModuleConnections.end());

            for (auto& p : node.crossModuleConnections) {
                if (p.first == moduleIdx && p.second > nodeIdx) p.second--;
            }
        }
    }

    module.walls.erase(std::remove_if(module.walls.begin(), module.walls.end(),
        [nodeIdx](const Wall& w) {
            return std::find(w.nodeIndices.begin(), w.nodeIndices.end(), nodeIdx) != w.nodeIndices.end();
        }), module.walls.end());

    for (auto& wall : module.walls) {
        for (auto& idx : wall.nodeIndices) {
            if (idx > nodeIdx) idx--;
        }
    }

    for (auto& node : module.nodes) {
        node.crossModuleConnections.erase(
            std::remove_if(node.crossModuleConnections.begin(), node.crossModuleConnections.end(),
                [moduleIdx, nodeIdx](const std::pair<int,int>& p){ return p.first == moduleIdx && p.second == nodeIdx; }),
            node.crossModuleConnections.end());
    }

    module.nodes.erase(module.nodes.begin() + nodeIdx);
}

void SaveState(std::deque<AppState>& history, const std::vector<GridModule>& modules, int nextModuleId, size_t maxHistory = 50) {
    AppState state;
    state.modules = modules;
    state.nextModuleId = nextModuleId;
    history.push_back(state);
    if (history.size() > maxHistory) history.pop_front();
}

bool RestoreState(std::deque<AppState>& history, std::vector<GridModule>& modules, int& nextModuleId) {
    if (history.size() <= 1) return false;
    history.pop_back();
    if (!history.empty()) {
        const AppState& prevState = history.back();
        modules = prevState.modules;
        nextModuleId = prevState.nextModuleId;
        return true;
    }
    return false;
}

void DrawWall(const Wall& wall, const std::vector<Node>& nodes, Color defaultColor, bool useTexture = false) {
    if (wall.nodeIndices.size() < 3) return;
    
    if (useTexture && wall.hasTexture) {
        std::vector<Vector3> vertices;
        for (int idx : wall.nodeIndices) {
            if (idx >= 0 && idx < (int)nodes.size()) {
                vertices.push_back(nodes[idx].position);
            }
        }
        
        if (vertices.size() >= 3) {
            Mesh mesh = {0};
            int triangleCount = (int)vertices.size() - 2;
            int vertexCount = triangleCount * 3;
            
            mesh.triangleCount = triangleCount;
            mesh.vertexCount = vertexCount;
            
            mesh.vertices = (float*)MemAlloc(vertexCount * 3 * sizeof(float));
            mesh.texcoords = (float*)MemAlloc(vertexCount * 2 * sizeof(float));
            mesh.normals = (float*)MemAlloc(vertexCount * 3 * sizeof(float));
            
            Vector3 v1 = vertices[0];
            Vector3 v2 = vertices[1];
            Vector3 v3 = vertices[2];
            Vector3 normal = Vector3Normalize(Vector3CrossProduct(
                Vector3Subtract(v2, v1),
                Vector3Subtract(v3, v1)
            ));
            
            float minX = vertices[0].x, maxX = vertices[0].x;
            float minY = vertices[0].y, maxY = vertices[0].y;
            float minZ = vertices[0].z, maxZ = vertices[0].z;
            
            for (const auto& v : vertices) {
                minX = fmin(minX, v.x); maxX = fmax(maxX, v.x);
                minY = fmin(minY, v.y); maxY = fmax(maxY, v.y);
                minZ = fmin(minZ, v.z); maxZ = fmax(maxZ, v.z);
            }
            
            float rangeX = maxX - minX;
            float rangeY = maxY - minY;
            float rangeZ = maxZ - minZ;
            
            int idx = 0;
            for (size_t i = 1; i < vertices.size() - 1; i++) {
                Vector3 p1 = vertices[0];
                Vector3 p2 = vertices[i];
                Vector3 p3 = vertices[i + 1];
                
                mesh.vertices[idx * 3 + 0] = p1.x;
                mesh.vertices[idx * 3 + 1] = p1.y;
                mesh.vertices[idx * 3 + 2] = p1.z;
                idx++;
                
                mesh.vertices[idx * 3 + 0] = p2.x;
                mesh.vertices[idx * 3 + 1] = p2.y;
                mesh.vertices[idx * 3 + 2] = p2.z;
                idx++;
                
                mesh.vertices[idx * 3 + 0] = p3.x;
                mesh.vertices[idx * 3 + 1] = p3.y;
                mesh.vertices[idx * 3 + 2] = p3.z;
                idx++;
            }
            
            idx = 0;
            bool useXY = (rangeX > rangeY && rangeX > rangeZ);
            bool useYZ = (!useXY && rangeY > rangeZ);
            
            for (size_t i = 1; i < vertices.size() - 1; i++) {
                if (useXY) {
                    mesh.texcoords[(idx + 0) * 2 + 0] = (vertices[0].x - minX) / rangeX;
                    mesh.texcoords[(idx + 0) * 2 + 1] = (vertices[0].y - minY) / rangeY;
                    mesh.texcoords[(idx + 1) * 2 + 0] = (vertices[i].x - minX) / rangeX;
                    mesh.texcoords[(idx + 1) * 2 + 1] = (vertices[i].y - minY) / rangeY;
                    mesh.texcoords[(idx + 2) * 2 + 0] = (vertices[i + 1].x - minX) / rangeX;
                    mesh.texcoords[(idx + 2) * 2 + 1] = (vertices[i + 1].y - minY) / rangeY;
                } else if (useYZ) {
                    mesh.texcoords[(idx + 0) * 2 + 0] = (vertices[0].y - minY) / rangeY;
                    mesh.texcoords[(idx + 0) * 2 + 1] = (vertices[0].z - minZ) / rangeZ;
                    mesh.texcoords[(idx + 1) * 2 + 0] = (vertices[i].y - minY) / rangeY;
                    mesh.texcoords[(idx + 1) * 2 + 1] = (vertices[i].z - minZ) / rangeZ;
                    mesh.texcoords[(idx + 2) * 2 + 0] = (vertices[i + 1].y - minY) / rangeY;
                    mesh.texcoords[(idx + 2) * 2 + 1] = (vertices[i + 1].z - minZ) / rangeZ;
                } else {
                    mesh.texcoords[(idx + 0) * 2 + 0] = (vertices[0].x - minX) / rangeX;
                    mesh.texcoords[(idx + 0) * 2 + 1] = (vertices[0].z - minZ) / rangeZ;
                    mesh.texcoords[(idx + 1) * 2 + 0] = (vertices[i].x - minX) / rangeX;
                    mesh.texcoords[(idx + 1) * 2 + 1] = (vertices[i].z - minZ) / rangeZ;
                    mesh.texcoords[(idx + 2) * 2 + 0] = (vertices[i + 1].x - minX) / rangeX;
                    mesh.texcoords[(idx + 2) * 2 + 1] = (vertices[i + 1].z - minZ) / rangeZ;
                }
                idx += 3;
            }
            
            for (int i = 0; i < vertexCount; i++) {
                mesh.normals[i * 3 + 0] = normal.x;
                mesh.normals[i * 3 + 1] = normal.y;
                mesh.normals[i * 3 + 2] = normal.z;
            }
            
            Mesh backMesh = {0};
            backMesh.triangleCount = triangleCount;
            backMesh.vertexCount = vertexCount;
            
            backMesh.vertices = (float*)MemAlloc(vertexCount * 3 * sizeof(float));
            backMesh.texcoords = (float*)MemAlloc(vertexCount * 2 * sizeof(float));
            backMesh.normals = (float*)MemAlloc(vertexCount * 3 * sizeof(float));
            
            for (int i = 0; i < vertexCount; i += 3) {
                backMesh.vertices[(i + 0) * 3 + 0] = mesh.vertices[(i + 2) * 3 + 0];
                backMesh.vertices[(i + 0) * 3 + 1] = mesh.vertices[(i + 2) * 3 + 1];
                backMesh.vertices[(i + 0) * 3 + 2] = mesh.vertices[(i + 2) * 3 + 2];
                
                backMesh.vertices[(i + 1) * 3 + 0] = mesh.vertices[(i + 1) * 3 + 0];
                backMesh.vertices[(i + 1) * 3 + 1] = mesh.vertices[(i + 1) * 3 + 1];
                backMesh.vertices[(i + 1) * 3 + 2] = mesh.vertices[(i + 1) * 3 + 2];
                
                backMesh.vertices[(i + 2) * 3 + 0] = mesh.vertices[(i + 0) * 3 + 0];
                backMesh.vertices[(i + 2) * 3 + 1] = mesh.vertices[(i + 0) * 3 + 1];
                backMesh.vertices[(i + 2) * 3 + 2] = mesh.vertices[(i + 0) * 3 + 2];
                
                backMesh.texcoords[(i + 0) * 2 + 0] = mesh.texcoords[(i + 2) * 2 + 0];
                backMesh.texcoords[(i + 0) * 2 + 1] = mesh.texcoords[(i + 2) * 2 + 1];
                
                backMesh.texcoords[(i + 1) * 2 + 0] = mesh.texcoords[(i + 1) * 2 + 0];
                backMesh.texcoords[(i + 1) * 2 + 1] = mesh.texcoords[(i + 1) * 2 + 1];
                
                backMesh.texcoords[(i + 2) * 2 + 0] = mesh.texcoords[(i + 0) * 2 + 0];
                backMesh.texcoords[(i + 2) * 2 + 1] = mesh.texcoords[(i + 0) * 2 + 1];
                
                backMesh.normals[(i + 0) * 3 + 0] = -normal.x;
                backMesh.normals[(i + 0) * 3 + 1] = -normal.y;
                backMesh.normals[(i + 0) * 3 + 2] = -normal.z;
                
                backMesh.normals[(i + 1) * 3 + 0] = -normal.x;
                backMesh.normals[(i + 1) * 3 + 1] = -normal.y;
                backMesh.normals[(i + 1) * 3 + 2] = -normal.z;
                
                backMesh.normals[(i + 2) * 3 + 0] = -normal.x;
                backMesh.normals[(i + 2) * 3 + 1] = -normal.y;
                backMesh.normals[(i + 2) * 3 + 2] = -normal.z;
            }
            
            UploadMesh(&mesh, false);
            UploadMesh(&backMesh, false);
            
            Material mat = LoadMaterialDefault();
            mat.maps[MATERIAL_MAP_DIFFUSE].texture = wall.texture;
            mat.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
            
            DrawMesh(mesh, mat, MatrixIdentity());
            DrawMesh(backMesh, mat, MatrixIdentity());
            
            UnloadMesh(mesh);
            UnloadMesh(backMesh);
        }
    } else {
        for (size_t i = 1; i < wall.nodeIndices.size() - 1; i++) {
            if (wall.nodeIndices[0] >= 0 && wall.nodeIndices[0] < (int)nodes.size() &&
                wall.nodeIndices[i] >= 0 && wall.nodeIndices[i] < (int)nodes.size() &&
                wall.nodeIndices[i + 1] >= 0 && wall.nodeIndices[i + 1] < (int)nodes.size()) {
                Vector3 p1 = nodes[wall.nodeIndices[0]].position;
                Vector3 p2 = nodes[wall.nodeIndices[i]].position;
                Vector3 p3 = nodes[wall.nodeIndices[i + 1]].position;
                
                DrawTriangle3D(p1, p2, p3, defaultColor);
                DrawTriangle3D(p3, p2, p1, defaultColor);
            }
        }
    }
}

bool ExportToOBJ(const std::vector<GridModule>& modules, const char* filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    file << "# OBJ file exported from GreyScaleCube\n";
    file << "# Generated model\n\n";
    
    int vertexOffset = 1;
    
    for (size_t m = 0; m < modules.size(); m++) {
        file << "# Module " << modules[m].id << "\n";
        for (const auto& node : modules[m].nodes) {
            file << "v " << node.position.x << " " << node.position.y << " " << node.position.z << "\n";
        }
    }
    
    file << "\n# Connections (lines)\n";
    
    vertexOffset = 1;
    for (size_t m = 0; m < modules.size(); m++) {
        for (size_t i = 0; i < modules[m].nodes.size(); i++) {
            for (int conn : modules[m].nodes[i].connections) {
                if ((int)i < conn) {
                    int v1 = vertexOffset + (int)i;
                    int v2 = vertexOffset + conn;
                    file << "l " << v1 << " " << v2 << "\n";
                }
            }
            for (const auto& crossConn : modules[m].nodes[i].crossModuleConnections) {
                 int targetMod = crossConn.first;
                 int targetNode = crossConn.second;
                 
                 int targetVertexOffset = 1;
                 for(int k=0; k<targetMod; k++) targetVertexOffset += modules[k].nodes.size();
                 
                 if ((int)m < targetMod) {
                     int v1 = vertexOffset + (int)i;
                     int v2 = targetVertexOffset + targetNode;
                     file << "l " << v1 << " " << v2 << "\n";
                 }
            }
        }
        vertexOffset += (int)modules[m].nodes.size();
    }
    
    file << "\n# Walls (faces)\n";
    
    vertexOffset = 1;
    for (size_t m = 0; m < modules.size(); m++) {
        for (const auto& wall : modules[m].walls) {
            if (wall.nodeIndices.size() >= 3) {
                file << "f";
                for (int nodeIdx : wall.nodeIndices) {
                    file << " " << (vertexOffset + nodeIdx);
                }
                file << "\n";
            }
        }
        vertexOffset += (int)modules[m].nodes.size();
    }
    
    file.close();
    return true;
}

bool ImportFromOBJ(std::vector<GridModule>& modules, int& nextModuleId, const char* filename, Camera3D& camera) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        printf("Failed to open file: %s\n", filename);
        return false;
    }
    
    modules.clear();
    
    GridModule newModule;
    newModule.id = nextModuleId++;
    newModule.center = {0.0f, 0.0f, 0.0f};
    
    std::vector<Vector3> vertices;
    std::vector<std::pair<int, int>> lines;
    std::vector<std::vector<int>> faces;
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string type;
        iss >> type;
        
        if (type == "v") {
            float x, y, z;
            iss >> x >> y >> z;
            vertices.push_back({x, y, z});
        }
        else if (type == "l") {
            int v1, v2;
            iss >> v1 >> v2;
            lines.push_back({v1 - 1, v2 - 1});
        }
        else if (type == "f") {
            std::vector<int> faceIndices;
            int idx;
            while (iss >> idx) {
                faceIndices.push_back(idx - 1);
            }
            if (faceIndices.size() >= 3) {
                faces.push_back(faceIndices);
            }
        }
    }
    
    file.close();
    
    if (vertices.empty()) {
        printf("No vertices found in OBJ file\n");
        return false;
    }
    
    for (const auto& v : vertices) {
        Node node;
        node.position = v;
        newModule.nodes.push_back(node);
    }
    
    for (const auto& l : lines) {
        if (l.first >= 0 && l.first < (int)newModule.nodes.size() &&
            l.second >= 0 && l.second < (int)newModule.nodes.size()) {
            
            bool exists = false;
            for (int conn : newModule.nodes[l.first].connections) {
                if (conn == l.second) {
                    exists = true;
                    break;
                }
            }
            
            if (!exists) {
                newModule.nodes[l.first].connections.push_back(l.second);
                newModule.nodes[l.second].connections.push_back(l.first);
            }
        }
    }
    
    for (const auto& f : faces) {
        bool validFace = true;
        for (int idx : f) {
            if (idx < 0 || idx >= (int)newModule.nodes.size()) {
                validFace = false;
                break;
            }
        }
        
        if (validFace) {
            Wall wall;
            wall.nodeIndices = f;
            wall.hasTexture = false;
            wall.texture = {};
            newModule.walls.push_back(wall);
        }
    }
    
    // Calculate Center and Bounding Box for Camera
    if (!newModule.nodes.empty()) {
        Vector3 minV = {FLT_MAX, FLT_MAX, FLT_MAX};
        Vector3 maxV = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
        Vector3 sum = {0, 0, 0};
        
        for (const auto& node : newModule.nodes) {
            sum.x += node.position.x;
            sum.y += node.position.y;
            sum.z += node.position.z;
            
            if (node.position.x < minV.x) minV.x = node.position.x;
            if (node.position.x > maxV.x) maxV.x = node.position.x;
            if (node.position.y < minV.y) minV.y = node.position.y;
            if (node.position.y > maxV.y) maxV.y = node.position.y;
            if (node.position.z < minV.z) minV.z = node.position.z;
            if (node.position.z > maxV.z) maxV.z = node.position.z;
        }
        
        newModule.center.x = sum.x / newModule.nodes.size();
        newModule.center.y = sum.y / newModule.nodes.size();
        newModule.center.z = sum.z / newModule.nodes.size();
        
        camera.target = newModule.center;
        
        float dimX = maxV.x - minV.x;
        float dimY = maxV.y - minV.y;
        float dimZ = maxV.z - minV.z;
        float maxDim = dimX;
        if(dimY > maxDim) maxDim = dimY;
        if(dimZ > maxDim) maxDim = dimZ;
        
        float dist = maxDim * 2.5f + 10.0f;
        Vector3 offset = {dist, dist * 0.5f, dist};
        camera.position = Vector3Add(newModule.center, offset);
    }
    
    modules.push_back(newModule);
    
    printf("Successfully imported OBJ: %zu vertices, %zu connections, %zu walls\n", 
           vertices.size(), lines.size(), faces.size());
           
    return true;
}

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1200, 900, "3D Grid Modules - Cross-Module Walls");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    int gridSize = 3;
    float gridTotalSize = 12.0f;
    float sphereRadius = 0.3f;
    
    std::vector<GridModule> modules;
    int nextModuleId = 0;
    std::deque<AppState> undoHistory;
    
    GridModule initialModule;
    initialModule.nodes = Create3DGridStructure({0.0f, 5.0f, 0.0f}, gridTotalSize, gridSize);
    initialModule.center = {0.0f, 5.0f, 0.0f};
    initialModule.id = nextModuleId++;
    modules.push_back(initialModule);
    SaveState(undoHistory, modules, nextModuleId);

    Camera3D camera{};
    camera.position = {25.0f, 20.0f, 25.0f};
    camera.target = {0.0f, 5.0f, 0.0f};
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    float cameraSpeed = 0.1f, maxSpeed = 0.5f, rotSpeed = 0.003f;
    bool wasMoving = false, isRotatingCamera = false, cursorEnabled = false;
    bool isDragging = false, isDraggingModule = false;
    bool showGrid = true, showConnections = true;
    Vector2 lastMousePos = {0, 0};
    int hoveredNode = -1, hoveredModule = -1, hoveredWall = -1;
    float dragDistance = 0.0f;
    Vector3 lastMouseWorld = {0.0f, 0.0f, 0.0f};
    int gridSlices = 20;
    
    enum Mode { MODE_SELECT, MODE_MOVE_VERTEX, MODE_MOVE_MODULE, MODE_ADD_NODE, MODE_CONNECT, MODE_ROTATE_MODULE, MODE_SCALE };
    Mode currentMode = MODE_SELECT;
    
    std::vector<NodeSelection> selectedNodes;
    int activeModule = -1;
    
    Vector3 previewNodePosition = {0.0f, 0.0f, 0.0f};
    bool showPreviewNode = false;
    float addNodeDistance = 15.0f;
    
    int connectStartNode = -1;
    int connectStartModule = -1;
    
    double lastClickTime = 0.0;
    int lastClickedNode = -1;
    int lastClickedModule = -1;
    const double doubleClickTime = 0.3;
    
    bool isDragSelecting = false;
    Vector2 dragSelectStart = {0, 0};
    Vector2 dragSelectEnd = {0, 0};
    
    int escPressCount = 0;
    double lastEscTime = 0.0;
    const char* escMessage = "";
    double escMessageTime = 0.0;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            double currentTime = GetTime();
            
            if (currentTime - lastEscTime > 2.0) {
                escPressCount = 0;
            }
            
            escPressCount++;
            lastEscTime = currentTime;
            
            if (escPressCount == 1) {
                bool hadSomethingToCancel = false;
                
                if (connectStartNode != -1 || connectStartModule != -1) {
                    connectStartNode = -1;
                    connectStartModule = -1;
                    hadSomethingToCancel = true;
                    escMessage = "Connection cancelled";
                }
                else if (!selectedNodes.empty()) {
                    selectedNodes.clear();
                    hadSomethingToCancel = true;
                    escMessage = "Selection cleared";
                }
                else if (activeModule != -1) {
                    activeModule = -1;
                    hadSomethingToCancel = true;
                    escMessage = "Active module cleared";
                }
                else if (isDragging || isDraggingModule) {
                    isDragging = false;
                    isDraggingModule = false;
                    hadSomethingToCancel = true;
                    escMessage = "Drag cancelled";
                }
                
                if (!hadSomethingToCancel) {
                    escMessage = "Press ESC again within 2 seconds to exit";
                }
                
                escMessageTime = currentTime;
            }
            else if (escPressCount >= 2) {
                break;
            }
        }

        if (IsFileDropped()) {
            FilePathList droppedFiles = LoadDroppedFiles();
            if (droppedFiles.count > 0) {
                if (ImportFromOBJ(modules, nextModuleId, droppedFiles.paths[0], camera)) {
                    SaveState(undoHistory, modules, nextModuleId);
                    hoveredNode = hoveredModule = hoveredWall = -1;
                    isDragging = isDraggingModule = false;
                    selectedNodes.clear();
                    activeModule = -1;
                    cursorEnabled = true;
                    EnableCursor();
                    escMessage = "Imported & Camera Focused";
                    escMessageTime = GetTime();
                }
            }
            UnloadDroppedFiles(droppedFiles);
        }

        if (IsKeyPressed(KEY_TAB)) {
            cursorEnabled = !cursorEnabled;
            cursorEnabled ? EnableCursor() : DisableCursor();
        }
        
        if (IsKeyPressed(KEY_F11)) {
            if (IsWindowMaximized()) {
                RestoreWindow();
            } else {
                MaximizeWindow();
            }
        }
        
        if (IsKeyPressed(KEY_G)) showGrid = !showGrid;
        if (IsKeyPressed(KEY_C)) showConnections = !showConnections;
        
        if (cursorEnabled && IsKeyPressed(KEY_ONE)) {
            currentMode = MODE_SELECT;
            isDragging = isDraggingModule = false;
            selectedNodes.clear();
            showPreviewNode = false;
            connectStartNode = connectStartModule = -1;
        }
        if (cursorEnabled && IsKeyPressed(KEY_TWO)) {
            currentMode = MODE_MOVE_VERTEX;
            isDragging = isDraggingModule = false;
            selectedNodes.clear();
            showPreviewNode = false;
            connectStartNode = connectStartModule = -1;
        }
        if (cursorEnabled && IsKeyPressed(KEY_THREE)) {
            currentMode = MODE_MOVE_MODULE;
            isDragging = isDraggingModule = false;
            selectedNodes.clear();
            showPreviewNode = false;
            connectStartNode = connectStartModule = -1;
        }
        if (cursorEnabled && IsKeyPressed(KEY_FOUR)) {
            currentMode = MODE_ADD_NODE;
            isDragging = isDraggingModule = false;
            selectedNodes.clear();
            showPreviewNode = true;
            connectStartNode = connectStartModule = -1;
        }
        if (cursorEnabled && IsKeyPressed(KEY_FIVE)) {
            currentMode = MODE_CONNECT;
            isDragging = isDraggingModule = false;
            selectedNodes.clear();
            showPreviewNode = false;
            connectStartNode = connectStartModule = -1;
        }
        if (cursorEnabled && IsKeyPressed(KEY_SIX)) {
            currentMode = MODE_ROTATE_MODULE;
            isDragging = isDraggingModule = false;
            selectedNodes.clear();
            showPreviewNode = false;
            connectStartNode = connectStartModule = -1;
        }
        if (cursorEnabled && IsKeyPressed(KEY_SEVEN)) {
            currentMode = MODE_SCALE;
            isDragging = isDraggingModule = false;
            showPreviewNode = false;
            connectStartNode = connectStartModule = -1;
        }
        
        // --- FIXED SPACE KEY FOR WALL CREATION ---
        if (currentMode == MODE_SELECT && IsKeyPressed(KEY_SPACE) && selectedNodes.size() >= 3) {
            std::vector<Vector3> positions;
            for (const auto& sel : selectedNodes) {
                positions.push_back(modules[sel.moduleIdx].nodes[sel.nodeIdx].position);
            }
            
            if (positions.size() >= 3) {
                Vector3 p1 = positions[0];
                Vector3 p2 = positions[1];
                Vector3 p3 = positions[2];
                
                Vector3 v1 = Vector3Subtract(p2, p1);
                Vector3 v2 = Vector3Subtract(p3, p1);
                
                // --- SAFETY FIX: Check for Collinear Points ---
                // If points are in a straight line, cross product length is 0 -> CRASH
                Vector3 cross = Vector3CrossProduct(v1, v2);
                float crossLength = Vector3Length(cross);
                
                if (crossLength < 0.001f) {
                    printf("Cannot create wall: Selected points are collinear (straight line).\n");
                    escMessage = "Collinear! Move nodes to form a triangle.";
                    escMessageTime = GetTime();
                    // Do not proceed to Normalization (which would crash)
                } else {
                    // Safe to normalize
                    Vector3 normal = Vector3Normalize(cross);
                    
                    bool coplanar = true;
                    for (size_t i = 3; i < positions.size(); i++) {
                        Vector3 v3 = Vector3Subtract(positions[i], p1);
                        float dot = fabs(Vector3DotProduct(normal, v3));
                        if (dot > 1.0f) {
                            coplanar = false;
                            break;
                        }
                    }
                    
                    if (coplanar) {
                        int targetModule = selectedNodes[0].moduleIdx;
                        
                        bool allSameModule = true;
                        for (const auto& sel : selectedNodes) {
                            if (sel.moduleIdx != targetModule) {
                                allSameModule = false;
                                break;
                            }
                        }
                        
                        if (allSameModule) {
                            Wall newWall;
                            for (const auto& sel : selectedNodes) {
                                newWall.nodeIndices.push_back(sel.nodeIdx);
                            }
                            newWall.hasTexture = false;
                            newWall.texture = {};
                            modules[targetModule].walls.push_back(newWall);
                            SaveState(undoHistory, modules, nextModuleId);
                            printf("Created wall with %zu nodes in module %d\n", newWall.nodeIndices.size(), targetModule);
                        } else {
                            std::vector<int> wallNodeIndices;
                            
                            for (const auto& sel : selectedNodes) {
                                if (sel.moduleIdx == targetModule) {
                                    wallNodeIndices.push_back(sel.nodeIdx);
                                } else {
                                    Node newNode = modules[sel.moduleIdx].nodes[sel.nodeIdx];
                                    modules[targetModule].nodes.push_back(newNode);
                                    int newIdx = (int)modules[targetModule].nodes.size() - 1;
                                    wallNodeIndices.push_back(newIdx);
                                    
                                    modules[targetModule].nodes[newIdx].crossModuleConnections.push_back({sel.moduleIdx, sel.nodeIdx});
                                    modules[sel.moduleIdx].nodes[sel.nodeIdx].crossModuleConnections.push_back({targetModule, newIdx});
                                }
                            }
                            
                            Wall newWall;
                            newWall.nodeIndices = wallNodeIndices;
                            newWall.hasTexture = false;
                            newWall.texture = {};
                            modules[targetModule].walls.push_back(newWall);
                            SaveState(undoHistory, modules, nextModuleId);
                            printf("Created cross-module wall with %zu nodes in module %d\n", wallNodeIndices.size(), targetModule);
                        }
                        
                        selectedNodes.clear();
                    } else {
                        printf("Selected nodes are not coplanar - cannot create wall\n");
                    }
                }
            }
        }
        
        // Clone module/selection
        if (currentMode == MODE_SELECT && (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))) {
            if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_C)) {
                if (!selectedNodes.empty()) {
                    GridModule clonedModule;
                    clonedModule.id = nextModuleId++;
                    
                    std::vector<std::pair<NodeSelection, int>> nodeMapping;
                    
                    Vector3 selectionCenter = {0, 0, 0};
                    for (const auto& sel : selectedNodes) {
                        selectionCenter.x += modules[sel.moduleIdx].nodes[sel.nodeIdx].position.x;
                        selectionCenter.y += modules[sel.moduleIdx].nodes[sel.nodeIdx].position.y;
                        selectionCenter.z += modules[sel.moduleIdx].nodes[sel.nodeIdx].position.z;
                    }
                    selectionCenter.x /= selectedNodes.size();
                    selectionCenter.y /= selectedNodes.size();
                    selectionCenter.z /= selectedNodes.size();
                    
                    Vector3 offset = {5.0f, 0.0f, 0.0f};
                    for (size_t i = 0; i < selectedNodes.size(); i++) {
                        NodeSelection sel = selectedNodes[i];
                        Node newNode = modules[sel.moduleIdx].nodes[sel.nodeIdx];
                        newNode.position = Vector3Add(newNode.position, offset);
                        newNode.connections.clear();
                        newNode.crossModuleConnections.clear();
                        
                        nodeMapping.push_back({sel, (int)i});
                        clonedModule.nodes.push_back(newNode);
                    }
                    
                    for (size_t i = 0; i < selectedNodes.size(); i++) {
                        NodeSelection sel = selectedNodes[i];
                        const Node& originalNode = modules[sel.moduleIdx].nodes[sel.nodeIdx];
                        
                        for (int conn : originalNode.connections) {
                            for (const auto& mapping : nodeMapping) {
                                if (mapping.first.moduleIdx == sel.moduleIdx && mapping.first.nodeIdx == conn) {
                                    clonedModule.nodes[i].connections.push_back(mapping.second);
                                    break;
                                }
                            }
                        }
                    }
                    
                    clonedModule.center = Vector3Add(selectionCenter, offset);
                    modules.push_back(clonedModule);
                    SaveState(undoHistory, modules, nextModuleId);
                    
                    printf("Cloned %zu selected nodes as new module %d\n", 
                           selectedNodes.size(), clonedModule.id);
                    
                    selectedNodes.clear();
                }
                else if (activeModule != -1 && activeModule < (int)modules.size()) {
                    GridModule clonedModule = modules[activeModule];
                    clonedModule.id = nextModuleId++;
                    
                    Vector3 offset = {10.0f, 0.0f, 0.0f};
                    clonedModule.center = Vector3Add(clonedModule.center, offset);
                    
                    for (auto& node : clonedModule.nodes) {
                        node.position = Vector3Add(node.position, offset);
                    }
                    
                    modules.push_back(clonedModule);
                    SaveState(undoHistory, modules, nextModuleId);
                    
                    printf("Module %d cloned as module %d\n", activeModule, clonedModule.id);
                } else {
                    printf("No selection or active module to clone\n");
                }
            }
        }

        if (IsKeyPressed(KEY_N)) {
            GridModule newModule;
            Vector3 newCenter = Vector3Add(modules.back().center, {15.0f, 0.0f, 0.0f});
            newModule.nodes = Create3DGridStructure(newCenter, gridTotalSize, gridSize);
            newModule.center = newCenter;
            newModule.id = nextModuleId++;
            modules.push_back(newModule);
            SaveState(undoHistory, modules, nextModuleId);
        }
        
        if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_S)) {
            const char* filename = "model.obj";
            if (ExportToOBJ(modules, filename)) {
                printf("Model exported to %s\n", filename);
                escMessage = "Exported to model.obj";
                escMessageTime = GetTime();
            } else {
                printf("Failed to export model to %s\n", filename);
            }
        }
        
        if (IsKeyPressed(KEY_F5)) {
            const char* filename = "model.obj";
            if (ExportToOBJ(modules, filename)) {
                printf("Model exported to %s\n", filename);
            } else {
                printf("Failed to export model to %s\n", filename);
            }
        }
        
        if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_O)) {
            const char* filename = "model.obj";
            if (ImportFromOBJ(modules, nextModuleId, filename, camera)) {
                printf("Model imported from %s\n", filename);
                SaveState(undoHistory, modules, nextModuleId);
                hoveredNode = hoveredModule = hoveredWall = -1;
                isDragging = isDraggingModule = false;
                selectedNodes.clear();
                activeModule = -1;
            } else {
                printf("Failed to import model from %s\n", filename);
            }
        }
        
        if (IsKeyPressed(KEY_F6)) {
            const char* filename = "model.obj";
            if (ImportFromOBJ(modules, nextModuleId, filename, camera)) {
                printf("Model imported from %s\n", filename);
                SaveState(undoHistory, modules, nextModuleId);
                hoveredNode = hoveredModule = hoveredWall = -1;
                isDragging = isDraggingModule = false;
                selectedNodes.clear();
                activeModule = -1;
            } else {
                printf("Failed to import model from %s\n", filename);
            }
        }
        
        if (((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_Z)) || IsKeyPressed(KEY_BACKSPACE)) {
            if (RestoreState(undoHistory, modules, nextModuleId)) {
                hoveredNode = hoveredModule = hoveredWall = -1;
                isDragging = isDraggingModule = false;
                selectedNodes.clear();
                activeModule = -1;
            }
        }
        
        if (cursorEnabled && activeModule != -1 && activeModule < (int)modules.size()) {
            float moveSpeed = 0.5f;
            Vector3 movement = {0, 0, 0};
            bool moved = false;
            
            if (IsKeyPressed(KEY_UP)) {
                movement.z = -moveSpeed;
                moved = true;
            }
            if (IsKeyPressed(KEY_DOWN)) {
                movement.z = moveSpeed;
                moved = true;
            }
            if (IsKeyPressed(KEY_LEFT)) {
                movement.x = -moveSpeed;
                moved = true;
            }
            if (IsKeyPressed(KEY_RIGHT)) {
                movement.x = moveSpeed;
                moved = true;
            }
            if (IsKeyPressed(KEY_PAGE_UP)) {
                movement.y = moveSpeed;
                moved = true;
            }
            if (IsKeyPressed(KEY_PAGE_DOWN)) {
                movement.y = -moveSpeed;
                moved = true;
            }
            
            bool rotated = false;
            float rotationAngle = 15.0f * DEG2RAD;
            
            if (IsKeyDown(KEY_R)) {
                Vector3 center = modules[activeModule].center;
                
                if (IsKeyPressed(KEY_LEFT)) {
                    for (auto& node : modules[activeModule].nodes) {
                        Vector3 offset = Vector3Subtract(node.position, center);
                        float x = offset.x * cos(rotationAngle) - offset.z * sin(rotationAngle);
                        float z = offset.x * sin(rotationAngle) + offset.z * cos(rotationAngle);
                        node.position = Vector3Add(center, {x, offset.y, z});
                    }
                    rotated = true;
                }
                if (IsKeyPressed(KEY_RIGHT)) {
                    for (auto& node : modules[activeModule].nodes) {
                        Vector3 offset = Vector3Subtract(node.position, center);
                        float x = offset.x * cos(-rotationAngle) - offset.z * sin(-rotationAngle);
                        float z = offset.x * sin(-rotationAngle) + offset.z * cos(-rotationAngle);
                        node.position = Vector3Add(center, {x, offset.y, z});
                    }
                    rotated = true;
                }
                if (IsKeyPressed(KEY_UP)) {
                    for (auto& node : modules[activeModule].nodes) {
                        Vector3 offset = Vector3Subtract(node.position, center);
                        float y = offset.y * cos(rotationAngle) - offset.z * sin(rotationAngle);
                        float z = offset.y * sin(rotationAngle) + offset.z * cos(rotationAngle);
                        node.position = Vector3Add(center, {offset.x, y, z});
                    }
                    rotated = true;
                }
                if (IsKeyPressed(KEY_DOWN)) {
                    for (auto& node : modules[activeModule].nodes) {
                        Vector3 offset = Vector3Subtract(node.position, center);
                        float y = offset.y * cos(-rotationAngle) - offset.z * sin(-rotationAngle);
                        float z = offset.y * sin(-rotationAngle) + offset.z * cos(-rotationAngle);
                        node.position = Vector3Add(center, {offset.x, y, z});
                    }
                    rotated = true;
                }
                if (IsKeyPressed(KEY_PAGE_UP)) {
                    for (auto& node : modules[activeModule].nodes) {
                        Vector3 offset = Vector3Subtract(node.position, center);
                        float x = offset.x * cos(rotationAngle) - offset.y * sin(rotationAngle);
                        float y = offset.x * sin(rotationAngle) + offset.y * cos(rotationAngle);
                        node.position = Vector3Add(center, {x, y, offset.z});
                    }
                    rotated = true;
                }
                if (IsKeyPressed(KEY_PAGE_DOWN)) {
                    for (auto& node : modules[activeModule].nodes) {
                        Vector3 offset = Vector3Subtract(node.position, center);
                        float x = offset.x * cos(-rotationAngle) - offset.y * sin(-rotationAngle);
                        float y = offset.x * sin(-rotationAngle) + offset.y * cos(-rotationAngle);
                        node.position = Vector3Add(center, {x, y, offset.z});
                    }
                    rotated = true;
                }
            }
            
            if (moved) {
                for (auto& node : modules[activeModule].nodes) {
                    node.position = Vector3Add(node.position, movement);
                }
                modules[activeModule].center = Vector3Add(modules[activeModule].center, movement);
                SaveState(undoHistory, modules, nextModuleId);
            }
            
            if (rotated) {
                SaveState(undoHistory, modules, nextModuleId);
                printf("Rotated module %d by 15 degrees\n", activeModule);
            }
        }

        if (!cursorEnabled) {
            Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
            Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
            Vector3 up = camera.up;
            bool moving = false;

            if (IsKeyDown(KEY_W)) { camera.position = Vector3Add(camera.position, Vector3Scale(forward, cameraSpeed)); camera.target = Vector3Add(camera.target, Vector3Scale(forward, cameraSpeed)); moving = true; }
            if (IsKeyDown(KEY_S)) { camera.position = Vector3Subtract(camera.position, Vector3Scale(forward, cameraSpeed)); camera.target = Vector3Subtract(camera.target, Vector3Scale(forward, cameraSpeed)); moving = true; }
            if (IsKeyDown(KEY_A)) { camera.position = Vector3Subtract(camera.position, Vector3Scale(right, cameraSpeed)); camera.target = Vector3Subtract(camera.target, Vector3Scale(right, cameraSpeed)); moving = true; }
            if (IsKeyDown(KEY_D)) { camera.position = Vector3Add(camera.position, Vector3Scale(right, cameraSpeed)); camera.target = Vector3Add(camera.target, Vector3Scale(right, cameraSpeed)); moving = true; }
            if (IsKeyDown(KEY_SPACE)) { camera.position = Vector3Add(camera.position, Vector3Scale(up, cameraSpeed)); camera.target = Vector3Add(camera.target, Vector3Scale(up, cameraSpeed)); moving = true; }
            if (IsKeyDown(KEY_LEFT_SHIFT)) { camera.position = Vector3Subtract(camera.position, Vector3Scale(up, cameraSpeed)); camera.target = Vector3Subtract(camera.target, Vector3Scale(up, cameraSpeed)); moving = true; }

            if (moving) { cameraSpeed += 0.005f; if (cameraSpeed > maxSpeed) cameraSpeed = maxSpeed; wasMoving = true; }
            else if (wasMoving) { cameraSpeed = 0.1f; wasMoving = false; }

            Vector2 mouseDelta = GetMouseDelta();
            if (mouseDelta.x != 0 || mouseDelta.y != 0) {
                forward = Vector3Transform(forward, MatrixRotateY(-mouseDelta.x * rotSpeed));
                Vector3 rightAxis = Vector3Normalize(Vector3CrossProduct(forward, up));
                forward = Vector3Transform(forward, MatrixRotate(rightAxis, -mouseDelta.y * rotSpeed));
                camera.target = Vector3Add(camera.position, forward);
            }
        }

        if (cursorEnabled) {
            if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !isDragging && !isDraggingModule) {
                isRotatingCamera = true;
                lastMousePos = GetMousePosition();
            }
            
            if (IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) {
                isRotatingCamera = false;
            }
            
            if (isRotatingCamera && !isDragging && !isDraggingModule) {
                Vector2 curMousePos = GetMousePosition();
                Vector2 delta = {curMousePos.x - lastMousePos.x, curMousePos.y - lastMousePos.y};
                
                Vector3 forward = Vector3Subtract(camera.target, camera.position);
                float dist = Vector3Length(forward);
                forward = Vector3Normalize(forward);
                
                forward = Vector3Transform(forward, MatrixRotateY(-delta.x * rotSpeed));
                Vector3 rightAxis = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
                forward = Vector3Transform(forward, MatrixRotate(rightAxis, -delta.y * rotSpeed));
                
                camera.position = Vector3Subtract(camera.target, Vector3Scale(forward, dist));
                lastMousePos = curMousePos;
            }
        }
        
        if (cursorEnabled) {
            if (!isDragging && !isDraggingModule) {
                hoveredModule = GetModuleUnderMouse(modules, camera, sphereRadius * 1.5f);
                hoveredNode = hoveredWall = -1;
                
                if (hoveredModule != -1) {
                    if (currentMode != MODE_ADD_NODE) {
                        hoveredWall = GetWallUnderMouse(modules[hoveredModule], camera);
                    }
                    if (hoveredWall == -1) {
                        hoveredNode = GetNodeUnderMouse(modules[hoveredModule], camera, sphereRadius * 1.5f);
                    }
                }
            }
            
            if (currentMode == MODE_ADD_NODE) {
                previewNodePosition = GetMouseWorldPosition(camera, addNodeDistance);
                
                float wheel = GetMouseWheelMove();
                if (wheel != 0) {
                    addNodeDistance += wheel * 2.0f;
                    if (addNodeDistance < 5.0f) addNodeDistance = 5.0f;
                    if (addNodeDistance > 50.0f) addNodeDistance = 50.0f;
                }
            }
            
            if (IsKeyPressed(KEY_DELETE)) {
                bool changed = false;
                
                if (currentMode == MODE_SELECT && !selectedNodes.empty()) {
                    std::vector<std::pair<int, int>> nodesToDelete;
                    for (const auto& sel : selectedNodes) {
                        nodesToDelete.push_back({sel.moduleIdx, sel.nodeIdx});
                    }
                    
                    std::sort(nodesToDelete.begin(), nodesToDelete.end(), 
                        [](const std::pair<int,int>& a, const std::pair<int,int>& b) {
                            if (a.first != b.first) return a.first > b.first;
                            return a.second > b.second;
                        });
                    
                    for (const auto& p : nodesToDelete) {
                        DeleteNode(modules, p.first, p.second);
                    }
                    
                    selectedNodes.clear();
                    changed = true;
                    printf("Deleted %zu selected nodes\n", nodesToDelete.size());
                }
                else if (currentMode == MODE_MOVE_MODULE && activeModule != -1 && modules.size() > 1) {
                    for (auto& wall : modules[activeModule].walls) {
                        if (wall.hasTexture) {
                            UnloadTexture(wall.texture);
                        }
                    }
                    modules.erase(modules.begin() + activeModule);
                    activeModule = -1;
                    changed = true;
                    printf("Deleted active module\n");
                }
                else if (hoveredWall != -1 && hoveredModule != -1) {
                    if (modules[hoveredModule].walls[hoveredWall].hasTexture) {
                        UnloadTexture(modules[hoveredModule].walls[hoveredWall].texture);
                    }
                    modules[hoveredModule].walls.erase(modules[hoveredModule].walls.begin() + hoveredWall);
                    hoveredWall = -1;
                    changed = true;
                } else if (hoveredNode != -1 && hoveredModule != -1) {
                    DeleteNode(modules, hoveredModule, hoveredNode);
                    hoveredNode = -1;
                    changed = true;
                } else if (hoveredModule != -1 && modules.size() > 1) {
                    for (auto& wall : modules[hoveredModule].walls) {
                        if (wall.hasTexture) {
                            UnloadTexture(wall.texture);
                        }
                    }
                    modules.erase(modules.begin() + hoveredModule);
                    hoveredModule = -1;
                    changed = true;
                }
                if (changed) SaveState(undoHistory, modules, nextModuleId);
            }
            
            if (IsKeyPressed(KEY_T)) {
                if (hoveredWall != -1 && hoveredModule != -1) {
                    printf("Attempting to load texture on wall %d in module %d\n", hoveredWall, hoveredModule);
                    
                    const char* texturePaths[] = {"texture.png", "texture.jpg", "wall.png", "wall.jpg", "tex.png", "tex.jpg"};
                    bool loaded = false;
                    
                    for (int i = 0; i < 6; i++) {
                        if (FileExists(texturePaths[i])) {
                            printf("Found texture file: %s\n", texturePaths[i]);
                            Texture2D tex = LoadTexture(texturePaths[i]);
                            if (tex.id != 0) {
                                if (modules[hoveredModule].walls[hoveredWall].hasTexture) {
                                    UnloadTexture(modules[hoveredModule].walls[hoveredWall].texture);
                                }
                                modules[hoveredModule].walls[hoveredWall].texture = tex;
                                modules[hoveredModule].walls[hoveredWall].hasTexture = true;
                                printf("Successfully loaded texture: %s\n", texturePaths[i]);
                                loaded = true;
                                break;
                            } else {
                                printf("Failed to load texture from file: %s\n", texturePaths[i]);
                            }
                        }
                    }
                    
                    if (!loaded) {
                        printf("No texture file found, creating default blue texture\n");
                        Image img = GenImageColor(256, 256, BLUE);
                        Texture2D tex = LoadTextureFromImage(img);
                        UnloadImage(img);
                        
                        if (modules[hoveredModule].walls[hoveredWall].hasTexture) {
                            UnloadTexture(modules[hoveredModule].walls[hoveredWall].texture);
                        }
                        modules[hoveredModule].walls[hoveredWall].texture = tex;
                        modules[hoveredModule].walls[hoveredWall].hasTexture = true;
                        printf("Created default blue texture for wall\n");
                    }
                } else {
                    printf("T key pressed but no wall hovered!\n");
                }
            }
            
            // MODE_SELECT
            if (currentMode == MODE_SELECT) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    double currentTime = GetTime();
                    
                    if (hoveredNode != -1 && hoveredModule != -1) {
                        if (hoveredNode == lastClickedNode && 
                            hoveredModule == lastClickedModule && 
                            (currentTime - lastClickTime) < doubleClickTime) {
                            
                            selectedNodes.clear();
                            for (int i = 0; i < (int)modules[hoveredModule].nodes.size(); i++) {
                                selectedNodes.push_back({hoveredModule, i});
                            }
                            printf("Double-click: Selected all %zu nodes in module %d\n", 
                                   selectedNodes.size(), hoveredModule);
                            
                            lastClickedNode = -1;
                            lastClickedModule = -1;
                            lastClickTime = 0.0;
                        } else {
                            NodeSelection sel = {hoveredModule, hoveredNode};
                            auto it = std::find_if(selectedNodes.begin(), selectedNodes.end(),
                                [&sel](const NodeSelection& s) {
                                    return s.moduleIdx == sel.moduleIdx && s.nodeIdx == sel.nodeIdx;
                                });
                            
                            if (it != selectedNodes.end()) {
                                selectedNodes.erase(it);
                            } else {
                                selectedNodes.push_back(sel);
                            }
                            
                            lastClickedNode = hoveredNode;
                            lastClickedModule = hoveredModule;
                            lastClickTime = currentTime;
                        }
                    } else if (hoveredModule != -1 && hoveredNode == -1) {
                        activeModule = hoveredModule;
                    } else {
                        isDragSelecting = true;
                        dragSelectStart = GetMousePosition();
                        dragSelectEnd = dragSelectStart;
                    }
                }
                
                if (isDragSelecting) {
                    dragSelectEnd = GetMousePosition();
                }
                
                if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && isDragSelecting) {
                    isDragSelecting = false;
                    
                    float minX = fmin(dragSelectStart.x, dragSelectEnd.x);
                    float maxX = fmax(dragSelectStart.x, dragSelectEnd.x);
                    float minY = fmin(dragSelectStart.y, dragSelectEnd.y);
                    float maxY = fmax(dragSelectStart.y, dragSelectEnd.y);
                    
                    for (size_t m = 0; m < modules.size(); m++) {
                        for (size_t i = 0; i < modules[m].nodes.size(); i++) {
                            Vector2 screenPos = GetWorldToScreen(modules[m].nodes[i].position, camera);
                            
                            if (screenPos.x >= minX && screenPos.x <= maxX &&
                                screenPos.y >= minY && screenPos.y <= maxY) {
                                
                                NodeSelection sel = {(int)m, (int)i};
                                auto it = std::find_if(selectedNodes.begin(), selectedNodes.end(),
                                    [&sel](const NodeSelection& s) {
                                        return s.moduleIdx == sel.moduleIdx && s.nodeIdx == sel.nodeIdx;
                                    });
                                
                                if (it == selectedNodes.end()) {
                                    selectedNodes.push_back(sel);
                                }
                            }
                        }
                    }
                    
                    if (!selectedNodes.empty()) {
                        printf("Drag selection: %zu nodes selected\n", selectedNodes.size());
                    }
                }
            }
            
            // MODE_MOVE_VERTEX
            if (currentMode == MODE_MOVE_VERTEX) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hoveredNode != -1 && hoveredModule != -1) {
                    isDragging = true;
                    activeModule = hoveredModule;
                    dragDistance = Vector3Distance(camera.position, modules[hoveredModule].nodes[hoveredNode].position);
                }
                
                if (isDragging && hoveredNode != -1 && hoveredModule != -1) {
                    modules[hoveredModule].nodes[hoveredNode].position = GetMouseWorldPosition(camera, dragDistance);
                }
                
                if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                    if (isDragging) {
                        SaveState(undoHistory, modules, nextModuleId);
                    }
                    isDragging = false;
                }
            }
            
            // MODE_MOVE_MODULE
            if (currentMode == MODE_MOVE_MODULE) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hoveredModule != -1) {
                    isDraggingModule = true;
                    activeModule = hoveredModule;
                    dragDistance = 20.0f;
                    lastMouseWorld = GetMouseWorldPosition(camera, dragDistance);
                }
                
                if (isDraggingModule && hoveredModule != -1) {
                    Vector3 cur = GetMouseWorldPosition(camera, dragDistance);
                    Vector3 delta = Vector3Subtract(cur, lastMouseWorld);
                    for (auto& node : modules[hoveredModule].nodes) {
                        node.position = Vector3Add(node.position, delta);
                    }
                    modules[hoveredModule].center = Vector3Add(modules[hoveredModule].center, delta);
                    lastMouseWorld = cur;
                }
                
                if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                    if (isDraggingModule) {
                        SaveState(undoHistory, modules, nextModuleId);
                    }
                    isDraggingModule = false;
                }
            }
            
            // MODE_ADD_NODE
            if (currentMode == MODE_ADD_NODE) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    float moduleAssignmentDistance = 15.0f;
                    
                    int closestModule = -1;
                    float closestDist = FLT_MAX;
                    for (size_t m = 0; m < modules.size(); m++) {
                        for (size_t i = 0; i < modules[m].nodes.size(); i++) {
                            float dist = Vector3Distance(previewNodePosition, modules[m].nodes[i].position);
                            if (dist < closestDist && dist <= moduleAssignmentDistance) {
                                closestDist = dist;
                                closestModule = (int)m;
                            }
                        }
                    }
                    
                    if (closestModule != -1 && hoveredModule == -1) {
                        hoveredModule = closestModule;
                    }
                    
                    if (hoveredModule != -1) {
                        Node newNode;
                        newNode.position = previewNodePosition;
                        modules[hoveredModule].nodes.push_back(newNode);
                        activeModule = hoveredModule;
                    } else {
                        GridModule newModule;
                        Node newNode;
                        newNode.position = previewNodePosition;
                        newModule.nodes.push_back(newNode);
                        newModule.center = previewNodePosition;
                        newModule.id = nextModuleId++;
                        modules.push_back(newModule);
                        activeModule = (int)modules.size() - 1;
                    }
                    
                    SaveState(undoHistory, modules, nextModuleId);
                }
            }
            
            // MODE_CONNECT
            if (currentMode == MODE_CONNECT) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hoveredNode != -1 && hoveredModule != -1) {
                    if (connectStartNode == -1) {
                        connectStartNode = hoveredNode;
                        connectStartModule = hoveredModule;
                    } else {
                        if (!(connectStartNode == hoveredNode && connectStartModule == hoveredModule)) {
                            Node& node1 = modules[connectStartModule].nodes[connectStartNode];
                            Node& node2 = modules[hoveredModule].nodes[hoveredNode];
                            
                            bool alreadyConnected = false;

                            if (connectStartModule == hoveredModule) {
                                for (int conn : node1.connections) {
                                    if (conn == hoveredNode) {
                                        alreadyConnected = true;
                                        break;
                                    }
                                }

                                if (!alreadyConnected) {
                                    node1.connections.push_back(hoveredNode);
                                    node2.connections.push_back(connectStartNode);
                                    SaveState(undoHistory, modules, nextModuleId);
                                }
                            } else {
                                for (const auto& p : node1.crossModuleConnections) {
                                    if (p.first == hoveredModule && p.second == hoveredNode) {
                                        alreadyConnected = true;
                                        break;
                                    }
                                }

                                if (!alreadyConnected) {
                                    node1.crossModuleConnections.push_back({hoveredModule, hoveredNode});
                                    node2.crossModuleConnections.push_back({connectStartModule, connectStartNode});
                                    SaveState(undoHistory, modules, nextModuleId);
                                }
                            }
                        }
                        
                        connectStartNode = -1;
                        connectStartModule = -1;
                    }
                }
            }
            
            // MODE_ROTATE_MODULE
            if (currentMode == MODE_ROTATE_MODULE) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hoveredModule != -1) {
                    activeModule = hoveredModule;
                    printf("Module %d selected for rotation. Use arrow keys or mouse drag to rotate.\n", activeModule);
                }
                
                if (activeModule != -1 && activeModule < (int)modules.size()) {
                    Vector3 center = modules[activeModule].center;
                    bool rotated = false;
                    float rotationAngle = 5.0f * DEG2RAD;
                    
                    // Keyboard rotation
                    if (IsKeyPressed(KEY_LEFT)) {
                        for (auto& node : modules[activeModule].nodes) {
                            Vector3 offset = Vector3Subtract(node.position, center);
                            float x = offset.x * cos(rotationAngle) - offset.z * sin(rotationAngle);
                            float z = offset.x * sin(rotationAngle) + offset.z * cos(rotationAngle);
                            node.position = Vector3Add(center, {x, offset.y, z});
                        }
                        rotated = true;
                    }
                    if (IsKeyPressed(KEY_RIGHT)) {
                        for (auto& node : modules[activeModule].nodes) {
                            Vector3 offset = Vector3Subtract(node.position, center);
                            float x = offset.x * cos(-rotationAngle) - offset.z * sin(-rotationAngle);
                            float z = offset.x * sin(-rotationAngle) + offset.z * cos(-rotationAngle);
                            node.position = Vector3Add(center, {x, offset.y, z});
                        }
                        rotated = true;
                    }
                    if (IsKeyPressed(KEY_UP)) {
                        for (auto& node : modules[activeModule].nodes) {
                            Vector3 offset = Vector3Subtract(node.position, center);
                            float y = offset.y * cos(rotationAngle) - offset.z * sin(rotationAngle);
                            float z = offset.y * sin(rotationAngle) + offset.z * cos(rotationAngle);
                            node.position = Vector3Add(center, {offset.x, y, z});
                        }
                        rotated = true;
                    }
                    if (IsKeyPressed(KEY_DOWN)) {
                        for (auto& node : modules[activeModule].nodes) {
                            Vector3 offset = Vector3Subtract(node.position, center);
                            float y = offset.y * cos(-rotationAngle) - offset.z * sin(-rotationAngle);
                            float z = offset.y * sin(-rotationAngle) + offset.z * cos(-rotationAngle);
                            node.position = Vector3Add(center, {offset.x, y, z});
                        }
                        rotated = true;
                    }
                    if (IsKeyPressed(KEY_PAGE_UP)) {
                        for (auto& node : modules[activeModule].nodes) {
                            Vector3 offset = Vector3Subtract(node.position, center);
                            float x = offset.x * cos(rotationAngle) - offset.y * sin(rotationAngle);
                            float y = offset.x * sin(rotationAngle) + offset.y * cos(rotationAngle);
                            node.position = Vector3Add(center, {x, y, offset.z});
                        }
                        rotated = true;
                    }
                    if (IsKeyPressed(KEY_PAGE_DOWN)) {
                        for (auto& node : modules[activeModule].nodes) {
                            Vector3 offset = Vector3Subtract(node.position, center);
                            float x = offset.x * cos(-rotationAngle) - offset.y * sin(-rotationAngle);
                            float y = offset.x * sin(-rotationAngle) + offset.y * cos(-rotationAngle);
                            node.position = Vector3Add(center, {x, y, offset.z});
                        }
                        rotated = true;
                    }
                    
                    // Mouse drag rotation
                    static bool isRotatingWithMouse = false;
                    static Vector2 lastRotateMousePos = {0, 0};
                    
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        isRotatingWithMouse = true;
                        lastRotateMousePos = GetMousePosition();
                    }
                    
                    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                        if (isRotatingWithMouse && rotated) {
                            SaveState(undoHistory, modules, nextModuleId);
                        }
                        isRotatingWithMouse = false;
                    }
                    
                    if (isRotatingWithMouse && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                        Vector2 currentMousePos = GetMousePosition();
                        Vector2 delta = {currentMousePos.x - lastRotateMousePos.x, currentMousePos.y - lastRotateMousePos.y};
                        
                        if (fabs(delta.x) > 0.5f || fabs(delta.y) > 0.5f) {
                            float mouseSensitivity = 0.003f;
                            
                            // Horizontal mouse movement rotates around Y axis
                            if (fabs(delta.x) > 0.5f) {
                                float angleY = -delta.x * mouseSensitivity;
                                for (auto& node : modules[activeModule].nodes) {
                                    Vector3 offset = Vector3Subtract(node.position, center);
                                    float x = offset.x * cos(angleY) - offset.z * sin(angleY);
                                    float z = offset.x * sin(angleY) + offset.z * cos(angleY);
                                    node.position = Vector3Add(center, {x, offset.y, z});
                                }
                            }
                            
                            // Vertical mouse movement rotates around X axis
                            if (fabs(delta.y) > 0.5f) {
                                float angleX = -delta.y * mouseSensitivity;
                                for (auto& node : modules[activeModule].nodes) {
                                    Vector3 offset = Vector3Subtract(node.position, center);
                                    float y = offset.y * cos(angleX) - offset.z * sin(angleX);
                                    float z = offset.y * sin(angleX) + offset.z * cos(angleX);
                                    node.position = Vector3Add(center, {offset.x, y, z});
                                }
                            }
                            
                            rotated = true;
                            lastRotateMousePos = currentMousePos;
                        }
                    }
                    
                    if (rotated && !isRotatingWithMouse) {
                        SaveState(undoHistory, modules, nextModuleId);
                    }
                }
            }
            
            // MODE_SCALE
            if (currentMode == MODE_SCALE) {
                if (!selectedNodes.empty()) {
                    // Calculate center of selected nodes
                    Vector3 selectionCenter = {0, 0, 0};
                    for (const auto& sel : selectedNodes) {
                        selectionCenter.x += modules[sel.moduleIdx].nodes[sel.nodeIdx].position.x;
                        selectionCenter.y += modules[sel.moduleIdx].nodes[sel.nodeIdx].position.y;
                        selectionCenter.z += modules[sel.moduleIdx].nodes[sel.nodeIdx].position.z;
                    }
                    selectionCenter.x /= selectedNodes.size();
                    selectionCenter.y /= selectedNodes.size();
                    selectionCenter.z /= selectedNodes.size();
                    
                    bool scaled = false;
                    float scaleStep = 0.05f;
                    
                    // Keyboard scaling
                    if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) {
                        // Scale up
                        for (const auto& sel : selectedNodes) {
                            Vector3& pos = modules[sel.moduleIdx].nodes[sel.nodeIdx].position;
                            Vector3 offset = Vector3Subtract(pos, selectionCenter);
                            offset = Vector3Scale(offset, 1.0f + scaleStep);
                            pos = Vector3Add(selectionCenter, offset);
                        }
                        scaled = true;
                    }
                    if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) {
                        // Scale down
                        for (const auto& sel : selectedNodes) {
                            Vector3& pos = modules[sel.moduleIdx].nodes[sel.nodeIdx].position;
                            Vector3 offset = Vector3Subtract(pos, selectionCenter);
                            offset = Vector3Scale(offset, 1.0f - scaleStep);
                            pos = Vector3Add(selectionCenter, offset);
                        }
                        scaled = true;
                    }
                    
                    // Mouse wheel scaling
                    float wheel = GetMouseWheelMove();
                    if (wheel != 0) {
                        float scaleFactor = 1.0f + (wheel * scaleStep);
                        for (const auto& sel : selectedNodes) {
                            Vector3& pos = modules[sel.moduleIdx].nodes[sel.nodeIdx].position;
                            Vector3 offset = Vector3Subtract(pos, selectionCenter);
                            offset = Vector3Scale(offset, scaleFactor);
                            pos = Vector3Add(selectionCenter, offset);
                        }
                        scaled = true;
                    }
                    
                    // Mouse drag scaling
                    static bool isScalingWithMouse = false;
                    static Vector2 lastScaleMousePos = {0, 0};
                    static float initialMouseY = 0;
                    
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        isScalingWithMouse = true;
                        lastScaleMousePos = GetMousePosition();
                        initialMouseY = lastScaleMousePos.y;
                    }
                    
                    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                        if (isScalingWithMouse && scaled) {
                            SaveState(undoHistory, modules, nextModuleId);
                        }
                        isScalingWithMouse = false;
                    }
                    
                    if (isScalingWithMouse && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                        Vector2 currentMousePos = GetMousePosition();
                        float deltaY = lastScaleMousePos.y - currentMousePos.y; // Inverted: up = scale up
                        
                        if (fabs(deltaY) > 0.5f) {
                            float scaleFactor = 1.0f + (deltaY * 0.001f); // Mouse sensitivity
                            for (const auto& sel : selectedNodes) {
                                Vector3& pos = modules[sel.moduleIdx].nodes[sel.nodeIdx].position;
                                Vector3 offset = Vector3Subtract(pos, selectionCenter);
                                offset = Vector3Scale(offset, scaleFactor);
                                pos = Vector3Add(selectionCenter, offset);
                            }
                            scaled = true;
                            lastScaleMousePos = currentMousePos;
                        }
                    }
                    
                    if (scaled && !isScalingWithMouse) {
                        SaveState(undoHistory, modules, nextModuleId);
                    }
                } else {
                    // No nodes selected - show instructions
                    static double lastMessageTime = 0;
                    if (GetTime() - lastMessageTime > 3.0) {
                        printf("Scale mode: Select nodes first (press 1 for Select mode)\n");
                        lastMessageTime = GetTime();
                    }
                }
            }
        }

        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode3D(camera);
        
        for (size_t m = 0; m < modules.size(); m++) {
            for (size_t w = 0; w < modules[m].walls.size(); w++) {
                const Wall& wall = modules[m].walls[w];
                
                Color wc = {100, 100, 150, 180};
                if (cursorEnabled && (int)m == hoveredModule && (int)w == hoveredWall) wc = {255, 100, 100, 220};
                
                DrawWall(wall, modules[m].nodes, wc, true);
            }
            
            if (showConnections) {
                for (size_t i = 0; i < modules[m].nodes.size(); i++) {
                    for (int conn : modules[m].nodes[i].connections) {
                        if ((int)i < conn) {
                            DrawLine3D(modules[m].nodes[i].position, modules[m].nodes[conn].position, Color{80, 80, 80, 255});
                        }
                    }
                }
                
                for (size_t i = 0; i < modules[m].nodes.size(); i++) {
                    for (const auto& conn : modules[m].nodes[i].crossModuleConnections) {
                        int targetModule = conn.first;
                        int targetNode = conn.second;
                        
                        if (targetModule >= 0 && targetModule < (int)modules.size() &&
                            targetNode >= 0 && targetNode < (int)modules[targetModule].nodes.size()) {
                            
                            if ((int)m < targetModule) {
                                DrawLine3D(modules[m].nodes[i].position, 
                                         modules[targetModule].nodes[targetNode].position, 
                                         Color{0, 200, 255, 255});
                            }
                        }
                    }
                }
            }
            
            for (size_t i = 0; i < modules[m].nodes.size(); i++) {
                Color nc = DARKPURPLE;
                
                if (cursorEnabled) {
                    bool isSelected = false;
                    for (const auto& sel : selectedNodes) {
                        if (sel.moduleIdx == (int)m && sel.nodeIdx == (int)i) {
                            isSelected = true;
                            break;
                        }
                    }
                    
                    if (isSelected) {
                        nc = YELLOW;
                    } else if (currentMode == MODE_CONNECT && connectStartNode == (int)i && connectStartModule == (int)m) {
                        nc = LIME;
                    } else if ((int)m == hoveredModule && (int)i == hoveredNode) {
                        nc = (currentMode == MODE_SELECT) ? GREEN : RED;
                    } else if ((int)m == hoveredModule) {
                        nc = SKYBLUE;
                    } else if ((int)m == activeModule) {
                        nc = ORANGE;
                    }
                }
                
                DrawSphere(modules[m].nodes[i].position, sphereRadius, nc);
            }
        }
        
        if (showPreviewNode && currentMode == MODE_ADD_NODE) {
            DrawSphere(previewNodePosition, sphereRadius * 1.2f, Color{255, 255, 0, 150});
            DrawSphereWires(previewNodePosition, sphereRadius * 1.2f, 8, 8, YELLOW);
        }
        
        if (currentMode == MODE_CONNECT && connectStartNode != -1 && connectStartModule != -1) {
            Vector3 startPos = modules[connectStartModule].nodes[connectStartNode].position;
            if (hoveredNode != -1 && hoveredModule != -1) {
                Vector3 endPos = modules[hoveredModule].nodes[hoveredNode].position;
                Color lineColor = (connectStartModule == hoveredModule) ? LIME : SKYBLUE;
                DrawLine3D(startPos, endPos, lineColor);
                DrawSphere(endPos, sphereRadius * 0.5f, lineColor);
            } else {
                Vector3 mouseWorld = GetMouseWorldPosition(camera, Vector3Distance(camera.position, startPos));
                DrawLine3D(startPos, mouseWorld, Color{0, 255, 0, 100});
            }
        }
        
        if (showGrid) {
            for (int i = -gridSlices; i <= gridSlices; i++) {
                if (i % 5 == 0) {
                    DrawLine3D({(float)i*3, 0, -(float)gridSlices*3}, {(float)i*3, 0, (float)gridSlices*3}, {60,60,60,255});
                    DrawLine3D({-(float)gridSlices*3, 0, (float)i*3}, {(float)gridSlices*3, 0, (float)i*3}, {60,60,60,255});
                } else {
                    DrawLine3D({(float)i*3, 0, -(float)gridSlices*3}, {(float)i*3, 0, (float)gridSlices*3}, {30,30,30,255});
                    DrawLine3D({-(float)gridSlices*3, 0, (float)i*3}, {(float)gridSlices*3, 0, (float)i*3}, {30,30,30,255});
                }
            }
        }
        
        EndMode3D();

        int tw = 0; 
        int totalConnections = 0;
        int crossModuleConnections = 0;
        
        for (const auto& mod : modules) {
            tw += mod.walls.size();
            for (const auto& node : mod.nodes) {
                totalConnections += node.connections.size();
                crossModuleConnections += node.crossModuleConnections.size();
            }
        }
        
        DrawText(TextFormat("Modules: %zu | Walls: %d | Connections: %d (%d cross) | FPS: %d", 
                           modules.size(), tw, totalConnections/2, crossModuleConnections/2, GetFPS()), 10, 10, 18, YELLOW);
        
        const char* modeName = "";
        Color modeColor = WHITE;
        if (currentMode == MODE_SELECT) {
            modeName = "SELECT MODE";
            modeColor = GREEN;
            DrawText(TextFormat("Selected: %zu nodes (cross-module) | SPACE: Wall | DEL: Delete", selectedNodes.size()), 10, 35, 16, modeColor);
        } else if (currentMode == MODE_MOVE_VERTEX) {
            modeName = "MOVE VERTEX MODE";
            modeColor = RED;
            DrawText("LMB: Drag vertex", 10, 35, 16, modeColor);
        } else if (currentMode == MODE_MOVE_MODULE) {
            modeName = "MOVE MODULE MODE";
            modeColor = BLUE;
            DrawText("LMB: Drag entire module | DEL: Delete active module", 10, 35, 16, modeColor);
        } else if (currentMode == MODE_ADD_NODE) {
            modeName = "ADD NODE MODE";
            modeColor = YELLOW;
            DrawText(TextFormat("LMB: Add node | Mouse Wheel: Distance (%.1f)", addNodeDistance), 10, 35, 16, modeColor);
        } else if (currentMode == MODE_CONNECT) {
            modeName = "CONNECT MODE";
            modeColor = LIME;
            if (connectStartNode == -1) {
                DrawText("Click first node to start connection", 10, 35, 16, modeColor);
            } else {
                DrawText("Click second node (any module) to connect", 10, 35, 16, modeColor);
            }
        } else if (currentMode == MODE_ROTATE_MODULE) {
            modeName = "ROTATE MODULE MODE";
            modeColor = MAGENTA;
            if (activeModule == -1) {
                DrawText("Click on a module to select it for rotation", 10, 35, 16, modeColor);
            } else {
                DrawText(TextFormat("Rotating Module %d | ARROWS: Rotate | LMB+Drag: Free rotate", activeModule), 10, 35, 16, modeColor);
            }
        } else if (currentMode == MODE_SCALE) {
            modeName = "SCALE MODE";
            modeColor = ORANGE;
            if (selectedNodes.empty()) {
                DrawText("No nodes selected! Press 1 to select nodes first", 10, 35, 16, modeColor);
            } else {
                DrawText(TextFormat("Scaling %zu nodes | +/-: Scale | Mouse Wheel: Scale | LMB+Drag: Scale", selectedNodes.size()), 10, 35, 16, modeColor);
            }
        }
        
        DrawText(TextFormat("Mode: %s", modeName), 10, 60, 18, modeColor);
        DrawText("1:Select | 2:Move Vertex | 3:Move Module | 4:Add | 5:Connect | 6:Rotate | 7:Scale", 10, 85, 14, LIGHTGRAY);
        DrawText("RMB: Rotate Camera | ARROWS: Move/Rotate | +/-: Scale (mode 7)", 10, 110, 14, LIGHTGRAY);
        DrawText("TAB: FPS Camera | N: Add module | CTRL+Z: Undo | DEL: Delete | F11: Max", 10, 135, 14, DARKGRAY);
        DrawText("CTRL+S/F5: Export | CTRL+O/F6: Import | CTRL+D/C: Clone | T: Texture", 10, 160, 14, DARKGRAY);
        DrawText("DRAG & DROP 'model.obj' to Import", 10, 185, 14, GREEN);
        DrawText("ESC: Cancel (press twice within 2s to exit)", 10, 210, 14, DARKGRAY);
        
        if (isDragSelecting) {
            float minX = fmin(dragSelectStart.x, dragSelectEnd.x);
            float maxX = fmax(dragSelectStart.x, dragSelectEnd.x);
            float minY = fmin(dragSelectStart.y, dragSelectEnd.y);
            float maxY = fmax(dragSelectStart.y, dragSelectEnd.y);
            
            DrawRectangleLines((int)minX, (int)minY, (int)(maxX - minX), (int)(maxY - minY), YELLOW);
            DrawRectangle((int)minX, (int)minY, (int)(maxX - minX), (int)(maxY - minY), Color{255, 255, 0, 30});
        }
        
        if (GetTime() - escMessageTime < 3.0) {
            int msgWidth = MeasureText(escMessage, 20);
            DrawRectangle(GetScreenWidth()/2 - msgWidth/2 - 20, GetScreenHeight() - 80, msgWidth + 40, 50, Color{0, 0, 0, 200});
            DrawText(escMessage, GetScreenWidth()/2 - msgWidth/2, GetScreenHeight() - 65, 20, YELLOW);
        }
        
        EndDrawing();
    }

    EnableCursor();
    CloseWindow();
    return 0;
}