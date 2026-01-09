#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include <vector>
#include <cmath>
#include <cstdio>
#include <cfloat>
#include <algorithm>
#include <deque>
#include <queue>
#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <map>

// Texture library to manage multiple textures
struct TextureLibrary {
    std::vector<Texture2D> textures;
    std::vector<std::string> textureNames;

    int AddTexture(Texture2D tex, const std::string& name) {
        textures.push_back(tex);
        textureNames.push_back(name);
        return (int)textures.size() - 1;
    }

    void Clear() {
        for (auto& tex : textures) {
            UnloadTexture(tex);
        }
        textures.clear();
        textureNames.clear();
    }
};

// Animation structure for animated planes
struct AnimatedPlane {
    std::vector<Texture2D> frames;
    std::vector<std::string> frameNames;
    Vector3 position;
    Vector3 size;
    int currentFrame;
    float frameTime;
    float timeSinceLastFrame;
    bool isPlaying;
    bool billboardMode;

    AnimatedPlane() : position({0, 0, 0}), size({5, 5, 0}), currentFrame(0),
                      frameTime(0.1f), timeSinceLastFrame(0), isPlaying(true), billboardMode(false) {}

    void AddFrame(Texture2D tex, const std::string& name) {
        frames.push_back(tex);
        frameNames.push_back(name);
    }

    void Update(float deltaTime) {
        if (isPlaying && frames.size() > 1) {
            timeSinceLastFrame += deltaTime;
            if (timeSinceLastFrame >= frameTime) {
                timeSinceLastFrame = 0;
                currentFrame = (currentFrame + 1) % (int)frames.size();
            }
        }
    }

    void NextFrame() {
        if (!frames.empty()) {
            currentFrame = (currentFrame + 1) % (int)frames.size();
        }
    }

    void PreviousFrame() {
        if (!frames.empty()) {
            currentFrame--;
            if (currentFrame < 0) currentFrame = (int)frames.size() - 1;
        }
    }

    void Clear() {
        for (auto& tex : frames) {
            UnloadTexture(tex);
        }
        frames.clear();
        frameNames.clear();
        currentFrame = 0;
    }
};

// Global texture library
TextureLibrary g_textureLibrary;
std::vector<AnimatedPlane> g_animatedPlanes;

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
    int textureId;  // Index into texture library (-1 if no texture)
    std::string textureName;  // Name of the texture file for export
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

// Auto-detect and create walls from connected coplanar nodes
void AutoDetectWalls(GridModule& module) {
    // Clear existing walls
    module.walls.clear();
    
    // Track which node groups we've already processed
    std::vector<bool> processed(module.nodes.size(), false);
    
    // For each node, try to find coplanar connected groups
    for (size_t i = 0; i < module.nodes.size(); i++) {
        if (processed[i]) continue;
        
        // BFS to find all connected nodes
        std::vector<int> connected;
        std::queue<int> toVisit;
        std::vector<bool> visited(module.nodes.size(), false);
        
        toVisit.push(i);
        visited[i] = true;
        
        while (!toVisit.empty()) {
            int current = toVisit.front();
            toVisit.pop();
            connected.push_back(current);
            
            for (int neighbor : module.nodes[current].connections) {
                if (!visited[neighbor]) {
                    visited[neighbor] = true;
                    toVisit.push(neighbor);
                }
            }
        }
        
        // If we have at least 3 connected nodes, check if they're coplanar
        if (connected.size() >= 3) {
            // Try to find the largest coplanar subset
            std::vector<int> bestGroup;
            
            // Start with first 3 nodes
            if (connected.size() >= 3) {
                std::vector<int> testGroup;
                
                // Try all combinations to find coplanar groups
                for (size_t a = 0; a < connected.size(); a++) {
                    for (size_t b = a + 1; b < connected.size(); b++) {
                        for (size_t c = b + 1; c < connected.size(); c++) {
                            testGroup.clear();
                            testGroup.push_back(connected[a]);
                            testGroup.push_back(connected[b]);
                            testGroup.push_back(connected[c]);
                            
                            // Try to add more nodes to this plane
                            Vector3 p1 = module.nodes[testGroup[0]].position;
                            Vector3 p2 = module.nodes[testGroup[1]].position;
                            Vector3 p3 = module.nodes[testGroup[2]].position;
                            
                            Vector3 v1 = Vector3Subtract(p2, p1);
                            Vector3 v2 = Vector3Subtract(p3, p1);
                            Vector3 normal = Vector3Normalize(Vector3CrossProduct(v1, v2));
                            
                            // Try adding other connected nodes
                            for (size_t d = 0; d < connected.size(); d++) {
                                if (d == a || d == b || d == c) continue;
                                
                                Vector3 v3 = Vector3Subtract(module.nodes[connected[d]].position, p1);
                                float dot = fabs(Vector3DotProduct(normal, v3));
                                
                                if (dot < 0.1f) { // Tolerance for coplanarity
                                    testGroup.push_back(connected[d]);
                                }
                            }
                            
                            // Keep the largest group found
                            if (testGroup.size() > bestGroup.size()) {
                                bestGroup = testGroup;
                            }
                        }
                    }
                }
            }
            
            // Create wall from best group if we found one
            if (bestGroup.size() >= 3) {
                Wall wall;
                wall.nodeIndices = bestGroup;
                wall.hasTexture = false;
                wall.textureId = -1;
                wall.textureName = "";
                module.walls.push_back(wall);
                
                // Mark these nodes as processed
                for (int idx : bestGroup) {
                    processed[idx] = true;
                }
            }
        }
    }
    
    printf("Auto-detected %d walls from node connections\n", (int)module.walls.size());
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

    // Create MTL filename
    std::string objPath(filename);
    std::string mtlPath = objPath.substr(0, objPath.find_last_of('.')) + ".mtl";
    std::string mtlFilename = mtlPath.substr(mtlPath.find_last_of("/\\") + 1);

    file << "# OBJ file exported from GreyScaleCube\n";
    file << "# Generated model\n";
    file << "mtllib " << mtlFilename << "\n\n";

    int vertexOffset = 1;
    int texCoordOffset = 1;

    // Write vertices
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

    file << "\n# Texture coordinates\n";

    // Generate texture coordinates for all walls
    vertexOffset = 1;
    for (size_t m = 0; m < modules.size(); m++) {
        for (const auto& wall : modules[m].walls) {
            if (wall.nodeIndices.size() >= 3 && wall.hasTexture) {
                // Get wall vertices
                std::vector<Vector3> vertices;
                for (int idx : wall.nodeIndices) {
                    if (idx >= 0 && idx < (int)modules[m].nodes.size()) {
                        vertices.push_back(modules[m].nodes[idx].position);
                    }
                }

                if (vertices.size() >= 3) {
                    // Calculate bounding box for texture mapping
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

                    // Determine dominant plane
                    bool useXY = (rangeX > rangeY && rangeX > rangeZ);
                    bool useYZ = (!useXY && rangeY > rangeZ);

                    // Write texture coordinates
                    for (const auto& v : vertices) {
                        float u, vt;
                        if (useXY) {
                            u = rangeX > 0.001f ? (v.x - minX) / rangeX : 0.5f;
                            vt = rangeY > 0.001f ? (v.y - minY) / rangeY : 0.5f;
                        } else if (useYZ) {
                            u = rangeY > 0.001f ? (v.y - minY) / rangeY : 0.5f;
                            vt = rangeZ > 0.001f ? (v.z - minZ) / rangeZ : 0.5f;
                        } else {
                            u = rangeX > 0.001f ? (v.x - minX) / rangeX : 0.5f;
                            vt = rangeZ > 0.001f ? (v.z - minZ) / rangeZ : 0.5f;
                        }
                        file << "vt " << u << " " << vt << "\n";
                    }
                }
            }
        }
        vertexOffset += (int)modules[m].nodes.size();
    }

    file << "\n# Walls (faces)\n";

    vertexOffset = 1;
    texCoordOffset = 1;
    std::string currentMaterial = "";

    for (size_t m = 0; m < modules.size(); m++) {
        for (const auto& wall : modules[m].walls) {
            if (wall.nodeIndices.size() >= 3) {
                // Set material if wall has texture
                if (wall.hasTexture && !wall.textureName.empty()) {
                    std::string matName = "material_" + wall.textureName;
                    if (matName != currentMaterial) {
                        file << "usemtl " << matName << "\n";
                        currentMaterial = matName;
                    }

                    // Write face with texture coordinates
                    file << "f";
                    for (size_t i = 0; i < wall.nodeIndices.size(); i++) {
                        int nodeIdx = wall.nodeIndices[i];
                        file << " " << (vertexOffset + nodeIdx) << "/" << (texCoordOffset + i);
                    }
                    file << "\n";
                    texCoordOffset += wall.nodeIndices.size();
                } else {
                    // Write face without texture coordinates
                    file << "f";
                    for (int nodeIdx : wall.nodeIndices) {
                        file << " " << (vertexOffset + nodeIdx);
                    }
                    file << "\n";
                }
            }
        }
        vertexOffset += (int)modules[m].nodes.size();
    }

    file.close();

    // Create MTL file
    std::ofstream mtlFile(mtlPath);
    if (mtlFile.is_open()) {
        mtlFile << "# MTL file exported from GreyScaleCube\n\n";

        // Collect unique textures
        std::map<std::string, bool> uniqueTextures;
        for (const auto& module : modules) {
            for (const auto& wall : module.walls) {
                if (wall.hasTexture && !wall.textureName.empty()) {
                    uniqueTextures[wall.textureName] = true;
                }
            }
        }

        // Write materials
        for (const auto& pair : uniqueTextures) {
            std::string matName = "material_" + pair.first;
            mtlFile << "newmtl " << matName << "\n";
            mtlFile << "Ka 1.000 1.000 1.000\n";
            mtlFile << "Kd 1.000 1.000 1.000\n";
            mtlFile << "Ks 0.000 0.000 0.000\n";
            mtlFile << "d 1.0\n";
            mtlFile << "illum 1\n";
            mtlFile << "map_Kd " << pair.first << "\n\n";
        }

        mtlFile.close();
        printf("MTL file created: %s\n", mtlPath.c_str());
    }

    return true;
}

