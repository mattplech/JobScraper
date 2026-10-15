# Declaration of JobScraper library
file(GLOB_RECURSE JOBSCRAPER_SRC_FILES "src/*.cpp" "include/*.hpp")

add_executable(JobScraper ${JOBSCRAPER_SRC_FILES}
    src/JobScraper.cpp)

# Compile definitions for JobScraper
target_compile_definitions(JobScraper
    PRIVATE

    # Debug macro
    $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:-DDEBUG -D_DEBUG>
	# Windows
	$<$<PLATFORM_ID:Windows>:-DWIN32_LEAN_AND_MEAN -DNOMINMAX>
)

target_include_directories(JobScraper
	PUBLIC

	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)

find_package(spdlog CONFIG REQUIRED)

target_link_libraries(JobScraper
	PUBLIC
	spdlog::spdlog
)

# Install JobScraper
include(GNUInstallDirs)

install(TARGETS JobScraper
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)