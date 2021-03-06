enable_testing()

macro(add_script_test_cpu_mpi script)
    # execute on two processors
    SET(nproc 2)
    if (ENABLE_MPI)
        if(NOT "${EXCLUDE_FROM_MPI}" MATCHES ${script})
            add_test(NAME example_plugin-${script}-mpi-cpu
                COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${nproc}
                ${MPIEXEC_POSTFLAGS} ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${script} "--mode=cpu")
                set_tests_properties(example_plugin-${script}-mpi-cpu PROPERTIES ENVIRONMENT "PYTHONPATH=${CMAKE_BINARY_DIR}:$ENV{PYTHONPATH}")
            endif()
    endif(ENABLE_MPI)
endmacro()

macro(add_script_test_gpu_mpi script)
    # execute on two processors
    SET(nproc 2)
    if (ENABLE_MPI)
        if(NOT "${EXCLUDE_FROM_MPI}" MATCHES ${script})
            add_test(NAME example_plugin-${script}-mpi-gpu
                COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${nproc}
                ${MPIEXEC_POSTFLAGS} ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${script} "--mode=gpu" "--gpu_error_checking")
            set_tests_properties(example_plugin-${script}-mpi-gpu PROPERTIES ENVIRONMENT "PYTHONPATH=${CMAKE_BINARY_DIR}:$ENV{PYTHONPATH}")
        endif()
    endif(ENABLE_MPI)
endmacro()


macro(add_script_test_cpu script)
    add_test(example_plugin-${script}-cpu ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${script} "--mode=cpu")
    set_tests_properties(example_plugin-${script}-cpu PROPERTIES ENVIRONMENT "PYTHONPATH=${CMAKE_BINARY_DIR}:$ENV{PYTHONPATH}")

    add_script_test_cpu_mpi(${script})
endmacro()

macro(add_script_test_gpu script)
    add_test(example_plugin-${script}-gpu ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${script} "--mode=gpu")
    set_tests_properties(example_plugin-${script}-gpu PROPERTIES ENVIRONMENT "PYTHONPATH=${CMAKE_BINARY_DIR}:$ENV{PYTHONPATH}")

    add_script_test_gpu_mpi(${script})
endmacro()

set(TEST_LIST_CPU
    test_example.py
    )

set(TEST_LIST_GPU
    test_example.py
    )

set(EXCLUDE_FROM_MPI
   )

set(MPI_ONLY
    )

foreach (CUR_TEST ${TEST_LIST_CPU})
    if (TEST_CPU_IN_GPU_BUILDS OR NOT ENABLE_HIP)
        add_script_test_cpu(${CUR_TEST})
    endif()
endforeach (CUR_TEST)

foreach (CUR_TEST ${MPI_ONLY})
    if (TEST_CPU_IN_GPU_BUILDS OR NOT ENABLE_HIP)
        add_script_test_cpu_mpi(${CUR_TEST})
    endif()
endforeach (CUR_TEST)

if (ENABLE_HIP)
foreach (CUR_TEST ${TEST_LIST_GPU})
    add_script_test_gpu(${CUR_TEST})
endforeach (CUR_TEST)
foreach (CUR_TEST ${MPI_ONLY})
    add_script_test_gpu_mpi(${CUR_TEST})
endforeach (CUR_TEST)
endif (ENABLE_HIP)
