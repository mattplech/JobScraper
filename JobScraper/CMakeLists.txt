# Declaration of JobScraper library
file(GLOB_RECURSE JOBSCRAPER_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
list(FILTER JOBSCRAPER_SRC_FILES EXCLUDE REGEX "/main\\.cpp$")

add_library(JobScraperCore STATIC ${JOBSCRAPER_SRC_FILES})

# Compile definitions for JobScraper
target_compile_definitions(JobScraperCore
    PRIVATE

    # Debug macro
    $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:-DDEBUG -D_DEBUG>
)

# Include JobScraper header files
target_include_directories(JobScraperCore
	PUBLIC

	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)

# Link libraries for JobScraper
find_package(spdlog CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)
find_package(cpr CONFIG REQUIRED)
find_package(LibXml2 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(JobScraperCore
	PUBLIC
	spdlog::spdlog
	libpqxx::pqxx
	cpr::cpr
	LibXml2::LibXml2
	nlohmann_json::nlohmann_json
)

# Install JobScraper
include(GNUInstallDirs)

add_executable(JobScraper src/main.cpp)
target_link_libraries(JobScraper PRIVATE JobScraperCore)

if(BUILD_TESTING)
	add_subdirectory(tests)
endif()

install(TARGETS JobScraper
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES "${PROJECT_SOURCE_DIR}/config.json" DESTINATION .)