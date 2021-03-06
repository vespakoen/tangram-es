#include "skybox.h"

Skybox::Skybox(std::string _file) : m_file(_file) {}

void Skybox::init() {

    std::string fragShaderSrcStr = stringFromResource("cubemap.fs");
    std::string vertShaderSrcStr = stringFromResource("cubemap.vs");

    m_shader = std::make_shared<ShaderProgram>();
    m_shader->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);

    m_texture = std::shared_ptr<Texture>(new TextureCube(m_file));
    auto layout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 3, GL_FLOAT, false, 0},
    }));

    m_mesh = std::shared_ptr<Mesh>(new Mesh(layout, GL_TRIANGLES));

    std::vector<int> indices = {
        5, 1, 3, 3, 7, 5, // +x
        6, 2, 0, 0, 4, 6, // -x
        2, 6, 7, 7, 3, 2, // +y
        5, 4, 0, 0, 1, 5, // -y
        0, 2, 3, 3, 1, 0, // +z
        7, 6, 4, 4, 5, 7  // -z
    };

    std::vector<PosVertex> vertices = {
        { -1.0, -1.0,  1.0 },
        {  1.0, -1.0,  1.0 },
        { -1.0,  1.0,  1.0 },
        {  1.0,  1.0,  1.0 },
        { -1.0, -1.0, -1.0 },
        {  1.0, -1.0, -1.0 },
        { -1.0,  1.0, -1.0 },
        {  1.0,  1.0, -1.0 }
    };

    m_mesh->addVertices(std::move(vertices), std::move(indices));
    m_mesh->compileVertexBuffer();

}

void Skybox::draw(const View& _view) {

    m_texture->bind(0);

    glm::mat4 vp = _view.getViewProjectionMatrix();

    m_shader->setUniformMatrix4f("u_modelViewProj", glm::value_ptr(vp));
    m_shader->setUniformi("u_tex", 0);
    m_mesh->draw(m_shader);

}


