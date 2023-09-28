#include "core/core.h"
#include "engine/engine.h"

int main()
{
	Logger::Init();

	Engine* engine = Engine::Create("Shaders Basics", 800, 400);
	engine->Run();
	delete engine;
}
