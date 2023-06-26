#include "core/core.h"
#include "engine/engine.h"

int main()
{
	Logger::Init();

	Engine* engine = Engine::Create("Shaders Basics");
	engine->Run();
	delete engine;
}
