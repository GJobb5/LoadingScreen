cmake_minimum_required (VERSION 3.15)

project ("LoadingScreen"
	VERSION 0.0
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)

# ----------------------------------------------------
# 1. ดึงไลบรารีที่จำเป็น (รวมถึง stb_image สำหรับโหลดรูป)
# ----------------------------------------------------
FetchContent_Declare(
	sampapi
	GIT_REPOSITORY https://github.com/BlastHackNet/SAMP-API.git
	GIT_TAG f89b35edb7eaf5db374e32a63f2366c4bb714418
)
FetchContent_Declare(
	ktsignal
	GIT_REPOSITORY https://github.com/kin4stat/ktsignal.git
	GIT_TAG f08a19d9a00ad5284871bf44178d835da7b26ebf
)
FetchContent_Declare(
	xbyak
	GIT_REPOSITORY https://github.com/herumi/xbyak.git
	GIT_TAG v6.69
)
FetchContent_Declare(
	kthook
	GIT_REPOSITORY https://github.com/kin4stat/kthook.git
	GIT_TAG 21ca70481273d1efcc98ca49063178e46277009b
)
FetchContent_Declare(
	imgui
	GIT_REPOSITORY https://github.com/ocornut/imgui.git
	GIT_TAG v1.89.9
)
FetchContent_Declare(
	stb
	GIT_REPOSITORY https://github.com/nothings/stb.git
	GIT_TAG master
)

FetchContent_MakeAvailable(sampapi ktsignal xbyak kthook imgui stb)

set(RESOURCE_RC "${CMAKE_CURRENT_SOURCE_DIR}/resources/resource.rc")
file(GLOB RESOURCE_DEPENDS CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/resources/*.png"
	"${CMAKE_CURRENT_SOURCE_DIR}/resources/*.ttf"
	"${CMAKE_CURRENT_SOURCE_DIR}/resources/*.h"
)
set_source_files_properties("${RESOURCE_RC}" PROPERTIES
	COMPILE_FLAGS "/i \"${CMAKE_CURRENT_SOURCE_DIR}/resources\""
	OBJECT_DEPENDS "${RESOURCE_DEPENDS}"
)

add_library(${PROJECT_NAME} SHARED)

# ----------------------------------------------------
# 2. รวมไฟล์โค้ด
# ----------------------------------------------------
target_sources(${PROJECT_NAME} PRIVATE
	src/main.cpp
	src/Plugin.cpp
	src/PluginRender.cpp
	${RESOURCE_RC}
	${imgui_SOURCE_DIR}/imgui.cpp
	${imgui_SOURCE_DIR}/imgui_draw.cpp
	${imgui_SOURCE_DIR}/imgui_tables.cpp
	${imgui_SOURCE_DIR}/imgui_widgets.cpp
	${imgui_SOURCE_DIR}/backends/imgui_impl_dx9.cpp
	${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
)

# ----------------------------------------------------
# 3. กำหนดโฟลเดอร์สำหรับ Include 
# ----------------------------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
	src
	resources
	${imgui_SOURCE_DIR}
	${imgui_SOURCE_DIR}/backends
	${xbyak_SOURCE_DIR}
	${stb_SOURCE_DIR}
)

# ----------------------------------------------------
# 4. ลิงก์ไลบรารี (เอา d3dx9 ออก เหลือแค่ d3d9)
# ----------------------------------------------------
target_link_libraries(${PROJECT_NAME} PUBLIC sampapi kthook d3d9)

# ----------------------------------------------------
# 5. ตั้งค่าคุณสมบัติการ Build
# ----------------------------------------------------
set_target_properties(${PROJECT_NAME}
	PROPERTIES
	OUTPUT_NAME ${PROJECT_NAME}
	SUFFIX ".asi"
)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

# ตรวจจับการเปลี่ยนไฟล์ Font หรือ Resource และบังคับ Compile Resource ใหม่เสมอ
add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E touch "${RESOURCE_RC}"
    COMMENT "Updating resource.rc timestamp to ensure resources are re-baked"
)

# คัดลอกไฟล์ .asi ไปที่ E:/SparkGlobal อัตโนมัติหลังคอมไพล์สำเร็จ (พร้อมระบบ Hot-swap ป้องกันติดล็อคขณะเปิดเกม)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND powershell -NoProfile -Command "if (Test-Path 'E:/SparkGlobal/$<TARGET_FILE_NAME:${PROJECT_NAME}>') { Move-Item -Path 'E:/SparkGlobal/$<TARGET_FILE_NAME:${PROJECT_NAME}>' -Destination 'E:/SparkGlobal/$<TARGET_FILE_NAME:${PROJECT_NAME}>.old' -Force -ErrorAction SilentlyContinue }; Copy-Item -Path '$<TARGET_FILE:${PROJECT_NAME}>' -Destination 'E:/SparkGlobal/$<TARGET_FILE_NAME:${PROJECT_NAME}>' -Force"
    COMMENT "Auto-deploying $<TARGET_FILE_NAME:${PROJECT_NAME}> to E:/SparkGlobal"
)