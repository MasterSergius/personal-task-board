function(add_ptb_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE ptb_model Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_ptb_test(test_markdownio)
add_ptb_test(test_appstate)

# Widget tests compile the UI sources they exercise and run headless.
add_executable(test_columnwidget
    test_columnwidget.cpp
    ../src/columnwidget.cpp
    ../src/cardwidget.cpp
)
target_include_directories(test_columnwidget PRIVATE ../src)
target_link_libraries(test_columnwidget PRIVATE ptb_model Qt6::Widgets Qt6::Test)
add_test(NAME test_columnwidget COMMAND test_columnwidget)
set_tests_properties(test_columnwidget PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
