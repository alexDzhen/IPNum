add_executable(ipnum_tests
    test_cli.cpp
    test_format.cpp
    test_ipv4.cpp
    test_ipv6.cpp
    test_main.cpp
    test_uint128.cpp)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_sources(ipnum_tests PRIVATE test_system_crosscheck.cpp)
endif()

target_include_directories(ipnum_tests SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/third_party/doctest)
target_compile_definitions(ipnum_tests PRIVATE DOCTEST_CONFIG_USE_STD_HEADERS)
target_link_libraries(ipnum_tests PRIVATE ipnum::core ipnum_warnings)
if(MINGW)
    target_link_options(ipnum_tests PRIVATE -static)
endif()

add_test(NAME unit_tests COMMAND ipnum_tests)

add_test(NAME cli_ipv4_decimal COMMAND ipnum --format dec 192.168.1.1)
set_tests_properties(cli_ipv4_decimal PROPERTIES PASS_REGULAR_EXPRESSION "^3232235777[\r\n]*$")

add_test(NAME cli_ipv6_words COMMAND ipnum --format words 2001:db8::ff00:42:8329)
set_tests_properties(cli_ipv6_words PROPERTIES
    PASS_REGULAR_EXPRESSION "^536939960 0 65280 4358953[\r\n]*$")

add_test(NAME cli_ipv6_decimal COMMAND ipnum --format dec 2001:db8::ff00:42:8329)
set_tests_properties(cli_ipv6_decimal PROPERTIES
    PASS_REGULAR_EXPRESSION "^42540766411282592856904265327123268393[\r\n]*$")

add_test(NAME cli_rejects_invalid_address COMMAND ipnum 256.1.1.1)
set_tests_properties(cli_rejects_invalid_address PROPERTIES WILL_FAIL TRUE)

add_test(NAME cli_rejects_unknown_option COMMAND ipnum --bogus)
set_tests_properties(cli_rejects_unknown_option PROPERTIES WILL_FAIL TRUE)

add_test(NAME cli_help COMMAND ipnum --help)
set_tests_properties(cli_help PROPERTIES PASS_REGULAR_EXPRESSION "Usage: ipnum")
