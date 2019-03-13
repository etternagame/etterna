# TODO: Determine if this is the best place for such a find_package function
find_package(GTK2 2.0)
if(GTK2_FOUND)
  message(${GTK2_INCLUDE_DIRS})
  return()
endif()

target_sources(Etterna PUBLIC LoadingWindow_GtkModule.cpp LoadingWindow_Gtk.cpp)
target_link_libraries(Etterna ${GTK2_LIBRARIES})
target_include_directories(Etterna ${GTK2_INCLUDE_DIRS})
#add_library(GtkModule LoadingWindow_GtkModule.cpp LoadingWindow_GtkModule.h)
#target_link_libraries(GtkModule ${GTK2_LIBRARIES})
#target_include_directories(GtkModule PUBLIC ${PROJECT_SOURCE_DIR}/src)
#target_include_directories(GtkModule PUBLIC ${PROJECT_BINARY_DIR}/generated)
#target_include_directories(GtkModule PUBLIC ${GTK2_INCLUDE_DIRS})

# It is normally not appropriate to set the prefix to the empty string.
# This is to maintain compatibility with the current source.
# At some point, it may be worth being more flexible.
#set_target_properties(GtkModule PROPERTIES PREFIX "")
#set_target_properties(GtkModule PROPERTIES OUTPUT_NAME GtkModule)
#set_target_properties(GtkModule PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${SM_ROOT_DIR})
#set_target_properties(GtkModule PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE ${SM_ROOT_DIR})
#set_target_properties(GtkModule PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG ${SM_ROOT_DIR})
#set_target_properties(GtkModule PROPERTIES LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${SM_ROOT_DIR})
#set_target_properties(GtkModule PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${SM_ROOT_DIR})
#set_property(TARGET "GtkModule" PROPERTY FOLDER "Internal Libraries")
#list(APPEND SM_GTK_INCLUDE_DIRS
#  "${SM_SRC_DIR}"
#  "${SM_SRC_DIR}/generated"
#  "${SM_SRC_DIR}/arch/LoadingWindow"
#  "${GTK2_INCLUDE_DIRS}")

#sm_add_compile_definition("GtkModule" CMAKE_POWERED)
#target_include_directories(GtkModule PUBLIC ${SM_GTK_INCLUDE_DIRS})
