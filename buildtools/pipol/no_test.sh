#THIS SCRIPT IS ONLY TO VERIFY CONFIGURATION

rm -f CMakeCache.txt

#ucontext
cmake -Denable_lua=on \
-Denable_ruby=on \
-Denable_lib_static=on \
-Denable_graphviz=on \
-Denable_model-checking=off \
-Denable_tracing=on \
-Denable_latency_bound_tracking=off \
-Denable_gtnets=off \
-Denable_java=on \
-Dwith_context=auto \
-Denable_compile_optimizations=off \
-Denable_compile_warnings=off \
-Denable_supernovae=off \
-Denable_smpi=on . 2>&1 | grep -i 'BUILDNAME'

#pthread
cmake -Denable_lua=on \
-Denable_ruby=on \
-Denable_lib_static=on \
-Denable_graphviz=on \
-Denable_model-checking=off \
-Denable_tracing=on \
-Denable_latency_bound_tracking=off \
-Denable_gtnets=off \
-Denable_java=on \
-Dwith_context=pthread \
-Denable_smpi=on . 2>&1 | grep -i 'BUILDNAME'

#gtnets
cmake -Denable_lua=on \
-Denable_ruby=on \
-Denable_lib_static=on \
-Denable_graphviz=on \
-Denable_model-checking=off \
-Denable_tracing=on \
-Denable_latency_bound_tracking=on \
-Denable_gtnets=on \
-Dgtnets_path=/usr \
-Denable_java=on \
-Dwith_context=auto \
-Denable_smpi=on . 2>&1 | grep -i 'BUILDNAME'

#full_flags
cmake -Denable_lua=on \
-Denable_ruby=on \
-Denable_lib_static=on \
-Denable_graphviz=on \
-Denable_model-checking=off \
-Denable_tracing=on \
-Denable_latency_bound_tracking=on \
-Denable_gtnets=off \
-Dgtnets_path=/usr \
-Denable_java=on \
-Dwith_context=auto \
-Denable_compile_optimizations=on \
-Denable_compile_warnings=on \
-Denable_smpi=on . 2>&1 | grep -i 'BUILDNAME'

#supernovae
cmake -Denable_lua=on \
-Denable_ruby=on \
-Denable_lib_static=on \
-Denable_graphviz=on \
-Denable_model-checking=off \
-Denable_tracing=on \
-Denable_latency_bound_tracking=on \
-Denable_gtnets=off \
-Dgtnets_path=/usr \
-Denable_java=on \
-Dwith_context=auto \
-Denable_compile_optimizations=off \
-Denable_compile_warnings=off \
-Denable_supernovae=on \
-Denable_smpi=on . 2>&1 | grep -i 'BUILDNAME'

#model checking
cmake -Denable_lua=on \
-Denable_ruby=on \
-Denable_lib_static=on \
-Denable_graphviz=on \
-Denable_model-checking=on \
-Denable_tracing=on \
-Denable_latency_bound_tracking=on \
-Denable_gtnets=on \
-Dgtnets_path=/usr \
-Denable_java=on \
-Dwith_context=auto \
-Denable_compile_optimizations=off \
-Denable_compile_warnings=off \
-Denable_supernovae=off \
-Denable_smpi=on . 2>&1 | grep -i 'BUILDNAME'

#memcheck
cmake -Denable_lua=on \
-Denable_ruby=on \
-Denable_lib_static=on \
-Denable_graphviz=on \
-Denable_model-checking=on \
-Denable_tracing=on \
-Denable_latency_bound_tracking=on \
-Denable_gtnets=on \
-Dgtnets_path=/usr \
-Denable_java=on \
-Dwith_context=auto \
-Denable_compile_optimizations=off \
-Denable_compile_warnings=off \
-Denable_supernovae=off \
-Denable_memcheck=on \
-Denable_smpi=on . 2>&1 | grep -i 'BUILDNAME'