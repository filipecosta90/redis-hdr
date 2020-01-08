file(REMOVE_RECURSE
  "redis_hdr.pdb"
  "redis_hdr.so"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/redis_hdr.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
