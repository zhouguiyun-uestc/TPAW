
find_package(CUDAToolkit REQUIRED)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(OpenMP REQUIRED)
find_package(GDAL REQUIRED CONFIG)

add_library(
    WatershedAlgorithms
	WatershedFlowPathTraversal.cpp
	WatershedFlowPathTraversal_OpenMP.cpp
	 
	)

target_link_libraries(WatershedAlgorithms PUBLIC 
		CUDA::cudart
		OpenMP::OpenMP_CXX
		GridLib
		)
