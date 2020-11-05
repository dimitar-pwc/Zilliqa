add_library (RemoteStorageDB  RemoteStorageDB.cpp)
set_target_properties (RemoteStorageDB PROPERTIES COMPILE_OPTIONS "-Wno-deprecated")

target_include_directories (RemoteStorageDB PUBLIC ${PROJECT_SOURCE_DIR}/src ${G3LOG_INCLUDE_DIRS})
target_link_libraries(RemoteStorageDB PRIVATE mongo::bsoncxx_shared)
target_link_libraries(RemoteStorageDB PUBLIC Server mongo::mongocxx_shared)