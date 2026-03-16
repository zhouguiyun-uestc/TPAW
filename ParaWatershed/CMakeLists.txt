
find_package(OpenMP REQUIRED)
find_package(MPI REQUIRED)
find_package(Cereal REQUIRED)
find_package(zstd REQUIRED)
add_library(
    ParaWatershedLib
	tool.cpp
	ParaWatershed.cpp
	grid_info.cpp
	SolveGlobal.cpp
	SolveLocal.cpp
	MPI_ParaWatershed.cpp
	MPI_Communicate.cpp
	MPI_main.cpp
	)

target_link_libraries(ParaWatershedLib PUBLIC 
		OpenMP::OpenMP_CXX
		GridLib
		WatershedAlgorithms
		MPI::MPI_CXX 
		Cereal::Cereal
		zstd::libzstd
		)

add_executable(
	ParaWatershed
	main.cpp
	)


target_link_libraries( ParaWatershed PUBLIC ParaWatershedLib)
if (CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo" )
	target_compile_options(ParaWatershedLib PUBLIC "/Od")
endif()

install(TARGETS ParaWatershed        
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        )
