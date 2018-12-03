// This is a common code snippet
// should be included in other source
// other than compile it independently
#include <sstream>
#include <algorithm>
#include <functional>
using namespace std;

void OpenGLGraphicsManagerCommonBase::Present()
{
    glFlush();
}

bool OpenGLGraphicsManagerCommonBase::setShaderParameter(const char* paramName, const Matrix4X4f& param)
{
    unsigned int location;

    location = glGetUniformLocation(m_CurrentShader, paramName);
    if(location == -1)
    {
            return false;
    }
    glUniformMatrix4fv(location, 1, false, param);

    return true;
}

bool OpenGLGraphicsManagerCommonBase::setShaderParameter(const char* paramName, const Matrix4X4f* param, const int32_t count)
{
    bool result = true;
    char uniformName[256];

    for (int32_t i = 0; i < count; i++)
    {
        sprintf(uniformName, "%s[%d]", paramName, i);
        result &= setShaderParameter(uniformName, *(param + i));
    }

    return result;
}

bool OpenGLGraphicsManagerCommonBase::setShaderParameter(const char* paramName, const Vector2f& param)
{
    unsigned int location;

    location = glGetUniformLocation(m_CurrentShader, paramName);
    if(location == -1)
    {
            return false;
    }
    glUniform2fv(location, 1, param);

    return true;
}

bool OpenGLGraphicsManagerCommonBase::setShaderParameter(const char* paramName, const Vector3f& param)
{
    unsigned int location;

    location = glGetUniformLocation(m_CurrentShader, paramName);
    if(location == -1)
    {
            return false;
    }
    glUniform3fv(location, 1, param);

    return true;
}

bool OpenGLGraphicsManagerCommonBase::setShaderParameter(const char* paramName, const Vector4f& param)
{
    unsigned int location;

    location = glGetUniformLocation(m_CurrentShader, paramName);
    if(location == -1)
    {
            return false;
    }
    glUniform4fv(location, 1, param);

    return true;
}

bool OpenGLGraphicsManagerCommonBase::setShaderParameter(const char* paramName, const float param)
{
    unsigned int location;

    location = glGetUniformLocation(m_CurrentShader, paramName);
    if(location == -1)
    {
            return false;
    }
    glUniform1f(location, param);

    return true;
}

bool OpenGLGraphicsManagerCommonBase::setShaderParameter(const char* paramName, const int32_t param)
{
    unsigned int location;

    location = glGetUniformLocation(m_CurrentShader, paramName);
    if(location == -1)
    {
            return false;
    }
    glUniform1i(location, param);

    return true;
}

bool OpenGLGraphicsManagerCommonBase::setShaderParameter(const char* paramName, const uint32_t param)
{
    unsigned int location;

    location = glGetUniformLocation(m_CurrentShader, paramName);
    if(location == -1)
    {
            return false;
    }
    glUniform1ui(location, param);

    return true;
}

bool OpenGLGraphicsManagerCommonBase::setShaderParameter(const char* paramName, const bool param)
{
    unsigned int location;

    location = glGetUniformLocation(m_CurrentShader, paramName);
    if(location == -1)
    {
            return false;
    }
    glUniform1f(location, param);

    return true;
}

static void getOpenGLTextureFormat(const Image& img, GLenum& format, GLenum& internal_format, GLenum& type)
{
    if(img.compressed)
    {
        format = GL_COMPRESSED_RGB;

        switch (img.compress_format)
        {
            case "DXT1"_u32:
                internal_format = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
                break;
            case "DXT3"_u32:
                internal_format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
                break;
            case "DXT5"_u32:
                internal_format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                break;
            default:
                assert(0);
        }

        type = GL_UNSIGNED_BYTE;
    }
    else
    {
        if(img.bitcount == 8)
        {
            format = GL_RED;
            internal_format = GL_R8;
            type = GL_UNSIGNED_BYTE;
        }
        else if(img.bitcount == 16)
        {
            format = GL_RED;
    #ifndef OPENGL_ES
            internal_format = GL_R16;
    #else
            internal_format = GL_RED;
    #endif
            type = GL_UNSIGNED_SHORT;
        }
        else if(img.bitcount == 24)
        {
            format = GL_RGB;
            internal_format = GL_RGB8;
            type = GL_UNSIGNED_BYTE;
        }
        else if(img.bitcount == 64)
        {
            format = GL_RGBA;
            if (img.is_float)
            {
                internal_format = GL_RGBA16F;
                type = GL_HALF_FLOAT;
            }
            else
            {
                internal_format = GL_RGBA16;
                type = GL_UNSIGNED_SHORT;
            }
        }
        else if(img.bitcount == 128)
        {
            format = GL_RGBA;
            if (img.is_float)
            {
                internal_format = GL_RGBA32F;
                type = GL_FLOAT;
            }
            else
            {
                internal_format = GL_RGBA;
                type = GL_UNSIGNED_INT;
            }
        }
        else
        {
            format = GL_RGBA;
            internal_format = GL_RGBA8;
            type = GL_UNSIGNED_BYTE;
        }
    }
}

