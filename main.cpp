#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <cmath>
#include <cstdio>
#include <cfloat>
#include <algorithm>
#include <deque>
#include <fstream>
#include <string>
#include <ctime>

struct Node {
    Vector3 position;
    std::vector<int> connections;
};

struct Wall {
    std::vector<int> nodeIndices; // Can be 3, 4, or more nodes
    Texture2D texture; // Texture for this wall
    bool hasTexture; // Whether this wall has a texture assigned
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

void ConnectNodeToNearby(GridModule& module, int newNodeIndex, float connectionDistance) {
    if (newNodeIndex < 0 || newNodeIndex >= (int)module.nodes.size()) return;
    
    Node& newNode = module.nodes[newNodeIndex];
    
    // Check all other nodes in the same module
    for (size_t i = 0; i < module.nodes.size(); i++) {
        if ((int)i == newNodeIndex) continue;
        
        float dist = Vector3Distance(newNode.position, module.nodes[i].position);
        
        if (dist <= connectionDistance) {
            // Check if connection already exists
            bool alreadyConnected = false;
            for (int conn : newNode.connections) {
                if (conn == (int)i) {
                    alreadyConnected = true;
                    break;
                }
            }
            
            if (!alreadyConnected) {
                // Add bidirectional connection
                newNode.connections.push_back((int)i);
                module.nodes[i].connections.push_back(newNodeIndex);
            }
        }
    }
}

void ConnectNodeToNearbyAcrossModules(std::vector<GridModule>& modules, int targetModuleIndex, int newNodeIndex, float connectionDistance) {
    if (targetModuleIndex < 0 || targetModuleIndex >= (int)modules.size()) return;
    if (newNodeIndex < 0 || newNodeIndex >= (int)modules[targetModuleIndex].nodes.size()) return;
    
    Node& newNode = modules[targetModuleIndex].nodes[newNodeIndex];
    
    // Check all nodes in ALL modules (including the same module)
    for (size_t m = 0; m < modules.size(); m++) {
        for (size_t i = 0; i < modules[m].nodes.size(); i++) {
            // Skip the new node itself
            if ((int)m == targetModuleIndex && (int)i == newNodeIndex) continue;
            
            float dist = Vector3Distance(newNode.position, modules[m].nodes[i].position);
            
            if (dist <= connectionDistance) {
                // Check if connection already exists
                bool alreadyConnected = false;
                for (int conn : newNode.connections) {
                    // For cross-module connections, we store them differently
                    // We'll use a simple approach: store module index * 10000 + node index
                    // But actually, let's keep it simple - only connect within same module for now
                    // and use a global connection system
                    if ((int)m == targetModuleIndex && conn == (int)i) {
                        alreadyConnected = true;
                        break;
                    }
                }
                
                // For now, only connect within the same module to avoid index issues
                // But we can extend this later if needed
                if (!alreadyConnected && (int)m == targetModuleIndex) {
                    // Add bidirectional connection within same module
                    newNode.connections.push_back((int)i);
                    modules[m].nodes[i].connections.push_back(newNodeIndex);
                }
            }
        }
    }
}

int GetWallUnderMouse(const GridModule& module, const Camera3D& camera) {
    Ray ray = GetMouseRay(GetMousePosition(), camera);
    int closestWall = -1;
    float closestDist = FLT_MAX;
    
    for (size_t w = 0; w < module.walls.size(); w++) {
        const Wall& wall = module.walls[w];
        
        // Triangulate the polygon and check each triangle
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
    if (indices.size() == 3) return true; // Triangles are always coplanar
    
    // For 4+ points, check if they're on the same plane
    Vector3 p1 = nodes[indices[0]].position;
    Vector3 p2 = nodes[indices[1]].position;
    Vector3 p3 = nodes[indices[2]].position;
    
    Vector3 v1 = Vector3Subtract(p2, p1);
    Vector3 v2 = Vector3Subtract(p3, p1);
    Vector3 normal = Vector3Normalize(Vector3CrossProduct(v1, v2));
    
    // Check if all other points lie on the same plane
    for (size_t i = 3; i < indices.size(); i++) {
        Vector3 v3 = Vector3Subtract(nodes[indices[i]].position, p1);
        float dot = fabs(Vector3DotProduct(normal, v3));
        if (dot > 1.0f) return false; // Not coplanar
    }
    
    return true;
}

bool FormValidPolygon(const std::vector<Node>& nodes, const std::vector<int>& indices) {
    // Any number of points can form a polygon as long as they're coplanar
    return indices.size() >= 3;
}

void CreateWallFromSelectedNodes(GridModule& module, const std::vector<int>& selected) {
    if (selected.size() < 3) return; // Need at least 3 nodes for a triangle
    if (!AreNodesCoplanar(module.nodes, selected)) return;
    
    // Check if wall with these exact nodes already exists
    for (const auto& wall : module.walls) {
        std::vector<int> wallNodes = wall.nodeIndices;
        std::sort(wallNodes.begin(), wallNodes.end());
        std::vector<int> sortedSelected = selected;
        std::sort(sortedSelected.begin(), sortedSelected.end());
        
        if (wallNodes == sortedSelected) {
            return; // Wall already exists
        }
    }
    
    Wall newWall;
    newWall.nodeIndices = selected;
    newWall.hasTexture = false;
    newWall.texture = {}; // Initialize empty texture
    module.walls.push_back(newWall);
}

void DeleteNode(GridModule& module, int nodeIdx) {
    if (nodeIdx < 0 || nodeIdx >= (int)module.nodes.size()) return;
    
    for (auto& node : module.nodes) {
        node.connections.erase(std::remove(node.connections.begin(), node.connections.end(), nodeIdx), node.connections.end());
        for (auto& conn : node.connections) {
            if (conn > nodeIdx) conn--;
        }
    }
    
    // Remove walls that contain this node
    module.walls.erase(std::remove_if(module.walls.begin(), module.walls.end(),
        [nodeIdx](const Wall& w) {
            return std::find(w.nodeIndices.begin(), w.nodeIndices.end(), nodeIdx) != w.nodeIndices.end();
        }), module.walls.end());
    
    // Update node indices in remaining walls
    for (auto& wall : module.walls) {
        for (auto& idx : wall.nodeIndices) {
            if (idx > nodeIdx) idx--;
        }
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

// Function to draw a wall with optional texture
void DrawWall(const Wall& wall, const std::vector<Node>& nodes, Color defaultColor, bool useTexture = false) {
    if (wall.nodeIndices.size() < 3) return;
    
    if (useTexture && wall.hasTexture) {
        // Create mesh for textured rendering
        std::vector<Vector3> vertices;
        for (int idx : wall.nodeIndices) {
            if (idx >= 0 && idx < (int)nodes.size()) {
                vertices.push_back(nodes[idx].position);
            }
        }
        
        if (vertices.size() >= 3) {
            // Create mesh from vertices
            Mesh mesh = {0};
            int triangleCount = (int)vertices.size() - 2;
            int vertexCount = triangleCount * 3;
            
            mesh.triangleCount = triangleCount;
            mesh.vertexCount = vertexCount;
            
            mesh.vertices = (float*)MemAlloc(vertexCount * 3 * sizeof(float));
            mesh.texcoords = (float*)MemAlloc(vertexCount * 2 * sizeof(float));
            mesh.normals = (float*)MemAlloc(vertexCount * 3 * sizeof(float));
            
            // Calculate normal
            Vector3 v1 = vertices[0];
            Vector3 v2 = vertices[1];
            Vector3 v3 = vertices[2];
            Vector3 normal = Vector3Normalize(Vector3CrossProduct(
                Vector3Subtract(v2, v1),
                Vector3Subtract(v3, v1)
            ));
            
            // Calculate bounding box for UV mapping
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
            // Create triangle fan
            for (size_t i = 1; i < vertices.size() - 1; i++) {
                Vector3 p1 = vertices[0];
                Vector3 p2 = vertices[i];
                Vector3 p3 = vertices[i + 1];
                
                // Vertices
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
            
            // UV coordinates
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
            
            // Normals
            for (int i = 0; i < vertexCount; i++) {
                mesh.normals[i * 3 + 0] = normal.x;
                mesh.normals[i * 3 + 1] = normal.y;
                mesh.normals[i * 3 + 2] = normal.z;
            }
            
            // Create back mesh with reversed winding and flipped normals BEFORE uploading
            Mesh backMesh = {0};
            backMesh.triangleCount = triangleCount;
            backMesh.vertexCount = vertexCount;
            
            backMesh.vertices = (float*)MemAlloc(vertexCount * 3 * sizeof(float));
            backMesh.texcoords = (float*)MemAlloc(vertexCount * 2 * sizeof(float));
            backMesh.normals = (float*)MemAlloc(vertexCount * 3 * sizeof(float));
            
            // Copy and reverse vertex order (swap v1 and v3 for each triangle)
            for (int i = 0; i < vertexCount; i += 3) {
                // Triangle vertices: copy in reverse order
                backMesh.vertices[(i + 0) * 3 + 0] = mesh.vertices[(i + 2) * 3 + 0];
                backMesh.vertices[(i + 0) * 3 + 1] = mesh.vertices[(i + 2) * 3 + 1];
                backMesh.vertices[(i + 0) * 3 + 2] = mesh.vertices[(i + 2) * 3 + 2];
                
                backMesh.vertices[(i + 1) * 3 + 0] = mesh.vertices[(i + 1) * 3 + 0];
                backMesh.vertices[(i + 1) * 3 + 1] = mesh.vertices[(i + 1) * 3 + 1];
                backMesh.vertices[(i + 1) * 3 + 2] = mesh.vertices[(i + 1) * 3 + 2];
                
                backMesh.vertices[(i + 2) * 3 + 0] = mesh.vertices[(i + 0) * 3 + 0];
                backMesh.vertices[(i + 2) * 3 + 1] = mesh.vertices[(i + 0) * 3 + 1];
                backMesh.vertices[(i + 2) * 3 + 2] = mesh.vertices[(i + 0) * 3 + 2];
                
                // Copy UVs in same reverse order
                backMesh.texcoords[(i + 0) * 2 + 0] = mesh.texcoords[(i + 2) * 2 + 0];
                backMesh.texcoords[(i + 0) * 2 + 1] = mesh.texcoords[(i + 2) * 2 + 1];
                
                backMesh.texcoords[(i + 1) * 2 + 0] = mesh.texcoords[(i + 1) * 2 + 0];
                backMesh.texcoords[(i + 1) * 2 + 1] = mesh.texcoords[(i + 1) * 2 + 1];
                
                backMesh.texcoords[(i + 2) * 2 + 0] = mesh.texcoords[(i + 0) * 2 + 0];
                backMesh.texcoords[(i + 2) * 2 + 1] = mesh.texcoords[(i + 0) * 2 + 1];
                
                // Flip normals
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
            
            // Create material with texture
            Material mat = LoadMaterialDefault();
            mat.maps[MATERIAL_MAP_DIFFUSE].texture = wall.texture;
            mat.maps[MATERIAL_MAP_DIFFUSE].color = WHITE; // Ensure full opacity
            
            // Draw both meshes
            DrawMesh(mesh, mat, MatrixIdentity());
            DrawMesh(backMesh, mat, MatrixIdentity());
            
            // Clean up
            UnloadMesh(mesh);
            UnloadMesh(backMesh);
        }
    } else {
        // Draw without texture (original method)
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
    
    // Write header
    file << "# OBJ file exported from GreyScaleCube\n";
    file << "# Generated model\n\n";
    
    // Count total vertices first to get proper indices
    int vertexOffset = 1; // OBJ indices start at 1
    
    // Export all vertices (nodes/spheres)
    for (size_t m = 0; m < modules.size(); m++) {
        file << "# Module " << modules[m].id << "\n";
        for (const auto& node : modules[m].nodes) {
            file << "v " << node.position.x << " " << node.position.y << " " << node.position.z << "\n";
        }
    }
    
    file << "\n# Connections (lines)\n";
    
    // Export connections as lines
    vertexOffset = 1;
    for (size_t m = 0; m < modules.size(); m++) {
        for (size_t i = 0; i < modules[m].nodes.size(); i++) {
            for (int conn : modules[m].nodes[i].connections) {
                // Only export each connection once (when from < to)
                if ((int)i < conn) {
                    int v1 = vertexOffset + (int)i;
                    int v2 = vertexOffset + conn;
                    file << "l " << v1 << " " << v2 << "\n";
                }
            }
        }
        vertexOffset += (int)modules[m].nodes.size();
    }
    
    file << "\n# Walls (faces)\n";
    
    // Export walls as faces
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

int main() {
    InitWindow(1200, 900, "3D Grid Modules - Mode-Based Movement");
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
    
    // Mode system
    enum Mode { MODE_SELECT, MODE_MOVE_VERTEX, MODE_MOVE_MODULE, MODE_ADD_NODE, MODE_CONNECT };
    Mode currentMode = MODE_SELECT;
    
    // Select & Fill mode variables
    std::vector<int> selectedNodes;
    int selectedModule = -1;
    int activeModule = -1;
    
    // Add node mode variables
    Vector3 previewNodePosition = {0.0f, 0.0f, 0.0f};
    bool showPreviewNode = false;
    float addNodeDistance = 15.0f;
    
    // Connect mode variables
    int connectStartNode = -1;
    int connectStartModule = -1;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_TAB)) {
            cursorEnabled = !cursorEnabled;
            cursorEnabled ? EnableCursor() : DisableCursor();
        }
        
        if (IsKeyPressed(KEY_G)) showGrid = !showGrid;
        if (IsKeyPressed(KEY_C)) showConnections = !showConnections;
        
        // Mode switching
        if (cursorEnabled && IsKeyPressed(KEY_ONE)) {
            currentMode = MODE_SELECT;
            isDragging = isDraggingModule = false;
            selectedNodes.clear();
            selectedModule = -1;
            showPreviewNode = false;
            connectStartNode = connectStartModule = -1;
        }
        if (cursorEnabled && IsKeyPressed(KEY_TWO)) {
            currentMode = MODE_MOVE_VERTEX;
            isDragging = isDraggingModule = false;
            selectedNodes.clear();
            selectedModule = -1;
            showPreviewNode = false;
            connectStartNode = connectStartModule = -1;
        }
        if (cursorEnabled && IsKeyPressed(KEY_THREE)) {
            currentMode = MODE_MOVE_MODULE;
            isDragging = isDraggingModule = false;
            selectedNodes.clear();
            selectedModule = -1;
            showPreviewNode = false;
            connectStartNode = connectStartModule = -1;
        }
        if (cursorEnabled && IsKeyPressed(KEY_FOUR)) {
            currentMode = MODE_ADD_NODE;
            isDragging = isDraggingModule = false;
            selectedNodes.clear();
            selectedModule = -1;
            showPreviewNode = true;
            connectStartNode = connectStartModule = -1;
        }
        if (cursorEnabled && IsKeyPressed(KEY_FIVE)) {
            currentMode = MODE_CONNECT;
            isDragging = isDraggingModule = false;
            selectedNodes.clear();
            selectedModule = -1;
            showPreviewNode = false;
            connectStartNode = connectStartModule = -1;
        }
        
        if (currentMode == MODE_SELECT && IsKeyPressed(KEY_ESCAPE)) {
            selectedNodes.clear();
            selectedModule = -1;
        }
        
        if (currentMode == MODE_SELECT && IsKeyPressed(KEY_SPACE) && selectedNodes.size() >= 3 && selectedModule != -1) {
            CreateWallFromSelectedNodes(modules[selectedModule], selectedNodes);
            SaveState(undoHistory, modules, nextModuleId);
            selectedNodes.clear();
            selectedModule = -1;
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
        
        // Export to OBJ file (Ctrl+S or F5)
        if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_S)) {
            const char* filename = "model.obj";
            if (ExportToOBJ(modules, filename)) {
                // Show success message (you could add a message system here)
                printf("Model exported to %s\n", filename);
            } else {
                printf("Failed to export model to %s\n", filename);
            }
        }
        
        if (IsKeyPressed(KEY_F5)) {
            // Alternative: F5 to save
            const char* filename = "model.obj";
            if (ExportToOBJ(modules, filename)) {
                printf("Model exported to %s\n", filename);
            } else {
                printf("Failed to export model to %s\n", filename);
            }
        }
        
        if (((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_Z)) || IsKeyPressed(KEY_BACKSPACE)) {
            if (RestoreState(undoHistory, modules, nextModuleId)) {
                hoveredNode = hoveredModule = hoveredWall = -1;
                isDragging = isDraggingModule = false;
                selectedNodes.clear();
                selectedModule = -1;
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
            
            if (moved) {
                for (auto& node : modules[activeModule].nodes) {
                    node.position = Vector3Add(node.position, movement);
                }
                modules[activeModule].center = Vector3Add(modules[activeModule].center, movement);
                SaveState(undoHistory, modules, nextModuleId);
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
            // Camera rotation for all modes
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
            // Always update hover detection for all modes (even during camera rotation)
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
            
            // Update preview position for add node mode
            if (currentMode == MODE_ADD_NODE) {
                previewNodePosition = GetMouseWorldPosition(camera, addNodeDistance);
                
                // Adjust distance with mouse wheel
                float wheel = GetMouseWheelMove();
                if (wheel != 0) {
                    addNodeDistance += wheel * 2.0f;
                    if (addNodeDistance < 5.0f) addNodeDistance = 5.0f;
                    if (addNodeDistance > 50.0f) addNodeDistance = 50.0f;
                }
            }
            
            if (IsKeyPressed(KEY_DELETE)) {
                bool changed = false;
                if (hoveredWall != -1 && hoveredModule != -1) {
                    modules[hoveredModule].walls.erase(modules[hoveredModule].walls.begin() + hoveredWall);
                    hoveredWall = -1; changed = true;
                } else if (hoveredNode != -1 && hoveredModule != -1) {
                    DeleteNode(modules[hoveredModule], hoveredNode);
                    hoveredNode = -1; changed = true;
                } else if (hoveredModule != -1 && modules.size() > 1) {
                    modules.erase(modules.begin() + hoveredModule);
                    hoveredModule = -1; changed = true;
                }
                if (changed) SaveState(undoHistory, modules, nextModuleId);
            }
            
            if (IsKeyPressed(KEY_DELETE)) {
                bool changed = false;
                if (hoveredWall != -1 && hoveredModule != -1) {
                    // Unload texture if it exists
                    if (modules[hoveredModule].walls[hoveredWall].hasTexture) {
                        UnloadTexture(modules[hoveredModule].walls[hoveredWall].texture);
                    }
                    modules[hoveredModule].walls.erase(modules[hoveredModule].walls.begin() + hoveredWall);
                    hoveredWall = -1; changed = true;
                } else if (hoveredNode != -1 && hoveredModule != -1) {
                    DeleteNode(modules[hoveredModule], hoveredNode);
                    hoveredNode = -1; changed = true;
                } else if (hoveredModule != -1 && modules.size() > 1) {
                    // Unload all textures in module before deleting
                    for (auto& wall : modules[hoveredModule].walls) {
                        if (wall.hasTexture) {
                            UnloadTexture(wall.texture);
                        }
                    }
                    modules.erase(modules.begin() + hoveredModule);
                    hoveredModule = -1; changed = true;
                }
                if (changed) SaveState(undoHistory, modules, nextModuleId);
            }
            
            // Load texture on wall (T key) - works when hovering over a wall
            if (IsKeyPressed(KEY_T)) {
                if (hoveredWall != -1 && hoveredModule != -1) {
                    printf("Attempting to load texture on wall %d in module %d\n", hoveredWall, hoveredModule);
                    
                    // Try to load texture from file - check common names
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
                        // Create a simple colored texture as fallback
                        Image img = GenImageColor(256, 256, BLUE);
                        Texture2D tex = LoadTextureFromImage(img);
                        UnloadImage(img);
                        
                        if (modules[hoveredModule].walls[hoveredWall].hasTexture) {
                            UnloadTexture(modules[hoveredModule].walls[hoveredWall].texture);
                        }
                        modules[hoveredModule].walls[hoveredWall].texture = tex;
                        modules[hoveredModule].walls[hoveredWall].hasTexture = true;
                        printf("Created default blue texture for wall (place texture.png in directory)\n");
                    }
                } else {
                    printf("T key pressed but no wall hovered! Hover over a wall first.\n");
                    printf("  hoveredWall=%d, hoveredModule=%d\n", hoveredWall, hoveredModule);
                    printf("  Make sure cursor is enabled (press TAB) and hover over a wall (it should turn red)\n");
                }
            }
            
            // MODE: SELECT - Click to select nodes for wall creation
            if (currentMode == MODE_SELECT) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hoveredNode != -1 && hoveredModule != -1) {
                    if (selectedModule == -1) {
                        selectedModule = hoveredModule;
                    }
                    
                    if (selectedModule == hoveredModule) {
                        auto it = std::find(selectedNodes.begin(), selectedNodes.end(), hoveredNode);
                        if (it != selectedNodes.end()) {
                            selectedNodes.erase(it);
                        } else {
                            selectedNodes.push_back(hoveredNode); // No limit on selection
                        }
                    }
                }
                
                // Click module to activate for arrow keys
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hoveredModule != -1 && hoveredNode == -1) {
                    activeModule = hoveredModule;
                }
            }
            
            // MODE: MOVE_VERTEX - Drag vertices
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
            
            // MODE: MOVE_MODULE - Drag entire modules
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
            
            // MODE: ADD_NODE - Click to add new nodes (manual connection only)
            if (currentMode == MODE_ADD_NODE) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    // Distance threshold for adding to existing module vs creating new one
                    float moduleAssignmentDistance = 15.0f;
                    int newNodeIndex = -1;
                    int targetModule = -1;
                    
                    // First, check if the new position is close to any existing module
                    // If so, add to that module instead of creating a new one
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
                        // Add to the closest module instead of creating a new one
                        hoveredModule = closestModule;
                    }
                    
                    if (hoveredModule != -1) {
                        // Add node to existing module
                        Node newNode;
                        newNode.position = previewNodePosition;
                        modules[hoveredModule].nodes.push_back(newNode);
                        newNodeIndex = (int)modules[hoveredModule].nodes.size() - 1;
                        targetModule = hoveredModule;
                        activeModule = hoveredModule;
                    } else {
                        // Create new module with single node
                        GridModule newModule;
                        Node newNode;
                        newNode.position = previewNodePosition;
                        newModule.nodes.push_back(newNode);
                        newModule.center = previewNodePosition;
                        newModule.id = nextModuleId++;
                        modules.push_back(newModule);
                        newNodeIndex = 0;
                        targetModule = (int)modules.size() - 1;
                        activeModule = targetModule;
                    }
                    
                    // No automatic connection - user must manually connect using MODE_CONNECT
                    
                    SaveState(undoHistory, modules, nextModuleId);
                }
            }
            
            // MODE: CONNECT - Click two nodes to connect them
            if (currentMode == MODE_CONNECT) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hoveredNode != -1 && hoveredModule != -1) {
                    if (connectStartNode == -1) {
                        // First node selected
                        connectStartNode = hoveredNode;
                        connectStartModule = hoveredModule;
                    } else {
                        // Second node selected - create connection
                        if (connectStartModule == hoveredModule && 
                            !(connectStartNode == hoveredNode && connectStartModule == hoveredModule)) {
                            // Connection within same module (and not the same node)
                            Node& node1 = modules[connectStartModule].nodes[connectStartNode];
                            Node& node2 = modules[hoveredModule].nodes[hoveredNode];
                            
                            // Add bidirectional connection if it doesn't exist
                            bool alreadyConnected = false;
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
                        }
                        // Reset selection after attempting connection
                        connectStartNode = -1;
                        connectStartModule = -1;
                    }
                }
                
