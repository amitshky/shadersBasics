#include "core/core.h"
#include "engine/engine.h"

int main()
{
	Logger::Init();

	Engine* engine = Engine::Create("Shaders Basics", 960, 540);
	engine->Run();
	delete engine;
}
