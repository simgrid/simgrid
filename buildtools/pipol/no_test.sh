#THIS SCRIPT IS ONLY TO VERIFY CONFIGURATION

rm -f CMakeCache.txt

cmake \
-Denable_lua=on \
-Denable_ruby=on \
-Denable_java=on \
-Denable_tracing=on \
-Denable_smpi=on \
-Denable_lib_static=off \
-Denable_model-checking=off \
-Denable_latency_bound_tracking=off \
-Denable_gtnets=off \
-Denable_compile_optimizations=off \
-Denable_compile_warnings=off \
-Denable_supernovae=off . \
2>&1 | grep -i 'BUILDNAME'

cmake \
-Denable_lua=on \
-Denable_ruby=on \
-Denable_java=on \
-Denable_tracing=on \
-Denable_smpi=on \
-Denable_compile_optimizations=on \
-Denable_compile_warnings=on \
-Denable_lib_static=off \
-Denable_model-checking=off \
-Denable_latency_bound_tracking=off \
-Denable_gtnets=off \
-Denable_supernovae=off . \
2>&1 | grep -i 'BUILDNAME'

cmake \
-Denable_lua=on \
-Denable_ruby=on \
-Denable_java=on \
-Denable_tracing=on \
-Denable_smpi=on \
-Denable_supernovae=on \
-Denable_compile_optimizations=off \
-Denable_compile_warnings=off \
-Denable_lib_static=off \
-Denable_model-checking=off \
-Denable_latency_bound_tracking=off \
-Denable_gtnets=off . \
2>&1 | grep -i 'BUILDNAME'

cmake \
-Denable_coverage=on \
-Denable_lua=on \
-Denable_ruby=on \
-Denable_java=on \
-Denable_model-checking=on \
-Denable_tracing=on \
-Denable_latency_bound_tracking=on \
-Denable_gtnets=off \
-Denable_compile_optimizations=off \
-Denable_compile_warnings=off \
-Denable_supernovae=off \
-Denable_lib_static=off \
-Denable_smpi=on . \
2>&1 | grep -i 'BUILDNAME'

cmake \
-Denable_lua=on \
-Denable_ruby=on \
-Denable_lib_static=on \
-Denable_model-checking=off \
-Denable_tracing=on \
-Denable_latency_bound_tracking=on \
-Denable_gtnets=on \
-Denable_java=on \
-Dgtnets_path=~/gtnets \
-Denable_coverage=off \
-Denable_smpi=on . \
2>&1 | grep -i 'BUILDNAME'

cmake \
-Denable_lua=off \
-Denable_ruby=off \
-Denable_lib_static=off \
-Denable_model-checking=off \
-Denable_tracing=off \
-Denable_latency_bound_tracking=off \
-Denable_coverage=off \
-Denable_gtnets=off \
-Denable_java=off \
-Denable_memcheck=on . \
2>&1 | grep -i 'BUILDNAME'