void OpenGLGraphicsManagerCommonBase::initializeGeometries(const Scene& scene)
{
    uint32_t batch_index = 0;

    // Geometries
    for (const auto& _it : scene.GeometryNodes)
    {
        const auto& pGeometryNode = _it.second.lock();
        if (pGeometryNode && pGeometryNode->Visible()) 
        {
            const auto& pGeometry = scene.GetGeometry(pGeometryNode->GetSceneObjectRef());
            assert(pGeometry);
            const auto& pMesh = pGeometry->GetMesh().lock();
            if (!pMesh) continue;

            // Set the number of vertex properties.
            const auto vertexPropertiesCount = pMesh->GetVertexPropertiesCount();

            // Allocate an OpenGL vertex array object.
            GLuint vao;
            glGenVertexArrays(1, &vao);

            // Bind the vertex array object to store all the buffers and vertex attributes we create here.
            glBindVertexArray(vao);

            GLuint buffer_id;

            for (uint32_t i = 0; i < vertexPropertiesCount; i++)
            {
                const SceneObjectVertexArray& v_property_array = pMesh->GetVertexPropertyArray(i);
                const auto v_property_array_data_size = v_property_array.GetDataSize();
                const auto v_property_array_data = v_property_array.GetData();

                // Generate an ID for the vertex buffer.
                glGenBuffers(1, &buffer_id);

                // Bind the vertex buffer and load the vertex (position and color) data into the vertex buffer.
                glBindBuffer(GL_ARRAY_BUFFER, buffer_id);
                glBufferData(GL_ARRAY_BUFFER, v_property_array_data_size, v_property_array_data, GL_STATIC_DRAW);

                glEnableVertexAttribArray(i);

                switch (v_property_array.GetDataType()) {
                    case VertexDataType::kVertexDataTypeFloat1:
                        glVertexAttribPointer(i, 1, GL_FLOAT, false, 0, 0);
                        break;
                    case VertexDataType::kVertexDataTypeFloat2:
                        glVertexAttribPointer(i, 2, GL_FLOAT, false, 0, 0);
                        break;
                    case VertexDataType::kVertexDataTypeFloat3:
                        glVertexAttribPointer(i, 3, GL_FLOAT, false, 0, 0);
                        break;
                    case VertexDataType::kVertexDataTypeFloat4:
                        glVertexAttribPointer(i, 4, GL_FLOAT, false, 0, 0);
                        break;
#ifndef OPENGL_ES
                    case VertexDataType::kVertexDataTypeDouble1:
                        glVertexAttribPointer(i, 1, GL_DOUBLE, false, 0, 0);
                        break;
                    case VertexDataType::kVertexDataTypeDouble2:
                        glVertexAttribPointer(i, 2, GL_DOUBLE, false, 0, 0);
                        break;
                    case VertexDataType::kVertexDataTypeDouble3:
                        glVertexAttribPointer(i, 3, GL_DOUBLE, false, 0, 0);
                        break;
                    case VertexDataType::kVertexDataTypeDouble4:
                        glVertexAttribPointer(i, 4, GL_DOUBLE, false, 0, 0);
                        break;
#endif
                    default:
                        assert(0);
                }

                m_Buffers.push_back(buffer_id);
            }

            const auto indexGroupCount = pMesh->GetIndexGroupCount();

            GLenum  mode;
            switch(pMesh->GetPrimitiveType())
            {
                case PrimitiveType::kPrimitiveTypePointList:
                    mode = GL_POINTS;
                    break;
                case PrimitiveType::kPrimitiveTypeLineList:
                    mode = GL_LINES;
                    break;
                case PrimitiveType::kPrimitiveTypeLineStrip:
                    mode = GL_LINE_STRIP;
                    break;
                case PrimitiveType::kPrimitiveTypeTriList:
                    mode = GL_TRIANGLES;
                    break;
                case PrimitiveType::kPrimitiveTypeTriStrip:
                    mode = GL_TRIANGLE_STRIP;
                    break;
                case PrimitiveType::kPrimitiveTypeTriFan:
                    mode = GL_TRIANGLE_FAN;
                    break;
                default:
                    // ignore
                    continue;
            }

            for (uint32_t i = 0; i < indexGroupCount; i++)
            {
                // Generate an ID for the index buffer.
                glGenBuffers(1, &buffer_id);

                const SceneObjectIndexArray& index_array      = pMesh->GetIndexArray(i);
                const auto index_array_size = index_array.GetDataSize();
                const auto index_array_data = index_array.GetData();

                // Bind the index buffer and load the index data into it.
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer_id);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_array_size, index_array_data, GL_STATIC_DRAW);

                // Set the number of indices in the index array.
                GLsizei indexCount = static_cast<GLsizei>(index_array.GetIndexCount());
                GLenum type;
                switch(index_array.GetIndexType())
                {
                    case IndexDataType::kIndexDataTypeInt8:
                        type = GL_UNSIGNED_BYTE;
                        break;
                    case IndexDataType::kIndexDataTypeInt16:
                        type = GL_UNSIGNED_SHORT;
                        break;
                    case IndexDataType::kIndexDataTypeInt32:
                        type = GL_UNSIGNED_INT;
                        break;
                    default:
                        // not supported by OpenGL
                        cerr << "Error: Unsupported Index Type " << index_array << endl;
                        cerr << "Mesh: " << *pMesh << endl;
                        cerr << "Geometry: " << *pGeometry << endl;
                        continue;
                }

                m_Buffers.push_back(buffer_id);

                auto dbc = make_shared<OpenGLDrawBatchContext>();

                const auto material_index = index_array.GetMaterialIndex();
                const auto& material_key = pGeometryNode->GetMaterialRef(material_index);
                const auto material = scene.GetMaterial(material_key);
                if (material) {
                    function<GLuint(const string, const shared_ptr<Image>&)> upload_texture = [this](const string texture_key, const shared_ptr<Image>& texture) {
                        GLuint texture_id;
                        glGenTextures(1, &texture_id);
                        glBindTexture(GL_TEXTURE_2D, texture_id);
                        GLenum format, internal_format, type;
                        getOpenGLTextureFormat(*texture, format, internal_format, type);
                        if (texture->compressed)
                        {
                            glCompressedTexImage2D(GL_TEXTURE_2D, 0, internal_format, texture->Width, texture->Height, 
                                0, static_cast<GLsizei>(texture->data_size), texture->data);
                        }
                        else
                        {
                            glTexImage2D(GL_TEXTURE_2D, 0, internal_format, texture->Width, texture->Height, 
                                0, format, type, texture->data);
                        }

                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                        glGenerateMipmap(GL_TEXTURE_2D);

                        glBindTexture(GL_TEXTURE_2D, 0);

                        m_Textures.push_back(texture_id);

                        return texture_id;
                    };

                    // base color / albedo
                    const auto& color = material->GetBaseColor();
                    if (color.ValueMap) {
                        const auto& texture_key = color.ValueMap->GetName();
                        const auto& texture = color.ValueMap->GetTextureImage();
                        GLuint texture_id = upload_texture(texture_key, texture);
                        dbc->material.diffuseMap = static_cast<int32_t>(texture_id);
                    }

                    // normal
                    const auto& normal = material->GetNormal();
                    if (normal.ValueMap) {
                        const auto& texture_key = normal.ValueMap->GetName();
                        const auto& texture = normal.ValueMap->GetTextureImage();
                        GLuint texture_id = upload_texture(texture_key, texture);
                        dbc->material.normalMap = static_cast<int32_t>(texture_id);
                    }

                    // metallic 
                    const auto& metallic = material->GetMetallic();
                    if (metallic.ValueMap) {
                        const auto& texture_key = metallic.ValueMap->GetName();
                        const auto& texture = metallic.ValueMap->GetTextureImage();
                        GLuint texture_id = upload_texture(texture_key, texture);
                        dbc->material.metallicMap = static_cast<int32_t>(texture_id);
                    }

                    // roughness 
                    const auto& roughness = material->GetRoughness();
                    if (roughness.ValueMap) {
                        const auto& texture_key = roughness.ValueMap->GetName();
                        const auto& texture = roughness.ValueMap->GetTextureImage();
                        GLuint texture_id = upload_texture(texture_key, texture);
                        dbc->material.roughnessMap = static_cast<int32_t>(texture_id);
                    }

                    // ao
                    const auto& ao = material->GetAO();
                    if (ao.ValueMap) {
                        const auto& texture_key = ao.ValueMap->GetName();
                        const auto& texture = ao.ValueMap->GetTextureImage();
                        GLuint texture_id = upload_texture(texture_key, texture);
                        dbc->material.aoMap = static_cast<int32_t>(texture_id);
                    }

                    // height map 
                    const auto& heightmap = material->GetHeight();
                    if (heightmap.ValueMap) {
                        const auto& texture_key = heightmap.ValueMap->GetName();
                        const auto& texture = heightmap.ValueMap->GetTextureImage();
                        GLuint texture_id = upload_texture(texture_key, texture);
                        dbc->material.heightMap = static_cast<int32_t>(texture_id);
                    }
                }

                glBindVertexArray(0);

                dbc->batchIndex = batch_index++;
                dbc->vao     = vao;
                dbc->mode    = mode;
                dbc->type    = type;
                dbc->count   = indexCount;
                dbc->node    = pGeometryNode;

                for (int32_t n = 0; n < GfxConfiguration::kMaxInFlightFrameCount; n++)
                {
                    m_Frames[n].batchContexts.push_back(dbc);
                }
            }
        }
    }

}

