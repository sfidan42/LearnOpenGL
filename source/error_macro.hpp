#pragma once

#define GL_CHECK(FN) \
		FN; \
		{ \
		GLenum err = glGetError(); \
		if(err != GL_NO_ERROR) \
		{ \
		std::cerr << "OpenGL error " << err << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
		std::cerr << "Function call: " << #FN << std::endl; \
		abort(); \
		} \
		}
