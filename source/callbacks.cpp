#include "callbacks.hpp"

Camera g_camera;

void framebuffer_size_callback(GLFWwindow* window, const int width, const int height)
{
	glViewport(0, 0, width, height);
	g_camera.setAspect(static_cast<float>(width), static_cast<float>(height));
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	switch(action)
	{
		case GLFW_PRESS:
			switch(key)
			{
				case GLFW_KEY_ESCAPE:
					glfwSetWindowShouldClose(window, true);
					break;
				case GLFW_KEY_W:
					g_camera.speed.z += 1.0f;
					break;
				case GLFW_KEY_S:
					g_camera.speed.z -= 1.0f;
					break;
				case GLFW_KEY_A:
					g_camera.speed.x -= 1.0f;
					break;
				case GLFW_KEY_D:
					g_camera.speed.x += 1.0f;
					break;
				default:
					break;
			}
			break;
		case GLFW_RELEASE:
			switch(key)
			{
				case GLFW_KEY_W:
					g_camera.speed.z -= 1.0f;
					break;
				case GLFW_KEY_S:
					g_camera.speed.z += 1.0f;
					break;
				case GLFW_KEY_A:
					g_camera.speed.x += 1.0f;
					break;
				case GLFW_KEY_D:
					g_camera.speed.x -= 1.0f;
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	static float lastX = 400.0f;
	static float lastY = 300.0f;
	static bool firstMouse = true;

	if(firstMouse)
	{
		lastX = static_cast<float>(xpos);
		lastY = static_cast<float>(ypos);
		firstMouse = false;
	}

	float xoffset = static_cast<float>(xpos) - lastX;
	float yoffset = lastY - static_cast<float>(ypos); // reversed since y-coordinates go from bottom to top

	lastX = static_cast<float>(xpos);
	lastY = static_cast<float>(ypos);

	g_camera.mouse(xoffset, yoffset);
}
