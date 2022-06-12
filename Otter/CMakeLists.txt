add_compile_definitions(GLM_FORCE_RADIANS)

set(IMGUI_DIR ../Libraries/imgui)

add_library(Otter SHARED
	../Libraries/loguru/loguru.cpp
	${IMGUI_DIR}/backends/imgui_impl_sdl.cpp
	${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
	${IMGUI_DIR}/imgui.cpp
	${IMGUI_DIR}/imgui_draw.cpp
	${IMGUI_DIR}/imgui_demo.cpp
	${IMGUI_DIR}/imgui_tables.cpp
	${IMGUI_DIR}/imgui_widgets.cpp
	Source/Core/Application.cpp
	Source/Core/Window.cpp
	Source/Systems/Renderer.cpp
	Source/Systems/TemplateSystem.cpp
	Source/Utilities/MD5.cpp
	Source/Utilities/ShaderUtilities.cpp
)
target_include_directories(Otter PRIVATE Include)
target_include_directories(Otter PUBLIC ../Libraries/SDL/include)
target_include_directories(Otter PUBLIC ../Libraries/stb)
target_include_directories(Otter PUBLIC ../Libraries/loguru)
target_include_directories(Otter PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends ..)

find_package(Vulkan REQUIRED)
target_link_libraries(Otter
	PUBLIC Vulkan::Vulkan
	PUBLIC SDL2::SDL2-static
	PUBLIC glm::glm
	PUBLIC glslang
	PUBLIC SPIRV
	PUBLIC VulkanMemoryAllocator
)

if(PLATFORM MATCHES "OS64")
	set(FRAMEWORK_NAME "Otter")
	set(FRAMEWORK_BUNDLE_IDENTIFIER "dannydebruijne.otter")
	set_target_properties(Otter PROPERTIES
		FRAMEWORK TRUE
		FRAMEWORK_VERSION A
		MACOSX_FRAMEWORK_IDENTIFIER ${FRAMEWORK_BUNDLE_IDENTIFIER}
		MACOSX_FRAMEWORK_INFO_PLIST ${CMAKE_CURRENT_LIST_DIR}/framework.plist.in
		# "current version" in semantic format in Mach-O binary file
		VERSION 1.0.0
		# "compatibility version" in semantic format in Mach-O binary file
		SOVERSION 1.0.0
		#PUBLIC_HEADER "${CMAKE_CURRENT_LIST_DIR}/Foo.hh"
		#XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET ${DEPLOYMENT_TARGET}
		#XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ${CODE_SIGN_IDENTITY}
		#XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ${DEVELOPMENT_TEAM_ID}
		#XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY ${DEVICE_FAMILY}
		XCODE_ATTRIBUTE_SKIP_INSTALL "YES"
	)

	# Symbol visibility setup, COMPILE_FLAGS only affect C++ so for Objective C we
	# have to use XCODE_ATTRIBUTE_OTHER_CFLAGS.
	set_target_properties(Otter PROPERTIES
	COMPILE_FLAGS "-fvisibility=hidden -fvisibility-inlines-hidden"
	XCODE_ATTRIBUTE_OTHER_CFLAGS "-fvisibility=hidden -fvisibility-inlines-hidden")

	add_custom_command(
		TARGET Otter
		POST_BUILD
		COMMAND /bin/bash -c "${CMAKE_CURRENT_LIST_DIR}/install_name.sh \${BUILT_PRODUCTS_DIR}/\${PRODUCT_NAME}.framework/\${PRODUCT_NAME}"
	)

	add_custom_command(
		TARGET Otter
		POST_BUILD
		COMMAND install_name_tool -id \"@rpath/\${PRODUCT_NAME}.framework/\${PRODUCT_NAME}\"
		\${BUILT_PRODUCTS_DIR}/\${PRODUCT_NAME}.framework/\${PRODUCT_NAME}
	)
endif()