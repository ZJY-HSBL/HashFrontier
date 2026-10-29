cmake_minimum_required(VERSION 3.20)
project(HashFrontier VERSION 0.24.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_library(hashfrontier INTERFACE)
target_include_directories(hashfrontier INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_executable(hf_benchmark src/benchmark.cpp)
target_link_libraries(hf_benchmark PRIVATE hashfrontier)
add_executable(hf_churn_benchmark src/churn_benchmark.cpp)
target_link_libraries(hf_churn_benchmark PRIVATE hashfrontier)
add_executable(hf_tests tests/test_tables.cpp)
target_link_libraries(hf_tests PRIVATE hashfrontier)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(hf_benchmark PRIVATE -O3 -Wall -Wextra -Wpedantic)
    target_compile_options(hf_churn_benchmark PRIVATE -O3 -Wall -Wextra -Wpedantic)
    target_compile_options(hf_tests PRIVATE -O2 -Wall -Wextra -Wpedantic)
endif()
enable_testing()
add_test(NAME randomized_differential COMMAND hf_tests)

# SODA 2025 §4 Basic Bubble-up independent reimplementation.
add_executable(hf_test_basic_bubble_up tests/test_basic_bubble_up.cpp)
target_link_libraries(hf_test_basic_bubble_up PRIVATE hashfrontier)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(hf_test_basic_bubble_up PRIVATE -O2 -Wall -Wextra -Wpedantic)
endif()
add_test(NAME basic_bubble_up_randomized COMMAND hf_test_basic_bubble_up)

# SODA 2025 Section 5.2: advanced insertion-and-query implementation.
add_executable(hf_test_advanced_bubble_up tests/test_advanced_bubble_up.cpp)
target_link_libraries(hf_test_advanced_bubble_up PRIVATE hashfrontier)
add_test(NAME advanced_bubble_up_oracle COMMAND hf_test_advanced_bubble_up)
add_executable(hf_bubble_paper_benchmark src/bubble_paper_benchmark.cpp)
target_link_libraries(hf_bubble_paper_benchmark PRIVATE hashfrontier)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(hf_test_advanced_bubble_up PRIVATE -O2 -Wall -Wextra -Wpedantic)
  target_compile_options(hf_bubble_paper_benchmark PRIVATE -O3 -Wall -Wextra -Wpedantic)
endif()

# SODA 2025 Appendix C: physical tombstones, rebuild and oracle tests.
add_executable(hf_appendix_c_tests tests/test_advanced_appendix_c.cpp)
target_link_libraries(hf_appendix_c_tests PRIVATE hashfrontier)
add_executable(hf_appendix_c_benchmark src/appendix_c_benchmark.cpp)
target_link_libraries(hf_appendix_c_benchmark PRIVATE hashfrontier)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(hf_appendix_c_tests PRIVATE -O2 -Wall -Wextra -Wpedantic)
  target_compile_options(hf_appendix_c_benchmark PRIVATE -O3 -Wall -Wextra -Wpedantic)
endif()
add_test(NAME appendix_c_tombstone_oracle COMMAND hf_appendix_c_tests)

# SODA 2025 Appendix B: paper-specific choice' collision/phase reproduction.
add_executable(hf_test_appendix_b tests/test_appendix_b.cpp)
target_link_libraries(hf_test_appendix_b PRIVATE hashfrontier)
add_executable(hf_appendix_b_benchmark src/appendix_b_benchmark.cpp)
target_link_libraries(hf_appendix_b_benchmark PRIVATE hashfrontier)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(hf_test_appendix_b PRIVATE -O2 -Wall -Wextra -Wpedantic)
  target_compile_options(hf_appendix_b_benchmark PRIVATE -O3 -Wall -Wextra -Wpedantic)
endif()
add_test(NAME appendix_b_collision_phase_and_oracle COMMAND hf_test_appendix_b)

# STOC 2025 §3.1 Partner Hashing: RAM pair-encoding (NOT full dictionary).
add_executable(hf_test_partner_section3 tests/test_partner_section3.cpp)
target_link_libraries(hf_test_partner_section3 PRIVATE hashfrontier)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(hf_test_partner_section3 PRIVATE -O2 -Wall -Wextra -Wpedantic)
endif()
add_test(NAME partner_ram_section3_permutation_oracle COMMAND hf_test_partner_section3)

# STOC 2025 §3.2 Algorithms 6–8: external-retrieval oracle-backed finite module.
# This is NOT the paper's Algorithms 4–5 or the fully implicit §4–5 hash table.
add_executable(hf_test_partner_dictionary_section3 tests/test_partner_dictionary_section3.cpp)
target_link_libraries(hf_test_partner_dictionary_section3 PRIVATE hashfrontier)
add_test(NAME partner_section3_2_dynamic_oracle COMMAND hf_test_partner_dictionary_section3)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(hf_test_partner_dictionary_section3 PRIVATE -O2 -Wall -Wextra -Wpedantic)
endif()

# STOC 2025 Algorithms 4/5: conditional read-via-RAM and deferred disjoint relocation.
add_executable(hf_test_partner_retrieval tests/test_partner_retrieval_section3.cpp)
target_link_libraries(hf_test_partner_retrieval PRIVATE hashfrontier)
add_test(NAME partner_alg4_5_conditional_oracle COMMAND hf_test_partner_retrieval)

# STOC25 Algorithm 5: staged freed-partner reuse, explicitly finite.
add_executable(hf_test_partner_batch_reuse tests/test_partner_batch_reuse.cpp)
target_link_libraries(hf_test_partner_batch_reuse PRIVATE hashfrontier)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(hf_test_partner_batch_reuse PRIVATE -O2 -Wall -Wextra -Wpedantic)
endif()
add_test(NAME partner_algorithm5_atomic_full_bin_reuse COMMAND hf_test_partner_batch_reuse)

# Experimental XOR retrieval encoded in Partner RAM. NOT DMPP06 or full Partner Hashing.
add_executable(hf_test_partner_xor_retrieval tests/test_partner_xor_retrieval.cpp)
target_link_libraries(hf_test_partner_xor_retrieval PRIVATE hashfrontier)
add_test(NAME partner_xor_encoded_retrieval_control COMMAND hf_test_partner_xor_retrieval)

# LATIN 2006 DMPP06 Section 2.1 + Section 3.1 first-level only.
add_executable(hf_test_dmpp06_first_level tests/test_dmpp06_first_level.cpp)
target_link_libraries(hf_test_dmpp06_first_level PRIVATE hashfrontier)
add_test(NAME dmpp06_finite_affine_quotient_first_level COMMAND hf_test_dmpp06_first_level)

# LATIN 2006 DMPP06 Section 3.2: finite multi-filter tile mechanism (not full DMPP06).
add_executable(hf_test_dmpp06_second_level tests/test_dmpp06_second_level.cpp)
target_link_libraries(hf_test_dmpp06_second_level PRIVATE hashfrontier)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(hf_test_dmpp06_second_level PRIVATE -O2 -Wall -Wextra -Wpedantic)
endif()
add_test(NAME dmpp06_second_level_tiles_stable_codes COMMAND hf_test_dmpp06_second_level)

# LATIN 2006 DMPP06 Section 3 finite three-level orchestration (not full theoretical dictionary).
add_executable(hf_test_dmpp06_three_level tests/test_dmpp06_three_level.cpp)
target_link_libraries(hf_test_dmpp06_three_level PRIVATE hashfrontier)
add_test(NAME dmpp06_three_tier_stable_code_differential COMMAND hf_test_dmpp06_three_level)

# DMPP06 LATIN 2006 §2.2 finite row-shift/column-permutation quotient mechanism.
add_executable(hf_test_dmpp06_column tests/test_dmpp06_column_quotient.cpp)
target_link_libraries(hf_test_dmpp06_column PRIVATE hashfrontier)
add_test(NAME dmpp06_finite_column_quotient_bijection COMMAND hf_test_dmpp06_column)

# DMPP06 §2.2 polynomial finite model (not Siegel family).
add_executable(hf_test_dmpp06_polynomial_quotient tests/test_dmpp06_polynomial_quotient.cpp)
target_link_libraries(hf_test_dmpp06_polynomial_quotient PRIVATE hashfrontier)
add_test(NAME dmpp06_polynomial_GF256_exhaustive COMMAND hf_test_dmpp06_polynomial_quotient)

# LATIN 2006 §2.2 Transformation 4 finite grouped first-order bucket permutations.
# Not the Siegel independence construction, not the complete Partner Hashing.
add_executable(hf_test_dmpp06_grouped_quotient tests/test_dmpp06_grouped_quotient.cpp)
target_link_libraries(hf_test_dmpp06_grouped_quotient PRIVATE hashfrontier)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(hf_test_dmpp06_grouped_quotient PRIVATE -O2 -Wall -Wextra -Wpedantic)
endif()
add_test(NAME dmpp06_transformation4_grouped_permutation_oracle COMMAND hf_test_dmpp06_grouped_quotient)
