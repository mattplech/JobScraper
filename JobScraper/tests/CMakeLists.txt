find_package(Catch2 3 CONFIG REQUIRED)

include(Catch)

add_executable(DatabaseTests DatabaseTests.cpp)
target_link_libraries(DatabaseTests PRIVATE JobScraperCore Catch2::Catch2)

catch_discover_tests(DatabaseTests PROPERTIES LABELS "integration;database")

add_executable(AlmaAdapterTests AlmaAdapterTests.cpp)
target_link_libraries(AlmaAdapterTests PRIVATE JobScraperCore Catch2::Catch2)

catch_discover_tests(AlmaAdapterTests PROPERTIES LABELS "integration;adapter")