void OpenGLGraphicsManagerCommonBase::initializeSkyBox(const Scene& scene)
{
    static const float skyboxVertices[] = {
         1.0f,  1.0f,  1.0f,  // 0
        -1.0f,  1.0f,  1.0f,  // 1
         1.0f, -1.0f,  1.0f,  // 2
         1.0f,  1.0f, -1.0f,  // 3
        -1.0f,  1.0f, -1.0f,  // 4
         1.0f, -1.0f, -1.0f,  // 5
        -1.0f, -1.0f,  1.0f,  // 6
        -1.0f, -1.0f, -1.0f   // 7
    };

    static const uint8_t skyboxIndices[] = {
        4, 7, 5,
        5, 3, 4,

        6, 7, 4,
        4, 1, 6,

        5, 2, 0,
        0, 3, 5,

        6, 1, 0,
        0, 2, 6,

        4, 3, 0,
        0, 1, 4,

        7, 6, 5,
        5, 6, 2
    };

    // load skybox, irradiance map and radiance map
    GLuint texture_id;
    const uint32_t kMaxMipLevels = 10;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, texture_id);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAX_LEVEL, kMaxMipLevels);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    assert(scene.SkyBox);

    // skybox, irradiance map
    for (uint32_t i = 0; i < 12; i++)
    {
        auto& texture = scene.SkyBox->GetTexture(i);
        const auto& pImage = texture.GetTextureImage();
        GLenum format, internal_format, type;
        getOpenGLTextureFormat(*pImage, format, internal_format, type);

        if (i == 0) // do this only once
        {
            const uint32_t faces = 6;
            const uint32_t indexies = 2;
            constexpr GLsizei depth = faces * indexies;
            glTexStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, kMaxMipLevels, internal_format, pImage->Width, pImage->Height, depth);
        }

        auto error = glGetError();
        assert(error == GL_NO_ERROR);

        GLint level = i / 6;
        GLint zoffset = i % 6;
        if (pImage->compressed)
        {
            glCompressedTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, level, 0, 0, zoffset, pImage->Width, pImage->Height, 1,
                internal_format, static_cast<GLsizei>(pImage->mipmaps[0].data_size), pImage->data);
        }
        else
        {
            glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, level, 0, 0, zoffset, pImage->Width, pImage->Height, 1,
                format, type, pImage->data);
        }

        error = glGetError();
        assert(error == GL_NO_ERROR);
    }

    // radiance map
    for (uint32_t i = 12; i < 18; i++)
    {
        auto& texture = scene.SkyBox->GetTexture(i);
        const auto& pImage = texture.GetTextureImage();
        GLenum format, internal_format, type;
        getOpenGLTextureFormat(*pImage, format, internal_format, type);

        GLint zoffset = (i % 6) + 6;
        for (decltype(pImage->mipmap_count) level = 0; level < std::min(pImage->mipmap_count, kMaxMipLevels); level++)
        {
            if (pImage->compressed)
            {
                glCompressedTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, level, 0, 0, zoffset, pImage->mipmaps[level].Width, pImage->mipmaps[level].Height, 1,
                    internal_format, static_cast<GLsizei>(pImage->mipmaps[level].data_size), pImage->data + pImage->mipmaps[level].offset);
            }
            else
            {
                glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, level, 0, 0, zoffset, pImage->mipmaps[level].Width, pImage->mipmaps[level].Height, 1,
                    format, type, pImage->data + pImage->mipmaps[level].offset);
            }

            auto error = glGetError();
            assert(error == GL_NO_ERROR);
        }
    }

    m_Textures.push_back(texture_id);

    for (int32_t i = 0; i < GfxConfiguration::kMaxInFlightFrameCount; i++)
    {
        m_Frames[i].frameContext.skybox = texture_id;
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);

    // skybox VAO
    GLuint skyboxVAO, skyboxVBO[2];
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(2, skyboxVBO);
    glBindVertexArray(skyboxVAO);
    // vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyboxVBO[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices), skyboxIndices, GL_STATIC_DRAW);

    glBindVertexArray(0);
    
    m_Buffers.push_back(skyboxVBO[0]);
    m_Buffers.push_back(skyboxVBO[1]);

    m_SkyBoxDrawBatchContext.vao     = skyboxVAO;
    m_SkyBoxDrawBatchContext.mode    = GL_TRIANGLES;
    m_SkyBoxDrawBatchContext.type    = GL_UNSIGNED_BYTE;
    m_SkyBoxDrawBatchContext.count   = sizeof(skyboxIndices) / sizeof(skyboxIndices[0]);
}

