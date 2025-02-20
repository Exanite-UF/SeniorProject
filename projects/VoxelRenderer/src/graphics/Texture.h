#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <src/utilities/NonCopyable.h>

class Texture : public NonCopyable
{
private:
    GLuint textureId;
    GLuint64 bindlessHandle;

public:
    explicit Texture(GLuint textureId);

    GLuint getTextureId();
    uint64_t getBindlessHandle();
};
