set(CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Core")
set(U8G2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Lib/u8g2/csrc")
set(ARDUBOY32_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Game/Arduboy3D/Arduboy32")
set(ARDUBOY3D_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Game/Arduboy3D/Arduboy3D")

file(GLOB U8G2_SOURCES "${U8G2_DIR}/*.c")
file(GLOB ARDUBOY2_SOURCES "${ARDUBOY2_DIR}/*.cpp")
file(GLOB ARDUBOY3D_SOURCES "${ARDUBOY3D_DIR}/*.cpp")

target_include_directories(
    ${CMAKE_PROJECT_NAME} PRIVATE
    ${CORE_DIR}/Inc
    ${U8G2_DIR}
    ${ARDUBOY32_DIR}
    ${ARDUBOY3D_DIR}
    ${ARDUBOY3D_DIR}/Generated
)

target_sources(
    ${CMAKE_PROJECT_NAME} PRIVATE
    ${CORE_SOURCES}
    ${U8G2_SOURCES}
    ${CORE_DIR}/Src/oled.c
    ${CORE_DIR}/Src/matrixkey.c
    ${CORE_DIR}/Src/my_fonts.c
    ${ARDUBOY32_DIR}/Arduboy32.cpp
    ${ARDUBOY32_DIR}/Game_CAPI.cpp
    ${ARDUBOY32_SOURCES}
    ${ARDUBOY3D_SOURCES}
    
)