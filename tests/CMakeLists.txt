function(add_ptb_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE ptb_model Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_ptb_test(test_markdownio)
add_ptb_test(test_appstate)