void OpenGLGraphicsManagerCommonBase::initializeTerrain(const Scene& scene)
{
    // skybox VAO
    GLuint terrainVAO, terrainVBO[2];
    glGenVertexArrays(1, &terrainVAO);
    glGenBuffers(2, terrainVBO);
    glBindVertexArray(terrainVAO);

    static const float patch_size = 32.0f;
    static const float _vertices[] = {
        0.0f,  patch_size, 0.0f,
        0.0f, 0.0f, 0.0f,
        patch_size, 0.0f, 0.0f,
        patch_size, patch_size, 0.0f
    };

    static const uint8_t _index[] = {
        0, 1, 2, 3
    };

    glBindBuffer(GL_ARRAY_BUFFER, terrainVBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(_vertices), _vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrainVBO[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(_index), _index, GL_STATIC_DRAW);

    glBindVertexArray(0);

    auto error = glGetError();
    assert(error == GL_NO_ERROR);
    
    m_Buffers.push_back(terrainVBO[0]);
    m_Buffers.push_back(terrainVBO[1]);

    m_TerrainDrawBatchContext.vao     = terrainVAO;
    m_TerrainDrawBatchContext.mode    = GL_PATCHES;
    m_TerrainDrawBatchContext.type    = GL_UNSIGNED_BYTE;
    m_TerrainDrawBatchContext.count   = sizeof(_index) / sizeof(_index[0]);

    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    auto& texture = scene.Terrain->GetTexture(0);
    const auto& pImage = texture.GetTextureImage();

    GLenum format, internal_format, type;
    getOpenGLTextureFormat(*pImage, format, internal_format, type);
    if (pImage->compressed)
    {
        glCompressedTexImage2D(GL_TEXTURE_2D, 0, internal_format, pImage->Width, pImage->Height, 
            0, static_cast<GLsizei>(pImage->data_size), pImage->data);
    }
    else
    {
        glTexImage2D(GL_TEXTURE_2D, 0, internal_format, pImage->Width, pImage->Height, 
            0, format, type, pImage->data);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    m_Textures.push_back(texture_id);

    for (int32_t i = 0; i < GfxConfiguration::kMaxInFlightFrameCount; i++)
    {
        m_Frames[i].frameContext.terrainHeightMap = texture_id;
    }
}

void OpenGLGraphicsManagerCommonBase::BeginScene(const Scene& scene)
{
    GraphicsManager::BeginScene(scene);

    initializeGeometries(scene);
    initializeTerrain(scene);
    initializeSkyBox(scene);

    return;
}

void OpenGLGraphicsManagerCommonBase::EndScene()
{
    for (int i = 0; i < GfxConfiguration::kMaxInFlightFrameCount; i++)
    {
        auto& batchContexts = m_Frames[i].batchContexts;

        for (auto& dbc : batchContexts) {
            glDeleteVertexArrays(1, &dynamic_pointer_cast<OpenGLDrawBatchContext>(dbc)->vao);
        }

        batchContexts.clear();

        if (m_uboDrawFrameConstant[i])
        {
            glDeleteBuffers(1, &m_uboDrawFrameConstant[i]);
        }
        
        if (m_uboDrawBatchConstant[i])
        {
            glDeleteBuffers(1, &m_uboDrawBatchConstant[i]);
        }
        
        if (m_uboShadowMatricesConstant[i])
        {
            glDeleteBuffers(1, &m_uboShadowMatricesConstant[i]);
        }
    }

    if (m_TerrainDrawBatchContext.vao)
    {
        glDeleteVertexArrays(1, &m_TerrainDrawBatchContext.vao);
    }

    if (m_SkyBoxDrawBatchContext.vao)
    {
        glDeleteVertexArrays(1, &m_SkyBoxDrawBatchContext.vao);
    }

    for (auto& buf : m_Buffers) {
        glDeleteBuffers(1, &buf);
    }

    for (auto& texture : m_Textures) {
        glDeleteTextures(1, &texture);
    }

    m_Buffers.clear();
    m_Textures.clear();

    GraphicsManager::EndScene();
}

void OpenGLGraphicsManagerCommonBase::BeginFrame()
{
    // Set the color to clear the screen to.
    glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
    // Clear the screen and depth buffer.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLGraphicsManagerCommonBase::EndFrame()
{

}

void OpenGLGraphicsManagerCommonBase::UseShaderProgram(const int32_t shaderProgram)
{
    m_CurrentShader = static_cast<GLuint>(shaderProgram);

    // Set the color shader as the current shader program and set the matrices that it will use for rendering.
    glUseProgram(m_CurrentShader);
}

void OpenGLGraphicsManagerCommonBase::SetPerFrameConstants(const DrawFrameContext& context)
{
    if (!m_uboDrawFrameConstant[m_nFrameIndex])
    {
        glGenBuffers(1, &m_uboDrawFrameConstant[m_nFrameIndex]);
    }

    glBindBuffer(GL_UNIFORM_BUFFER, m_uboDrawFrameConstant[m_nFrameIndex]);

    PerFrameConstants constants = static_cast<PerFrameConstants>(context);

    glBufferData(GL_UNIFORM_BUFFER, kSizePerFrameConstantBuffer, &constants, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_UNIFORM_BUFFER, 0);

}

void OpenGLGraphicsManagerCommonBase::SetLightInfo(const LightInfo& lightInfo)
{
    if (!m_uboLightInfo[m_nFrameIndex])
    {
        glGenBuffers(1, &m_uboLightInfo[m_nFrameIndex]);
    }

    glBindBuffer(GL_UNIFORM_BUFFER, m_uboLightInfo[m_nFrameIndex]);

    glBufferData(GL_UNIFORM_BUFFER, kSizeLightInfo, &lightInfo, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_UNIFORM_BUFFER, 0);

}

void OpenGLGraphicsManagerCommonBase::SetPerBatchConstants(const std::vector<std::shared_ptr<DrawBatchContext>>& batches)
{
    uint8_t* pBuff = new uint8_t[kSizePerBatchConstantBuffer * batches.size()];

    if (!m_uboDrawBatchConstant[m_nFrameIndex])
    {
        glGenBuffers(1, &m_uboDrawBatchConstant[m_nFrameIndex]);
    }

    glBindBuffer(GL_UNIFORM_BUFFER, m_uboDrawBatchConstant[m_nFrameIndex]);

    for (auto& pBatch : batches)
    {
        const PerBatchConstants& constants = static_cast<PerBatchConstants&>(*pBatch);
        memcpy(pBuff + pBatch->batchIndex * kSizePerBatchConstantBuffer, &constants, kSizePerBatchConstantBuffer);
    }

    glBufferData(GL_UNIFORM_BUFFER, kSizePerBatchConstantBuffer * batches.size(), pBuff, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    delete pBuff;
}

void OpenGLGraphicsManagerCommonBase::DrawBatch(const std::vector<std::shared_ptr<DrawBatchContext>>& batches)
{
    // Prepare & Bind per frame constant buffer
    GLuint blockIndex = glGetUniformBlockIndex(m_CurrentShader, "PerFrameConstants");

    if (blockIndex == GL_INVALID_INDEX)
    {
        // the shader does not use "PerFrameConstants"
        // simply return here
        return;
    }

    glUniformBlockBinding(m_CurrentShader, blockIndex, 10);

    glBindBufferBase(GL_UNIFORM_BUFFER, 10, m_uboDrawFrameConstant[m_nFrameIndex]);

    // Prepare & Bind light info
    blockIndex = glGetUniformBlockIndex(m_CurrentShader, "LightInfo");

    if (blockIndex != GL_INVALID_INDEX)
    {
        glUniformBlockBinding(m_CurrentShader, blockIndex, 12);
        glBindBufferBase(GL_UNIFORM_BUFFER, 12, m_uboLightInfo[m_nFrameIndex]);
    }

    // Prepare per batch constant buffer binding point
    blockIndex = glGetUniformBlockIndex(m_CurrentShader, "PerBatchConstants");

    if (blockIndex == GL_INVALID_INDEX)
    {
        // the shader does not use "PerBatchConstants"
        // simply return here
        return;
    }

    glUniformBlockBinding(m_CurrentShader, blockIndex, 11);

    glEnable(GL_CULL_FACE);

    for (auto& pDbc : batches)
    {
        const OpenGLDrawBatchContext& dbc = dynamic_cast<const OpenGLDrawBatchContext&>(*pDbc);

        // Bind per batch constant buffer
        glBindBufferRange(GL_UNIFORM_BUFFER, 11, m_uboDrawBatchConstant[m_nFrameIndex], 
                        dbc.batchIndex * kSizePerBatchConstantBuffer, kSizePerBatchConstantBuffer);

        // Bind textures
        if (dbc.material.diffuseMap >= 0) {
            setShaderParameter("SPIRV_Cross_CombineddiffuseMapsamp0", 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, dbc.material.diffuseMap);
        }

        if (dbc.material.normalMap >= 0) {
            setShaderParameter("SPIRV_Cross_CombinednormalMapsamp0", 1);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, dbc.material.normalMap);
        }

        if (dbc.material.metallicMap >= 0) {
            setShaderParameter("SPIRV_Cross_CombinedmetallicMapsamp0", 2);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, dbc.material.metallicMap);
        }

        if (dbc.material.roughnessMap >= 0) {
            setShaderParameter("SPIRV_Cross_CombinedroughnessMapsamp0", 3);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, dbc.material.roughnessMap);
        }

        if (dbc.material.aoMap >= 0) {
            setShaderParameter("SPIRV_Cross_CombinedaoMapsamp0", 4);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, dbc.material.aoMap);
        }

        if (dbc.material.heightMap >= 0) {
            setShaderParameter("SPIRV_Cross_CombinedheightMapsamp0", 5);
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, dbc.material.heightMap);
        }

        glBindVertexArray(dbc.vao);

        glDrawElements(dbc.mode, dbc.count, dbc.type, 0x00);
    }

    glBindVertexArray(0);
}

int32_t OpenGLGraphicsManagerCommonBase::GenerateCubeShadowMapArray(const uint32_t width, const uint32_t height, const uint32_t count)
{
    // Depth texture. Slower than a depth buffer, but you can sample it later in your shader
    GLuint shadowMap;

    glGenTextures(1, &shadowMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, shadowMap);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 1, GL_DEPTH_COMPONENT24, width, height, count * 6);

    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);

    // register the shadow map
    return static_cast<int32_t>(shadowMap);
}

int32_t OpenGLGraphicsManagerCommonBase::GenerateShadowMapArray(const uint32_t width, const uint32_t height, const uint32_t count)
{
    // Depth texture. Slower than a depth buffer, but you can sample it later in your shader
    GLuint shadowMap;

    glGenTextures(1, &shadowMap);
    glBindTexture(GL_TEXTURE_2D_ARRAY, shadowMap);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_DEPTH_COMPONENT24, width, height, count);

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    // register the shadow map
    return static_cast<int32_t>(shadowMap);
}

