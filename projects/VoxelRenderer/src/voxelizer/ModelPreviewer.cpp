#include <src/voxelizer/ModelPreviewer.h>

void ModelPreviewer::CreateWindowTriangle()
{
    // Create Triangle Window
    if (!triangleWindow)
    {
        triangleWindow = glfwCreateWindow(windowSize.x, windowSize.y, "Model Triangle View", NULL, NULL);
        if (!triangleWindow)
        {
            printf("FAILED TO CREATE MODEL TRIANGLE VIEW WINDOW!\n");
            return;
        }
    }
}

void ModelPreviewer::CreateWindowVoxel()
{
    // Create Triangle Window
    if (!voxelWindow)
    {
        voxelWindow = glfwCreateWindow(windowSize.x, windowSize.y, "Model Voxel View", NULL, NULL);
        if (!voxelWindow)
        {
            printf("FAILED TO CREATE MODEL VOXEL VIEW WINDOW!\n");
            return;
        }
    }
}

void ModelPreviewer::RenderWindowTriangle()
{
    if (triangleWindow)
    {
        glfwMakeContextCurrent(triangleWindow);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT); // clear depth?

        glfwSwapBuffers(triangleWindow);

        if (glfwWindowShouldClose(triangleWindow))
        {
            glfwDestroyWindow(triangleWindow);
            triangleWindow = nullptr;
        }
    }
}

void ModelPreviewer::RenderWindowVoxel()
{
    if (voxelWindow)
    {
        glfwMakeContextCurrent(voxelWindow);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT); // clear depth?

        glfwSwapBuffers(voxelWindow);

        if (glfwWindowShouldClose(voxelWindow))
        {
            glfwDestroyWindow(voxelWindow);
            voxelWindow = nullptr;
        }
    }
}



