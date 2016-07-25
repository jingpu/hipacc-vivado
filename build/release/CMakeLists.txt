cmake_minimum_required (VERSION 2.8)
project (main_renderscript)

list (APPEND CMAKE_MODULE_PATH "/horowitz/users/jingpu/nobackup/hipacc-vivado/cmake/modules")
find_package (RenderScript REQUIRED)

file (GLOB PROJECT_CPP *.cc)
file (GLOB PROJECT_RS *.rs
                      *.fs)

rs_definitions (-DSIZE_X=${SIZE_X}
                -DSIZE_Y=${SIZE_Y}
                -DWIDTH=${WIDTH}
                -DHEIGHT=${HEIGHT})

rs_wrap_scripts (PROJECT_CPP ${PROJECT_RS})

rs_include_directories (/horowitz/users/jingpu/nobackup/hipacc-vivado/build/release/include)

rs_link_libraries (${PROJECT_NAME} stdc++)

rs_add_executable (${PROJECT_NAME} ${PROJECT_CPP})