void OpenGLGraphicsManagerCommonBase::BeginShadowMap(const Light& light, const int32_t shadowmap, const uint32_t width, const uint32_t height, const uint32_t layer_index)
{
    // The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
    glGenFramebuffers(1, &m_ShadowMapFramebufferName);

    glBindFramebuffer(GL_FRAMEBUFFER, m_ShadowMapFramebufferName);

    if (light.lightType == LightType::Omni)
    {
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, (GLuint) shadowmap, 0);
    }
    else
    {
        // we only bind the single layer to FBO
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, (GLuint) shadowmap, 0, layer_index);
    }

    // Always check that our framebuffer is ok
    auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE)
    {
        assert(0);
    }

    glDrawBuffers(0, nullptr); // No color buffer is drawn to.
    glDepthMask(GL_TRUE);
    // make sure omni light shadowmap arrays get cleared only
    // once, because glClear will clear all cubemaps in the array
    if (light.lightType != LightType::Omni || layer_index == 0)
    {
        glClear(GL_DEPTH_BUFFER_BIT);
    }
    glViewport(0, 0, width, height);

    switch (light.lightType)
    {
        case LightType::Omni:
        {
            Matrix4X4f shadowMatrices[6];
            static const Vector3f direction[6] = {
                { 1.0f, 0.0f, 0.0f },
                {-1.0f, 0.0f, 0.0f },
                { 0.0f, 1.0f, 0.0f },
                { 0.0f,-1.0f, 0.0f },
                { 0.0f, 0.0f, 1.0f },
                { 0.0f, 0.0f,-1.0f }
            };
            static const Vector3f up[6] = {
                { 0.0f,-1.0f, 0.0f },
                { 0.0f,-1.0f, 0.0f },
                { 0.0f, 0.0f, 1.0f },
                { 0.0f, 0.0f,-1.0f },
                { 0.0f,-1.0f, 0.0f },
                { 0.0f,-1.0f, 0.0f }
            };

            float nearClipDistance = 0.1f;
            float farClipDistance = 10.0f;
            float fieldOfView = PI / 2.0f; // 90 degree for each cube map face
            float screenAspect = (float)width / (float)height;
            Matrix4X4f projection;

            // Build the perspective projection matrix.
            BuildPerspectiveFovRHMatrix(projection, fieldOfView, screenAspect, nearClipDistance, farClipDistance);

            Vector3f pos = {light.lightPosition[0], light.lightPosition[1], light.lightPosition[2]};
            for (int32_t i = 0; i < 6; i++)
            {
                BuildViewRHMatrix(shadowMatrices[i], pos, pos + direction[i], up[i]);
                shadowMatrices[i] = shadowMatrices[i] * projection;
            }

            GLuint blockIndex = glGetUniformBlockIndex(m_CurrentShader, "ShadowMatrices");

            assert(blockIndex != GL_INVALID_INDEX);

            GLint blockSize;

            glGetActiveUniformBlockiv(m_CurrentShader, blockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);

            if (!m_uboShadowMatricesConstant[m_nFrameIndex])
            {
                glGenBuffers(1, &m_uboShadowMatricesConstant[m_nFrameIndex]);
            }

            glBindBuffer(GL_UNIFORM_BUFFER, m_uboShadowMatricesConstant[m_nFrameIndex]);

            assert(blockSize == sizeof(shadowMatrices));
            glBufferData(GL_UNIFORM_BUFFER, blockSize, shadowMatrices, GL_DYNAMIC_DRAW);

            glUniformBlockBinding(m_CurrentShader, blockIndex, 2);
            glBindBufferBase(GL_UNIFORM_BUFFER, 2, m_uboShadowMatricesConstant[m_nFrameIndex]);

            setShaderParameter("u_gsPushConstants.layer_index", static_cast<int>(layer_index));
            setShaderParameter("u_lightParams.lightPos", pos);
            setShaderParameter("u_lightParams.far_plane", farClipDistance);

            break;
        }
        default:
        {
            setShaderParameter("u_pushConstants.depthVP", light.lightVP);
        }
    }

    glCullFace(GL_FRONT);
}

void OpenGLGraphicsManagerCommonBase::EndShadowMap(const int32_t shadowmap, uint32_t layer_index)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDeleteFramebuffers(1, &m_ShadowMapFramebufferName);

    const GfxConfiguration& conf = g_pApp->GetConfiguration();
    glViewport(0, 0, conf.screenWidth, conf.screenHeight);

    glCullFace(GL_BACK);
}

void OpenGLGraphicsManagerCommonBase::SetShadowMaps(const Frame& frame)
{
    GLuint texture_id = (GLuint) frame.frameContext.shadowMap;
    setShaderParameter("SPIRV_Cross_CombinedshadowMapsamp0", 7);
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture_id);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, color);	

    texture_id = (GLuint) frame.frameContext.globalShadowMap;
    setShaderParameter("SPIRV_Cross_CombinedglobalShadowMapsamp0", 8);
    glActiveTexture(GL_TEXTURE8);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture_id);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, color);	

    texture_id = (GLuint) frame.frameContext.cubeShadowMap;
    setShaderParameter("SPIRV_Cross_CombinedcubeShadowMapsamp0", 9);
    glActiveTexture(GL_TEXTURE9);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, texture_id);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void OpenGLGraphicsManagerCommonBase::DestroyShadowMap(int32_t& shadowmap)
{
    GLuint id = (GLuint) shadowmap;
    glDeleteTextures(1, &id);
    shadowmap = -1;
}

// skybox
void OpenGLGraphicsManagerCommonBase::SetSkyBox(const DrawFrameContext& context)
{
    GLuint texture_id = (GLuint) context.skybox;
    setShaderParameter("SPIRV_Cross_Combinedskyboxsamp0", 10);
    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, texture_id);
}

void OpenGLGraphicsManagerCommonBase::DrawSkyBox()
{
    // Prepare & Bind per frame constant buffer
    GLuint blockIndex = glGetUniformBlockIndex(m_CurrentShader, "PerFrameConstants");

    if (blockIndex == GL_INVALID_INDEX)
    {
        // the shader does not use "PerFrameConstants"
        // simply return here
        return;
    }

    glUniformBlockBinding(m_CurrentShader, blockIndex, 10);

    glBindBufferBase(GL_UNIFORM_BUFFER, 10, m_uboDrawFrameConstant[m_nFrameIndex]);

    glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
    glBindVertexArray(m_SkyBoxDrawBatchContext.vao);

    glDrawElements(m_SkyBoxDrawBatchContext.mode, m_SkyBoxDrawBatchContext.count, m_SkyBoxDrawBatchContext.type, 0x00);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS); // set depth function back to default
}

// terrain 
void OpenGLGraphicsManagerCommonBase::SetTerrain(const DrawFrameContext& context)
{
    GLuint terrainHeightMap = (GLuint) context.terrainHeightMap;
    setShaderParameter("SPIRV_Cross_CombinedterrainHeightMapsamp0", 11);
    glActiveTexture(GL_TEXTURE11);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, terrainHeightMap);
}

void OpenGLGraphicsManagerCommonBase::DrawTerrain()
{
#if 0
    glBindVertexArray(m_TerrainDrawBatchContext.vao);

    glPatchParameteri(GL_PATCH_VERTICES, 4);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    const float patch_size = 32.0f;
    const int32_t patch_num_row = 10;
    const int32_t patch_num_col = 10;

    for (int32_t i = -patch_num_row / 2; i < patch_num_row / 2; i++)
    {
        for (int32_t j = -patch_num_col / 2; j < patch_num_col / 2; j++)
        {
            MatrixTranslation(m_TerrainDrawBatchContext.modelMatrix, patch_size * i, patch_size * j, 0.0f);
            glDrawElements(m_TerrainDrawBatchContext.mode, m_TerrainDrawBatchContext.count, m_TerrainDrawBatchContext.type, 0x00);
        }
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBindVertexArray(0);
#endif
}

int32_t OpenGLGraphicsManagerCommonBase::GenerateTexture(const char* id, const uint32_t width, const uint32_t height)
{
    // Depth texture. Slower than a depth buffer, but you can sample it later in your shader
    GLuint texture;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RG16F, width, height);

    m_Textures.push_back(texture);

    // register the shadow map
    return static_cast<int32_t>(texture);
}

