#include "Renderer.hpp"

int main()
{
	Renderer renderer;

	vector<string> shader_filepaths = {
		"shaders/programs.glsl",
	};

	if(!renderer.init(shader_filepaths))
	{
		cout << "Failed to initialize renderer" << endl;
		return -1;
	}

	renderer.run();

	return 0;
}
