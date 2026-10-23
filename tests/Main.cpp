#define CATCH_CONFIG_RUNNER
#include <Siv3D.hpp>
#ifdef __pre
#undef __pre
#endif
#include <catch2/catch.hpp>
#include <cstdlib>
#ifdef __linux__
SIV3D_SET(EngineOption::Renderer::Headless)
#endif

void Main()
{
	Catch::Session session;
#ifdef _WIN32
	char* report = nullptr;
	size_t length = 0;
	_dupenv_s(&report, &length, "KSM_TEST_REPORT");
#else
	const char* report = std::getenv("KSM_TEST_REPORT");
#endif
	if (report)
	{
		session.configData().reporterName = "junit";
		session.configData().outputFilename = report;
	}
#ifdef _WIN32
	std::free(report);
#endif
	session.run();
}