                // ESC to cancel connection (removed RMB since it's used for camera)
                if (IsKeyPressed(KEY_ESCAPE)) {
                    connectStartNode = -1;
                    connectStartModule = -1;
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
                
                // Draw wall with texture if available, otherwise use default color
                DrawWall(wall, modules[m].nodes, wc, true);
            }
            
            if (showConnections) {
                for (size_t i = 0; i < modules[m].nodes.size(); i++) {
                    for (int conn : modules[m].nodes[i].connections) {
                        if ((int)i < conn) {
                            DrawLine3D(modules[m].nodes[i].position, modules[m].nodes[conn].position, Color{32,32,32,255});
                        }
                    }
                }
            }
            
            for (size_t i = 0; i < modules[m].nodes.size(); i++) {
                Color nc = DARKPURPLE;
                
                if (cursorEnabled) {
                    if (currentMode == MODE_SELECT && selectedModule == (int)m && 
                        std::find(selectedNodes.begin(), selectedNodes.end(), (int)i) != selectedNodes.end()) {
                        nc = YELLOW;
                    } else if (currentMode == MODE_CONNECT && connectStartNode == (int)i && connectStartModule == (int)m) {
                        nc = LIME; // First selected node for connection
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
        
        // Draw preview node in add mode
        if (showPreviewNode && currentMode == MODE_ADD_NODE) {
            DrawSphere(previewNodePosition, sphereRadius * 1.2f, Color{255, 255, 0, 150});
            DrawSphereWires(previewNodePosition, sphereRadius * 1.2f, 8, 8, YELLOW);
        }
        
        // Draw connection line preview in connect mode
        if (currentMode == MODE_CONNECT && connectStartNode != -1 && connectStartModule != -1) {
            Vector3 startPos = modules[connectStartModule].nodes[connectStartNode].position;
            if (hoveredNode != -1 && hoveredModule != -1 && hoveredModule == connectStartModule) {
                Vector3 endPos = modules[hoveredModule].nodes[hoveredNode].position;
                DrawLine3D(startPos, endPos, LIME);
                DrawSphere(endPos, sphereRadius * 0.5f, LIME);
            } else {
                // Draw line to mouse cursor
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

        int tw = 0; for (const auto& mod : modules) tw += mod.walls.size();
        DrawText(TextFormat("Modules: %zu | Walls: %d | FPS: %d | Active: %d", modules.size(), tw, GetFPS(), activeModule), 10, 10, 18, YELLOW);
        
        const char* modeName = "";
        Color modeColor = WHITE;
        if (currentMode == MODE_SELECT) {
            modeName = "SELECT MODE";
            modeColor = GREEN;
            DrawText(TextFormat("Selected: %zu nodes | SPACE: Fill (min 3) | ESC: Clear", selectedNodes.size()), 10, 35, 16, modeColor);
        } else if (currentMode == MODE_MOVE_VERTEX) {
            modeName = "MOVE VERTEX MODE";
            modeColor = RED;
            DrawText("LMB: Drag vertex", 10, 35, 16, modeColor);
        } else if (currentMode == MODE_MOVE_MODULE) {
            modeName = "MOVE MODULE MODE";
            modeColor = BLUE;
            DrawText("LMB: Drag entire module", 10, 35, 16, modeColor);
        } else if (currentMode == MODE_ADD_NODE) {
            modeName = "ADD NODE MODE";
            modeColor = YELLOW;
            DrawText(TextFormat("LMB: Add node (no auto-connect) | Mouse Wheel: Distance (%.1f)", addNodeDistance), 10, 35, 16, modeColor);
        } else if (currentMode == MODE_CONNECT) {
            modeName = "CONNECT MODE";
            modeColor = LIME;
            if (connectStartNode == -1) {
                DrawText("Click first node to start connection", 10, 35, 16, modeColor);
            } else {
                DrawText("Click second node (same module) to connect | ESC: Cancel", 10, 35, 16, modeColor);
            }
        }
        
        DrawText(TextFormat("Mode: %s", modeName), 10, 60, 18, modeColor);
        DrawText("1:Select | 2:Move Vertex | 3:Move Module | 4:Add Node | 5:Connect", 10, 85, 14, LIGHTGRAY);
        DrawText("RMB: Rotate Camera | ARROWS: Move active | G: Grid | C: Connections", 10, 110, 14, LIGHTGRAY);
        DrawText("TAB: FPS Camera | N: Add module | CTRL+Z: Undo | DEL: Delete", 10, 135, 14, DARKGRAY);
        DrawText("CTRL+S or F5: Export to OBJ (model.obj)", 10, 160, 14, DARKGRAY);
        DrawText("T: Load texture on hovered wall (needs texture.png in directory)", 10, 185, 14, DARKGRAY);
        DrawText("CTRL+S or F5: Export to OBJ (model.obj)", 10, 160, 14, DARKGRAY);
        DrawText("T: Load texture on hovered wall (needs texture.png in directory)", 10, 185, 14, DARKGRAY);
        
        EndDrawing();
    }

    EnableCursor();
    CloseWindow();
    return 0;
}