add_executable(run
  run.c)

target_link_libraries(run ai-isa-jit OpenCL)