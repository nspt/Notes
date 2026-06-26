# Assimp 介绍

3D 建模软件生成的模型文件有很多格式类型，要想支持所有类型的 3D 模型就需要自行解析各类文件，Assimp 库（Open Asset Import Library）提供了一种实现方式，我们可以借助 Assimp 实现对各种 3D 模型文件的加载。Assimp 加载的 3D 模型最终会呈现为以下结构：

![assimp struct](images/assimp_structure.png)

1. 模型/场景所包含的所有数据，都在 *Scene* 对象中，然后 Scene 对象还有一个指向根节点（*Node*）的指针 `mRootNode`。
2. 根节点是访问所有其它 Node 对象的起点，每个 Node 对象都有一个 `mChildren[]` 数组和一个 `mMeshes[]` 数组：
    1. `mChildren[]` 数组包含此 Node 的所有子 Node 对象的指针，子 Node 数量通过 `node->mNumChildren` 获取。
    2. `mMeshes[]` 数组包含此 Node 的所有 *Mesh* 的 **索引**，访问 Mesh 对象需要通过 Scene 对象的 `mMeshes[]` 数组，即 `scene->mMeshes[meshIndex]`。
3. Mesh 对象本身包含渲染所需的所有相关数据，包括顶点位置、法线向量、纹理坐标、面和对象的材质：
    1. `mNumVertices` 为顶点数量，相关数据有：
        1. `mVertices[]` 为顶点坐标数组。
        2. `mNormals[]` 为顶点法线数组。
        3. `mTextureCoords[][]` 为顶点纹理坐标（UV）数组。第一维表示 UV 通道（*Texture Coordinate Channel*），用于存储多套 UV；第二维表示顶点编号。可通过 mesh->HasTextureCoords(channel) 判断某个 UV 通道是否存在。
    2. `mNumFaces` 表示表面（*Face*）数量，相关数据：
        1. `mFaces[]` 为 Face 数组，Face 对象包含构成图元的顶点索引（如三个索引组成三角形图元）通过：
            1. `mNumIndices` 确定一个 Face 对象包含的索引数量
            2. `mIndices[i]` 获取 Face 对象的顶点索引。
    3. `mMaterialIndex` 表示 Mesh 的材质（*Material*）对象的索引，且仅当索引位于 $[0, scene->mNumMaterials)$ 范围时表示 Mesh 有材质。访问 Material 对象需要通过 Scene 对象的 `mMaterials[]` 数组，即 `scene->mMaterials[materialIndex]`。Material 对象包含各类纹理：
        1. 通过 `material->GetTextureCount(type)` 判断某类纹理的数量，类型有`aiTextureType_DIFFUSE`、`aiTextureType_SPECULAR` 等。
        2. 通过 `aiString str;material->GetTexture(type, i, &str);` 获取材质中记录的第 i 个 type 类型纹理的引用（通常是相对路径，也可能是绝对路径或嵌入纹理标识）。

# 示例代码

加载 Scene：
```cpp
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(
        path.c_str(),
        aiProcess_Triangulate | aiProcess_FlipUVs
    );
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        throw std::runtime_error{ "ERROR::ASSIMP::"s + importer.GetErrorString() };
    }
```

`aiProcess_Triangulate` 表示将模型中所有不是三角形的面（Polygon）转换为三角形（Triangle）。很多模型格式（例如 FBX、OBJ、Collada）允许一个面拥有任意数量的顶点，例如四边形、五边形等。

`aiProcess_FlipUVs` 表示将所有纹理坐标（UV）的 V（或 T）分量进行翻转，即 $v = 1.0 - v$。当模型的 UV 原点与渲染时采用的纹理坐标约定不一致时，可以通过该选项进行修正。OpenGL 的纹理坐标 (0, 0) 位于左下角，而很多建模软件（以及 DirectX 生态）纹理坐标 (0, 0) 位于左上角。UV 本身没有统一标准。

处理节点：
```cpp
void Model::processNode(
    const aiNode *node,
    const aiScene *scene,
    const std::string_view &dir)
{
    for(unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]]; 
        objects_.push_back(processMesh(mesh, scene, dir));			
    }
    for(unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, dir);
    }
}
```

处理网格和材质：

```cpp
std::shared_ptr<RenderObject> Model::processMesh(
    const aiMesh *mesh,
    const aiScene *scene,
    const std::string_view &dir)
{
    auto obj = std::make_shared<RenderObject>();

    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;

    // 当前实现要求存在 UV 通道 0
    if (!mesh->HasTextureCoords(0))
        throw std::runtime_error{ "Mesh has no texture 0" };
    
    // 处理顶点
    for(unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        vertex.position.x = mesh->mVertices[i].x;
        vertex.position.y = mesh->mVertices[i].y;
        vertex.position.z = mesh->mVertices[i].z;
        vertex.normal.x = mesh->mNormals[i].x;
        vertex.normal.y = mesh->mNormals[i].y;
        vertex.normal.z = mesh->mNormals[i].z;
        vertex.texCoord.x = mesh->mTextureCoords[0][i].x; 
        vertex.texCoord.y = mesh->mTextureCoords[0][i].y;
        vertices.push_back(vertex);
    }
    
    // 处理索引/Face
    for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for(unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    // 生成自定义 Mesh
    std::vector<VertexAttrib> attributes {
        VertexAttrib {
            0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, position))
        },
        VertexAttrib {
            1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, normal))
        },
        VertexAttrib {
            2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, texCoord))
        }
    };
    obj->mesh_ = std::make_shared<Mesh>(vertices, attributes, indices);

    // 处理材质
    obj->material_ = std::make_shared<Material>();
    obj->material_->shininess_ = 32.0f;
    if(mesh->mMaterialIndex < scene->mNumMaterials) {
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
        obj->material_->diffuse_textures_ = loadTextureFrom(material, scene, aiTextureType_DIFFUSE, dir);
        obj->material_->specular_textures_ = loadTextureFrom(material, scene, aiTextureType_SPECULAR, dir);
        material->Get(AI_MATKEY_SHININESS, obj->material_->shininess_);
    }

    return obj;
}
```