void OpenGLGraphicsManagerCommonBase::BeginRenderToTexture(int32_t& context, const int32_t texture, const uint32_t width, const uint32_t height)
{
    GLuint framebuffer;
    // The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
    glGenFramebuffers(1, &framebuffer);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, (GLuint) texture, 0);

    // Always check that our framebuffer is ok
    auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE)
    {
        assert(0);
    }

    context = (int32_t) framebuffer;

    GLenum buf[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, buf);
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, width, height);
}

void OpenGLGraphicsManagerCommonBase::EndRenderToTexture(int32_t& context)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    GLuint framebuffer = (GLuint) context;
    glDeleteFramebuffers(1, &framebuffer);
    context = 0;

    const GfxConfiguration& conf = g_pApp->GetConfiguration();
    glViewport(0, 0, conf.screenWidth, conf.screenHeight);

    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_BACK);
}

int32_t OpenGLGraphicsManagerCommonBase::GenerateAndBindTextureForWrite(const char* id, const uint32_t width, const uint32_t height)
{
    GLuint tex_output;
    glGenTextures(1, &tex_output);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_output);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, width, height, 0, GL_RG, GL_FLOAT, NULL);

    // Bind it as Write-only Texture
    if(GLAD_GL_ARB_compute_shader)
    {
        glBindImageTexture(0, tex_output, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
    }

    // Bind it as Read-only Texture
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, tex_output);

    m_Textures.push_back(tex_output);
    return static_cast<int32_t>(tex_output);
}

void OpenGLGraphicsManagerCommonBase::Dispatch(const uint32_t width, const uint32_t height, const uint32_t depth)
{
    if(GLAD_GL_ARB_compute_shader)
    {
        glDispatchCompute((GLuint)width, (GLuint)height, (GLuint)depth);
        // make sure writing to image has finished before read
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
}

#ifdef DEBUG

void OpenGLGraphicsManagerCommonBase::DrawPoint(const Point &point, const Vector3f& color)
{
    GLuint vao;
    glGenVertexArrays(1, &vao);

    // Bind the vertex array object to store all the buffers and vertex attributes we create here.
    glBindVertexArray(vao);

    GLuint buffer_id;

    // Generate an ID for the vertex buffer.
    glGenBuffers(1, &buffer_id);

    // Bind the vertex buffer and load the vertex (position and color) data into the vertex buffer.
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Point), point, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);

    m_DebugBuffers.push_back(buffer_id);

    DebugDrawBatchContext& dbc = *(new DebugDrawBatchContext);
    dbc.vao     = vao;
    dbc.mode    = GL_POINTS;
    dbc.count   = 1;
    dbc.color   = color;
    BuildIdentityMatrix(dbc.trans);

    m_DebugDrawBatchContext.push_back(std::move(dbc));
}

void OpenGLGraphicsManagerCommonBase::drawPoints(const Point* buffer, const size_t count, const Matrix4X4f& trans, const Vector3f& color)
{
    GLuint vao;
    glGenVertexArrays(1, &vao);

    // Bind the vertex array object to store all the buffers and vertex attributes we create here.
    glBindVertexArray(vao);

    GLuint buffer_id;

    // Generate an ID for the vertex buffer.
    glGenBuffers(1, &buffer_id);

    // Bind the vertex buffer and load the vertex (position and color) data into the vertex buffer.
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Point) * count, buffer, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);

    m_DebugBuffers.push_back(buffer_id);

    DebugDrawBatchContext& dbc = *(new DebugDrawBatchContext);
    dbc.vao     = vao;
    dbc.mode    = GL_POINTS;
    dbc.count   = static_cast<GLsizei>(count);
    dbc.color   = color;
    dbc.trans   = trans;

    m_DebugDrawBatchContext.push_back(std::move(dbc));
}

void OpenGLGraphicsManagerCommonBase::DrawPointSet(const PointSet& point_set, const Vector3f& color)
{
    Matrix4X4f trans;
    BuildIdentityMatrix(trans);

    DrawPointSet(point_set, trans, color);
}

void OpenGLGraphicsManagerCommonBase::DrawPointSet(const PointSet& point_set, const Matrix4X4f& trans, const Vector3f& color)
{
    const auto count = point_set.size();
    Point* buffer = new Point[count];
    int i = 0;
    for(const auto& point_ptr : point_set)
    {
        buffer[i++] = *point_ptr;
    }

    drawPoints(buffer, count, trans, color);

    delete[] buffer;
}

void OpenGLGraphicsManagerCommonBase::DrawLine(const PointList& vertices, const Matrix4X4f& trans, const Vector3f& color)
{
    const auto count = vertices.size();
    GLfloat* _vertices = new GLfloat[3 * count];

    for (auto i = 0; i < count; i++)
    {
        _vertices[3 * i] = vertices[i]->data[0];
        _vertices[3 * i + 1] = vertices[i]->data[1];
        _vertices[3 * i + 2] = vertices[i]->data[2];
    }

    GLuint vao;
    glGenVertexArrays(1, &vao);

    // Bind the vertex array object to store all the buffers and vertex attributes we create here.
    glBindVertexArray(vao);

    GLuint buffer_id;

    // Generate an ID for the vertex buffer.
    glGenBuffers(1, &buffer_id);

    // Bind the vertex buffer and load the vertex (position and color) data into the vertex buffer.
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * count, _vertices, GL_STATIC_DRAW);

    delete[] _vertices;

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);

    m_DebugBuffers.push_back(buffer_id);

    DebugDrawBatchContext& dbc = *(new DebugDrawBatchContext);
    dbc.vao     = vao;
    dbc.mode    = GL_LINES;
    dbc.count   = static_cast<GLsizei>(count);
    dbc.color   = color;
    dbc.trans   = trans;

    m_DebugDrawBatchContext.push_back(std::move(dbc));
}

void OpenGLGraphicsManagerCommonBase::DrawLine(const PointList& vertices, const Vector3f& color)
{
    Matrix4X4f trans;
    BuildIdentityMatrix(trans);

    DrawLine(vertices, trans, color);
}

void OpenGLGraphicsManagerCommonBase::DrawLine(const Point& from, const Point& to, const Vector3f& color)
{
    PointList point_list;
    point_list.push_back(make_shared<Point>(from));
    point_list.push_back(make_shared<Point>(to));

    DrawLine(point_list, color);
}

void OpenGLGraphicsManagerCommonBase::DrawTriangle(const PointList& vertices, const Vector3f& color)
{
    Matrix4X4f trans;
    BuildIdentityMatrix(trans);

    DrawTriangle(vertices, trans, color);
}