// Save complete project including texture library, animated planes, and wall textures
bool SaveProject(const std::vector<GridModule>& modules, const char* filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    file << "# Project Save File\n";
    file << "# This file contains texture library, wall textures, and animated plane data\n\n";

    printf("=== Saving Project: %s ===\n", filename);

    // Save texture library
    file << "TEXTURE_LIBRARY_START\n";
    file << g_textureLibrary.textures.size() << "\n";
    printf("Saving %d textures to library...\n", (int)g_textureLibrary.textures.size());
    for (size_t i = 0; i < g_textureLibrary.textures.size(); i++) {
        file << g_textureLibrary.textureNames[i] << "\n";
        printf("  Texture %d: %s (ID: %d)\n", (int)i, g_textureLibrary.textureNames[i].c_str(), g_textureLibrary.textures[i].id);
    }
    file << "TEXTURE_LIBRARY_END\n\n";

    // Save wall texture mappings
    file << "WALL_TEXTURES_START\n";
    int wallTexCount = 0;
    printf("Checking %d modules for textured walls...\n", (int)modules.size());
    for (size_t m = 0; m < modules.size(); m++) {
        printf("  Module %d has %d walls\n", (int)m, (int)modules[m].walls.size());
        for (size_t w = 0; w < modules[m].walls.size(); w++) {
            const Wall& wall = modules[m].walls[w];
            printf("    Wall %d: hasTexture=%d, textureId=%d\n", (int)w, wall.hasTexture, wall.textureId);
            if (wall.hasTexture && wall.textureId != -1) {
                file << "WALL_TEX " << m << " " << w << " " << wall.textureId << " " << wall.textureName << "\n";
                printf("      ✓ SAVING Wall texture: Module %d, Wall %d, TexID %d, TextureName: %s\n", 
                       (int)m, (int)w, wall.textureId, wall.textureName.c_str());
                wallTexCount++;
            }
        }
    }
    fflush(stdout);
    file << "WALL_TEXTURES_END\n\n";
    printf("Saved %d wall texture mappings\n", wallTexCount);

    // Save animated planes
    file << "ANIMATED_PLANES_START\n";
    file << g_animatedPlanes.size() << "\n";
    printf("Saving %d animated planes...\n", (int)g_animatedPlanes.size());
    for (size_t i = 0; i < g_animatedPlanes.size(); i++) {
        const AnimatedPlane& plane = g_animatedPlanes[i];
        file << "PLANE " << i << "\n";
        file << "POSITION " << plane.position.x << " " << plane.position.y << " " << plane.position.z << "\n";
        file << "SIZE " << plane.size.x << " " << plane.size.y << "\n";
        file << "FRAMETIME " << plane.frameTime << "\n";
        file << "PLAYING " << (plane.isPlaying ? 1 : 0) << "\n";
        file << "BILLBOARD " << (plane.billboardMode ? 1 : 0) << "\n";
        file << "FRAMES " << plane.frames.size() << "\n";
        printf("  Plane %d: Position(%.1f, %.1f, %.1f), %d frames\n", 
               (int)i, plane.position.x, plane.position.y, plane.position.z, (int)plane.frames.size());
        for (size_t j = 0; j < plane.frameNames.size(); j++) {
            file << plane.frameNames[j] << "\n";
            printf("    Frame %d: %s\n", (int)j, plane.frameNames[j].c_str());
        }
    }
    file << "ANIMATED_PLANES_END\n";

    file.close();
    printf("Project saved to %s\n", filename);
    return true;
}

