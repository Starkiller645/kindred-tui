cmake_minimum_required (VERSION 3.11)
 
# --- Fetch FTXUI --------------------------------------------------------------
include(FetchContent)
 
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  # Specify a GIT_TAG here.
)
 
FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
  FetchContent_Populate(ftxui)
  add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
 
# ------------------------------------------------------------------------------

# --- Fetch CPP Requests -------------------------------------------------------

FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git GIT_TAG f4622efcb59d84071ae11404ae61bd821c1c344b) # the commit hash for 1.6.2
FetchContent_MakeAvailable(cpr)

# ------------------------------------------------------------------------------

find_package(nlohmann_json 3.2.0 REQUIRED)
 
project(kindredtui
  LANGUAGES CXX
  VERSION 1.0.0
)
 
add_executable(kindredtui src/main.cpp)
target_include_directories(kindredtui PRIVATE src)
 
target_link_libraries(kindredtui
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component
  PRIVATE cpr::cpr
  PRIVATE nlohmann_json::nlohmann_json
)

install(TARGETS kindredtui
	RUNTIME DESTINATION /usr/local/bin/
)