void OpenGLGraphicsManagerCommonBase::DrawTriangle(const PointList& vertices, const Matrix4X4f& trans, const Vector3f& color)
{
    const auto count = vertices.size();
    assert(count >= 3);

    GLuint vao;
    glGenVertexArrays(1, &vao);

    // Bind the vertex array object to store all the buffers and vertex attributes we create here.
    glBindVertexArray(vao);

    GLuint buffer_id;

    // Generate an ID for the vertex buffer.
    glGenBuffers(1, &buffer_id);

    // Bind the vertex buffer and load the vertex (position and color) data into the vertex buffer.
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id);
    Vector3f* data = new Vector3f[count];
    for(auto i = 0; i < count; i++)
    {
        data[i] = *vertices[i];
    }
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3f) * count, data, GL_STATIC_DRAW);
    delete[] data;

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);

    m_DebugBuffers.push_back(buffer_id);

    DebugDrawBatchContext& dbc = *(new DebugDrawBatchContext);
    dbc.vao     = vao;
    dbc.mode    = GL_TRIANGLES;
    dbc.count   = static_cast<GLsizei>(vertices.size());
    dbc.color   = color;
    dbc.trans   = trans;

    m_DebugDrawBatchContext.push_back(std::move(dbc));
}

void OpenGLGraphicsManagerCommonBase::DrawTriangleStrip(const PointList& vertices, const Vector3f& color)
{
    const auto count = vertices.size();
    assert(count >= 3);

    GLuint vao;
    glGenVertexArrays(1, &vao);

    // Bind the vertex array object to store all the buffers and vertex attributes we create here.
    glBindVertexArray(vao);

    GLuint buffer_id;

    // Generate an ID for the vertex buffer.
    glGenBuffers(1, &buffer_id);

    // Bind the vertex buffer and load the vertex (position and color) data into the vertex buffer.
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id);
    Vector3f* data = new Vector3f[count];
    for(auto i = 0; i < count; i++)
    {
        data[i] = *vertices[i];
    }
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3f) * count, data, GL_STATIC_DRAW);
    delete[] data;

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);

    m_DebugBuffers.push_back(buffer_id);

    DebugDrawBatchContext& dbc = *(new DebugDrawBatchContext);
    dbc.vao     = vao;
    dbc.mode    = GL_TRIANGLE_STRIP;
    dbc.count   = static_cast<GLsizei>(vertices.size());
    dbc.color   = color * 0.5f;

    m_DebugDrawBatchContext.push_back(std::move(dbc));
}

void OpenGLGraphicsManagerCommonBase::ClearDebugBuffers()
{
    for (auto& dbc : m_DebugDrawBatchContext) {
        glDeleteVertexArrays(1, &dbc.vao);
    }

    m_DebugDrawBatchContext.clear();

    for (auto& buf : m_DebugBuffers) {
        glDeleteBuffers(1, &buf);
    }

    m_DebugBuffers.clear();
}

void OpenGLGraphicsManagerCommonBase::RenderDebugBuffers()
{
    const auto debugShaderProgram = g_pShaderManager->GetDefaultShaderProgram(DefaultShaderIndex::Debug);

    // Set the color shader as the current shader program and set the matrices that it will use for rendering.
    UseShaderProgram(debugShaderProgram);

    for (const auto& dbc : m_DebugDrawBatchContext)
    {
        setShaderParameter("u_pushConstants.FrontColor", dbc.color);

        glBindVertexArray(dbc.vao);
        glDrawArrays(dbc.mode, 0x00, dbc.count);
    }
}

void OpenGLGraphicsManagerCommonBase::DrawTextureOverlay(const int32_t texture, float vp_left, float vp_top, float vp_width, float vp_height)
{
    GLuint texture_id = (GLuint) texture;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    GLfloat vertices[] = {
        vp_left, vp_top, 0.0f,
        vp_left, vp_top - vp_height, 0.0f,
        vp_left + vp_width, vp_top, 0.0f,
        vp_left + vp_width, vp_top - vp_height, 0.0f
    };

    GLfloat uv[] = {
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 0.0f
    };

    GLuint vao;
    glGenVertexArrays(1, &vao);

    // Bind the vertex array object to store all the buffers and vertex attributes we create here.
    glBindVertexArray(vao);

    GLuint buffer_id[2];

    // Generate an ID for the vertex buffer.
    glGenBuffers(2, buffer_id);

    // Bind the vertex buffer and load the vertex (position) data into the vertex buffer.
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);

    // Bind the vertex buffer and load the vertex (uv) data into the vertex buffer.
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uv), uv, GL_STATIC_DRAW);

    glEnableVertexAttribArray(1);

    glVertexAttribPointer(1, 2, GL_FLOAT, false, 0, 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0x00, 4);

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(2, buffer_id);
}

void OpenGLGraphicsManagerCommonBase::DrawTextureArrayOverlay(const int32_t texture, uint32_t layer_index, float vp_left, float vp_top, float vp_width, float vp_height)
{
    GLuint texture_id = (GLuint) texture;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture_id);
    bool result = setShaderParameter("u_pushConstants.layer_index", (float) layer_index);
    assert(result);

    GLfloat vertices[] = {
        vp_left, vp_top, 0.0f,
        vp_left, vp_top - vp_height, 0.0f,
        vp_left + vp_width, vp_top, 0.0f,
        vp_left + vp_width, vp_top - vp_height, 0.0f
    };

    GLfloat uv[] = {
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 0.0f
    };

    GLuint vao;
    glGenVertexArrays(1, &vao);

    // Bind the vertex array object to store all the buffers and vertex attributes we create here.
    glBindVertexArray(vao);

    GLuint buffer_id[2];

    // Generate an ID for the vertex buffer.
    glGenBuffers(2, buffer_id);

    // Bind the vertex buffer and load the vertex (position) data into the vertex buffer.
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);

    // Bind the vertex buffer and load the vertex (uv) data into the vertex buffer.
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uv), uv, GL_STATIC_DRAW);

    glEnableVertexAttribArray(1);

    glVertexAttribPointer(1, 2, GL_FLOAT, false, 0, 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0x00, 4);

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(2, buffer_id);
}

