#include "core/core.h"
#include "engine/engine.h"

int main()
{
	Logger::Init();

	Engine* engine = Engine::Create("Shaders Basics", 1600, 900);
	engine->Run();
	delete engine;
}
