# Build the app, using the HAL

include_directories(include)
file(GLOB MY_SOURCES "src/*.c")
add_executable(as2 ${MY_SOURCES})

# Make use of the HAL library
target_link_libraries(as2 LINK_PRIVATE hal)

# Copy executable to final location (change `hello_world` to project name as needed)
add_custom_command(TARGET as2 POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:as2>"
     "~/ensc351/public/myApps/as2" 
  COMMENT "Copying ARM executable to public NFS directory")
