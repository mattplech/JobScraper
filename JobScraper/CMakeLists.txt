# Declaration of JobScraper library
file(GLOB_RECURSE JOBSCRAPER_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")

add_executable(JobScraper ${JOBSCRAPER_SRC_FILES})

# Compile definitions for JobScraper
target_compile_definitions(JobScraper
    PRIVATE

    # Debug macro
    $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:-DDEBUG -D_DEBUG>
)

# Include JobScraper header files
target_include_directories(JobScraper
	PUBLIC

	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)

# Link libraries for JobScraper
find_package(spdlog CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)

target_link_libraries(JobScraper
	PUBLIC
	spdlog::spdlog
	libpqxx::pqxx
)

# Install JobScraper
include(GNUInstallDirs)

install(TARGETS JobScraper
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)