void OpenGLGraphicsManagerCommonBase::DrawCubeMapOverlay(const int32_t cubemap, float vp_left, float vp_top, float vp_width, float vp_height, float level)
{
    GLuint texture_id = (GLuint) cubemap;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id);

    bool result = setShaderParameter("u_pushConstants.level", level);
    assert(result);

    const float cell_height = vp_height * 0.5f;
    const float cell_width = vp_width * (1.0f / 3.0f);
    GLfloat vertices[] = {
        // face 1
        vp_left, vp_top, 0.0f,
        vp_left, vp_top - cell_height, 0.0f,
        vp_left + cell_width, vp_top, 0.0f,

        vp_left + cell_width, vp_top, 0.0f,
        vp_left, vp_top - cell_height, 0.0f,
        vp_left + cell_width, vp_top - cell_height, 0.0f,

        // face 2
        vp_left + cell_width, vp_top, 0.0f,
        vp_left + cell_width, vp_top - cell_height, 0.0f,
        vp_left + cell_width * 2.0f, vp_top, 0.0f,

        vp_left + cell_width * 2.0f, vp_top, 0.0f,
        vp_left + cell_width, vp_top - cell_height, 0.0f,
        vp_left + cell_width * 2.0f, vp_top - cell_height, 0.0f,

        // face 3
        vp_left + cell_width * 2.0f, vp_top, 0.0f,
        vp_left + cell_width * 2.0f, vp_top - cell_height, 0.0f,
        vp_left + cell_width * 3.0f, vp_top, 0.0f,

        vp_left + cell_width * 3.0f, vp_top, 0.0f,
        vp_left + cell_width * 2.0f, vp_top - cell_height, 0.0f,
        vp_left + cell_width * 3.0f, vp_top - cell_height, 0.0f,

        // face 4
        vp_left, vp_top - cell_height, 0.0f,
        vp_left, vp_top - cell_height * 2.0f, 0.0f,
        vp_left + cell_width, vp_top - cell_height, 0.0f,

        vp_left + cell_width, vp_top - cell_height, 0.0f,
        vp_left, vp_top - cell_height * 2.0f, 0.0f,
        vp_left + cell_width, vp_top - cell_height * 2.0f, 0.0f,

        // face 5
        vp_left + cell_width, vp_top - cell_height, 0.0f,
        vp_left + cell_width, vp_top - cell_height * 2.0f, 0.0f,
        vp_left + cell_width * 2.0f, vp_top - cell_height, 0.0f,

        vp_left + cell_width * 2.0f, vp_top - cell_height, 0.0f,
        vp_left + cell_width, vp_top - cell_height * 2.0f, 0.0f,
        vp_left + cell_width * 2.0f, vp_top - cell_height * 2.0f, 0.0f,

        // face 6
        vp_left + cell_width * 2.0f, vp_top - cell_height, 0.0f,
        vp_left + cell_width * 2.0f, vp_top - cell_height * 2.0f, 0.0f,
        vp_left + cell_width * 3.0f, vp_top - cell_height, 0.0f,

        vp_left + cell_width * 3.0f, vp_top - cell_height, 0.0f,
        vp_left + cell_width * 2.0f, vp_top - cell_height * 2.0f, 0.0f,
        vp_left + cell_width * 3.0f, vp_top - cell_height * 2.0f, 0.0f,
    };

    const GLfloat uvw[] = {
         // back
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        
        // left
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,

        // front
        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        // right
         1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,

        // top
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,

        // bottom
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f
    };

    GLuint vao;
    glGenVertexArrays(1, &vao);

    // Bind the vertex array object to store all the buffers and vertex attributes we create here.
    glBindVertexArray(vao);

    GLuint buffer_id[2];

    // Generate an ID for the vertex buffer.
    glGenBuffers(2, buffer_id);

    // Bind the vertex buffer and load the vertex (position) data into the vertex buffer.
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);

    // Bind the vertex buffer and load the vertex (uvw) data into the vertex buffer.
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uvw), uvw, GL_STATIC_DRAW);

    glEnableVertexAttribArray(1);

    glVertexAttribPointer(1, 3, GL_FLOAT, false, 0, 0);

    glDrawArrays(GL_TRIANGLES, 0x00, 36);

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(2, buffer_id);
}

void OpenGLGraphicsManagerCommonBase::DrawCubeMapArrayOverlay(const int32_t cubemap, uint32_t layer_index, float vp_left, float vp_top, float vp_width, float vp_height, float level)
{
    GLuint texture_id = (GLuint) cubemap;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, texture_id);
    bool result = setShaderParameter("u_pushConstants.layer_index", (float) layer_index);
    assert(result);

    result = setShaderParameter("u_pushConstants.level", level);

    const float cell_height = vp_height * 0.5f;
    const float cell_width = vp_width * (1.0f / 3.0f);
    GLfloat vertices[] = {
        // face 1
        vp_left, vp_top, 0.0f,
        vp_left, vp_top - cell_height, 0.0f,
        vp_left + cell_width, vp_top, 0.0f,

        vp_left + cell_width, vp_top, 0.0f,
        vp_left, vp_top - cell_height, 0.0f,
        vp_left + cell_width, vp_top - cell_height, 0.0f,

        // face 2
        vp_left + cell_width, vp_top, 0.0f,
        vp_left + cell_width, vp_top - cell_height, 0.0f,
        vp_left + cell_width * 2.0f, vp_top, 0.0f,

        vp_left + cell_width * 2.0f, vp_top, 0.0f,
        vp_left + cell_width, vp_top - cell_height, 0.0f,
        vp_left + cell_width * 2.0f, vp_top - cell_height, 0.0f,

        // face 3
        vp_left + cell_width * 2.0f, vp_top, 0.0f,
        vp_left + cell_width * 2.0f, vp_top - cell_height, 0.0f,
        vp_left + cell_width * 3.0f, vp_top, 0.0f,

        vp_left + cell_width * 3.0f, vp_top, 0.0f,
        vp_left + cell_width * 2.0f, vp_top - cell_height, 0.0f,
        vp_left + cell_width * 3.0f, vp_top - cell_height, 0.0f,

        // face 4
        vp_left, vp_top - cell_height, 0.0f,
        vp_left, vp_top - cell_height * 2.0f, 0.0f,
        vp_left + cell_width, vp_top - cell_height, 0.0f,

        vp_left + cell_width, vp_top - cell_height, 0.0f,
        vp_left, vp_top - cell_height * 2.0f, 0.0f,
        vp_left + cell_width, vp_top - cell_height * 2.0f, 0.0f,

        // face 5
        vp_left + cell_width, vp_top - cell_height, 0.0f,
        vp_left + cell_width, vp_top - cell_height * 2.0f, 0.0f,
        vp_left + cell_width * 2.0f, vp_top - cell_height, 0.0f,

        vp_left + cell_width * 2.0f, vp_top - cell_height, 0.0f,
        vp_left + cell_width, vp_top - cell_height * 2.0f, 0.0f,
        vp_left + cell_width * 2.0f, vp_top - cell_height * 2.0f, 0.0f,

        // face 6
        vp_left + cell_width * 2.0f, vp_top - cell_height, 0.0f,
        vp_left + cell_width * 2.0f, vp_top - cell_height * 2.0f, 0.0f,
        vp_left + cell_width * 3.0f, vp_top - cell_height, 0.0f,

        vp_left + cell_width * 3.0f, vp_top - cell_height, 0.0f,
        vp_left + cell_width * 2.0f, vp_top - cell_height * 2.0f, 0.0f,
        vp_left + cell_width * 3.0f, vp_top - cell_height * 2.0f, 0.0f,
    };

    const GLfloat uvw[] = {
         // back
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        
        // left
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,

        // front
        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        // right
         1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,

        // top
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,

        // bottom
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f
    };

    GLuint vao;
    glGenVertexArrays(1, &vao);

    // Bind the vertex array object to store all the buffers and vertex attributes we create here.
    glBindVertexArray(vao);

    GLuint buffer_id[2];

    // Generate an ID for the vertex buffer.
    glGenBuffers(2, buffer_id);

    // Bind the vertex buffer and load the vertex (position) data into the vertex buffer.
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);

    // Bind the vertex buffer and load the vertex (uvw) data into the vertex buffer.
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uvw), uvw, GL_STATIC_DRAW);

    glEnableVertexAttribArray(1);

    glVertexAttribPointer(1, 3, GL_FLOAT, false, 0, 0);

    glDrawArrays(GL_TRIANGLES, 0x00, 36);

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(2, buffer_id);
}

#endif

void OpenGLGraphicsManagerCommonBase::DrawFullScreenQuad()
{
    GLfloat vertices[] = {
        -1.0f,  1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,
         1.0f, -1.0f, 0.0f
    };

    GLfloat uv[] = {
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 0.0f
    };

    GLuint vao;
    glGenVertexArrays(1, &vao);

    // Bind the vertex array object to store all the buffers and vertex attributes we create here.
    glBindVertexArray(vao);

    GLuint buffer_id[2];

    // Generate an ID for the vertex buffer.
    glGenBuffers(2, buffer_id);

    // Bind the vertex buffer and load the vertex (position) data into the vertex buffer.
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);

    // Bind the vertex buffer and load the vertex (uv) data into the vertex buffer.
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uv), uv, GL_STATIC_DRAW);

    glEnableVertexAttribArray(1);

    glVertexAttribPointer(1, 2, GL_FLOAT, false, 0, 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0x00, 4);

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(2, buffer_id);
}