// Load complete project including texture library, animated planes, and wall textures
bool LoadProject(std::vector<GridModule>& modules, const char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        printf("Failed to open project file: %s\n", filename);
        return false;
    }

    printf("=== Loading Project: %s ===\n", filename);
    std::string line;
    
    // Skip header comments
    while (std::getline(file, line)) {
        if (line.find("TEXTURE_LIBRARY_START") != std::string::npos) {
            break;
        }
    }

    // Load texture library
    int numTextures = 0;
    if (std::getline(file, line)) {
        numTextures = std::stoi(line);
    }

    printf("Loading %d textures...\n", numTextures);

    // Clear existing library
    g_textureLibrary.Clear();

    for (int i = 0; i < numTextures; i++) {
        if (std::getline(file, line)) {
            printf("  Texture %d: %s\n", i, line.c_str());
            if (FileExists(line.c_str())) {
                Texture2D tex = LoadTexture(line.c_str());
                if (tex.id > 0) {
                    g_textureLibrary.AddTexture(tex, line);
                    printf("    ✓ Loaded successfully (ID: %d)\n", tex.id);
                } else {
                    printf("    ✗ Failed to load texture\n");
                }
            } else {
                printf("    ✗ File not found\n");
            }
        }
    }

    printf("Texture library now has %d textures\n", (int)g_textureLibrary.textures.size());

    // Skip to wall textures section
    while (std::getline(file, line)) {
        if (line.find("WALL_TEXTURES_START") != std::string::npos) {
            printf("Found WALL_TEXTURES_START\n");
            break;
        }
    }

    // Load wall texture mappings
    int wallTextureCount = 0;
    while (std::getline(file, line)) {
        if (line.find("WALL_TEXTURES_END") != std::string::npos) {
            printf("Found WALL_TEXTURES_END\n");
            break;
        }
        if (line.find("WALL_TEX") != std::string::npos) {
            int moduleIdx, wallIdx, texId;
            char texName[512];
            printf("  Parsing: %s\n", line.c_str());
            if (sscanf(line.c_str(), "WALL_TEX %d %d %d %511[^\n]", &moduleIdx, &wallIdx, &texId, texName) == 4) {
                printf("    Module %d, Wall %d, TexID %d, Name: %s\n", moduleIdx, wallIdx, texId, texName);
                if (moduleIdx < (int)modules.size() && wallIdx < (int)modules[moduleIdx].walls.size()) {
                    if (texId >= 0 && texId < (int)g_textureLibrary.textures.size()) {
                        modules[moduleIdx].walls[wallIdx].texture = g_textureLibrary.textures[texId];
                        modules[moduleIdx].walls[wallIdx].hasTexture = true;
                        modules[moduleIdx].walls[wallIdx].textureId = texId;
                        modules[moduleIdx].walls[wallIdx].textureName = texName;
                        printf("    ✓ Applied texture to wall (Texture ID: %d)\n", modules[moduleIdx].walls[wallIdx].texture.id);
                        wallTextureCount++;
                    } else {
                        printf("    ✗ TexID %d out of range (library size: %d)\n", texId, (int)g_textureLibrary.textures.size());
                    }
                } else {
                    printf("    ✗ Module/Wall index out of range (modules: %d, walls in module: %d)\n", 
                           (int)modules.size(), 
                           moduleIdx < (int)modules.size() ? (int)modules[moduleIdx].walls.size() : 0);
                }
            } else {
                printf("    ✗ Failed to parse line\n");
            }
        }
    }

    printf("Applied textures to %d walls\n", wallTextureCount);

    // Skip to animated planes section
    while (std::getline(file, line)) {
        if (line.find("ANIMATED_PLANES_START") != std::string::npos) {
            break;
        }
    }

    int numPlanes = 0;
    if (std::getline(file, line)) {
        numPlanes = std::stoi(line);
    }

    // Clear existing planes
    for (auto& plane : g_animatedPlanes) {
        plane.Clear();
    }
    g_animatedPlanes.clear();

    for (int i = 0; i < numPlanes; i++) {
        AnimatedPlane plane;
        
        // Read plane header
        std::getline(file, line); // PLANE i
        
        // Read position
        std::getline(file, line);
        sscanf(line.c_str(), "POSITION %f %f %f", &plane.position.x, &plane.position.y, &plane.position.z);
        
        // Read size
        std::getline(file, line);
        sscanf(line.c_str(), "SIZE %f %f", &plane.size.x, &plane.size.y);
        
        // Read frame time
        std::getline(file, line);
        sscanf(line.c_str(), "FRAMETIME %f", &plane.frameTime);
        
        // Read playing state
        std::getline(file, line);
        int playing;
        sscanf(line.c_str(), "PLAYING %d", &playing);
        plane.isPlaying = (playing != 0);
        
        // Read billboard mode
        std::getline(file, line);
        int billboard;
        sscanf(line.c_str(), "BILLBOARD %d", &billboard);
        plane.billboardMode = (billboard != 0);
        
        // Read frames
        std::getline(file, line);
        int numFrames;
        sscanf(line.c_str(), "FRAMES %d", &numFrames);
        
        for (int j = 0; j < numFrames; j++) {
            std::getline(file, line);
            if (FileExists(line.c_str())) {
                Texture2D tex = LoadTexture(line.c_str());
                if (tex.id > 0) {
                    plane.AddFrame(tex, line);
                }
            } else {
                printf("Animation frame not found: %s\n", line.c_str());
            }
        }
        
        g_animatedPlanes.push_back(plane);
        printf("Loaded animated plane %d with %d frames\n", i, (int)plane.frames.size());
    }

    file.close();
    printf("=== Load Complete ===\n");
    printf("Total animated planes loaded: %d\n", (int)g_animatedPlanes.size());
    printf("Project loaded from %s\n", filename);
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
            wall.textureId = -1;
            wall.textureName = "";
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

    // Animation plane UI state
    bool showAnimationUI = false;
    int selectedPlaneIdx = -1;

    // Texture library UI state
    bool showTextureLibraryUI = false;
    int selectedTextureIdx = -1;

    // Save/Load dialog state
    bool showSaveDialog = false;
    bool showLoadDialog = false;
    char saveFileName[128] = "model";
    char loadFileName[128] = "model";
    int saveFileNameCursor = 5; // After "model"
    int loadFileNameCursor = 5;

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

    // Animated plane dragging
    bool isDraggingPlane = false;
    int hoveredPlane = -1;
    float planeDragDistance = 0.0f;

    enum Mode { MODE_SELECT, MODE_MOVE_VERTEX, MODE_MOVE_MODULE, MODE_ADD_NODE, MODE_CONNECT, MODE_ROTATE_MODULE, MODE_SCALE, MODE_SCALE_SPHERE };
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

    // Animation import state
    bool showAnimationImport = false;
    std::vector<std::string> pendingAnimationFiles;

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
                // Check if it's an OBJ file
                std::string firstFile = droppedFiles.paths[0];
                if (firstFile.find(".obj") != std::string::npos || firstFile.find(".OBJ") != std::string::npos) {
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
                } else {
                    // Check if files are images
                    bool allImages = true;
                    bool allPNG = true;
                    for (int i = 0; i < droppedFiles.count; i++) {
                        std::string file = droppedFiles.paths[i];
                        bool isPNG = file.find(".png") != std::string::npos || file.find(".PNG") != std::string::npos;
                        bool isImage = isPNG || file.find(".jpg") != std::string::npos || file.find(".JPG") != std::string::npos ||
                                      file.find(".jpeg") != std::string::npos || file.find(".JPEG") != std::string::npos ||
                                      file.find(".bmp") != std::string::npos || file.find(".BMP") != std::string::npos;
                        if (!isPNG) allPNG = false;
                        if (!isImage) {
                            allImages = false;
                            break;
                        }
                    }

                    // If texture library UI is open, add textures to library
                    if (allImages && showTextureLibraryUI) {
                        int addedCount = 0;
                        for (int i = 0; i < droppedFiles.count; i++) {
                            Texture2D tex = LoadTexture(droppedFiles.paths[i]);
                            if (tex.id > 0) {
                                // Check if already in library
                                bool alreadyExists = false;
                                for (size_t j = 0; j < g_textureLibrary.textureNames.size(); j++) {
                                    if (g_textureLibrary.textureNames[j] == droppedFiles.paths[i]) {
                                        alreadyExists = true;
                                        break;
                                    }
                                }
                                if (!alreadyExists) {
                                    g_textureLibrary.AddTexture(tex, droppedFiles.paths[i]);
                                    addedCount++;
                                }
                            }
                        }
                        escMessage = TextFormat("Added %d textures to library", addedCount);
                        escMessageTime = GetTime();
                    }
                    // PNG files for animation
                    else if (allPNG) {
                        // Load PNG files as animation frames
                        // If a plane is selected and animation UI is open, add to that plane
                        // Otherwise create a new plane
                        if (selectedPlaneIdx >= 0 && selectedPlaneIdx < (int)g_animatedPlanes.size() && showAnimationUI) {
                            // Add frames to existing selected plane
                            int addedCount = 0;
                            for (int i = 0; i < droppedFiles.count; i++) {
                                Texture2D tex = LoadTexture(droppedFiles.paths[i]);
                                if (tex.id > 0) {
                                    printf("Loaded texture: %s (ID: %d, Format: %d, Width: %d, Height: %d)\n", 
                                           droppedFiles.paths[i], tex.id, tex.format, tex.width, tex.height);
                                    g_animatedPlanes[selectedPlaneIdx].AddFrame(tex, droppedFiles.paths[i]);
                                    addedCount++;
                                }
                            }
                            escMessage = TextFormat("Added %d frames to plane %d", addedCount, selectedPlaneIdx);
                            escMessageTime = GetTime();
                        } else {
                            // Create new plane with automatic offset
                            AnimatedPlane newPlane;
                            float offsetX = (float)g_animatedPlanes.size() * 7.0f;
                            newPlane.position = {offsetX, 5, 0};
                            newPlane.size = {5, 5, 0};

                            for (int i = 0; i < droppedFiles.count; i++) {
                                Texture2D tex = LoadTexture(droppedFiles.paths[i]);
                                if (tex.id > 0) {
                                    printf("Loaded texture: %s (ID: %d, Format: %d, Width: %d, Height: %d)\n", 
                                           droppedFiles.paths[i], tex.id, tex.format, tex.width, tex.height);
                                    newPlane.AddFrame(tex, droppedFiles.paths[i]);
                                }
                            }

                            if (newPlane.frames.size() > 0) {
                                g_animatedPlanes.push_back(newPlane);
                                selectedPlaneIdx = (int)g_animatedPlanes.size() - 1;
                                showAnimationUI = true;
                                escMessage = TextFormat("Created plane with %d frames", (int)newPlane.frames.size());
                                escMessageTime = GetTime();
                            }
                        }
                    }
                }
            }
            UnloadDroppedFiles(droppedFiles);
        }

        if (IsKeyPressed(KEY_TAB)) {
            cursorEnabled = !cursorEnabled;
            cursorEnabled ? EnableCursor() : DisableCursor();
        }

        if (IsKeyPressed(KEY_A) && IsKeyDown(KEY_LEFT_CONTROL)) {
            showAnimationUI = !showAnimationUI;
            if (showAnimationUI && !cursorEnabled) {
                cursorEnabled = true;
                EnableCursor();
            }
        }

        // Toggle Texture Library UI
        if (IsKeyPressed(KEY_L) && IsKeyDown(KEY_LEFT_CONTROL)) {
            showTextureLibraryUI = !showTextureLibraryUI;
            if (showTextureLibraryUI && !cursorEnabled) {
                cursorEnabled = true;
                EnableCursor();
            }
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
                            newWall.textureId = -1;
                            newWall.textureName = "";
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
                            newWall.textureId = -1;
                            newWall.textureName = "";
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
            showSaveDialog = true;
            cursorEnabled = true;
            EnableCursor();
        }

        if (IsKeyPressed(KEY_F5)) {
            showSaveDialog = true;
            cursorEnabled = true;
            EnableCursor();
        }

        if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_O)) {
            showLoadDialog = true;
            cursorEnabled = true;
            EnableCursor();
        }

        if (IsKeyPressed(KEY_F6)) {
            showLoadDialog = true;
            cursorEnabled = true;
            EnableCursor();
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
                    printf("========================================\n");
                    printf("T KEY PRESSED - Attempting to load texture on wall %d in module %d\n", hoveredWall, hoveredModule);
                    fflush(stdout);

                    bool loaded = false;

                    // Use texture library if it has textures
                    if (!g_textureLibrary.textures.empty()) {
                        printf("Using texture library (%d textures available)\n", (int)g_textureLibrary.textures.size());
                        fflush(stdout);

                        // If wall already has a texture, cycle to the next one
                        int nextTextureIdx = 0;
                        if (modules[hoveredModule].walls[hoveredWall].hasTexture && modules[hoveredModule].walls[hoveredWall].textureId != -1) {
                            // Find current texture in library and cycle to next
                            int currentId = modules[hoveredModule].walls[hoveredWall].textureId;
                            nextTextureIdx = (currentId + 1) % (int)g_textureLibrary.textures.size();
                        } else if (selectedTextureIdx >= 0 && selectedTextureIdx < (int)g_textureLibrary.textures.size()) {
                            // Use selected texture from library
                            nextTextureIdx = selectedTextureIdx;
                        }

                        // Apply texture from library (don't unload - library owns it)
                        modules[hoveredModule].walls[hoveredWall].texture = g_textureLibrary.textures[nextTextureIdx];
                        modules[hoveredModule].walls[hoveredWall].hasTexture = true;
                        modules[hoveredModule].walls[hoveredWall].textureName = g_textureLibrary.textureNames[nextTextureIdx];
                        modules[hoveredModule].walls[hoveredWall].textureId = nextTextureIdx;

                        printf("SUCCESS! Applied texture: %s\n", g_textureLibrary.textureNames[nextTextureIdx].c_str());
                        printf("  Texture ID: %d (texture %d/%d in library)\n", nextTextureIdx, nextTextureIdx + 1, (int)g_textureLibrary.textures.size());
                        printf("  Wall hasTexture: %d, textureId: %d\n", modules[hoveredModule].walls[hoveredWall].hasTexture, modules[hoveredModule].walls[hoveredWall].textureId);
                        printf("========================================\n");
                        fflush(stdout);
                        loaded = true;
                    }

                    if (!loaded) {
                        printf("ERROR: No textures in library. Use CTRL+L to open texture library and drag & drop images.\n");
                        printf("========================================\n");
                        fflush(stdout);
                        // Create default blue texture
                        Image img = GenImageColor(256, 256, BLUE);
                        Texture2D tex = LoadTextureFromImage(img);
                        UnloadImage(img);

                        if (modules[hoveredModule].walls[hoveredWall].hasTexture && modules[hoveredModule].walls[hoveredWall].textureId == -1) {
                            UnloadTexture(modules[hoveredModule].walls[hoveredWall].texture);
                        }
                        modules[hoveredModule].walls[hoveredWall].texture = tex;
                        modules[hoveredModule].walls[hoveredWall].hasTexture = true;
                        modules[hoveredModule].walls[hoveredWall].textureName = "default_blue";
                        modules[hoveredModule].walls[hoveredWall].textureId = -1;
                        printf("Created default blue texture for wall\n");
                    }
                } else {
                    printf("========================================\n");
                    printf("ERROR: T key pressed but no wall hovered!\n");
                    printf("  hoveredWall: %d, hoveredModule: %d\n", hoveredWall, hoveredModule);
                    printf("========================================\n");
                    fflush(stdout);
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

            // Animated plane movement (works in any mode when animation UI is visible)
            if (showAnimationUI && !isDragSelecting) {
                // Check which plane is hovered
                hoveredPlane = -1;
                Ray ray = GetMouseRay(GetMousePosition(), camera);
                
                for (int i = 0; i < (int)g_animatedPlanes.size(); i++) {
                    if (g_animatedPlanes[i].frames.empty()) continue;
                    
                    Vector3 pos = g_animatedPlanes[i].position;
                    Vector3 size = g_animatedPlanes[i].size;
                    
                    // Simple plane-ray intersection check
                    // Check if ray intersects with the plane's bounding box
                    BoundingBox planeBounds = {
                        {pos.x - size.x/2, pos.y - size.y/2, pos.z - 0.1f},
                        {pos.x + size.x/2, pos.y + size.y/2, pos.z + 0.1f}
                    };
                    
                    RayCollision collision = GetRayCollisionBox(ray, planeBounds);
                    if (collision.hit) {
                        hoveredPlane = i;
                        break;
                    }
                }
                
                // Start dragging plane
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hoveredPlane != -1) {
                    isDraggingPlane = true;
                    selectedPlaneIdx = hoveredPlane;
                    planeDragDistance = Vector3Distance(camera.position, g_animatedPlanes[hoveredPlane].position);
                }
                
                // Drag plane
                if (isDraggingPlane && selectedPlaneIdx >= 0 && selectedPlaneIdx < (int)g_animatedPlanes.size()) {
                    g_animatedPlanes[selectedPlaneIdx].position = GetMouseWorldPosition(camera, planeDragDistance);
                }
                
                // Stop dragging
                if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                    isDraggingPlane = false;
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

        // Draw opaque geometry first (walls, nodes, connections)
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

        // Draw transparent animated planes LAST for proper alpha blending
        // Flush any pending render batch before transparency
        rlDrawRenderBatchActive();
        
        float deltaTime = GetFrameTime();
        for (size_t i = 0; i < g_animatedPlanes.size(); i++) {
            g_animatedPlanes[i].Update(deltaTime);

            if (!g_animatedPlanes[i].frames.empty()) {
                Texture2D currentTex = g_animatedPlanes[i].frames[g_animatedPlanes[i].currentFrame];
                Vector3 pos = g_animatedPlanes[i].position;
                Vector3 size = g_animatedPlanes[i].size;

                // Draw plane with texture
                if (g_animatedPlanes[i].billboardMode) {
                    DrawBillboard(camera, currentTex, pos, size.x, WHITE);
                } else {
                    // Draw textured quad (double-sided with transparency)
                    
                    // Flush and prepare for transparency
                    rlDrawRenderBatchActive();
                    rlDisableBackfaceCulling();  // Disable for double-sided rendering
                    rlDisableDepthMask();        // Don't write to depth buffer for transparent objects
                    
                    rlPushMatrix();
                    rlTranslatef(pos.x, pos.y, pos.z);

                    // Bind texture
                    rlSetTexture(currentTex.id);

                    // Draw FRONT face
                    rlBegin(RL_QUADS);
                    rlColor4ub(255, 255, 255, 255);
                    rlNormal3f(0.0f, 0.0f, 1.0f);

                    rlTexCoord2f(0.0f, 0.0f);
                    rlVertex3f(-size.x/2, -size.y/2, 0);

                    rlTexCoord2f(1.0f, 0.0f);
                    rlVertex3f(size.x/2, -size.y/2, 0);

                    rlTexCoord2f(1.0f, 1.0f);
                    rlVertex3f(size.x/2, size.y/2, 0);

                    rlTexCoord2f(0.0f, 1.0f);
                    rlVertex3f(-size.x/2, size.y/2, 0);
                    rlEnd();

                    // Draw BACK face (reversed winding order for back-face visibility)
                    rlBegin(RL_QUADS);
                    rlColor4ub(255, 255, 255, 255);
                    rlNormal3f(0.0f, 0.0f, -1.0f);

                    rlTexCoord2f(0.0f, 0.0f);
                    rlVertex3f(-size.x/2, -size.y/2, 0);

                    rlTexCoord2f(0.0f, 1.0f);
                    rlVertex3f(-size.x/2, size.y/2, 0);

                    rlTexCoord2f(1.0f, 1.0f);
                    rlVertex3f(size.x/2, size.y/2, 0);

                    rlTexCoord2f(1.0f, 0.0f);
                    rlVertex3f(size.x/2, -size.y/2, 0);
                    rlEnd();

                    rlSetTexture(0);

                    rlPopMatrix();
                    
                    // Restore default state
                    rlDrawRenderBatchActive();
                    rlEnableDepthMask();
                    rlEnableBackfaceCulling();
                }

                // Highlight selected or hovered plane
                if ((int)i == selectedPlaneIdx && showAnimationUI) {
                    DrawCubeWires(pos, size.x, size.y, 0.1f, YELLOW);
                } else if ((int)i == hoveredPlane && showAnimationUI) {
                    DrawCubeWires(pos, size.x, size.y, 0.1f, ORANGE);
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
        DrawText("DRAG & DROP files | CTRL+A: Animation | CTRL+L: Texture Library", 10, 185, 14, GREEN);
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

        // Animation UI Panel
        if (showAnimationUI) {
            int panelX = GetScreenWidth() - 320;
            int panelY = 10;
            int panelW = 310;
            int panelH = 500;

            DrawRectangle(panelX, panelY, panelW, panelH, Color{40, 40, 40, 240});
            DrawRectangleLines(panelX, panelY, panelW, panelH, YELLOW);
            DrawText("ANIMATION PANEL", panelX + 10, panelY + 10, 16, YELLOW);
            DrawText("CTRL+A to toggle", panelX + 10, panelY + 30, 12, GRAY);

            int yOffset = panelY + 55;

            DrawText(TextFormat("Planes: %d", (int)g_animatedPlanes.size()), panelX + 10, yOffset, 14, WHITE);
            yOffset += 25;

            // Plane list
            for (int i = 0; i < (int)g_animatedPlanes.size(); i++) {
                Color btnColor = (i == selectedPlaneIdx) ? Color{80, 80, 255, 255} : Color{60, 60, 60, 255};
                DrawRectangle(panelX + 10, yOffset, panelW - 20, 25, btnColor);
                DrawRectangleLines(panelX + 10, yOffset, panelW - 20, 25, WHITE);

                Vector2 mousePos = GetMousePosition();
                if (mousePos.x >= panelX + 10 && mousePos.x <= panelX + panelW - 10 &&
                    mousePos.y >= yOffset && mousePos.y <= yOffset + 25) {
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        selectedPlaneIdx = i;
                    }
                }

                DrawText(TextFormat("Plane %d (%d frames)", i, (int)g_animatedPlanes[i].frames.size()),
                         panelX + 15, yOffset + 5, 12, WHITE);
                yOffset += 30;
            }

            yOffset += 10;

            // Button to create new empty plane
            Rectangle newPlaneBtn = {(float)(panelX + 10), (float)yOffset, (float)(panelW - 20), 30};
            bool newPlaneHover = CheckCollisionPointRec(GetMousePosition(), newPlaneBtn);
            DrawRectangle((int)newPlaneBtn.x, (int)newPlaneBtn.y, (int)newPlaneBtn.width, (int)newPlaneBtn.height,
                         newPlaneHover ? Color{80, 150, 80, 255} : Color{50, 100, 50, 255});
            DrawRectangleLines((int)newPlaneBtn.x, (int)newPlaneBtn.y, (int)newPlaneBtn.width, (int)newPlaneBtn.height, WHITE);
            DrawText("+ New Animation Plane", (int)newPlaneBtn.x + 40, (int)newPlaneBtn.y + 8, 12, WHITE);
            if (newPlaneHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                AnimatedPlane newPlane;
                // Offset each new plane so they don't stack
                float offsetX = (float)g_animatedPlanes.size() * 7.0f;
                newPlane.position = {offsetX, 5, 0};
                newPlane.size = {5, 5, 0};
                g_animatedPlanes.push_back(newPlane);
                selectedPlaneIdx = (int)g_animatedPlanes.size() - 1;
            }

            yOffset += 40;

            // Controls for selected plane
            if (selectedPlaneIdx >= 0 && selectedPlaneIdx < (int)g_animatedPlanes.size()) {
                DrawLine(panelX + 10, yOffset, panelX + panelW - 10, yOffset, GRAY);
                yOffset += 15;

                AnimatedPlane& plane = g_animatedPlanes[selectedPlaneIdx];

                DrawText(TextFormat("Frame: %d/%d", plane.currentFrame + 1, (int)plane.frames.size()),
                         panelX + 10, yOffset, 14, WHITE);
                yOffset += 25;

                // Previous Frame Button
                Rectangle prevBtn = {(float)(panelX + 10), (float)yOffset, 60, 25};
                bool prevHover = CheckCollisionPointRec(GetMousePosition(), prevBtn);
                DrawRectangle((int)prevBtn.x, (int)prevBtn.y, (int)prevBtn.width, (int)prevBtn.height,
                             prevHover ? Color{100, 100, 100, 255} : Color{60, 60, 60, 255});
                DrawRectangleLines((int)prevBtn.x, (int)prevBtn.y, (int)prevBtn.width, (int)prevBtn.height, WHITE);
                DrawText("<", (int)prevBtn.x + 25, (int)prevBtn.y + 5, 14, WHITE);
                if (prevHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    plane.PreviousFrame();
                }

                // Next Frame Button
                Rectangle nextBtn = {(float)(panelX + 80), (float)yOffset, 60, 25};
                bool nextHover = CheckCollisionPointRec(GetMousePosition(), nextBtn);
                DrawRectangle((int)nextBtn.x, (int)nextBtn.y, (int)nextBtn.width, (int)nextBtn.height,
                             nextHover ? Color{100, 100, 100, 255} : Color{60, 60, 60, 255});
                DrawRectangleLines((int)nextBtn.x, (int)nextBtn.y, (int)nextBtn.width, (int)nextBtn.height, WHITE);
                DrawText(">", (int)nextBtn.x + 25, (int)nextBtn.y + 5, 14, WHITE);
                if (nextHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    plane.NextFrame();
                }

                // Play/Pause Button
                Rectangle playBtn = {(float)(panelX + 150), (float)yOffset, 60, 25};
                bool playHover = CheckCollisionPointRec(GetMousePosition(), playBtn);
                DrawRectangle((int)playBtn.x, (int)playBtn.y, (int)playBtn.width, (int)playBtn.height,
                             playHover ? Color{100, 100, 100, 255} : Color{60, 60, 60, 255});
                DrawRectangleLines((int)playBtn.x, (int)playBtn.y, (int)playBtn.width, (int)playBtn.height, WHITE);
                DrawText(plane.isPlaying ? "||" : ">", (int)playBtn.x + 22, (int)playBtn.y + 5, 14, WHITE);
                if (playHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    plane.isPlaying = !plane.isPlaying;
                }

                yOffset += 35;

                // FPS control
                DrawText(TextFormat("FPS: %.1f", 1.0f / plane.frameTime), panelX + 10, yOffset, 12, WHITE);
                yOffset += 20;

                Rectangle fpsSlider = {(float)(panelX + 10), (float)yOffset, (float)(panelW - 20), 10};
                DrawRectangle((int)fpsSlider.x, (int)fpsSlider.y, (int)fpsSlider.width, (int)fpsSlider.height, Color{60, 60, 60, 255});

                float fps = 1.0f / plane.frameTime;
                float normalizedFPS = (fps - 1.0f) / 59.0f; // 1-60 FPS range
                int sliderPos = (int)(fpsSlider.x + normalizedFPS * fpsSlider.width);
                DrawCircle(sliderPos, (int)(fpsSlider.y + fpsSlider.height/2), 8, WHITE);

                if (CheckCollisionPointRec(GetMousePosition(), {fpsSlider.x - 10, fpsSlider.y - 10, fpsSlider.width + 20, 30}) &&
                    IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                    float mouseX = GetMousePosition().x;
                    float newNormalized = (mouseX - fpsSlider.x) / fpsSlider.width;
                    newNormalized = fmaxf(0.0f, fminf(1.0f, newNormalized));
                    float newFPS = 1.0f + newNormalized * 59.0f;
                    plane.frameTime = 1.0f / newFPS;
                }

                yOffset += 30;

                // Position controls
                DrawText("Position:", panelX + 10, yOffset, 12, WHITE);
                yOffset += 20;
                DrawText(TextFormat("X: %.1f Y: %.1f Z: %.1f", plane.position.x, plane.position.y, plane.position.z),
                         panelX + 10, yOffset, 10, LIGHTGRAY);
                yOffset += 20;

                // Size controls
                DrawText(TextFormat("Size: %.1f x %.1f", plane.size.x, plane.size.y), panelX + 10, yOffset, 12, WHITE);
                yOffset += 25;

                // Billboard toggle
                Rectangle billboardBtn = {(float)(panelX + 10), (float)yOffset, 120, 25};
                bool billboardHover = CheckCollisionPointRec(GetMousePosition(), billboardBtn);
                DrawRectangle((int)billboardBtn.x, (int)billboardBtn.y, (int)billboardBtn.width, (int)billboardBtn.height,
                             plane.billboardMode ? Color{100, 200, 100, 255} : Color{60, 60, 60, 255});
                DrawRectangleLines((int)billboardBtn.x, (int)billboardBtn.y, (int)billboardBtn.width, (int)billboardBtn.height, WHITE);
                DrawText("Billboard", (int)billboardBtn.x + 20, (int)billboardBtn.y + 5, 12, WHITE);
                if (billboardHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    plane.billboardMode = !plane.billboardMode;
                }

                yOffset += 35;

                // Delete button
                Rectangle delBtn = {(float)(panelX + 10), (float)yOffset, 100, 25};
                bool delHover = CheckCollisionPointRec(GetMousePosition(), delBtn);
                DrawRectangle((int)delBtn.x, (int)delBtn.y, (int)delBtn.width, (int)delBtn.height,
                             delHover ? Color{200, 50, 50, 255} : Color{150, 30, 30, 255});
                DrawRectangleLines((int)delBtn.x, (int)delBtn.y, (int)delBtn.width, (int)delBtn.height, WHITE);
                DrawText("Delete", (int)delBtn.x + 20, (int)delBtn.y + 5, 12, WHITE);
                if (delHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    plane.Clear();
                    g_animatedPlanes.erase(g_animatedPlanes.begin() + selectedPlaneIdx);
                    selectedPlaneIdx = -1;
                }
            } else {
                DrawText("Create a plane above, then", panelX + 10, yOffset, 12, LIGHTGRAY);
                yOffset += 15;
                DrawText("drag & drop PNG files to", panelX + 10, yOffset, 12, LIGHTGRAY);
                yOffset += 15;
                DrawText("add animation frames", panelX + 10, yOffset, 12, LIGHTGRAY);
            }

            // Help text at bottom
            yOffset = panelY + panelH - 60;
            DrawLine(panelX + 10, yOffset, panelX + panelW - 10, yOffset, GRAY);
            yOffset += 10;
            DrawText("TIP: Select a plane, then", panelX + 10, yOffset, 10, DARKGRAY);
            yOffset += 12;
            DrawText("drag & drop PNGs to add", panelX + 10, yOffset, 10, DARKGRAY);
            yOffset += 12;
            DrawText("more frames to it!", panelX + 10, yOffset, 10, DARKGRAY);
        }

        // Texture Library UI Panel
        if (showTextureLibraryUI) {
            int texPanelX = 10;
            int texPanelY = 250;
            int texPanelW = 300;
            int texPanelH = 400;

            DrawRectangle(texPanelX, texPanelY, texPanelW, texPanelH, Color{40, 40, 40, 240});
            DrawRectangleLines(texPanelX, texPanelY, texPanelW, texPanelH, SKYBLUE);
            DrawText("TEXTURE LIBRARY", texPanelX + 10, texPanelY + 10, 16, SKYBLUE);
            DrawText("CTRL+L to toggle", texPanelX + 10, texPanelY + 30, 12, GRAY);

            int yOffset = texPanelY + 55;

            DrawText(TextFormat("Textures: %d", (int)g_textureLibrary.textures.size()), texPanelX + 10, yOffset, 14, WHITE);
            yOffset += 25;

            // Texture list with thumbnails
            for (int i = 0; i < (int)g_textureLibrary.textures.size(); i++) {
                Color btnColor = (i == selectedTextureIdx) ? Color{80, 150, 200, 255} : Color{60, 60, 60, 255};
                int itemHeight = 70;
                
                DrawRectangle(texPanelX + 10, yOffset, texPanelW - 20, itemHeight, btnColor);
                DrawRectangleLines(texPanelX + 10, yOffset, texPanelW - 20, itemHeight, WHITE);

                Vector2 mousePos = GetMousePosition();
                if (mousePos.x >= texPanelX + 10 && mousePos.x <= texPanelX + texPanelW - 10 &&
                    mousePos.y >= yOffset && mousePos.y <= yOffset + itemHeight) {
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        selectedTextureIdx = i;
                    }
                }

                // Draw texture thumbnail
                Texture2D tex = g_textureLibrary.textures[i];
                float thumbSize = 60;
                float scale = thumbSize / fmaxf(tex.width, tex.height);
                Rectangle source = {0, 0, (float)tex.width, (float)tex.height};
                Rectangle dest = {(float)(texPanelX + 15), (float)(yOffset + 5), tex.width * scale, tex.height * scale};
                DrawTexturePro(tex, source, dest, {0, 0}, 0.0f, WHITE);

                // Draw texture name (truncated)
                std::string name = g_textureLibrary.textureNames[i];
                if (name.length() > 20) {
                    name = "..." + name.substr(name.length() - 17);
                }
                DrawText(name.c_str(), texPanelX + 80, yOffset + 10, 10, WHITE);
                DrawText(TextFormat("%dx%d", tex.width, tex.height), texPanelX + 80, yOffset + 25, 9, LIGHTGRAY);
                
                yOffset += itemHeight + 5;
            }

            yOffset += 10;

            // Help text
            if (g_textureLibrary.textures.empty()) {
                DrawText("Drag & drop image files", texPanelX + 10, yOffset, 12, LIGHTGRAY);
                yOffset += 15;
                DrawText("to add to library", texPanelX + 10, yOffset, 12, LIGHTGRAY);
            } else {
                DrawText("Hover wall & press T to", texPanelX + 10, yOffset, 11, LIGHTGRAY);
                yOffset += 14;
                DrawText("cycle through textures", texPanelX + 10, yOffset, 11, LIGHTGRAY);
            }
        }

        // Save Dialog
        if (showSaveDialog) {
            int dialogW = 500;
            int dialogH = 200;
            int dialogX = (GetScreenWidth() - dialogW) / 2;
            int dialogY = (GetScreenHeight() - dialogH) / 2;

            DrawRectangle(dialogX, dialogY, dialogW, dialogH, Color{30, 30, 30, 250});
            DrawRectangleLines(dialogX, dialogY, dialogW, dialogH, YELLOW);
            DrawText("SAVE PROJECT", dialogX + 20, dialogY + 20, 20, YELLOW);

            DrawText("Filename (without extension):", dialogX + 20, dialogY + 60, 14, WHITE);
            
            // Text input box
            Rectangle textBox = {(float)(dialogX + 20), (float)(dialogY + 85), (float)(dialogW - 40), 30};
            DrawRectangle((int)textBox.x, (int)textBox.y, (int)textBox.width, (int)textBox.height, Color{50, 50, 50, 255});
            DrawRectangleLines((int)textBox.x, (int)textBox.y, (int)textBox.width, (int)textBox.height, WHITE);
            DrawText(saveFileName, (int)textBox.x + 5, (int)textBox.y + 7, 16, WHITE);

            // Handle text input
            int key = GetCharPressed();
            while (key > 0) {
                if ((key >= 32) && (key <= 125) && saveFileNameCursor < 120) {
                    saveFileName[saveFileNameCursor] = (char)key;
                    saveFileName[saveFileNameCursor + 1] = '\0';
                    saveFileNameCursor++;
                }
                key = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE) && saveFileNameCursor > 0) {
                saveFileNameCursor--;
                saveFileName[saveFileNameCursor] = '\0';
            }

            // Buttons
            Rectangle saveBtn = {(float)(dialogX + 20), (float)(dialogY + 140), 100, 30};
            Rectangle cancelBtn = {(float)(dialogX + 130), (float)(dialogY + 140), 100, 30};

            bool saveHover = CheckCollisionPointRec(GetMousePosition(), saveBtn);
            bool cancelHover = CheckCollisionPointRec(GetMousePosition(), cancelBtn);

            DrawRectangle((int)saveBtn.x, (int)saveBtn.y, (int)saveBtn.width, (int)saveBtn.height,
                         saveHover ? Color{50, 150, 50, 255} : Color{30, 100, 30, 255});
            DrawRectangleLines((int)saveBtn.x, (int)saveBtn.y, (int)saveBtn.width, (int)saveBtn.height, WHITE);
            DrawText("SAVE", (int)saveBtn.x + 25, (int)saveBtn.y + 7, 16, WHITE);

            DrawRectangle((int)cancelBtn.x, (int)cancelBtn.y, (int)cancelBtn.width, (int)cancelBtn.height,
                         cancelHover ? Color{150, 50, 50, 255} : Color{100, 30, 30, 255});
            DrawRectangleLines((int)cancelBtn.x, (int)cancelBtn.y, (int)cancelBtn.width, (int)cancelBtn.height, WHITE);
            DrawText("CANCEL", (int)cancelBtn.x + 15, (int)cancelBtn.y + 7, 16, WHITE);

            if ((saveHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) || IsKeyPressed(KEY_ENTER)) {
                std::string objFile = std::string(saveFileName) + ".obj";
                std::string datFile = std::string(saveFileName) + ".dat";
                
                if (ExportToOBJ(modules, objFile.c_str())) {
                    printf("Model exported to %s\n", objFile.c_str());
                    if (SaveProject(modules, datFile.c_str())) {
                        printf("Project data saved to %s\n", datFile.c_str());
                        escMessage = TextFormat("Saved to %s", saveFileName);
                        escMessageTime = GetTime();
                    }
                }
                showSaveDialog = false;
            }

            if ((cancelHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) || IsKeyPressed(KEY_ESCAPE)) {
                showSaveDialog = false;
            }
        }

        // Load Dialog
        if (showLoadDialog) {
            int dialogW = 500;
            int dialogH = 200;
            int dialogX = (GetScreenWidth() - dialogW) / 2;
            int dialogY = (GetScreenHeight() - dialogH) / 2;

            DrawRectangle(dialogX, dialogY, dialogW, dialogH, Color{30, 30, 30, 250});
            DrawRectangleLines(dialogX, dialogY, dialogW, dialogH, SKYBLUE);
            DrawText("LOAD PROJECT", dialogX + 20, dialogY + 20, 20, SKYBLUE);

            DrawText("Filename (without extension):", dialogX + 20, dialogY + 60, 14, WHITE);
            
            // Text input box
            Rectangle textBox = {(float)(dialogX + 20), (float)(dialogY + 85), (float)(dialogW - 40), 30};
            DrawRectangle((int)textBox.x, (int)textBox.y, (int)textBox.width, (int)textBox.height, Color{50, 50, 50, 255});
            DrawRectangleLines((int)textBox.x, (int)textBox.y, (int)textBox.width, (int)textBox.height, WHITE);
            DrawText(loadFileName, (int)textBox.x + 5, (int)textBox.y + 7, 16, WHITE);

            // Handle text input
            int key = GetCharPressed();
            while (key > 0) {
                if ((key >= 32) && (key <= 125) && loadFileNameCursor < 120) {
                    loadFileName[loadFileNameCursor] = (char)key;
                    loadFileName[loadFileNameCursor + 1] = '\0';
                    loadFileNameCursor++;
                }
                key = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE) && loadFileNameCursor > 0) {
                loadFileNameCursor--;
                loadFileName[loadFileNameCursor] = '\0';
            }

            // Buttons
            Rectangle loadBtn = {(float)(dialogX + 20), (float)(dialogY + 140), 100, 30};
            Rectangle cancelBtn = {(float)(dialogX + 130), (float)(dialogY + 140), 100, 30};

            bool loadHover = CheckCollisionPointRec(GetMousePosition(), loadBtn);
            bool cancelHover = CheckCollisionPointRec(GetMousePosition(), cancelBtn);

            DrawRectangle((int)loadBtn.x, (int)loadBtn.y, (int)loadBtn.width, (int)loadBtn.height,
                         loadHover ? Color{50, 100, 200, 255} : Color{30, 70, 150, 255});
            DrawRectangleLines((int)loadBtn.x, (int)loadBtn.y, (int)loadBtn.width, (int)loadBtn.height, WHITE);
            DrawText("LOAD", (int)loadBtn.x + 25, (int)loadBtn.y + 7, 16, WHITE);

            DrawRectangle((int)cancelBtn.x, (int)cancelBtn.y, (int)cancelBtn.width, (int)cancelBtn.height,
                         cancelHover ? Color{150, 50, 50, 255} : Color{100, 30, 30, 255});
            DrawRectangleLines((int)cancelBtn.x, (int)cancelBtn.y, (int)cancelBtn.width, (int)cancelBtn.height, WHITE);
            DrawText("CANCEL", (int)cancelBtn.x + 15, (int)cancelBtn.y + 7, 16, WHITE);

            if ((loadHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) || IsKeyPressed(KEY_ENTER)) {
                std::string objFile = std::string(loadFileName) + ".obj";
                std::string datFile = std::string(loadFileName) + ".dat";
                
                if (ImportFromOBJ(modules, nextModuleId, objFile.c_str(), camera)) {
                    printf("Model imported from %s\n", objFile.c_str());
                    SaveState(undoHistory, modules, nextModuleId);
                    hoveredNode = hoveredModule = hoveredWall = -1;
                    isDragging = isDraggingModule = false;
                    selectedNodes.clear();
                    activeModule = -1;
                    
                    if (FileExists(datFile.c_str())) {
                        if (LoadProject(modules, datFile.c_str())) {
                            printf("Project data loaded from %s\n", datFile.c_str());
                            escMessage = TextFormat("Loaded %s", loadFileName);
                            escMessageTime = GetTime();
                        }
                    }
                }
                showLoadDialog = false;
            }

            if ((cancelHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) || IsKeyPressed(KEY_ESCAPE)) {
                showLoadDialog = false;
            }
        }

        EndDrawing();
    }

    // Cleanup texture library
    g_textureLibrary.Clear();

    // Cleanup animated planes
    for (auto& plane : g_animatedPlanes) {
        plane.Clear();
    }
    g_animatedPlanes.clear();

    EnableCursor();
    CloseWindow();
    return 0;
}
