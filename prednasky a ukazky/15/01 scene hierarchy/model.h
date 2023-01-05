class Model
{
public:
    std::vector<Mesh> meshes;

    Model(const std::string& path)
    {
        this->loadModel(path);
    }

    void Draw()
    {
        for (auto const& mesh : meshes)
            mesh.Draw();
    }



    //...

}
